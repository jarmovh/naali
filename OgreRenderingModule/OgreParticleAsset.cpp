// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"
#include "DebugOperatorNew.h"
#include "OgreConversionUtils.h"
#include "OgreParticleAsset.h"
#include "OgreRenderingModule.h"
#include "OgreMaterialUtils.h"
#include "Renderer.h"
#include "AssetAPI.h"
#include "LoggingFunctions.h"
#include <Ogre.h>
#include "MemoryLeakCheck.h"

using namespace OgreRenderer;

OgreParticleAsset::~OgreParticleAsset()
{
    Unload();
}

std::vector<AssetReference> OgreParticleAsset::FindReferences() const
{
    return references_;
}

void OgreParticleAsset::DoUnload()
{
    RemoveTemplates();
}

bool OgreParticleAsset::DeserializeFromData(const u8 *data_, size_t numBytes)
{
    RemoveTemplates();
    references_.clear();

    if (!data_)
    {
        ::LogError("Null source asset data pointer");     
        return false;
    }
    if (numBytes == 0)
    {
        ::LogError("Zero sized particle system asset");     
        return false;
    }

    // Detected template names
    StringVector new_templates;

    std::vector<u8> tempData(data_, data_ + numBytes);
#include "DisableMemoryLeakCheck.h"
    Ogre::DataStreamPtr data = Ogre::DataStreamPtr(new Ogre::MemoryDataStream(&tempData[0], numBytes));
#include "EnableMemoryLeakCheck.h"
    try
    {
        int brace_level = 0;
        bool skip_until_next = false;
        int skip_brace_level = 0;
        // Parsed/modified script
        std::ostringstream output;

        while(!data->eof())
        {
            Ogre::String line = data->getLine();
            // Skip empty lines & comments
            if ((line.length()) && (line.substr(0, 2) != "//"))
            {
                // Split line to components
                std::vector<Ogre::String> line_vec;

#if OGRE_VERSION_MAJOR == 1 && OGRE_VERSION_MINOR == 6 
		      line_vec = Ogre::StringUtil::split(line, "\t ");
#else 
		      Ogre::vector<Ogre::String>::type vec = Ogre::StringUtil::split(line,"\t ");
		      int size = vec.size();
		      line_vec.resize(size);
		      
		      for(int i = 0; i < size; ++i)
			line_vec[i] = vec[i];
#endif               

                // Process opening/closing braces
                if (!ProcessBraces(line, brace_level))
                {
                    // If not a brace and on level 0, it should be a new particlesystem; replace name with resource ID + ordinal
                    if (brace_level == 0)
                    {
                        line = SanitateAssetIdForOgre(this->Name().toStdString()) + "_" + boost::lexical_cast<std::string>(new_templates.size());
                        new_templates.push_back(line);
                        // New script compilers need this
                        line = "particle_system " + line;
                    }
                    else
                    {
                        // Check for ColourImage, which is a risky affector and may easily crash if image can't be loaded
                        if (line_vec[0] == "affector")
                        {   
                           if (line_vec.size() >= 2)
                            {
                                if (line_vec[1] == "ColourImage")
                                {
                                    skip_until_next = true;
                                    skip_brace_level = brace_level;
                                }
                            }
                        }
                        // Check for material definition
                        else if (line_vec[0] == "material")
                        {
                            if (line_vec.size() >= 2)
                            {
                                std::string mat_name = line_vec[1];
                                AssetReference assetRef(assetAPI->ResolveAssetRef(Name(), mat_name.c_str()));
                                references_.push_back(assetRef);
                                line = "material " + SanitateAssetIdForOgre(assetRef.ref);
                            }
                        }
                    }
                    // Write line to the copy
                    if (!skip_until_next)
                    {
                        output << line << std::endl;
                    }
                    else
                        ::LogDebug("Skipping risky particle effect line: " + line);
                }
                else
                {
                    // Write line to the copy
                    if (!skip_until_next)
                    {
                        output << line << std::endl;
                    }
                    else
                        ::LogDebug("Skipping risky particle effect line: " + line);

                    if (brace_level <= skip_brace_level)
                        skip_until_next = false;
                }
            } 
        }

        std::string output_str = output.str();
#include "DisableMemoryLeakCheck.h"
        Ogre::DataStreamPtr modified_data = Ogre::DataStreamPtr(new Ogre::MemoryDataStream(&output_str[0], output_str.size()));
#include "EnableMemoryLeakCheck.h"
        Ogre::ParticleSystemManager::getSingleton().parseScript(modified_data, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    }
    catch(Ogre::Exception& e)
    {
        ::LogWarning(e.what());
        ::LogWarning("Failed to parse Ogre particle script " + Name().toStdString() + ".");
    }
    
    // Check which templates actually succeeded
    for(uint i = 0; i < new_templates.size(); ++i)
    {
        if (Ogre::ParticleSystemManager::getSingleton().getTemplate(new_templates[i]))
        {
            templates_.push_back(new_templates[i]);
            ::LogDebug("Ogre particle system template " + new_templates[i] + " created");
        }
    }
    
    // Give only the name of the first template
    internal_name_ = SanitateAssetIdForOgre(Name().toStdString()) + "_0";
    
    // Theoretical success if at least one template was created
    return GetNumTemplates() > 0;
}

int OgreParticleAsset::GetNumTemplates() const
{
    return templates_.size();
}

QString OgreParticleAsset::GetTemplateName(int index) const
{
    if (index < 0 || index >= (int)templates_.size())
        return "";

    return templates_[index].c_str();
}

bool OgreParticleAsset::IsLoaded() const
{
    return templates_.size() > 0;
}

void OgreParticleAsset::RemoveTemplates()
{
    for(unsigned i = 0; i < templates_.size(); ++i)
    {
        try
        {
            Ogre::ParticleSystemManager::getSingleton().removeTemplate(templates_[i]);
        } catch(...) {}
    }
    templates_.clear();
}


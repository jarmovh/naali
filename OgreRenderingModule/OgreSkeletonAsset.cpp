#include "StableHeaders.h"
#include "DebugOperatorNew.h"
#include "MemoryLeakCheck.h"
#include "OgreSkeletonAsset.h"
#include "OgreRenderingModule.h"
#include "OgreConversionUtils.h"
#include "AssetAPI.h"

#include <QFile>

using namespace OgreRenderer;

OgreSkeletonAsset::~OgreSkeletonAsset()
{
    Unload();
}

bool OgreSkeletonAsset::DeserializeFromData(const u8 *data_, size_t numBytes)
{
    if (!data_)
    {
        OgreRenderingModule::LogError("Null source asset data pointer");
        return false;
    }
    if (numBytes == 0)
    {
        OgreRenderingModule::LogError("Zero sized skeleton asset");
        return false;
    }

    try
    {
        if (ogreSkeleton.isNull())
        {
            ogreSkeleton = Ogre::SkeletonManager::getSingleton().create(
                SanitateAssetIdForOgre(this->Name().toStdString()), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

            if (ogreSkeleton.isNull())
            {
                OgreRenderingModule::LogError("Failed to create skeleton " + this->Name().toStdString());
                return false; 
            }
        }

        std::vector<u8> tempData(data_, data_ + numBytes);
#include "DisableMemoryLeakCheck.h"
        Ogre::DataStreamPtr stream(new Ogre::MemoryDataStream(&tempData[0], numBytes, false));
#include "EnableMemoryLeakCheck.h"
        Ogre::SkeletonSerializer serializer;
        serializer.importSkeleton(stream, ogreSkeleton.getPointer());
    }
    catch (Ogre::Exception &e)
    {
        OgreRenderingModule::LogError("Failed to create skeleton " + this->Name().toStdString() + ": " + std::string(e.what()));
        Unload();
        return false;
    }

    internal_name_ = SanitateAssetIdForOgre(this->Name().toStdString());
    OgreRenderingModule::LogDebug("Ogre skeleton " + this->Name().toStdString() + " created");
    return true;
}

bool OgreSkeletonAsset::SerializeTo(std::vector<u8> &data, const QString &serializationParameters)
{
    if (ogreSkeleton.isNull())
    {
        OgreRenderingModule::LogWarning("Tried to export non-existing Ogre skeleton " + Name().toStdString() + ".");
        return false;
    }
    try
    {
        Ogre::SkeletonSerializer serializer;
        QString tempFilename = assetAPI->GenerateTemporaryNonexistingAssetFilename(Name());
        // Ogre has a limitation of not supporting serialization to a file in memory, so serialize directly to a temporary file,
        // load it up and delete the temporary file.
        serializer.exportSkeleton(ogreSkeleton.get(), tempFilename.toStdString());
        bool success = LoadFileToVector(tempFilename.toStdString().c_str(), data);
        QFile::remove(tempFilename); // Delete the temporary file we used for serialization.
        if (!success)
            return false;
    } catch (std::exception &e)
    {
        OgreRenderingModule::LogError("Failed to export Ogre skeleton " + Name().toStdString() + ":");
        if (e.what())
            OgreRenderingModule::LogError(e.what());
        return false;
    }
    return true;
}

void OgreSkeletonAsset::DoUnload()
{
    if (ogreSkeleton.isNull())
        return;

    std::string skeleton_name = ogreSkeleton->getName();
    ogreSkeleton.setNull();

    try
    {
        Ogre::SkeletonManager::getSingleton().remove(skeleton_name);
    }
    catch (...) {}
}

bool OgreSkeletonAsset::IsLoaded() const
{
    return ogreSkeleton.get() != 0;
}

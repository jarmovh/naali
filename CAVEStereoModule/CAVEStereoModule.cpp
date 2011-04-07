// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "CAVEStereoModule.h"
#include "CAVEManager.h"
#include "CAVESettingsWidget.h"
#include "StereoController.h"
#include "StereoWidget.h"

#include "OgreRenderingModule.h"

#include "MemoryLeakCheck.h"

namespace CAVEStereo
{
    std::string CAVEStereoModule::type_name_static_ = "CAVEStereo";

    CAVEStereoModule::CAVEStereoModule() :
        IModule(type_name_static_),
        stereo_(0),
        cave_(0)
    {
    }

    CAVEStereoModule::~CAVEStereoModule()
    {
        SAFE_DELETE(stereo_);
        SAFE_DELETE(cave_);
    }

    void CAVEStereoModule::PostInitialize()
    {
        OgreRenderer::OgreRenderingModule *rendererModule = framework_->GetModuleManager()->GetModule<OgreRenderer::OgreRenderingModule>().lock().get();
        if (rendererModule)
        {
            OgreRenderer::RendererPtr renderer = rendererModule->GetRenderer();
            if (renderer)
            {
                stereo_ = new StereoController(renderer.get(),this);
                cave_ = new CAVEManager(renderer);
                stereo_->InitializeUi();
                cave_->InitializeUi();
            }
        }
    }

    void CAVEStereoModule::Update(f64 frametime)
    {
        RESETPROFILER;
    }

    QVector<Ogre::RenderWindow*> CAVEStereoModule::GetCAVERenderWindows()
    {
        return cave_->GetExternalWindows();
    }

    void CAVEStereoModule::ShowStereoscopyWindow()
    {
        stereo_->GetStereoWidget()->show();
    }

    void CAVEStereoModule::ShowCaveWindow()
    {
        cave_->GetCaveWidget()->show();
    }
}

extern "C" void POCO_LIBRARY_API SetProfiler(Foundation::Profiler *profiler);
void SetProfiler(Foundation::Profiler *profiler)
{
    Foundation::ProfilerSection::SetProfiler(profiler);
}

using namespace CAVEStereo;

POCO_BEGIN_MANIFEST(IModule)
   POCO_EXPORT_CLASS(CAVEStereoModule)
POCO_END_MANIFEST

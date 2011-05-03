// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"
#include "DebugOperatorNew.h"
#include "MemoryLeakCheck.h" 

#include "CommunicationsModule.h"
#include "Service.h"

namespace CommunicationsService
{
    std::string CommunicationsModule::type_name_static_ = "Communications";

    CommunicationsModule::CommunicationsModule()
        : IModule(type_name_static_)
    {
    }

    CommunicationsModule::~CommunicationsModule()
    {
    }

    void CommunicationsModule::Load()
    {
    }

    void CommunicationsModule::Unload()
    {
    }

    void CommunicationsModule::Initialize() 
    {
        communications_service_ = Service::IntancePtr();
        if (!communications_service_)
        {
            LogError("Cannot get communications service instance for registering the service!");
            return;
        }

        framework_->GetServiceManager()->RegisterService(::Service::ST_Communications, communications_service_);
        LogInfo("Communications service registered.");
        framework_->RegisterDynamicObject("communications_service", communications_service_.get());
    }

    void CommunicationsModule::PostInitialize()
    {
        /// \todo Opensim stuff, can be removed?
        //CommunicationsWidget = new CommunicationsWidget();
        // Create Communication widgets
        //boost::shared_ptr<UiServices::UiModule> ui_module = framework_->GetModuleManager()->GetModule<UiServices::UiModule>().lock();
        //if (ui_module)
        //{
            //im_ui_proxy_widget_ = ui_module->GetInworldSceneController()->AddWidgetToScene(im_ui_, widget_properties);
            //if (im_ui_proxy_widget_)
            //    ui_module->GetInworldSceneController()->SetImWidget(im_ui_proxy_widget_);
        //}
    }

    void CommunicationsModule::Uninitialize()
    {
        if (communications_service_)
        {
            framework_->GetServiceManager()->UnregisterService(communications_service_);
            QString message("Communications service unregistered.");
            LogInfo(message.toStdString());
        }
    }

    void CommunicationsModule::Update(f64 frametime)
    {
    }

    bool CommunicationsModule::HandleEvent(event_category_id_t category_id, event_id_t event_id, IEventData* data)
    {
        return false;
    }

} // CommunicationsService

extern "C" void POCO_LIBRARY_API SetProfiler(Foundation::Profiler *profiler);
void SetProfiler(Foundation::Profiler *profiler)
{
    Foundation::ProfilerSection::SetProfiler(profiler);
}

using namespace CommunicationsService;

POCO_BEGIN_MANIFEST(IModule)
POCO_EXPORT_CLASS(CommunicationsModule)
POCO_END_MANIFEST

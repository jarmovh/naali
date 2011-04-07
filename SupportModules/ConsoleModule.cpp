// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"
#include "DebugOperatorNew.h"
#include "MemoryLeakCheck.h"
#include "ConsoleModule.h"
#include "ConsoleManager.h"
#include "ConsoleEvents.h"
#include "UiConsoleManager.h"

#include "InputContext.h"
#include "Framework.h"
#include "Profiler.h"
#include "ServiceManager.h"
#include "EventManager.h"
#include "ModuleManager.h"
#include "UiAPI.h"
#include "NaaliGraphicsView.h"

#include <QObject>

namespace Console
{
    std::string ConsoleModule::type_name_static_ = "Console";

    ConsoleModule::ConsoleModule() : IModule(type_name_static_), ui_console_manager_(0)
    {
    }

    ConsoleModule::~ConsoleModule()
    {
    }

    // virtual
    void ConsoleModule::PreInitialize()
    {
        manager_ = ConsolePtr(new ConsoleManager(this));
    }

    // virtual
    void ConsoleModule::Initialize()
    {
        framework_->GetServiceManager()->RegisterService(Service::ST_Console, manager_);
        framework_->GetServiceManager()->RegisterService(Service::ST_ConsoleCommand,
            checked_static_cast<ConsoleManager*>(manager_.get())->GetCommandManager());
    }

    void ConsoleModule::PostInitialize()
    {
        QGraphicsView *ui_view = GetFramework()->Ui()->GraphicsView();
        if (ui_view)
            ui_console_manager_ = new UiConsoleManager(GetFramework(), ui_view);

        consoleEventCategory_ = framework_->GetEventManager()->QueryEventCategory("Console");
        manager_->SetUiInitialized(!manager_->IsUiInitialized());

        inputContext = framework_->Input()->RegisterInputContext("Console", 100);
        inputContext->SetTakeKeyboardEventsOverQt(true);
        connect(inputContext.get(), SIGNAL(OnKeyEvent(KeyEvent *)), SLOT(HandleKeyEvent(KeyEvent *)));
    }

    void ConsoleModule::ToggleConsole()
    {
        if (ui_console_manager_)
            ui_console_manager_->ToggleConsole();
    }

    void ConsoleModule::HandleKeyEvent(KeyEvent *keyEvent)
    {
        if (keyEvent->keyCode == Qt::Key_F1 && keyEvent->eventType == KeyEvent::KeyPressed)
            ui_console_manager_->ToggleConsole();
    }

    // virtual 
    void ConsoleModule::Uninitialize()
    {
        framework_->GetServiceManager()->UnregisterService(manager_);
        framework_->GetServiceManager()->UnregisterService(checked_static_cast< ConsoleManager* >(manager_.get())->GetCommandManager());
        SAFE_DELETE(ui_console_manager_);
        assert (manager_);
        ConsoleManager *mgr = dynamic_cast<ConsoleManager*>(manager_.get());
        if (mgr)
            mgr->UnsubscribeLogListener();
        manager_.reset();
    }

    void ConsoleModule::Update(f64 frametime)
    {
        {
            PROFILE(ConsoleModule_Update);
            assert(manager_);
            manager_->Update(frametime);
        }
        RESETPROFILER;
    }

    // virtual
    bool ConsoleModule::HandleEvent(event_category_id_t category_id, event_id_t event_id, IEventData* data)
    {
        PROFILE(ConsoleModule_HandleEvent);
        ///\todo Now that console UI has been moved from UiModule to ConsoleModule many
        /// of these console events are silly and just call the module itself. Clean/refactor the events.
        if (consoleEventCategory_ == category_id)
        {
            switch(event_id)
            {
            case Console::Events::EVENT_CONSOLE_COMMAND_ISSUED:
            {
                Console::ConsoleEventData *console_data = dynamic_cast<Console::ConsoleEventData *>(data);
                manager_->ExecuteCommand(console_data->message);
                break;
            }
            case Console::Events::EVENT_CONSOLE_PRINT_LINE:
            {
                ConsoleEventData *console_data = dynamic_cast<Console::ConsoleEventData*>(data);
                if (ui_console_manager_)
                    ui_console_manager_->QueuePrintRequest(QString(console_data->message.c_str()));
                break;
            }
            default:
                return false;
            }

            return true;
        }
        return false;
    }
}

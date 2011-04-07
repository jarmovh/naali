// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_LoginScreenModule_LoginScreenModule_h
#define incl_LoginScreenModule_LoginScreenModule_h

#include "IModule.h"
#include "ModuleLoggingFunctions.h"

#include <QObject>
#include <QMap>

class KeyEvent;
class InputContext;

class LoginWidget;

class LoginScreenModule : public QObject, public IModule
{
    Q_OBJECT

public:
    /// Default constructor.
    LoginScreenModule();

    /// Destructor.
    ~LoginScreenModule();

    /// IModule override.
    void PostInitialize();

    /// IModule override.
    void Uninitialize();

    /// IModule override.
    void Update(f64 frametime);

    /// IModule override.
    bool HandleEvent(event_category_id_t category_id, event_id_t event_id, IEventData* data);

    MODULE_LOGGING_FUNCTIONS

    /// Returns name of this module. Needed for logging.
    static const std::string &NameStatic() { return type_name_static_; }

private:
    /// Type name of the module.
    static std::string type_name_static_;

    /// Login window.
    LoginWidget *window_;

    /// Input context for this module.
    boost::shared_ptr<InputContext> input_;

    /// "Framework" event category ID.
    event_category_id_t framework_category_;

    /// "NetworkState" event category ID.
    event_category_id_t network_category_;

    /// "Tundra" event category ID.
    event_category_id_t tundra_category_;
    
    /// Are we currently connected to world.
    bool connected_;

private slots:
    /** Handles key events from input service.
     *  @param key Key event.
     */
    void HandleKeyEvent(KeyEvent *key);

    //! Handles Tundra login (send an event, which TundraLogicModule will handle)
    void ProcessTundraLogin(const QMap<QString, QString> &data);
    
    /// Exits the program.
    void Exit();
};

#endif

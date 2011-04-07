// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_WorldBuildingModule_h
#define incl_WorldBuildingModule_h

#include "IModule.h"
#include "ModuleLoggingFunctions.h"
#include "WorldBuildingModuleApi.h"
#include "InputAPI.h"

#include <QObject>
#include <QStringList>
#include <QMap>

namespace WorldBuilding
{
    class BuildSceneManager;
    class OpenSimSceneService;
    typedef boost::shared_ptr<BuildSceneManager> BuildServicePtr;
    typedef boost::shared_ptr<OpenSimSceneService> OpenSimSceneServicePtr;

    class WB_MODULE_API WorldBuildingModule : public QObject, public IModule
    {

    Q_OBJECT
    
    
    static const std::string &NameStatic();

    public:
        WorldBuildingModule();
        virtual ~WorldBuildingModule();

        void Initialize();
        void PostInitialize();
        bool HandleEvent(event_category_id_t category_id, event_id_t event_id, IEventData* data);

        MODULE_LOGGING_FUNCTIONS

    public slots:
        //! Return the python handler
        QObject *GetPythonHandler();
    
    private slots:
        //! Populate service_category_identifiers_
        void SubscribeToEventCategories();    

    private:
        //! Current query categories
        QStringList event_query_categories_;

        //! Current subscribed category events
        QMap<QString, event_category_id_t> service_category_identifiers_;

        //! World building input context
        InputContextPtr input_context_;

        //! WorldBuilding service
        BuildServicePtr build_scene_manager_;

        //! OpenSim scene service
        OpenSimSceneServicePtr opensim_scene_service_;
    };
}
#endif
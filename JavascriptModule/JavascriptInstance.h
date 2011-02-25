/**
 *  For conditions of distribution and use, see copyright notice in license.txt
 *
 *  @file   JavascriptInstance.h
 *  @brief  Javascript script instance used wit EC_Script.
 */

#ifndef incl_JavascriptModule_JavascriptInstance_h
#define incl_JavascriptModule_JavascriptInstance_h

#include "IScriptInstance.h"
#include "ForwardDefines.h"
#include "AssetFwd.h"
#include "ScriptFwd.h"

#include <QtScript>
//#ifndef QT_NO_SCRIPTTOOLS
//#include <QScriptEngineDebugger>
//#endif

class JavascriptModule;

/// Javascript script instance used wit EC_Script.
class JavascriptInstance : public IScriptInstance
{
    Q_OBJECT

public:
    /// Creates script engine for this script instance and loads the script but doesn't run it yet.
    /** @param scriptRef Script asset reference.
        @param module Javascript module. */
    JavascriptInstance(const QString &fileName, JavascriptModule *module);

    /// Creates script engine for this script instance and loads the script but doesn't run it yet.
    /** @param scriptRef Script asset reference.
        @param module Javascript module. */
    JavascriptInstance(ScriptAssetPtr scriptRef, JavascriptModule *module);

    /// Destroys script engine created for this script instance.
    virtual ~JavascriptInstance();

    //! Overload from IScriptInstance
    void Load();

    //! Overload from IScriptInstance
    void Unload();

    //! Overload from IScriptInstance
    void Run();

    //! Register new service to java script engine.
    void RegisterService(QObject *serviceObject, const QString &name);

    //void SetPrototype(QScriptable *prototype, );
    QScriptEngine* GetEngine() const { return engine_; }

    /// Sets owner (EC_Script) component.
    /** @param owner Owner component.
    */
    void SetOwnerComponent(const ComponentPtr &owner) { owner_ = owner; }

    /// If the script content is loaded using the Asset API, this points to the asset that is loaded.
    //made public so that can check in JavascriptModule::ScriptAssetChanged if this actually changed.
    ScriptAssetPtr scriptRef_; 

public slots:
    /// Loads a given script in engine. This function can be used to create a property as you could include js-files.
    /** Multiple inclusion of same file is prevented. (by using simple string compare)
        @param path is relative path from bin/ to file. Example jsmodules/apitest/myscript.js */
    void IncludeFile(const QString &file);

    /// Imports the given QtScript extension plugin into the current script instance.
    void ImportExtension(const QString &scriptExtensionName);

private:
    /// Creates new script context/engine.
    void CreateEngine();

    /// Deletes script context/engine.
    void DeleteEngine();

    QScriptEngine *engine_; ///< Qt script engine.
    
    QString LoadScript(const QString &fileName);

    // The script content for a JavascriptInstance is loaded either using the Asset API or 
    // using an absolute path name from the local file system.

    /// If the script content is loaded directly from local file, this points to the actual script content.  
    QString program_;
    /// Specifies the absolute path of the source file where the script is loaded from, if the content is directly loaded from file.
    QString sourceFile;

    ComponentWeakPtr owner_; ///< Owner (EC_Script) component, if existing.
    JavascriptModule *module_; ///< Javascript module.
    bool evaluated; ///< Has the script program been evaluated.
    //QScriptEngineDebugger *debugger_;

    /// Already included files for preventing multi-inclusion
    std::vector<QString> included_files_; 
    
private slots:
    void OnSignalHandlerException(const QScriptValue& exception);
};

#endif

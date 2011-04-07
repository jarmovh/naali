// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_Scene_Entity_h
#define incl_Scene_Entity_h

#include "CoreTypes.h"
#include "SceneFwd.h"
#include "IComponent.h"
#include "IAttribute.h"
#include "EntityAction.h"

#include <boost/enable_shared_from_this.hpp>

#include <kNetFwd.h>

#include <QObject>
#include <QMap>

class QDomDocument;
class QDomElement;

namespace Scene
{
    //! Local entity ID flag (high bit)
    static const entity_id_t LocalEntity = 0x80000000;

    //! Represents an entity in the world.
    /*! An entity is just a collection of components, the components define what
        the entity is and what it does.
        Entities should not be directly created, instead use SceneManager::CreateEntity().

        Each component type that is added to this entity is registered as
        Q_PROPERTY as in following syntax EC_Light -> light, where EC_ is cutted off
        and name is converted to low case format. This allow scripter to get access to
        component using a following code "entity.mesh.SetMesh("mesh id");"
        
        \note If there are several components that have a same typename only first component
        is accessible through Q_PROPERTY and if you want to edit other same type of components
        you should use GetComponent method instead.

        When component is removed from the entity a Q_PROPERTY connection is destroyed from
        the component. In case that there are several components with the same typename, there is 
        a name check that ensures that both components names are same before Q_PROPERTY destroyed.

        \note   Entity can have multiple components with same component type name as long as
                the component names are unique.

        \ingroup Scene_group
    */
    class Entity : public QObject, public boost::enable_shared_from_this<Entity>
    {
        Q_OBJECT
        Q_PROPERTY (uint Id READ GetId)
        Q_PROPERTY (QString Name READ GetName)

        friend class SceneManager;

    public:
        //! Returns a component with type 'type_name' or empty pointer if component was not found
        /*! If there are several components with the specified type, returns the first component found (arbitrary).
            \param type_name type of the component
        */
        ComponentPtr GetComponent(const QString &type_name) const;

        //! This is an overloaded function.
        /*! \param type_hash Type name hash of the component.
        */
        ComponentPtr GetComponent(uint type_hash) const;

        //! This is an overloaded function.
        /*! \param type_name type of the component
            \param name name of the component
        */
        ComponentPtr GetComponent(const QString &type_name, const QString &name) const;

        //! This is an overloaded function.
        /*! \param type_has Type name hash of the component
            \param name name of the component
        */
        ComponentPtr GetComponent(uint type_hash, const QString &name) const;

        //! Returns a component with type 'type_name' or creates & adds it if not found. If could not create, returns empty pointer
        /*! \param type_name type of the component
            \param change Network replication mode, in case component has to be created
            \param syncEnabled Whether new component will have network sync enabled
            \return Pointer to the component, or an empty pointer if the component could be retrieved or created.
        */
        ComponentPtr GetOrCreateComponent(const QString &type_name, AttributeChange::Type change = AttributeChange::Default, bool syncEnabled = true);

        //! This is an overloaded function.
        /*! \param type_name type of the component
            \param change Network replication mode, in case component has to be created
            \param syncEnabled Whether new component will have network sync enabled
        */
        ComponentPtr GetOrCreateComponent(const QString &type_name, const QString &name, AttributeChange::Type change = AttributeChange::Default, bool syncEnabled = true);

        //! This is an overloaded function.
        /*! \param type_name type of the component
            \param change Network replication mode, in case component has to be created
            \param syncEnabled Whether new component will have network sync enabled
        */
        ComponentPtr GetOrCreateComponent(uint type_hash, AttributeChange::Type change = AttributeChange::Default);

        //! This is an overloaded function.
        /*! \param type_name type of the component
            \param change Network replication mode, in case component has to be created
            \param syncEnabled Whether new component will have network sync enabled
        */
        ComponentPtr GetOrCreateComponent(uint type_hash, const QString &name, AttributeChange::Type change = AttributeChange::Default);

        //! Creates a new component using the ComponentManager and attaches it to this entity. 
        /*! 
            \param type_name type of the component
            \param change Network replication mode, in case component has to be created
            \param syncEnabled Whether new component will have networksync enabled
            \return Retuns a pointer to the newly created component, or null if creation failed. Common causes for failing to create an component
            is that a component with the same (typename, name) pair exists, or that components of the given typename are not recognized by the system.
        */
        ComponentPtr CreateComponent(const QString &type_name, AttributeChange::Type change = AttributeChange::Default, bool syncEnabled = true);
        ComponentPtr CreateComponent(const QString &type_name, const QString &name, AttributeChange::Type change = AttributeChange::Default, bool syncEnabled = true);
        ComponentPtr CreateComponent(uint type_hash, AttributeChange::Type change = AttributeChange::Default);
        ComponentPtr CreateComponent(uint type_hash, const QString &name, AttributeChange::Type change = AttributeChange::Default);

        typedef std::vector<ComponentPtr> ComponentVector; //!< Component container.
        typedef QMap<QString, EntityAction *> ActionMap; //!< Action container

        //! destructor
        ~Entity();

        //! Returns true if the two entities have the same id, false otherwise
        virtual bool operator == (const Entity &other) const { return GetId() == other.GetId(); }

        //! Returns true if the two entities have different id, false otherwise
        virtual bool operator != (const Entity &other) const { return !(*this == other); }

        //! comparison by id
        virtual bool operator < (const Entity &other) const { return GetId() < other.GetId(); }

        //! Attachs an existing parentless component to this entity.
        /*! Entities can contain any number of components of any type.
            It is also possible to have several components of the same type,
            although in most cases it is probably not sensible.

            Each component type that is added to this entity is registered as
            Q_PROPERTY as in following syntax EC_Light -> light, where EC_ is cutted off
            and name is converted to low case format. This allow scripter to get access to
            component using a following code "entity.mesh.SetMesh("mesh id");"

            \param component The component to add to this entity. The component must be parentless, i.e.
                          previously created using ComponentManager::CreateComponent.
            \param change Network replication mode
        */
        void AddComponent(const ComponentPtr &component, AttributeChange::Type change = AttributeChange::Default);

        //! Remove the component from this entity.
        /*! When component is removed from the entity a Q_PROPERTY connection is destroyed from
            the component. In case where there are several components with the same typename, there is 
            a name check that ensures that both components names are same before Q_PROPERTY destroyed.
            
            \param component Pointer to the component to remove
        */
        void RemoveComponent(const ComponentPtr &component, AttributeChange::Type change = AttributeChange::Default);

        //! Returns a component with type typename and name or empty pointer if component was not found
        /*! If there are several components with the specified type, returns the first component found (arbitrary).

            \param component component that we want to find.
        */
        ComponentPtr GetComponent(const IComponent *component) const;

        //! Returns list of components with type 'type_name' or empty list if no components were found.
        //! \param type_name type of the component
        ComponentVector GetComponents(const QString &type_name) const;

        //! Returns a component with certain type, already cast to correct type, or empty pointer if component was not found
        /*! If there are several components with the specified type, returns the first component found (arbitrary).
        */
        template <class T>
        boost::shared_ptr<T> GetComponent() const
        {
            return boost::dynamic_pointer_cast<T>(GetComponent(T::TypeNameStatic()));
        }

        /*! Returns list of components with certain class type, already cast to correct type.
            \param T Component class type.
            \return List of components with certain class type, or empty list if no components was found.
        */
        template <class T>
        std::vector<boost::shared_ptr<T> > GetComponents() const
        {
            std::vector<boost::shared_ptr<T> > ret;
            for(size_t i = 0; i < components_.size() ; ++i)
            {
                boost::shared_ptr<T> t = boost::dynamic_pointer_cast<T>(components_[i]);
                if (t)
                    ret.push_back(t);
            }
            return ret;
        }

        //! Returns a component with certain type and name, already cast to correct type, or empty pointer if component was not found
        /*! 
            \param name name of the component
        */
        template <class T>
        boost::shared_ptr<T> GetComponent(const QString& name) const
        {
            return boost::dynamic_pointer_cast<T>(GetComponent(T::TypeNameStatic(), name));
        }

        //! Returns the unique id of this entity
        entity_id_t GetId() const { return id_; }

        //! introspection for the entity, returns all components
        const ComponentVector &Components() const { return components_; }

        //! Returns framework
        Foundation::Framework *GetFramework() const { return framework_; }

        //! Returns scene
        SceneManager* GetScene() const { return scene_; }

       /*! Returns pointer to the first attribute with specific name.
            \param T Type name/class of the attribute.
            \param name Name of the attribute.
            \return Pointer to the attribute.
            \note Always remember to check for null pointer.
        */
        template<typename T>
        Attribute<T> *GetAttribute(const std::string &name) const
        {
            for(size_t i = 0; i < components_.size() ; ++i)
            {
                Attribute<T> *t = components_[i]->GetAttribute<T>(name);
                if (t)
                    return t;
            }
            return 0;
        }

        //! Returns attribute interface pointer to attribute with specific name.
        /*! \param name Name of the attribute.
            \return IAttribute pointer to the attribute.
            \note Always remember to check for null pointer.
        */
        IAttribute *GetAttribute(const std::string &name) const;

        //! Returns list of attributes with specific name.
        /*! \param T Type name/class of the attribute.
            \param name Name of the attribute.
            \return List of attributes, or empty list if no attributes are found.
        */
        template<typename T>
        std::vector<Attribute<T> > GetAttributes(const std::string &name) const
        {
            std::vector<Attribute<T> > ret;
            for(size_t i = 0; i < components_.size() ; ++i)
            {
                Attribute<T> *t = components_[i]->GetAttribute<T>(name);
                if (t)
                    return ret.push_back(t);
            }
            return ret;
        }

        //! Returns list of attributes with specific name.
        /*! \param name Name of the attribute.
            \return List of attribute interface pointers, or empty list if no attributes are found.
        */
        AttributeVector GetAttributes(const std::string &name) const;

        /// In the following, deserialization functions are now disabled since deserialization can't safely
        /// process the exact same data that was serialized, or it risks receiving entity ID conflicts in the scene.
        /// \todo Implement a deserialization flow that takes that into account. In the meanwhile, use SceneManager
        /// functions for achieving the same.

        void SerializeToBinary(kNet::DataSerializer &dst) const;
//        void DeserializeFromBinary(kNet::DataDeserializer &src, AttributeChange::Type change);

    signals:
        //! A component has been added to the entity
        /*! Note: when this signal is received on new entity creation, the attributes might not be filled yet!
         */ 
        void ComponentAdded(IComponent* component, AttributeChange::Type change);
        
        //! A component has been removed from the entity
        /*! Note: when this signal is received on new entity creation, the attributes might not be filled yet!
         */ 
        void ComponentRemoved(IComponent* component, AttributeChange::Type change);

        //! Signal when this entity is deleted
        void EntityRemoved(Scene::Entity* entity, AttributeChange::Type change);

    public slots:
        void SerializeToXML(QDomDocument& doc, QDomElement& base_element) const;
//        void DeserializeFromXML(QDomElement& element, AttributeChange::Type change);

        QString SerializeToXMLString() const;
//        bool DeserializeFromXMLString(const QString &src, AttributeChange::Type change);

        IComponent* GetComponentRaw(const QString &type_name) const { return GetComponent(type_name).get(); }
        IComponent* GetComponentRaw(const QString &type_name, const QString &name) const { return GetComponent(type_name, name).get(); }
        
        IComponent* GetOrCreateComponentRaw(const QString &type_name, AttributeChange::Type change = AttributeChange::Default, bool syncEnabled = true) { return GetOrCreateComponent(type_name, change, syncEnabled).get(); }
        IComponent* GetOrCreateComponentRaw(const QString &type_name, const QString &name, AttributeChange::Type change = AttributeChange::Default, bool syncEnabled = true) { return GetOrCreateComponent(type_name, name, change, syncEnabled).get(); }

        void RemoveComponent(const QString &type_name, AttributeChange::Type change = AttributeChange::Default) { RemoveComponent(GetComponent(type_name), change); }
        void RemoveComponent(const QString &type_name, const QString &name, AttributeChange::Type change = AttributeChange::Default) { RemoveComponent(GetComponent(type_name, name), change); }

        //! Returns list of components with type @c type_name or if @c type_name is empty return all components
        //! \param type_name type of the component
        QObjectList GetComponentsRaw(const QString &type_name) const;

        //! Returns whether or not this entity has a component with certain type and name.
        /*! \param type_name Type of the component.
        */
        bool HasComponent(const QString &type_name) const;

        //! This is an overloaded function.
        /*! \param type_has Type name hash of the component.
        */
        bool HasComponent(uint type_hash) const;

        //! Returns whether or not this entity has a component with certain type and name.
        /*! \param type_name type of the component
            \param name name of the component
        */
        bool HasComponent(const QString &type_name, const QString &name) const;

        //! This is an overloaded function.
        /*! \param type_has Type name has of the component.
            \param name name of the component
        */
        bool HasComponent(uint type_hash, const QString &name) const;

        //! Sets name of the entity to EC_Name component. If the component doesn't exist, it will be created.
        /*! @param name Name.
        */
        void SetName(const QString &name);

        //! Returns name of this entity if EC_Name is available, empty string otherwise.
        QString GetName() const;

        //! Sets description of the entity to EC_Name component. If the component doesn't exist, it will be created.
        /*! @param desc Description.
        */
        void SetDescription(const QString &desc);

        //! Returns description of this entity if EC_Name is available, empty string otherwise.
        QString GetDescription() const;

        //! Return by name and type, 'cause can't call RemoveComponent with comp as shared_py
        void RemoveComponentRaw(QObject* comp);

        //! Returns actions map for introspection/reflection.
        const ActionMap &Actions() const { return actions_; }

        /// Creates and registers new action for this entity, or returns an existing action.
        /** Use this function from scripting languages.
            @param name Name of the action.
            @note Never returns null pointer
            @note Never store the returned pointer.
        */
        EntityAction *Action(const QString &name);

        /// Find & Delete EntityAction object from EntityActions map.
        /** Use this function from scripting languages.
            @param name Name of the action.
        */
        void RemoveAction(const QString &name);

        /// Connects action with a specific name to a receiver object with member slot.
        /** @param name Name of the action.
            @param receiver Receiver object.
            @param member Member slot.
        */
        void ConnectAction(const QString &name, const QObject *receiver, const char *member);

        /// Executes an arbitrary action for all components of this entity.
        /** The components may or may not handle the action.
            @param type Execution type, i.e. where the actions is executed. @note Currently int instead of the real enum for javascript compability!
            @param action Name of the action.
            @param p1 1st parameter for the action, if applicable.
            @param p2 2nd parameter for the action, if applicable.
            @param p3 3rd parameter for the action, if applicable.
        */
        void Exec(int /*EntityAction::ExecutionType*/ type, const QString &action, const QString &p1 = "", const QString &p2 = "", const QString &p3 = "");

        /// This is an overloaded function.
        /** @param type Execution type, i.e. where the actions is executed. @note Currently int instead of the real enum for javascript compability!
            @param action Name of the action.
            @param params List of parameters for the action.
        */
        void Exec(int /*EntityAction::ExecutionType*/ type, const QString &action, const QStringList &params);

        /// This is an overloaded function. Experimental overload using QVariant. Converts the variants to strings.
        /** @param type Execution type, i.e. where the actions is executed. @note Currently int instead of the real enum for javascript compability!
            @param action Name of the action.
            @param params List of parameters for the action.
        */
        void Exec(int /*EntityAction::ExecutionType*/ type, const QString &action, const QVariantList &params);

        //! Sets whether entity is temporary. Temporary entities won't be saved when the scene is saved.
        /*! By definition, all components of a temporary entity are temporary as well.
         */
        void SetTemporary(bool enable);

        //! Returns whether entity is temporary. Temporary entities won't be saved when the scene is saved.
        /*! By definition, all components of a temporary entity are temporary as well.
         */
        bool IsTemporary() const { return temporary_; }

        //! Returns if this entity is local
        bool IsLocal() const { return (id_ & LocalEntity) != 0; }

        QString ToString() const;
    private:
        //! constructor
        /*!
            \param framework Framework
            \param scene Scene this entity belongs to
        */
        Entity(Foundation::Framework* framework, SceneManager* scene);

        //! constructor that takes an id for the entity
        /*!
            \param framework Framework
            \param id unique id for the entity.
            \param scene Scene this entity belongs to
        */
        Entity(Foundation::Framework* framework, entity_id_t id, SceneManager* scene);

        //! Set new id
        void SetNewId(entity_id_t id) { id_ = id; }

        //! Set new scene
        void SetScene(SceneManager* scene) { scene_ = scene; }

        /// Validates that the action has receivers. If not, deletes the action and removes it from the registered actions.
        /** @param action Action to be validated.
        */
        bool HasReceivers(EntityAction *action);

        //! Emit a entity deletion signal. Called from SceneManager
        void EmitEntityRemoved(AttributeChange::Type change);

        //! a list of all components
        ComponentVector components_;

        //! Unique id for this entity
        entity_id_t id_;

        //! Pointer to framework
        Foundation::Framework* framework_;

        //! Pointer to scene
        SceneManager* scene_;

        //! Map of registered entity actions.
        ActionMap actions_;

        //! Temporary-flag
        bool temporary_;
   };
}

#endif


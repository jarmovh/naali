/**
 *  For conditions of distribution and use, see copyright notice in license.txt
 *
 *  @file   ComponentGroup.h
 *  @brief  Data structure that hold all information that ECBrowser need to know about the component.
 */

#ifndef incl_ECEditorModule_ComponentGroup_h
#define incl_ECEditorModule_ComponentGroup_h

#include "SceneFwd.h"

class QtProperty;
class QTreeWidgetItem;

class ECComponentEditor;

//! One ComponentGroup data memeber per. ECComponentEditor.
/*! Each component that we want to add to this object need to share exatly same
    attributes as other components in this object's vector.
*/
struct ComponentGroup
{
    ComponentGroup(ComponentPtr component = ComponentPtr(),
                  ECComponentEditor *editor = 0,
                  bool isDynamic = false);

    ~ComponentGroup();

    //! Checks if it's safe to use this component.
    bool IsValid() const { return components_.size() && editor_; }

    //! Checks if components type and name are same.
    /*! Note! If the component is dynamic component attributes are aswell tested
     *  and if they different from each other, mehtod will return a false.
     *  @param component The component we want to compare.
     *  @return Return true if component is identical with component group's items.
     */
    bool IsSameComponent(ComponentPtr component) const;

    //! Fast check for testing if given component is included in this component group.
    bool ContainsComponent(ComponentPtr component) const;

    bool ContainsAttribute(const QString &name) const;

    //! If component isn't already added to component group, add it.
    bool AddComponent(ComponentPtr comp);

    //! Try to find right component from component info and if found remove it from this object and ECComponentEditor.
    /*! If ECComponentEditor doesn't contain any of components editor object will be released.*/
    bool RemoveComponent(ComponentPtr comp);

    bool IsDynamic() const { return isDynamic_; }

    //! Check if spesific QtProperty is owned by this component.
    bool HasRootProperty(QtProperty *property) const;

    std::vector<ComponentWeakPtr> components_;
    ECComponentEditor *editor_;
    QString name_;
    QString typeName_;
    bool isDynamic_;
};

#endif

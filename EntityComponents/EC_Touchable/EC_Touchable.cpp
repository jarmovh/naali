/**
 *  For conditions of distribution and use, see copyright notice in license.txt
 *
 *  @file   EC_Touchable.cpp
 *  @brief  EC_Touchable enables visual effect for scene entity in cases where 
 *          the entity can be considered to have some kind of functionality
 *          when it is clicked i.e. touched. The effect is not visible by default.
 *          You must call Show() function separately. The effect is visible only
 *          for certain time.
 *  @note   The entity must have EC_Placeable and EC_Mesh (if mesh) or
 *          EC_OgreCustomObject (if prim) components available in advance.
 */

#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "EC_Touchable.h"

#include "IModule.h"
#include "Entity.h"
#include "Renderer.h"
//#include "OgreMaterialUtils.h"
#include "EC_Placeable.h"
#include "EC_Mesh.h"
#include "EC_OgreCustomObject.h"
#include "LoggingFunctions.h"

DEFINE_POCO_LOGGING_FUNCTIONS("EC_Touchable");

#include <QApplication>
#include <QTimer>

#include "MemoryLeakCheck.h"

EC_Touchable::~EC_Touchable()
{
    // OgreRendering module might be already deleted. If so, the cloned entity is also already deleted.
    // In this case, just set pointer to 0.
    if (!renderer_.expired())
    {
        Ogre::SceneManager *sceneMgr = renderer_.lock()->GetSceneManager();
        if (entityClone_)
            sceneMgr->destroyEntity(entityClone_);
    }
    else
    {
        entityClone_ = 0;
        sceneNode_ = 0;
    }
}

void EC_Touchable::Show()
{
    if (highlightOnHover.Get())
    {
        if (!entityClone_)
            Create();

        if (!entityClone_)
        {
            LogError("EC_Touchable not initialized properly.");
            return;
        }

        if (entityClone_ && sceneNode_)
            sceneNode_->getAttachedObject(cloneName_)->setVisible(true);
    }
}

void EC_Touchable::Hide()
{
    if (entityClone_ && sceneNode_)
        sceneNode_->getAttachedObject(cloneName_)->setVisible(false);
}

bool EC_Touchable::IsVisible() const
{
    if (entityClone_ && sceneNode_)
        return sceneNode_->getAttachedObject(cloneName_)->isVisible();
    return false;
}

EC_Touchable::EC_Touchable(IModule *module) :
    IComponent(module->GetFramework()),
    entityClone_(0),
    sceneNode_(0),
    materialName(this, "material name", "Touchable"),
    highlightOnHover(this, "highligh on hover", true),
    hoverCursor(this, "hover cursor", Qt::ArrowCursor)
{
    static AttributeMetadata metadata;
    static bool metadataInitialized = false;
    if(!metadataInitialized)
    {
        metadata.enums[Qt::ArrowCursor] = "ArrowCursor";
        metadata.enums[Qt::UpArrow] = "UpArrowCursor";
        metadata.enums[Qt::CrossCursor] = "CrossCursor";
        metadata.enums[Qt::WaitCursor] = "WaitCursor";
        metadata.enums[Qt::IBeamCursor] = "IBeamCursor";
        metadata.enums[Qt::SizeVerCursor] = "SizeVerCursor";
        metadata.enums[Qt::SizeHorCursor] = "SizeHorCursor";
        metadata.enums[Qt::SizeBDiagCursor] = "SizeBDiagCursor";
        metadata.enums[Qt::SizeFDiagCursor] = "SizeFDiagCursor";
        metadata.enums[Qt::SizeAllCursor] = "SizeAllCursor";
        metadata.enums[Qt::BlankCursor] = "BlankCursor";
        metadata.enums[Qt::SplitVCursor] = "SplitVCursor";
        metadata.enums[Qt::SplitHCursor] = "SplitHCursor";
        metadata.enums[Qt::PointingHandCursor] = "PointingHandCursor";
        metadata.enums[Qt::ForbiddenCursor] = "ForbiddenCursor";
        metadata.enums[Qt::WhatsThisCursor] = "WhatsThisCursor";
        metadata.enums[Qt::BusyCursor] = "BusyCursor";
        metadata.enums[Qt::OpenHandCursor] = "OpenHandCursor";
        metadata.enums[Qt::ClosedHandCursor] = "ClosedHandCursor";
        metadataInitialized = true;
    }

    hoverCursor.SetMetadata(&metadata);

    renderer_ = module->GetFramework()->GetServiceManager()->GetService<OgreRenderer::Renderer>(Service::ST_Renderer);
    connect(this, SIGNAL(AttributeChanged(IAttribute*, AttributeChange::Type)), SLOT(UpdateMaterial()));
    connect(this, SIGNAL(ParentEntitySet()), SLOT(RegisterActions()));
//    connect(this, SIGNAL(ParentEntitySet()), SLOT(Create()));
}

void EC_Touchable::Create()
{
    if (renderer_.expired())
        return;

    Scene::Entity *entity = GetParentEntity();
    assert(entity);
    if (!entity)
        return;

    EC_Placeable *placeable = entity->GetComponent<EC_Placeable>().get();
    assert(placeable);
    if (!placeable)
        return;

    // Check out if this entity has EC_Mesh or EC_OgreCustomObject.
    Ogre::Entity *originalEntity  = 0;
    if (entity->GetComponent(EC_Mesh::TypeNameStatic()))
    {
        EC_Mesh *ec_mesh= entity->GetComponent<EC_Mesh>().get();
        assert(ec_mesh);

        originalEntity = ec_mesh->GetEntity();
        sceneNode_ = ec_mesh->GetAdjustmentSceneNode();
    }
    else if(entity->GetComponent(EC_OgreCustomObject::TypeNameStatic()))
    {
        EC_OgreCustomObject *ec_custom = entity->GetComponent<EC_OgreCustomObject>().get();
        assert(ec_custom);
        if (!ec_custom->IsCommitted())
        {
            LogError("Mesh entity have not been created for the target primitive. Cannot create EC_Touchable.");
            return;
        }

        originalEntity = ec_custom->GetEntity();
        sceneNode_ = placeable->GetSceneNode();
    }
    else
    {
        LogError("This entity doesn't have either EC_Mesh or EC_OgreCustomObject present. Cannot create EC_Touchable.");
        return;
    }

    assert(originalEntity && sceneNode_);
    if (!originalEntity || !sceneNode_)
        return;

    // Clone the Ogre entity.
    cloneName_ = renderer_.lock()->GetUniqueObjectName("EC_Touchable_entity");
    entityClone_ = originalEntity->clone(cloneName_);
    assert(entityClone_);

    // Disable casting of shadows for the clone.
    entityClone_->setCastShadows(false);

    ///\todo If original entity has skeleton, (try to) link it to the clone.
/*
    if (originalEntity->hasSkeleton())
    {
        Ogre::SkeletonInstance *skel = originalEntity->getSkeleton();
        // If sharing a skeleton, force the attachment mesh to use the same skeleton
        // This is theoretically quite a scary operation, for there is possibility for things to go wrong
        Ogre::SkeletonPtr entity_skel = originalEntity->getMesh()->getSkeleton();
        if (entity_skel.isNull())
        {
            LogError("Cannot share skeleton for attachment, not found");
        }
        else
        {
            try
            {
                entityClone_->getMesh()->_notifySkeleton(entity_skel);
            }
            catch (Ogre::Exception &e)
            {
                LogError("Could not set shared skeleton for attachment: " + std::string(e.what()));
            }
        }
    }
*/

    try
    {
        //OgreRenderer::CloneMaterial("Touchable", materialName.Get());
        entityClone_->setMaterialName(materialName.Get().toStdString());
    }
    catch (Ogre::Exception &e)
    {
        LogError("Could not set material \"" + materialName.Get().toStdString() + "\": " + std::string(e.what()));
        return;
    }

    sceneNode_->attachObject(entityClone_);
    Hide();
}

void EC_Touchable::UpdateMaterial()
{
    if (!entityClone_ ||!sceneNode_)
        return;
    try
    {
        entityClone_->setMaterialName(materialName.Get().toStdString());
    }
    catch (Ogre::Exception &e)
    {
        LogError("Could not set material \"" + materialName.Get().toStdString() + "\": " + std::string(e.what()));
        return;
    }

    // Remove current cursors when attributes change
    QCursor *current_cursor = QApplication::overrideCursor();
    while (current_cursor)
    {
        QApplication::restoreOverrideCursor();
        current_cursor = QApplication::overrideCursor();
    }
}

///\todo Remove this altogether. -jj. Integrate switching mouse cursors to input contexts so that 
/// it is not possible to "miss" calling restoreOverrideCursor and leak a cursor to stay on screen. Releasing a context
/// also releases the cursor.
void EC_Touchable::SetCursorVisible(bool visible) 
{
    if (hoverCursor.Get() != Qt::ArrowCursor)
    {
        QCursor *current_cursor = QApplication::overrideCursor();
        if (visible)
        {
            if (current_cursor)
            {
                if (current_cursor->shape() != hoverCursor.Get())
                    QApplication::setOverrideCursor(QCursor((Qt::CursorShape)(hoverCursor.Get())));
            }
            else
                QApplication::setOverrideCursor(QCursor((Qt::CursorShape)(hoverCursor.Get())));
        }
        else
        {
            while (current_cursor)
            {
                QApplication::restoreOverrideCursor();
                current_cursor = QApplication::overrideCursor();
            }
        }
    }
}

void EC_Touchable::OnHoverIn()
{
    SetCursorVisible(true);
    emit MouseHoverIn();
}

void EC_Touchable::OnHover()
{
    if (!IsVisible())
        Show();
    emit MouseHover();
}

void EC_Touchable::OnHoverOut()
{
    if (IsVisible())
        Hide();

    SetCursorVisible(false);
    emit MouseHoverOut();
}

void EC_Touchable::OnClick()
{
    emit MousePressed();
}

void EC_Touchable::RegisterActions()
{
    Scene::Entity *entity = GetParentEntity();
    assert(entity);
    if (entity)
    {
        // Generic actions
        entity->ConnectAction("Show", this, SLOT(Show()));
        entity->ConnectAction("Hide", this, SLOT(Hide()));
        // Mouse actions
        entity->ConnectAction("MouseHover", this, SLOT(OnHover()));
        entity->ConnectAction("MouseHoverIn", this, SLOT(OnHoverIn()));
        entity->ConnectAction("MouseHoverOut", this, SLOT(OnHoverOut()));
        entity->ConnectAction("MousePress", this, SLOT(OnClick()));
    }
}


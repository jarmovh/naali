/**
 *  For conditions of distribution and use, see copyright notice in license.txt
 *
 *  @file   EC_SoundRuler.h
 *  @brief  EC_SoundRuler enables visual sound properties effect for scene entity.
 *  @note   The entity must have EC_Placeable and EC_OpenSimPrim 
 *  @author Nathan Letwory | http://www.letworyinteractive.com
 */

#include "StableHeaders.h"
#include "DebugOperatorNew.h"
#include "EC_SoundRuler.h"
#include "IModule.h"
#include "Entity.h"
#include "Renderer.h"
#include "OgreMaterialUtils.h"
#include "EC_Placeable.h"
#include "EC_Mesh.h"
#include "EC_OgreCustomObject.h"
#include "EC_OpenSimPrim.h"
#include "LoggingFunctions.h"
#include "RexUUID.h"
#include <Ogre.h>

DEFINE_POCO_LOGGING_FUNCTIONS("EC_SoundRuler")

#include "MemoryLeakCheck.h"

EC_SoundRuler::EC_SoundRuler(IModule *module) :
    IComponent(module->GetFramework()),
    radiusAttr_(this, "radius", 5),
    volumeAttr_(this, "volume", 5),
    segmentsAttr_(this, "segments", 29),
    rulerObject(0),
    sceneNode_(0)
{
    renderer_ = module->GetFramework()->GetServiceManager()->GetService<OgreRenderer::Renderer>(Service::ST_Renderer);
    
    QObject::connect(this, SIGNAL(AttributeChanged(IAttribute*, AttributeChange::Type)), this, SLOT(UpdateSoundRuler()));
    
    RexUUID uuid = RexUUID::CreateRandom();
    rulerName = uuid.ToString() + "ruler";
    nodeName = uuid.ToString() + "node";
}

EC_SoundRuler::~EC_SoundRuler()
{
    // OgreRendering module might be already deleted. If so, the cloned entity is also already deleted.
    // In this case, just set pointer to 0.
    if (!renderer_.expired() && rulerObject)
    {
        Ogre::SceneManager *sceneMgr = renderer_.lock()->GetSceneManager();
            sceneMgr->destroyManualObject(rulerObject);
    }
    else
    {
        rulerObject = 0;
        sceneNode_ = 0;
    }
}

void  EC_SoundRuler::Show()
{
    if (!rulerObject)
        Create();

    if (!rulerObject)
    {
        LogError("EC_SoundRuler not initialized properly.");
        return;
    }

    rulerObject->setVisible(true);
}

void  EC_SoundRuler::Hide()
{
    if (!rulerObject)
        Create();
        
    if (!rulerObject)
    {
        LogError("EC_SoundRuler not initialized properly.");
        return;
    }
    
    rulerObject->setVisible(false);
}

bool EC_SoundRuler::IsVisible() const
{
    if (rulerObject)
        return rulerObject->isVisible();

    return false;
}

void EC_SoundRuler::Create()
{
    if (renderer_.expired())
        return;

    Ogre::SceneManager *scene_mgr= renderer_.lock()->GetSceneManager();
    assert(scene_mgr);
    if (!scene_mgr)
        return;

    Scene::Entity *entity = GetParentEntity();
    assert(entity);
    if (!entity)
        return;

    EC_Placeable *placeable = entity->GetComponent<EC_Placeable>().get();
    assert(placeable);
    if (!placeable)
        return;
    sceneNode_ = placeable->GetSceneNode();

    assert(sceneNode_);
    if (!sceneNode_)
        return;
    
    if(scene_mgr->hasManualObject(rulerName)) {
        rulerObject = scene_mgr->getManualObject(rulerName);
        if(rulerObject->isAttached())
#if OGRE_VERSION_MINOR <= 6 && OGRE_VERSION_MAJOR <= 1
            rulerObject->detatchFromParent();
#else
            rulerObject->detachFromParent();
#endif
    } else {
        rulerObject = scene_mgr->createManualObject(rulerName);
    }
    
    SetupSoundRuler();

    // get soundRulerNode only when we are working in world space
    if(scene_mgr->hasSceneNode(nodeName)) {
        globalSceneNode = scene_mgr->getSceneNode(nodeName);
    } else {
        globalSceneNode = scene_mgr->getRootSceneNode()->createChildSceneNode(nodeName);
        globalSceneNode->setVisible(true);
    }
    assert(globalSceneNode);
    if(!globalSceneNode)
        return;

    globalSceneNode->setPosition(sceneNode_->getParent()->getPosition());
    globalSceneNode->attachObject(rulerObject);
}

void EC_SoundRuler::SetupSoundRuler()
{
    if(!rulerObject)
        return;
        
    Scene::Entity *entity = GetParentEntity();
    assert(entity);
    if (!entity)
        return;
        
    EC_OpenSimPrim *prim = entity->GetComponent<EC_OpenSimPrim>().get();
    assert(prim);
    if(!prim)
        return;
        
    float const radius = prim->SoundRadius;
    float const volume = prim->SoundVolume;
    float const segments = segmentsAttr_.Get();
    unsigned int const spikeInterval = (unsigned int)(segments / 4.0);

    rulerObject->clear();
    rulerObject->setCastShadows(false);
    rulerObject->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_LINE_STRIP);

    unsigned int i = 0;
    for(float theta = 0; theta <= 2 * Ogre::Math::PI; theta += Ogre::Math::PI / segments) {
        rulerObject->position(radius * cos(theta), radius * sin(theta), 0);
        if(i % spikeInterval == 0) {
            rulerObject->position(radius * cos(theta), radius * sin(theta), volume);
            rulerObject->position(radius * cos(theta), radius * sin(theta), 0);
        }
        i++;
    }
    rulerObject->end();
    
    rulerObject->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_LINE_STRIP);
    for(float theta = 0; theta <= 2 * Ogre::Math::PI; theta += Ogre::Math::PI / segments) {
        rulerObject->position(0, radius * cos(theta), radius * sin(theta));
    }
    rulerObject->end();
    
    rulerObject->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_LINE_STRIP);
    for(float theta = 0; theta <= 2 * Ogre::Math::PI; theta += Ogre::Math::PI / segments) {
        rulerObject->position(radius * cos(theta), 0, radius * sin(theta));
    }
    rulerObject->end();
}

void EC_SoundRuler::UpdateSoundRuler() {
    Create();
    Show();
}


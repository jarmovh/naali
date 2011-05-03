/**
 *  For conditions of distribution and use, see copyright notice in license.txt
 *
 *  @file   EC_SoundListener.cpp
 *  @brief  Entity-component which provides sound listener position for in-world 3D audio.
 *          Updates parent entity's placeable component's position to the sound service each frame.
 *  @note   Only one entity can have active sound listener at a time.
 */

#include "StableHeaders.h"
#include "DebugOperatorNew.h"
#include "MemoryLeakCheck.h"
#include "EC_SoundListener.h"
#include "IModule.h"
#include "Entity.h"
#include "EC_Placeable.h"
#include "SceneManager.h"
#include "LoggingFunctions.h"
#include "AudioAPI.h"
#include "SceneAPI.h"
#include "FrameAPI.h"

DEFINE_POCO_LOGGING_FUNCTIONS("EC_SoundListener")

EC_SoundListener::EC_SoundListener(IModule *module):
    IComponent(module->GetFramework()),
    active(this, "active", false)
{
    // By default, this component is NOT network-serialized
    SetNetworkSyncEnabled(false);

    connect(this, SIGNAL(ParentEntitySet()), SLOT(RetrievePlaceable()));
    connect(GetFramework()->Frame(), SIGNAL(Updated(float)), SLOT(Update()));
    connect(this, SIGNAL(AttributeChanged(IAttribute*, AttributeChange::Type)), SLOT(OnActiveChanged()));
    connect(this, SIGNAL(ParentEntitySet()), SLOT(RegisterActions()));
}

EC_SoundListener::~EC_SoundListener()
{
}

void EC_SoundListener::RetrievePlaceable()
{
    if (!GetParentEntity())
        LogError("Couldn't find an parent entity for EC_SoundListener. Cannot retrieve placeable component.");

    placeable_ = GetParentEntity()->GetComponent<EC_Placeable>();
    if (!placeable_.lock())
        LogError("Couldn't find an EC_Placeable component from the parent entity.");
}

void EC_SoundListener::Update()
{
    if (active.Get() && !placeable_.expired())
        GetFramework()->Audio()->SetListener(placeable_.lock()->GetPosition(), placeable_.lock()->GetOrientation());
}

void EC_SoundListener::OnActiveChanged()
{
    Scene::ScenePtr scene = GetFramework()->Scene()->GetDefaultScene();
    if (!scene)
    {
        LogError("Failed on OnActiveChanged method cause default world scene wasn't set.");
        return;
    }

    if (active.Get())
    {
        // Disable all the other listeners, only one can be active at a time.
        ///\todo Maybe not the most sophisticated way to handle this here; do this in RexLogicModule maybe.
        Scene::EntityList listeners = scene->GetEntitiesWithComponent("EC_SoundListener");
        foreach(Scene::EntityPtr listener, listeners)
        {
            EC_SoundListener *ec = listener->GetComponent<EC_SoundListener>().get();
            if (ec != this)
                listener->GetComponent<EC_SoundListener>()->active.Set(false, AttributeChange::Default);
        }
    }
}

void EC_SoundListener::RegisterActions()
{
    Scene::Entity *entity = GetParentEntity();
    assert(entity);
    if (entity)
    {
        entity->ConnectAction("Active", this, SLOT(OnActiveChanged()));
    }
    else
    {
        LogError("Fail to register actions cause component's parent entity is null.");
    }
}

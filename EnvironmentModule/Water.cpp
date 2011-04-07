// For conditions of distribution and use, see copyright notice in license.txt

/// @file Water.cpp
/// @brief Manages Water-related Rex logic.

#include "StableHeaders.h"
#include "DebugOperatorNew.h"
#include "MemoryLeakCheck.h"
#include "Water.h"
#include "EnvironmentModule.h"
#include "SceneAPI.h"
#include "Entity.h"

// Ogre renderer -specific.
#include <OgreManualObject.h>
#include <OgreSceneManager.h>
#include <OgreMaterialManager.h>
#include <OgreTextureManager.h>
#include <OgreIteratorWrappers.h>
#include <OgreTechnique.h>

#include "EC_Placeable.h"
#include "EC_WaterPlane.h"
#include "Renderer.h"
#include "OgreMaterialUtils.h"
#include "SceneManager.h"


namespace Environment
{

Water::Water(EnvironmentModule *owner) :
    owner_(owner) 
{
}

Water::~Water()
{
    owner_ = 0;
}

void Water::CreateWaterGeometry(float height, AttributeChange::Type change)
{
    // Is there entity
    EC_WaterPlane* plane = GetEnvironmentWaterComponent();
    if ( plane == 0)
    {
         QString name = "WaterEnvironment";
         owner_->CreateEnvironmentEntity(name, EC_WaterPlane::TypeNameStatic()); 
         plane = GetEnvironmentWaterComponent();
         if ( plane == 0)
             return;
    }

    Vector3df vec = plane->position.Get();
    vec.z = height;
    plane->position.Set(vec, change);
}

void Water::RemoveWaterGeometry()
{
    Scene::ScenePtr active_scene = owner_->GetFramework()->Scene()->GetDefaultScene();
    Scene::Entity* entity = active_scene->GetEntityByName("WaterEnvironment").get();
    if ( entity == 0)
    {
       entity = active_scene->GetEntityByName("LocalEnvironment").get();
       if ( entity == 0)
           return;
    }

    entity->RemoveComponent(entity->GetComponent(EC_WaterPlane::TypeNameStatic()),AttributeChange::Default);  
}

void Water::Update()
{
    // This is a hack, because of environment entity comes after then RexWater- package. We create LocalEnviroment-entity. 
    // Now we need to update scene, if there exist a real environment entity, so that we do not get two ocean water planes. 
    EC_WaterPlane* plane = GetEnvironmentWaterComponent();
    UNREFERENCED_PARAM(plane);

    if (IsWaterPlane())
        emit ExistWater(true);
    else
        emit ExistWater(false);
}

void Water::SetWaterHeight(float height, AttributeChange::Type type)
{
    EC_WaterPlane* plane = GetEnvironmentWaterComponent();
    if ( plane == 0)
        return;

    Vector3df vec = plane->position.Get();
    vec.z = height;
    plane->position.Set(vec, type);
    //plane->ComponentChanged(type);
    emit HeightChanged(static_cast<double>(height));
}

EC_WaterPlane* Water::GetEnvironmentWaterComponent()
{
    Scene::ScenePtr active_scene = owner_->GetFramework()->Scene()->GetDefaultScene();
    Scene::Entity* entity = active_scene->GetEntityByName("WaterEnvironment").get();
    if (entity != 0 )
        owner_->RemoveLocalEnvironment();
    else
    {
        entity =  active_scene->GetEntityByName("LocalEnvironment").get();
        if ( entity == 0)
            return 0;
    }

    return entity->GetComponent<EC_WaterPlane >().get();
}

bool Water::IsWaterPlane() const
{
    Scene::ScenePtr active_scene = owner_->GetFramework()->Scene()->GetDefaultScene();
    Scene::Entity* entity = active_scene->GetEntityByName("WaterEnvironment").get();
    if ( entity == 0)
    {
        entity = active_scene->GetEntityByName("LocalEnvironment").get();
        if ( entity == 0)
            return false;
    }

    if (entity->GetComponent<EC_WaterPlane >().get() != 0)
        return true;

    return false;
}

float Water::GetWaterHeight()
{
    EC_WaterPlane* plane = GetEnvironmentWaterComponent();
    if (plane == 0)
        return 0.0;

    return plane->position.Get().z;
}

void Water::SetWaterFog(float fogStart, float fogEnd, const QVector<float>& color)
{
    EC_WaterPlane* plane = GetEnvironmentWaterComponent();
    if (plane == 0)
        return;

    plane->fogStartDistance.Set(fogStart,AttributeChange::Default);
    plane->fogEndDistance.Set(fogEnd, AttributeChange::Default);
    plane->fogColor.Set(Color(color[0], color[1], color[2],1.0), AttributeChange::Default);
    //plane->ComponentChanged(AttributeChange::Default);

    emit WaterFogAdjusted(fogStart, fogEnd, color);
}

QVector<float> Water::GetFogWaterColor()
{
    EC_WaterPlane* plane = GetEnvironmentWaterComponent();
    if (plane == 0)
        return QVector<float>();

    Ogre::ColourValue color = plane->GetFogColorAsOgreValue();
    QVector<float> vec; 
    vec<<color[0]<<color[1]<<color[2];
    return vec;
}

void Water::SetWaterFogColor(const QVector<float>& color)
{
   EC_WaterPlane* plane = GetEnvironmentWaterComponent();
    if (plane == 0)
        return;

   Color col(color[0], color[1], color[2],1.0);
   plane->fogColor.Set(col, AttributeChange::Default); 
   //plane->ComponentChanged(AttributeChange::Default);
}

void Water::SetWaterFogDistance(float fogStart, float fogEnd)
{
    EC_WaterPlane* plane = GetEnvironmentWaterComponent();
    if (plane == 0)
        return;

    plane->fogStartDistance.Set(fogStart, AttributeChange::Default);
    plane->fogEndDistance.Set(fogEnd, AttributeChange::Default);
    //plane->ComponentChanged(AttributeChange::Default);
}

float Water::GetWaterFogStartDistance() 
{
    EC_WaterPlane* plane = GetEnvironmentWaterComponent();
    return plane->fogStartDistance.Get();
}

float Water::GetWaterFogEndDistance() 
{
    EC_WaterPlane* plane = GetEnvironmentWaterComponent();
    return plane->fogEndDistance.Get();
}

}

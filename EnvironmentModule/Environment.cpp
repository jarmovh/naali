/**
 *  For conditions of distribution and use, see copyright notice in license.txt
 *
 *  @file   Environment.cpp
 *  @brief  Manages environment-related reX-logic, e.g. world time and lighting.
 */

#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "Environment.h"
#include "EnvironmentModule.h"
#include "EC_WaterPlane.h"
#include "EC_Fog.h"

#include "EC_OgreEnvironment.h"
#include "SceneManager.h"
#include "SceneAPI.h"
#include "Entity.h"
#include "NetworkMessages/NetInMessage.h"
#include "Renderer.h"

#ifdef CAELUM
#include <Caelum.h>
#endif

#include "MemoryLeakCheck.h"

namespace Environment
{

/// Utility tool for clamping fog distance
void ClampFog(float& start, float& end, float farclip)
{
    if (farclip < 10.0) 
        farclip = 10.0;
    if (end > farclip - 10.0)
        end = farclip - 10.0;
    if (start > farclip/3.0)
        start = farclip/3.0;
}

Environment::Environment(EnvironmentModule *owner) :
    owner_(owner),
    activeEnvEntity_(Scene::EntityWeakPtr()),
    time_override_(false),
    usecSinceStart_(0),
    secPerDay_(0),
    secPerYear_(0),
    sunPhase_(0.0)
{

#ifdef CAELUM
    caelumComponents_ = Caelum::CaelumSystem::CAELUM_COMPONENTS_NONE;
    caelumComponents_ = caelumComponents_ |
        Caelum::CaelumSystem::CAELUM_COMPONENT_SKY_DOME |
        Caelum::CaelumSystem::CAELUM_COMPONENT_MOON |
        Caelum::CaelumSystem::CAELUM_COMPONENT_SUN |
        Caelum::CaelumSystem::CAELUM_COMPONENT_POINT_STARFIELD |
        Caelum::CaelumSystem::CAELUM_COMPONENT_SCREEN_SPACE_FOG |
        Caelum::CaelumSystem::CAELUM_COMPONENT_GROUND_FOG;
#endif
}

Environment::~Environment()
{
    if ( activeEnvEntity_.lock().get() != 0)
    {
        Scene::EntityPtr entity = activeEnvEntity_.lock();
        if ( entity != 0)
        {
            // Remove entity from scene.
            Scene::SceneManager* manager = entity->GetScene();
            if ( manager != 0)
                manager->RemoveEntity(entity->GetId());
        }

        entity.reset();
    }

    activeEnvEntity_.reset();
    owner_ = 0;
}

EC_OgreEnvironment* Environment::GetEnvironmentComponent()
{
    if (activeEnvEntity_.expired())
        return 0;

    return activeEnvEntity_.lock()->GetComponent<EC_OgreEnvironment>().get();  
}

void Environment::CreateEnvironment()
{
    Scene::ScenePtr active_scene = owner_->GetFramework()->Scene()->GetDefaultScene();
    Scene::EntityPtr entity = active_scene->CreateEntity(active_scene->GetNextFreeIdLocal());

    // The environment entity exists due to legacy from Taiga. It is bad design to create the entity here.
    entity->SetTemporary(true);
    active_scene->EmitEntityCreated(entity);
    activeEnvEntity_ = entity;

    // Creates Caelum component!!
    entity->AddComponent(owner_->GetFramework()->GetComponentManager()->CreateComponent("EC_OgreEnvironment"));
    
    // Creates default fog component
    
    // Does there exist allready Enviroment-entity?
    //Scene::ScenePtr active_scene = owner_->GetFramework()->Scene()->GetDefaultScene();
    
    Scene::Entity* enviroment = active_scene->GetEntityByName("FogEnvironment").get();
    
    if (enviroment != 0 )
    {
        owner_->RemoveLocalEnvironment();
        
        // Has it allready EC_Fog ?
        if ( !enviroment->HasComponent(EC_Fog::TypeNameStatic()))
            enviroment->AddComponent(owner_->GetFramework()->GetComponentManager()->CreateComponent(EC_Fog::TypeNameStatic())); 
    }
    else
    {
        enviroment =  active_scene->GetEntityByName("LocalEnvironment").get();
        if ( enviroment != 0 && !enviroment->HasComponent(EC_Fog::TypeNameStatic()))
            enviroment->AddComponent(owner_->GetFramework()->GetComponentManager()->CreateComponent(EC_Fog::TypeNameStatic())); 
    }
}

bool Environment::HandleSimulatorViewerTimeMessage(ProtocolUtilities::NetworkEventInboundData *data)
{
    ProtocolUtilities::NetInMessage &msg = *data->message;
    msg.ResetReading();

    // SimulatorViewerTimeMessage seems to be corrupted with 0.4 & 0.5 servers.
    try
    {
        usecSinceStart_ = (time_t)msg.ReadU64();
        secPerDay_ = msg.ReadU32();
        secPerYear_ = msg.ReadU32();
        sunDirection_ = msg.ReadVector3();
        sunPhase_ = msg.ReadF32();
        sunAngVelocity_ = msg.ReadVector3();
    }
    catch(NetMessageException &)
    {
        //! todo crashes with 0.4 server add error message.
    }
    
    // Calculate time of day from sun phase, which seems the most reliable way to do it
    float dayphase;
    
    if (sunPhase_ < 1.25 * PI)
        dayphase = (sunPhase_ / PI + 1.0) * 0.33333333;
    else
        dayphase = -0.5 + (sunPhase_ / PI);

    
    EC_OgreEnvironment* env = GetEnvironmentComponent();
    if (!env)
        return false; 

    // Update the sunlight direction and angle velocity.
    ///\note Not needed anymore as we use Caelum now.
    //EC_OgreEnvironment &env = *checked_static_cast<EC_OgreEnvironment*>
    //    (component.get());
    //env.SetSunDirection(-sunDirection_);

    if (!time_override_)
    {
        EC_EnvironmentLight* light = GetEnvironmentLight();
        if ( light != 0 )
        {
            if ( light->fixedTimeAttr.Get() == false)
                light->currentTimeAttr.Set(dayphase, AttributeChange::LocalOnly);
        }
        else if ( light == 0)
        {
            // Create EC_EnvironmentLight 
            QString name = "LightEnvironment";
            owner_->CreateEnvironmentEntity(name, EC_EnvironmentLight::TypeNameStatic()); 
            light = GetEnvironmentLight();
            if ( light->fixedTimeAttr.Get() == false)
                light->currentTimeAttr.Set(dayphase, AttributeChange::LocalOnly);
        }
        //else
        //    env->SetTime(dayphase);
    }
    return false;
}

EC_EnvironmentLight* Environment::GetEnvironmentLight()
{
    Scene::ScenePtr active_scene = owner_->GetFramework()->Scene()->GetDefaultScene();
    Scene::Entity* entity = active_scene->GetEntityByName("LightEnvironment").get();
    
    if (entity != 0 )
    {
        owner_->RemoveLocalEnvironment();
        if ( !entity->HasComponent(EC_EnvironmentLight::TypeNameStatic()) )
            entity->AddComponent(owner_->GetFramework()->GetComponentManager()->CreateComponent(EC_EnvironmentLight::TypeNameStatic()));
    }
    else
    {
        entity =  active_scene->GetEntityByName("LocalEnvironment").get();
        
        if ( entity == 0 || !entity->HasComponent(EC_EnvironmentLight::TypeNameStatic()))
        {
            ///todo Search first entity which has light environment component! this might be slow, work around somehow?
            Scene::EntityList lst = active_scene->GetEntitiesWithComponent(EC_EnvironmentLight::TypeNameStatic());
            if ( lst.size() == 0 )
                return 0;
            
            entity = lst.front().get();
        }
        
       
    }

    return entity->GetComponent<EC_EnvironmentLight >().get();
}

void Environment::SetGroundFog(float fogStart, float fogEnd, const QVector<float>& color)
{
    EC_Fog *fog = GetEnvironmentFog();
    if ( fog == 0)
       return;

    fog->startDistance.Set(fogStart,AttributeChange::Default);
    fog->endDistance.Set(fogEnd, AttributeChange::Default);
    fog->color.Set(Color(color[0], color[1], color[2], 1.0), AttributeChange::Default);
    //fog->ComponentChanged(AttributeChange::Default);

    emit GroundFogAdjusted(fogStart, fogEnd, color);
}

void Environment::SetFogColorOverride(bool enabled)
{
    EC_Fog *fog = GetEnvironmentFog();
    if (fog)
        fog->use.Set(enabled, AttributeChange::Default);
}

bool Environment::GetFogColorOverride() 
{
    EC_Fog *fog = GetEnvironmentFog();
    return fog ? fog->use.Get() : false;
}

QVector<float> Environment::GetFogGroundColor()
{
   EC_Fog *fog = GetEnvironmentFog();
   if ( fog == 0)
    return QVector<float>();

    Ogre::ColourValue color = fog->GetColorAsOgreValue();
    return QVector<float>(QVector<float>() << color[0] <<color[1] <<color[2]);
}

void Environment::Update(f64 frametime)
{
    PROFILE(Environment_Update);

    // We are still little depend of old EC_OgreEnvironment component,
    /// todo refactor away depency of old EC_OgreEnvironmen component 
    EC_OgreEnvironment* env = GetEnvironmentComponent();

    EC_EnvironmentLight* light = GetEnvironmentLight();
    if ( light != 0)
    {
        light->Update(frametime);
    }
    else
    {
       
        if (!env)
            return;
        // Currently updates other then water and fog.
        env->UpdateVisualEffects(frametime);
    }

#ifdef CAELUM
     if (!env)
        return;
    Caelum::CaelumSystem* caelumSystem = env->GetCaelum();
    if (!caelumSystem)
        return;
#endif

    boost::shared_ptr<OgreRenderer::Renderer> renderer = owner_->GetFramework()->GetService<OgreRenderer::Renderer>(Service::ST_Renderer).lock();
    if (!renderer)
       return;

    Ogre::Camera *camera = renderer.get()->GetCurrentCamera();
    Ogre::Viewport *viewport = renderer.get()->GetViewport();
    if (!viewport)
        return;
    Ogre::SceneManager *sceneManager = renderer.get()->GetSceneManager();
    float cameraFarClip = renderer.get()->GetViewDistance();
    
     
    // Go through all water components.

    Scene::ScenePtr scene = owner_->GetFramework()->Scene()->GetDefaultScene();
    Scene::EntityList lst = scene->GetEntitiesWithComponent(EC_WaterPlane::TypeNameStatic());
    Scene::EntityList::iterator iter = lst.begin();

    bool underWater = false;
    EC_WaterPlane* plane = 0;
    for (; iter != lst.end(); ++iter)
    {
        Scene::EntityPtr ent = *iter;
        Scene::Entity* e = ent.get();
        plane = e->GetComponent<EC_WaterPlane >().get();
        if ( plane != 0 && plane->IsCameraInsideWaterCube() )
        {
            underWater = true;
            break;
        }
    }

    // Ground fog value. 
    // Caelum calculates this value depending of time ( sun position? )
    Ogre::ColourValue fogColor;
    EC_Fog* fog = GetEnvironmentFog();

#ifdef CAELUM
       fogColor = caelumSystem->getGroundFog()->getColour();
       if ( fog != 0 && fog->use.Get() )
           fogColor = fog->GetColorAsOgreValue();
#else
    if (fog != 0)
        fogColor = fog->GetColorAsOgreValue();
#endif 
    if (underWater)
    {
       // We're below the water.
       float fogStart = plane->fogStartDistance.Get();
       float fogEnd = plane->fogEndDistance.Get();
       float farClip = fogEnd+10.f;
       Ogre::FogMode mode = static_cast<Ogre::FogMode>(plane->fogMode.Get());
       
       if (farClip > cameraFarClip)
           farClip = cameraFarClip;

       ClampFog(fogStart, fogEnd, farClip);
#ifdef CAELUM
            // Hide the Caelum subsystems.
            caelumSystem->forceSubcomponentVisibilityFlags(Caelum::CaelumSystem::CAELUM_COMPONENTS_NONE);
#endif    
            Color col = plane->fogColor.Get();
            Ogre::ColourValue color = plane->GetFogColorAsOgreValue();
            
            //@note default values are 0.2f, 0.4f, 0.35f
            sceneManager->setFog(mode, fogColor * color, 0.001f, fogStart, fogEnd);
            viewport->setBackgroundColour(fogColor *  color);
            camera->setFarClipDistance(farClip);
    }
    else
    {
        float fogStart = 100.f;
        float fogEnd = 2000.f;
        Ogre::FogMode mode = Ogre::FOG_LINEAR;
        if ( fog != 0 )
        {
            fogStart = fog->startDistance.Get();
            fogEnd = fog->endDistance.Get();
            mode = static_cast<Ogre::FogMode>(fog->mode.Get());
        }
          
        ClampFog(fogStart, fogEnd, cameraFarClip);
#ifdef CAELUM
        caelumSystem->forceSubcomponentVisibilityFlags(caelumComponents_);
#endif
        sceneManager->setFog(mode, fogColor, 0.001f, fogStart, fogEnd);
        viewport->setBackgroundColour(fogColor);
        camera->setFarClipDistance(cameraFarClip);
    }
}

EC_Fog* Environment::GetEnvironmentFog()
{
    Scene::ScenePtr active_scene = owner_->GetFramework()->Scene()->GetDefaultScene();
    Scene::Entity* entity = active_scene->GetEntityByName("FogEnvironment").get();
    if (entity != 0 )
    {
        owner_->RemoveLocalEnvironment();
        if ( !entity->HasComponent(EC_Fog::TypeNameStatic()) )
            entity->AddComponent(owner_->GetFramework()->GetComponentManager()->CreateComponent(EC_Fog::TypeNameStatic()));
    }
    else
    {
        entity =  active_scene->GetEntityByName("LocalEnvironment").get();
        
        if ( entity == 0 || !entity->HasComponent(EC_Fog::TypeNameStatic()))
        {
            ///todo Search first entity which has light environment component! this might be slow, work around somehow?
            Scene::EntityList lst = active_scene->GetEntitiesWithComponent(EC_Fog::TypeNameStatic());
            if ( lst.size() == 0 )
                return 0;
            
            entity = lst.front().get();
        }
    }

    return entity->GetComponent<EC_Fog >().get();
}

bool Environment::IsCaelum()
{
    EC_OgreEnvironment* env = GetEnvironmentComponent();
    if (!env)
        return false;

    return env->IsCaelumUsed();
}

void Environment::SetGroundFogColor(const QVector<float>& color)
{
   EC_Fog *fog = GetEnvironmentFog();

   if ( fog == 0)
    return;

   fog->color.Set(Color(color[0], color[1], color[2], 1.0), AttributeChange::Default);
   //fog->ComponentChanged(AttributeChange::Default);
}

void Environment::SetGroundFogDistance(float fogStart, float fogEnd)
{
   EC_Fog *fog = GetEnvironmentFog();

   if ( fog == 0)
    return;

    fog->startDistance.Set(fogStart, AttributeChange::Default);
    fog->endDistance.Set(fogEnd, AttributeChange::Default);
    //fog->ComponentChanged(AttributeChange::Default);
}

float Environment::GetGroundFogStartDistance()
{
   EC_Fog *fog = GetEnvironmentFog();

   if ( fog == 0)
    return 0.f;
    
    return fog->startDistance.Get();
}

float Environment::GetGroundFogEndDistance()
{
   EC_Fog *fog = GetEnvironmentFog();

   if ( fog == 0)
    return 0.f;
 
    return fog->endDistance.Get();
}

void Environment::SetSunDirection(const QVector<float>& vector)
{
     // Assure that we do not given too near of zero vector values, a la HACK. 
    float squaredLength = vector[0] * vector[0] + vector[1]* vector[1] + vector[2] * vector[2];
    // Length must be different then zero, so we say that value must be higher then our tolerance. 
    float tolerance = 0.001f;
   
    if ( squaredLength < tolerance) 
        return;

    EC_EnvironmentLight* light = GetEnvironmentLight();
    if ( light != 0)
    {
        light->sunDirectionAttr.Set(Vector3df(vector[0], vector[1], vector[2]), AttributeChange::Default);
        return;
    }

    EC_OgreEnvironment* env = GetEnvironmentComponent();
    if (!env)
        return;

    env->SetSunDirection(Vector3df(vector[0], vector[1], vector[2]));
}

QVector<float> Environment::GetSunDirection() 
{
    EC_EnvironmentLight* light = GetEnvironmentLight();
    if ( light != 0)
    {
        Vector3df vec = light->sunDirectionAttr.Get();
        QVector<float> vector;
        vector.append(vec.x), vector.append(vec.y), vector.append(vec.z);
        return vector;
    }

    EC_OgreEnvironment* env = GetEnvironmentComponent();
    
    if (!env)
        return QVector<float>(3);

    Vector3df vec = env->GetSunDirection();
    QVector<float> vector;
    vector.append(vec.x), vector.append(vec.y), vector.append(vec.z);
    return vector;
    
}

void Environment::SetSunColor(const QVector<float>& vector)
{
    Color color;
    if (vector.size() == 4)
       color = Color(vector[0], vector[1], vector[2], vector[3]);
    else if ( vector.size() == 3 )
      color = Color(vector[0], vector[1], vector[2], 1.0);

    EC_EnvironmentLight* light = GetEnvironmentLight();
    if (light != 0)
    {
        light->sunColorAttr.Set(color, AttributeChange::Default);
        return;
    }

    EC_OgreEnvironment* env = GetEnvironmentComponent();
    if (!env)
        return;

    env->SetSunColor(color);
}

QVector<float> Environment::GetSunColor()
{
    EC_EnvironmentLight* light = GetEnvironmentLight();
    
    if ( light != 0)
    {
       Color color =  light->sunColorAttr.Get();
       
       QVector<float> vec(4);
       vec[0] = color.r, vec[1] = color.g, vec[2] = color.b, vec[3] = color.a;
       return vec;
    }

    EC_OgreEnvironment* env = GetEnvironmentComponent();
    if (!env)
        return QVector<float>(4);

    Color color = env->GetSunColor();
    QVector<float> vec(4);
    vec[0] = color.r, vec[1] = color.g, vec[2] = color.b, vec[3] = color.a;
    return vec;
}

QVector<float> Environment::GetAmbientLight()
{
    EC_EnvironmentLight* light = GetEnvironmentLight();
    if ( light != 0)
    {
        Color color = light->ambientColorAttr.Get();
        QVector<float> vec(3);
        vec[0] = color.r, vec[1] = color.g, vec[2] = color.b;
        return vec;
    }
    
    EC_OgreEnvironment* env = GetEnvironmentComponent();
    if (!env)
        return QVector<float>(3);

   Color color = env->GetAmbientLightColor(); 
   QVector<float> vec(3);
   vec[0] = color.r, vec[1] = color.g, vec[2] = color.b;
   return vec;
}

void Environment::SetAmbientLight(const QVector<float>& vector)
{
    EC_EnvironmentLight* light = GetEnvironmentLight();
    if ( light != 0)
    {
        light->ambientColorAttr.Set(Color(vector[0], vector[1], vector[2]), AttributeChange::Default);
        return;
    }

    EC_OgreEnvironment* env = GetEnvironmentComponent();
    if (!env)
        return;

    env->SetAmbientLightColor(Color(vector[0], vector[1], vector[2]));
}

}

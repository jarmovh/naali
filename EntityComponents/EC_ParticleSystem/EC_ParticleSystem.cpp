// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"
#include "DebugOperatorNew.h"
#include "EC_ParticleSystem.h"
#include "IModule.h"
#include "Entity.h"
#include "Renderer.h"
#include "EC_Placeable.h"
#include "OgreParticleAsset.h"
#include "OgreConversionUtils.h"
#include "SceneManager.h"
#include "AssetAPI.h"
#include "IAssetTransfer.h"
#include "LoggingFunctions.h"

DEFINE_POCO_LOGGING_FUNCTIONS("EC_ParticleSystem")

#include <Ogre.h>
#include "MemoryLeakCheck.h"

using namespace OgreRenderer;

EC_ParticleSystem::EC_ParticleSystem(IModule *module):
    IComponent(module->GetFramework()),
    particleRef(this, "Particle ref" ),
    castShadows(this, "Cast shadows", false),
    renderingDistance(this, "Rendering distance", 0.0f)
{
    static AttributeMetadata particleRefMetadata;
    AttributeMetadata::ButtonInfoList particleRefButtons;
    particleRefButtons.push_back(AttributeMetadata::ButtonInfo(particleRef.GetName(), "V", "View"));
    particleRefMetadata.buttons = particleRefButtons;
    particleRef.SetMetadata(&particleRefMetadata);

    renderer_ = GetFramework()->GetServiceManager()->GetService<Renderer>();
    connect(this, SIGNAL(ParentEntitySet()), this, SLOT(EntitySet()));
    connect(this, SIGNAL(AttributeChanged(IAttribute*, AttributeChange::Type)), this, SLOT(OnAttributeUpdated(IAttribute*)));
}

EC_ParticleSystem::~EC_ParticleSystem()
{
    DeleteParticleSystems();
}

void EC_ParticleSystem::View(const QString &attributeName)
{
    //! @todo add implementation.
}

void EC_ParticleSystem::CreateParticleSystem(const QString &systemName)
{
    if (!ViewEnabled())
        return;
    if (renderer_.expired())
        return;
    RendererPtr renderer = renderer_.lock();

    try
    {
        EC_Placeable *placeable = dynamic_cast<EC_Placeable *>(FindPlaceable().get());
        if (!placeable)
        {
            LogError("Fail to create a new particle system, make sure that entity has EC_Placeable component created.");
            return;
        }

        Ogre::SceneManager* scene_mgr = renderer->GetSceneManager();
        Ogre::ParticleSystem* system = scene_mgr->createParticleSystem(renderer->GetUniqueObjectName("EC_Particlesystem"), SanitateAssetIdForOgre(systemName.toStdString()));
        if (system)
        {
            placeable->GetSceneNode()->attachObject(system);
            particleSystems_.push_back(system);
            system->setCastShadows(castShadows.Get());
            system->setRenderingDistance(renderingDistance.Get());
            return;
        }
    }
    catch (Ogre::Exception& e)
    {
        LogError("Could not add particle system " + Name().toStdString() + ": " + std::string(e.what()));
    }

    return;
}

void EC_ParticleSystem::DeleteParticleSystems()
{
    if (renderer_.expired())
        return;
    RendererPtr renderer = renderer_.lock();

    Ogre::SceneManager* scene_mgr = renderer->GetSceneManager();
    if (!scene_mgr)
        return;

    try
    {
        EC_Placeable *placeable = dynamic_cast<EC_Placeable *>(FindPlaceable().get());
        if (placeable)
        {
            Ogre::SceneNode *node = placeable->GetSceneNode();
            if (!node)
                return;
            for (unsigned i = 0; i < particleSystems_.size(); ++i)
                node->detachObject(particleSystems_[i]);
        }
        for (unsigned i = 0; i < particleSystems_.size(); ++i)
            scene_mgr->destroyParticleSystem(particleSystems_[i]);
    }
    catch (Ogre::Exception& /*e*/)
    {
        LogError("Could not delete particle systems");
    }
    
    particleSystems_.clear();
    
    return;
}

void EC_ParticleSystem::OnAttributeUpdated(IAttribute *attribute)
{
    if(attribute == &castShadows)
    {
        for (unsigned i = 0; i < particleSystems_.size(); ++i)
            particleSystems_[i]->setCastShadows(castShadows.Get());
    }
    else if (attribute == &renderingDistance)
    {
        for (unsigned i = 0; i < particleSystems_.size(); ++i)
            particleSystems_[i]->setRenderingDistance(renderingDistance.Get());
    }
    else if (attribute == &particleRef)
    {
        if (!ViewEnabled())
            return;

        // Request the new particle system resource. Once it has loaded, ParticleSystemAssetLoaded() will be called.
        AssetTransferPtr transfer = GetFramework()->Asset()->RequestAsset(particleRef.Get());
        if ( transfer.get() != 0)
        {
            connect(transfer.get(), SIGNAL(Loaded(AssetPtr)), SLOT(ParticleSystemAssetLoaded()), Qt::UniqueConnection);
        }
        else
        {
            DeleteParticleSystems();
        }
    }
}

ComponentPtr EC_ParticleSystem::FindPlaceable() const
{
    if (GetParentEntity())
        return GetParentEntity()->GetComponent<EC_Placeable>();
    else
        return ComponentPtr();
}

void EC_ParticleSystem::ParticleSystemAssetLoaded()
{
    IAssetTransfer *transfer = dynamic_cast<IAssetTransfer*>(sender());
    assert(transfer);
    if (!transfer)
        return;

    OgreParticleAsset *asset = dynamic_cast<OgreParticleAsset*>(transfer->asset.get());
    if (!asset)
    {
        LogWarning("EC_ParticleSystem::ParticleSystemAssetLoaded: Asset transfer finished, but asset pointer was null!");
        return;
    }

    if (asset->GetNumTemplates() > 0)
    {
        DeleteParticleSystems();
        for (int i = 0 ; i< asset->GetNumTemplates(); ++i)
            CreateParticleSystem(asset->GetTemplateName(i));
    }
}

void EC_ParticleSystem::EntitySet()
{
    Scene::Entity *entity = this->GetParentEntity();
    if (!entity)
    {
        LogError("Failed to connect entity signals, component's parent entity is null");
        return;
    }
    
    QObject::connect(entity, SIGNAL(ComponentRemoved(IComponent*, AttributeChange::Type)), this, SLOT(OnComponentRemoved(IComponent*, AttributeChange::Type)), Qt::UniqueConnection);
}

void EC_ParticleSystem::OnComponentRemoved(IComponent *component, AttributeChange::Type change)
{
    // If the component is the Placeable, delete particle systems now for safety
    if (component->TypeName() == EC_Placeable::TypeNameStatic())
        DeleteParticleSystems();
}

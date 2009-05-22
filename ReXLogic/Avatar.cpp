// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"
#include "RexNetworkUtils.h"
#include "Avatar.h"
#include "RexLogicModule.h"
#include "EC_OpenSimAvatar.h"
#include "EC_NetworkPosition.h"
#include <Ogre.h>
#include "../OgreRenderingModule/EC_OgreMesh.h"
#include "../OgreRenderingModule/EC_OgrePlaceable.h"
#include "../OgreRenderingModule/EC_OgreMovableTextOverlay.h"
#include "../OgreRenderingModule/EC_OgreAnimationController.h"
#include "../OgreRenderingModule/Renderer.h"
#include "ConversionUtils.h"



namespace RexLogic
{
    Avatar::Avatar(RexLogicModule *rexlogicmodule)
    { 
        rexlogicmodule_ = rexlogicmodule;

        avatar_anims_[RexTypes::RexUUID("6ed24bd8-91aa-4b12-ccc7-c97c857ab4e0")] = AVATAR_ANIM_WALK;
        avatar_anims_[RexTypes::RexUUID("2408fe9e-df1d-1d7d-f4ff-1384fa7b350f")] = AVATAR_ANIM_STAND;
        avatar_anims_[RexTypes::RexUUID("aec4610c-757f-bc4e-c092-c6e9caf18daf")] = AVATAR_ANIM_FLY;
        avatar_anims_[RexTypes::RexUUID("1c7600d6-661f-b87b-efe2-d7421eb93c86")] = AVATAR_ANIM_SIT_GROUND;               
        
        default_avatar_mesh_ = rexlogicmodule_->GetFramework()->GetDefaultConfig().DeclareSetting("RexAvatar", "default_mesh_name", std::string("Jack.mesh"));
    }

    Avatar::~Avatar()
    {
    }
    
    Scene::EntityPtr Avatar::GetOrCreateAvatarEntity(Core::entity_id_t entityid, const RexUUID &fullid)
    {
        // Make sure scene exists
        Scene::ScenePtr scene = rexlogicmodule_->GetCurrentActiveScene();
        if (!scene)
            return Scene::EntityPtr();

        Scene::EntityPtr entity = rexlogicmodule_->GetAvatarEntity(entityid);
        if (!entity)
        {
            entity = CreateNewAvatarEntity(entityid);
            
            if (entity)
            {
                rexlogicmodule_->RegisterFullId(fullid,entityid);
            
                EC_OpenSimAvatar &avatar = *checked_static_cast<EC_OpenSimAvatar*>(entity->GetComponent(EC_OpenSimAvatar::NameStatic()).get());
                avatar.LocalId = entityid; ///\note In current design it holds that localid == entityid, but I'm not sure if this will always be so?
                avatar.FullId = fullid;
            }
        }
        return entity;
    }    

    Scene::EntityPtr Avatar::CreateNewAvatarEntity(Core::entity_id_t entityid)
    {
        Scene::ScenePtr scene = rexlogicmodule_->GetCurrentActiveScene();
        if (!scene || !rexlogicmodule_->GetFramework()->GetComponentManager()->CanCreate(OgreRenderer::EC_OgrePlaceable::NameStatic()))
            return Scene::EntityPtr();
        
        Core::StringVector defaultcomponents;
        defaultcomponents.push_back(EC_OpenSimAvatar::NameStatic());
        defaultcomponents.push_back(EC_NetworkPosition::NameStatic());
        defaultcomponents.push_back(OgreRenderer::EC_OgrePlaceable::NameStatic());
        defaultcomponents.push_back(OgreRenderer::EC_OgreMovableTextOverlay::NameStatic());
        defaultcomponents.push_back(OgreRenderer::EC_OgreMesh::NameStatic());
        defaultcomponents.push_back(OgreRenderer::EC_OgreAnimationController::NameStatic());
        
        Scene::EntityPtr entity = scene->CreateEntity(entityid, defaultcomponents);

        Foundation::ComponentPtr placeable = entity->GetComponent(OgreRenderer::EC_OgrePlaceable::NameStatic());
        if (placeable)
        {
            OgreRenderer::EC_OgrePlaceable &ogrepos = *checked_static_cast<OgreRenderer::EC_OgrePlaceable*>(placeable.get());
            DebugCreateOgreBoundingBox(rexlogicmodule_,
                entity->GetComponent(OgreRenderer::EC_OgrePlaceable::NameStatic()), "AmbientGreen", Vector3(0.5,0.5,1.5));
            
            CreateNameOverlay(ogrepos, entityid);
            CreateDefaultAvatarMesh(entityid);
        }
        
        return entity;
    } 

    bool Avatar::HandleOSNE_ObjectUpdate(OpenSimProtocol::NetworkEventInboundData* data)
    {
        NetInMessage *msg = data->message;
 
        msg->ResetReading();
        uint64_t regionhandle = msg->ReadU64();
        msg->SkipToNextVariable(); // TimeDilation U16 ///\todo Unhandled inbound variable 'TimeDilation'.
        
        // Variable block: Object Data
        size_t instance_count = data->message->ReadCurrentBlockInstanceCount();
        for(size_t i = 0; i < instance_count; ++i)
        {
            uint32_t localid = msg->ReadU32(); 
            msg->SkipToNextVariable();		// State U8 ///\todo Unhandled inbound variable 'State'.
            RexUUID fullid = msg->ReadUUID();
            msg->SkipToNextVariable();		// CRC U32 ///\todo Unhandled inbound variable 'CRC'.
            uint8_t pcode = msg->ReadU8();

            Scene::EntityPtr entity = GetOrCreateAvatarEntity(localid, fullid);
            if (!entity)
                return false;
                
            EC_OpenSimAvatar &avatar = *checked_static_cast<EC_OpenSimAvatar*>(entity->GetComponent(EC_OpenSimAvatar::NameStatic()).get());
            EC_NetworkPosition &netpos = *checked_static_cast<EC_NetworkPosition*>(entity->GetComponent(EC_NetworkPosition::NameStatic()).get());

            avatar.RegionHandle = regionhandle;
            
            // Get position from objectdata
            msg->SkipToFirstVariableByName("ObjectData");
            size_t bytes_read = 0;
            const uint8_t *objectdatabytes = msg->ReadBuffer(&bytes_read);
            if (bytes_read >= 28)
            {
                // The data contents:
                // ofs 16 - pos xyz - 3 x float (3x4 bytes)
                netpos.position_ = Core::OpenSimToOgreCoordinateAxes(*reinterpret_cast<const Core::Vector3df*>(&objectdatabytes[16]));
                netpos.Updated();
            }                
                        
            msg->SkipToFirstVariableByName("ParentID");
            avatar.ParentId = msg->ReadU32();
            
            // NameValue contains: FirstName STRING RW SV " + firstName + "\nLastName STRING RW SV " + lastName
            msg->SkipToFirstVariableByName("NameValue");
            std::string namevalue = msg->ReadString();
            size_t pos = namevalue.find("\n");
            if(pos != std::string::npos)
            {
                avatar.SetFirstName(namevalue.substr(23,pos-23));
                avatar.SetLastName(namevalue.substr(pos+23));
            }
            
            // Set own avatar
            if (avatar.FullId == rexlogicmodule_->GetServerConnection()->GetInfo().agentID)
                rexlogicmodule_->GetAvatarController()->SetAvatarEntity(entity);

            ShowAvatarNameOverlay(avatar.LocalId);

            msg->SkipToFirstVariableByName("JointAxisOrAnchor");
            msg->SkipToNextVariable(); // To next instance
        }
        
        return false;
    }
    
    void Avatar::HandleTerseObjectUpdate_30bytes(const uint8_t* bytes)
    {
        // The data contents:
        // ofs  0 - localid - packed to 4 bytes
        // ofs  4 - position xyz - 3 x float (3x4 bytes)
        // ofs 16 - velocity xyz - packed to 6 bytes
        // ofs 22 - rotation - packed to 8 bytes
        
        //! \todo handle endians
        int i = 0;
        uint32_t localid = *reinterpret_cast<uint32_t*>((uint32_t*)&bytes[i]);                
        i += 4;

        Scene::EntityPtr entity = rexlogicmodule_->GetAvatarEntity(localid);
        if(!entity) return;
        EC_NetworkPosition &netpos = *checked_static_cast<EC_NetworkPosition*>(entity->GetComponent(EC_NetworkPosition::NameStatic()).get());

        Core::Vector3df position = GetProcessedVector(&bytes[i]);
        i += sizeof(Core::Vector3df);
        
        netpos.velocity_ = GetProcessedScaledVectorFromUint16(&bytes[i],128);
        i += 6;
        
        Core::Quaternion rotation = GetProcessedQuaternion(&bytes[i]);
     
        if (rexlogicmodule_->GetAvatarController()->GetAvatarEntity() && entity->GetId() == rexlogicmodule_->GetAvatarController()->GetAvatarEntity()->GetId())
        {
            rexlogicmodule_->GetAvatarController()->HandleNetworkUpdate(position, rotation);
        }
        else
        {
            netpos.position_ = position;
            netpos.rotation_ = rotation;
        }
                  
        //! \todo what to do with acceleration & rotation velocity? zero them currently
        netpos.accel_ = Core::Vector3df::ZERO;
        netpos.rotvel_ = Core::Vector3df::ZERO;
        
        netpos.Updated();                 
    }    
    
    void Avatar::HandleTerseObjectUpdateForAvatar_60bytes(const uint8_t* bytes)
    {
        // The data contents:
        // ofs  0 - localid - packed to 4 bytes
        // ofs  4 - 0
        // ofs  5 - 1
        // ofs  6 - empty 14 bytes
        // ofs 20 - 128
        // ofs 21 - 63
        // ofs 22 - position xyz - 3 x float (3x4 bytes)
        // ofs 34 - velocity xyz - packed to 6 bytes        
        // ofs 40 - acceleration xyz - packed to 6 bytes           
        // ofs 46 - rotation - packed to 8 bytes 
        // ofs 54 - rotational vel - packed to 6 bytes
        
        //! \todo handle endians
        int i = 0;
        uint32_t localid = *reinterpret_cast<uint32_t*>((uint32_t*)&bytes[i]);                
        i += 22;
        
        // set values
        Scene::EntityPtr entity = rexlogicmodule_->GetAvatarEntity(localid);
        if(!entity) return;        
        EC_NetworkPosition &netpos = *checked_static_cast<EC_NetworkPosition*>(entity->GetComponent(EC_NetworkPosition::NameStatic()).get());

        Core::Vector3df position = GetProcessedVector(&bytes[i]);
        i += sizeof(Core::Vector3df);

        netpos.velocity_ = GetProcessedScaledVectorFromUint16(&bytes[i],128);
        i += 6;
        
        netpos.accel_ = GetProcessedVectorFromUint16(&bytes[i]);
        i += 6;

        Core::Quaternion rotation = GetProcessedQuaternion(&bytes[i]);
        i += 8;        

        netpos.rotvel_ = GetProcessedScaledVectorFromUint16(&bytes[i],128);

        if (rexlogicmodule_->GetAvatarController()->GetAvatarEntity() && entity->GetId() == rexlogicmodule_->GetAvatarController()->GetAvatarEntity()->GetId())
        {
            rexlogicmodule_->GetAvatarController()->HandleNetworkUpdate(position, rotation);
        }
        else
        {
            netpos.position_ = position;
            netpos.rotation_ = rotation;
        }
        
        netpos.Updated();
                            
    }
        
    bool Avatar::HandleRexGM_RexAppearance(OpenSimProtocol::NetworkEventInboundData* data)
    {        
        data->message->ResetReading();    
        data->message->SkipToFirstVariableByName("Parameter");

        // Variable block begins
        size_t instance_count = data->message->ReadCurrentBlockInstanceCount();

        bool overrideappearance = false;

        if (instance_count >= 2)
        {
            std::string avataraddress = data->message->ReadString();
            RexUUID avatarid(data->message->ReadString());
        
            if (instance_count >= 3)
                overrideappearance = ParseBool(data->message->ReadString());
        
            Scene::EntityPtr entity = rexlogicmodule_->GetAvatarEntity(avatarid);
            if(entity)
            {
                EC_OpenSimAvatar &avatar = *checked_static_cast<EC_OpenSimAvatar*>(entity->GetComponent(EC_OpenSimAvatar::NameStatic()).get());        
                avatar.SetAppearanceAddress(avataraddress,overrideappearance);
            }
        }
        
        return false;
    }
    
    bool Avatar::HandleOSNE_KillObject(uint32_t objectid)
    {
        Scene::ScenePtr scene = rexlogicmodule_->GetCurrentActiveScene();
        if (!scene)
            return false;

        RexTypes::RexUUID fullid;
        fullid.SetNull();
        Scene::EntityPtr entity = scene->GetEntity(objectid);
        if(!entity)
            return false;

        Foundation::ComponentPtr component = entity->GetComponent(EC_OpenSimAvatar::NameStatic());
        if(component)
        {
            EC_OpenSimAvatar &avatar = *checked_static_cast<EC_OpenSimAvatar*>(component.get());
            fullid = avatar.FullId;
        }
        
        scene->RemoveEntity(objectid);
        rexlogicmodule_->UnregisterFullId(fullid);
        return false;
    }
   
    bool Avatar::HandleOSNE_AvatarAnimation(OpenSimProtocol::NetworkEventInboundData* data)   
    {        
        data->message->ResetReading();
        RexUUID avatarid = data->message->ReadUUID();
     
        size_t animlistcount = data->message->ReadCurrentBlockInstanceCount();
        for(size_t i = 0; i < animlistcount; i++)
        {
            RexUUID animid = data->message->ReadUUID();
            Core::s32 animsequence = data->message->ReadS32();

            if(avatar_anims_.find(animid) != avatar_anims_.end())
            {
                /// \todo handle known animation: avatar_anims_[animid]                        
            }
        }        
        
        size_t animsourcelistcount = data->message->ReadCurrentBlockInstanceCount();
        for(size_t i = 0; i < animsourcelistcount; i++)
        {
            RexUUID objectid = data->message->ReadUUID();  
        }
        // PhysicalAvatarEventList not used
                
        return false;
    }     
    void Avatar::UpdateAvatarNameOverlayPositions()
    {
        Scene::ScenePtr scene = rexlogicmodule_->GetCurrentActiveScene();
        if (!scene)
            return;
        
        for(Scene::SceneManagerInterface::EntityIterator iter = scene->begin();
            iter != scene->end(); ++iter)
        {
            Scene::EntityInterface &entity = *iter;
            Foundation::ComponentPtr overlay_ptr = entity.GetComponent(OgreRenderer::EC_OgreMovableTextOverlay::NameStatic());
            if (!overlay_ptr)
                continue;
            
            OgreRenderer::EC_OgreMovableTextOverlay &name_overlay = *checked_static_cast<OgreRenderer::EC_OgreMovableTextOverlay*>(overlay_ptr.get());
            name_overlay.Update();
        }
    }
    
    void Avatar::CreateNameOverlay(OgreRenderer::EC_OgrePlaceable &placeable, Core::entity_id_t entity_id)
    {
        Scene::ScenePtr scene = rexlogicmodule_->GetCurrentActiveScene();
        if (!scene)
            return;
        
        Scene::EntityPtr entity = scene->GetEntity(entity_id);
        if (!entity)
            return;
        
        Foundation::ComponentPtr overlay = entity->GetComponent(OgreRenderer::EC_OgreMovableTextOverlay::NameStatic());
        EC_OpenSimAvatar &avatar = *checked_static_cast<EC_OpenSimAvatar*>(entity->GetComponent(EC_OpenSimAvatar::NameStatic()).get());
        if (overlay)
        {
            OgreRenderer::EC_OgreMovableTextOverlay &name_overlay = *checked_static_cast<OgreRenderer::EC_OgreMovableTextOverlay*>(overlay.get());
            name_overlay.SetText(avatar.GetFullName());
            name_overlay.SetParentNode(placeable.GetSceneNode());
        }
    }   
    void Avatar::ShowAvatarNameOverlay(Core::entity_id_t entity_id)
    {
        Scene::ScenePtr scene = rexlogicmodule_->GetCurrentActiveScene();
        if (!scene)
            return;
        
        Scene::EntityPtr entity = scene->GetEntity(entity_id);
        if (!entity)
            return;
        
        Foundation::ComponentPtr overlay = entity->GetComponent(OgreRenderer::EC_OgreMovableTextOverlay::NameStatic());
        EC_OpenSimAvatar &avatar = *checked_static_cast<EC_OpenSimAvatar*>(entity->GetComponent(EC_OpenSimAvatar::NameStatic()).get());
        if (overlay)
        {
            OgreRenderer::EC_OgreMovableTextOverlay &name_overlay = *checked_static_cast<OgreRenderer::EC_OgreMovableTextOverlay*>(overlay.get());
            name_overlay.SetText(avatar.GetFullName());
            name_overlay.SetVisible(true);
        }
    }
    

    void Avatar::CreateDefaultAvatarMesh(Core::entity_id_t entity_id)
    {
        Scene::EntityPtr entity = rexlogicmodule_->GetAvatarEntity(entity_id);
        if (!entity)
            return;
            
        Foundation::ComponentPtr placeableptr = entity->GetComponent(OgreRenderer::EC_OgrePlaceable::NameStatic());
        Foundation::ComponentPtr meshptr = entity->GetComponent(OgreRenderer::EC_OgreMesh::NameStatic());
        Foundation::ComponentPtr animctrlptr = entity->GetComponent(OgreRenderer::EC_OgreAnimationController::NameStatic());
        
        if (placeableptr && meshptr)
        {
            OgreRenderer::EC_OgrePlaceable &placeable = *checked_static_cast<OgreRenderer::EC_OgrePlaceable*>(placeableptr.get());
            OgreRenderer::EC_OgreMesh &mesh = *checked_static_cast<OgreRenderer::EC_OgreMesh*>(meshptr.get());
            
            mesh.SetPlaceable(placeableptr);
            mesh.SetMesh(default_avatar_mesh_);
            // Set adjustment orientation for mesh (Ogre meshes usually have Y-axis as vertical)
            Core::Quaternion adjust(Core::PI/2, 0, -Core::PI/2);
            mesh.SetAdjustOrientation(adjust);
            // Position approximately within the bounding box
            mesh.SetAdjustPosition(Core::Vector3df(0,0,-0.75));
            
        }
        
        if (animctrlptr && meshptr)
        {
            OgreRenderer::EC_OgreAnimationController &animctrl = *checked_static_cast<OgreRenderer::EC_OgreAnimationController*>(animctrlptr.get());
            
            animctrl.SetMeshEntity(meshptr);
            animctrl.EnableAnimation("Walk");
            animctrl.SetAnimationSpeed("Walk", 0.0);
        }
    }
    
}

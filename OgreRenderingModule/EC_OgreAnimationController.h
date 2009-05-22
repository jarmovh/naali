// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_OgreRenderer_EC_OgreAnimationController_h
#define incl_OgreRenderer_EC_OgreAnimationController_h

#include "ComponentInterface.h"
#include "Foundation.h"
#include "OgreModuleApi.h"

namespace Ogre
{
    class Entity;
    class AnimationState;
}

namespace OgreRenderer
{
    //! Ogre-specific mesh entity animation controller
    /*! Needs to be told of an EC_OgreMesh component to be usable
        \ingroup OgreRenderingModuleClient
     */
    class OGRE_MODULE_API EC_OgreAnimationController : public Foundation::ComponentInterface
    {
        DECLARE_EC(EC_OgreAnimationController);
    public:
        virtual ~EC_OgreAnimationController();

        //! gets mesh entity component
        Foundation::ComponentPtr GetMeshEntity() const { return mesh_entity_; }
        
        //! sets mesh entity component
        void SetMeshEntity(Foundation::ComponentPtr mesh_entity);
        
        //! updates animation(s) by elapsed time
        void Update(Core::f64 frametime);
        
        //! Enable animation, with optional fade-in period. Returns true if success (animation exists)
        bool EnableAnimation(const std::string& name, bool looped = true, Core::f32 fadein = 0.0f);
	
        //! Enable an exclusive animation (fades out all other animations with fadeOut parameter)
        bool EnableExclusiveAnimation(const std::string& name, bool looped, Core::f32 fadein = 0.0f, Core::f32 fadeout = 0.0f);

        //! Check whether non-looping animation has finished
        /*! If looping, returns always false
        */
        bool HasAnimationFinished(const std::string& name);

        //! Check whether animation is active
        //! \param check_fade_out if true, also fade-out (until totally faded) phase is interpreted as "active"
        bool IsAnimationActive(const std::string& name, bool check_fadeout = true);

       //! Disable animation, with optional fade-out period. Returns true if success (animation exists)
        bool DisableAnimation(const std::string& name, Core::f32 fadeout = 0.0f);

        //! Disable all animations with the same fadeout period
        void DisableAllAnimations(Core::f32 fadeout = 0.0f);

        //! forwards animation to end, useful if animation is played in reverse
        void SetAnimationToEnd(const std::string& name);

        //! Set relative speed of active animation. Once disabled, the speed is forgotten! Returns true if success (animation exists)
        bool SetAnimationSpeed(const std::string& name, Core::f32 speedfactor);

        //! Change weight of an active animation (default 1.0). Return false if the animation doesn't exist or isn't active
        bool SetAnimationWeight(const std::string& name, Core::f32 weight);

        //! Set time position of an active animation.
        bool SetAnimationTimePosition(const std::string& name, Core::f32 new_position);

        //! Set autostop on animation
        bool SetAnimationAutoStop(const std::string& name, bool enable);

        //! set number of times the animation is repeated (0 = repeat indefinitely)
        bool SetAnimationNumLoops(const std::string& name, Core::uint repeats);
        
    private:
        
        //! Enumeration of animation phase
        enum AnimationPhase
        {
            PHASE_FADEIN,
            PHASE_PLAY,
            PHASE_FADEOUT,
            PHASE_STOP
        };

        //! Structure for an ongoing animation
        struct Animation
        {
            //! Autostop at end (default false)
            bool auto_stop_;

            //! Time in milliseconds it takes to fade in/out an animation completely
            Core::f32 fade_period_;
            
            //! Weight of an animation in animation blending, maximum 1.0
            Core::f32 weight_;

            //! Weight adjust
            Core::f32 weight_factor_;

            //! How an animation is sped up or slowed down, default 1.0 (original speed)
            Core::f32 speed_factor_;

            //! loop animation through num_repeats times, or loop if zero
            Core::uint num_repeats_;

            //! current phase
            AnimationPhase phase_;

            Animation() :
                auto_stop_(false),
                fade_period_(0.0),
                weight_(0.0),
                weight_factor_(1.0),
                speed_factor_(1.0),
                num_repeats_(0),
                phase_(PHASE_STOP)
            {
            }
        };
        
        typedef std::map<std::string, Animation> AnimationMap;
        
        //! constructor
        /*! \param module renderer module
         */
        EC_OgreAnimationController(Foundation::ModuleInterface* module);
        
        //! Gets Ogre entity from the mesh entity component and checks if it has changed; in that case resets internal state
        Ogre::Entity* GetEntity();

        //! Gets animationstate from Ogre entity safely
        /*! \param entity Ogre entity
            \param name Animation name
            \return animationstate, or null if not found
         */
        Ogre::AnimationState* GetAnimationState(Ogre::Entity* entity, const std::string& name);
        
        //! Resets internal state
        void ResetState();
        
        //! Mesh entity component 
        Foundation::ComponentPtr mesh_entity_;
        
        //! Current mesh name
        std::string mesh_name_;
        
        //! Current animations
        AnimationMap animations_;
    };
}

#endif

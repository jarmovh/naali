// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_EnvironmentModule_EC_SkyPlane_h
#define incl_EnvironmentModule_EC_SkyPlane_h

#include "IComponent.h"
#include "IAttribute.h"
#include "Declare_EC.h"
#include "CoreTypes.h"
#include "RexTypes.h"
#include "OgreModuleFwd.h"
#include "AssetReference.h"

namespace Environment
{
/// Makes the entity a SkyPlane
/**
<table class="header">
<tr>
<td>
<h2>SkyPlane</h2>

Registered by Enviroment::EnvironmentModule. 

<h3> Using component to synchronize sky in Taiga </h3>

Currently (not in Tundra) EC_SkyPlane component can be used to synchronize sky in Taiga worlds. This can be done
so that user creates entity and sets entity EC_Name-component. If this component name is set as "SkyEnvironment" our current implementation
will create automatically a EC_SkyPlane-component on it. This component is now usable for every users and all changes on it will be transfered 
to all users. This synchronized sky plane component can also edit through environment editor (in world tools). Currently Caelum must be disabled 
before these features can be used.

<b>Attributes</b>:
<ul>
<li> AssetReference: materialRef.
<div> Sky material reference.</div>
<li> AssetReference: textureRef.
<div> Sky texture reference.</div>
<li> float : tilingAttr.
<div> How many times to tile the texture(s) across the plane. </div>
<li> float : scaleAttr.
<div> The scaling applied to the sky plane - higher values mean a bigger sky plane  </div>
<li> float : bowAttr.
<div> If zero, the plane will be completely flat (like previous versions. If above zero, the plane will be curved,
allowing the sky to appear below camera level. 
      Curved sky planes are similar to sky domes, but are more compatible with fog.  </div>
<li> float : distanceAttr.
<div> Plane distance in world coordinates from the camera </div>
<li> int : xSegmentsAttr.
<div> Defines how many vertices is used in drawing in x - axis.  </div>
<li> int : ySegmentsAttr.
<div> Defines how many vertices is used in drawing in y - axis.  </div>
<li> bool : drawFirstAttr.
<div> If true, the plane is drawn before all other geometry in the scene. </div>

</ul>

</table>
*/
    class EC_SkyPlane : public IComponent
    {
        Q_OBJECT
        DECLARE_EC(EC_SkyPlane);

    public:
        virtual ~EC_SkyPlane();

        virtual bool IsSerializable() const { return true; }

        /// Sky material reference
        DEFINE_QPROPERTY_ATTRIBUTE(AssetReference, materialRef);
        Q_PROPERTY(AssetReference materialRef READ getmaterialRef WRITE setmaterialRef);

        /// Sky texture reference.
        DEFINE_QPROPERTY_ATTRIBUTE(AssetReference, textureRef);
        Q_PROPERTY(AssetReference textureRef READ gettextureRef WRITE settextureRef);

        /// How many times to tile the texture(s) across the plane.
        DEFINE_QPROPERTY_ATTRIBUTE(float, tilingAttr);
        Q_PROPERTY(float tilingAttr READ gettilingAttr WRITE settilingAttr);

        /// The scaling applied to the sky plane - higher values mean a bigger sky plane 
        DEFINE_QPROPERTY_ATTRIBUTE(float, scaleAttr);
        Q_PROPERTY(float scaleAttr READ getscaleAttr WRITE setscaleAttr);

        /// Defines how much sky bows
        DEFINE_QPROPERTY_ATTRIBUTE(float, bowAttr);
        Q_PROPERTY(float bowAttr READ getbowAttr WRITE setbowAttr);

        DEFINE_QPROPERTY_ATTRIBUTE(float, distanceAttr);
        Q_PROPERTY(float distanceAttr READ getdistanceAttr WRITE setdistanceAttr);

        /// Defines how many vertices is used in drawing in x - axis.
        DEFINE_QPROPERTY_ATTRIBUTE(int, xSegmentsAttr);
        Q_PROPERTY(int xSegmentsAttr READ getxSegmentsAttr WRITE setxSegmentsAttr);

        /// Defines how many vertices is used in drawing in y - axis.
        DEFINE_QPROPERTY_ATTRIBUTE(int, ySegmentsAttr);
        Q_PROPERTY(int ySegmentsAttr READ getySegmentsAttr WRITE setySegmentsAttr);

         /// Defines that is sky drawn first
        DEFINE_QPROPERTY_ATTRIBUTE(bool, drawFirstAttr);
        Q_PROPERTY(bool drawFirstAttr READ getdrawFirstAttr WRITE setdrawFirstAttr);

    public slots: 
        void View(const QString &attributeName);
        /// Called If some of the attributes has been changed.
        void OnAttributeUpdated(IAttribute* attribute, AttributeChange::Type change);
        void DisableSky();

    private:
       /// Constructor.
        /** @param module Module where component belongs.
        */
        explicit EC_SkyPlane(IModule *module);

       /// Helper function which is used to update sky plane state. 
        void ChangeSkyPlane(IAttribute* attribute);

        void CreateSky();
        void SetTexture();
        QString lastMaterial_;
        float lastTiling_;
        float lastScale_;
        float lastBow_;
        float lastDistance_;
        int lastxSegments_;
        int lastySegments_;
        bool lastDrawFirst_;

        /// Renderer
        OgreRenderer::RendererWeakPtr renderer_;
    };
}

#endif

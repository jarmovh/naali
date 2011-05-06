/**
 *  For conditions of distribution and use, see copyright notice in license.txt
 *
 *  @file   SceneInteract.h
 *  @brief  Transforms generic mouse and keyboard input events to 
 *          input-related Entity Action for scene entities.
 */

#ifndef incl_Scene_SceneInteract_h
#define incl_Scene_SceneInteract_h

#include "ForwardDefines.h"
#include "SceneFwd.h"
#include "InputFwd.h"

#include <QObject>

class RaycastResult;

//! Transforms generic mouse and keyboard input events to input-related entity action for scene entities and Qt signals. 
/**
<table class="header"><tr><td>
<h2>SceneInteract</h2>

Transforms generic mouse and keyboard input events to input-related entity action for scene entities and Qt signals. 
Performs a raycast to the mouse position each frame and executes entity actions depending on the result.

Owned by SceneAPI.

<b>Local entity actions executed to the hit entity:</b>
<ul>
<li>"MousePress" - Executed when mouse is clicked on the entity.
<div>String parameters: (int)"Qt::MouseButton", (float,float,float)"x,y,z", (int)"submesh index"</div>
<li>"MouseRelease" - Executed when mouse click is released on the entity.
<div>String parameters: (int)"Qt::MouseButton", (float,float,float)"x,y,z", (int)"submesh index"</div>
<li>"MouseScroll" - Executed when mouse wheel is scrolled on the entity. Delta is the relative wheel movement. Can be positive or negative, depending on the scroll direction.
<div>String parameters: (int)"relative delta", (float,float,float)"x,y,z", (int)"submesh index"</div>
<li>"MouseHoverIn" - Executed when mouse hover enters the entity.
<div>No parameters.</div>
<li>"MouseHover" - Executed when mouse hover moves on the entity.
<div>No parameters</div>
<li>"MouseHoverOut" - Executed when mouse hover leaves the entity.
<div></div>
</ul>

<b>Qt signals emitted by SceneInteract object:</b>
<ul>
<li>EntityMousePressed(Scene::Entity*, Qt::MouseButton, RaycastResult*) - Emitted when mouse click occurs on top of 3D scene and raycast hits an entity.
<div>Parameters: hit entity, clicked mouse button, raycast result or the hit.</div>
<li>EntityMouseReleased(Scene::Entity*, Qt::MouseButton, RaycastResult*) - Emitted when mouse click released occurs on top of 3D scene and raycast hits an entity..
<div>Parameters: hit entity, clicked mouse button, raycast result or the hit.</div>
</ul>

</td></tr></table>
*/
class SceneInteract : public QObject
{
    Q_OBJECT

public:
    //! Constructor. This object does not have a parent. It is stored in a QWeakPointer in SceneAPI and released in its dtor.
    SceneInteract();

    //! Destructor.
    ~SceneInteract() {}

    //! Initialize this object. Must be done for this object to work. Called by SceneAPI
    /// \param Foundation::Framework Framework pointer.
    void Initialize(Foundation::Framework *framework);

    //! PostInitialize this object. Must be done after modules have been loaded. Called by SceneAPI.
    void PostInitialize();

signals:
    //! Emitted when mouse click occurs on top of 3D scene and raycast hits an entity.
    /// \param entity Hit entity.
    /// \param Qt::MouseButton Qt enum of the clicked mouse button
    /// \param RaycastResult Raycast result data object.
    void EntityMousePressed(Scene::Entity *entity, Qt::MouseButton button, RaycastResult *raycastResult);

    //! Emitted when mouse click released occurs on top of 3D scene and raycast hits an entity.
    /// \param entity Hit entity.
    /// \param Qt::MouseButton Qt enum of the clicked mouse button
    /// \param RaycastResult Raycast result data object.
    void EntityMouseReleased(Scene::Entity *entity, Qt::MouseButton button, RaycastResult *raycastResult);

private:
    //! Performs raycast to last known mouse cursor position.
    /// \return RaycastResult Result data of the raycast.
    RaycastResult* Raycast();

    Foundation::Framework *framework_; ///< Framework.
    InputContextPtr input_; ///< Input context.
    Foundation::RendererWeakPtr renderer_; ///< Renderer pointer.
    int lastX_; ///< Last known mouse cursor's x position.
    int lastY_; ///< Last known mouse cursor's y position.
    bool itemUnderMouse_; ///< Was there widget under mouse in last known position.
    Scene::EntityWeakPtr lastHitEntity_; ///< Last entity raycast has hit.

private slots:
    //! Executes "MouseHover" action each frame is raycast has hit and entity.
    void Update();

    //! Handles key events from input service.
    /// \param e Key event.
    void HandleKeyEvent(KeyEvent *e);

    //! Handles mouse events from input service.
    /// \param e Mouse event.
    void HandleMouseEvent(MouseEvent *e);
};

#endif

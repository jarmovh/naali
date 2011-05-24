// !ref: local://simpleavatar.js

// Avatar application. Will handle switching logic between avatar & freelook camera (clientside), and
// spawning avatars for clients (serverside). Note: this is not a startup script, but is meant to be
// placed in an entity in a scene that wishes to implement avatar functionality.

var avatar_area_size = 10;
var avatar_area_x = 0;
var avatar_area_y = 0;
var avatar_area_z = 20;

var isserver = server.IsRunning();

if (isserver == false) {
    var inputmapper = me.GetOrCreateComponentRaw("EC_InputMapper");
    inputmapper.SetTemporary(true);
    inputmapper.contextPriority = 102;
    inputmapper.RegisterMapping("Ctrl+Tab", "ToggleCamera", 1);

    engine.ImportExtension("qt.core");
    engine.ImportExtension("qt.gui");

    var freecamToggleAction = new QAction(0);
    freecamToggleAction.icon = new QIcon(new QPixmap("./data/ui/images/iconSwitch.png"));
    freecamToggleAction.text = "Toggle free camera mode";
    ui.EmitAddAction(freecamToggleAction);
    freecamToggleAction['triggered(bool)'].connect(ClientHandleToggleCamera);

    var returnButton = new QPushButton("Return To Avatar");
    returnButton.resize(150, 50);
    returnButton.font = new QFont("Arial",12);
    var returnButtonProxy = new UiProxyWidget(returnButton);
    returnButtonProxy.windowFlags = 0;
    ui.AddProxyWidgetToScene(returnButtonProxy);
    var style = "QWidget { background-color: transparent; } QPushButton { background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(228, 228, 228, 255), stop:1 rgba(82, 82, 82, 255)); border: 1px solid grey; border-radius: 10px; color: white; } QPushButton::hover { background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0.699, y2:1, stop:0 rgba(228, 228, 228, 255), stop:1 rgba(82, 82, 82, 255)); } QPushButton::pressed { color: rgb(248, 248, 248); background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(228, 228, 228, 255), stop:1 rgba(82, 82, 82, 255)); }"
    returnButton.styleSheet = style;
    returnButton.setAttribute(Qt.WA_OpaquePaintEvent);
    returnButtonProxy.pos = new QPointF(ui.GraphicsView().width/2 - returnButtonProxy.size.width()/2, 100);

    ui.GraphicsView().scene().sceneRectChanged.connect(SceneRectChanged);
    returnButton.clicked.connect(ReturnToAvatar);
    me.Action("ToggleCamera").Triggered.connect(ClientHandleToggleCamera);
} else {
    server.UserAboutToConnect.connect(ServerHandleUserAboutToConnect);
    server.UserConnected.connect(ServerHandleUserConnected);
    server.UserDisconnected.connect(ServerHandleUserDisconnected);
    
    // If there are connected users when this script was added, add av for all of them
    var userIdList = server.GetConnectionIDs();
    if (userIdList.length > 0)
        print("[Avatar Application] Application started. Creating avatars for logged in clients.");

    for (var i=0; i < userIdList.length; i++)
    {
        var userId = userIdList[i];
        var userConnection = server.GetUserConnection(userId);
        if (userConnection != null)
            ServerHandleUserConnected(userId, userConnection);
    }
}

function SceneRectChanged(rect)
{
    returnButtonProxy.pos = new QPointF(rect.width()/2 - returnButtonProxy.size.width()/2, 100);
}

function ReturnToAvatar()
{
    var freelookcameraentity = scene.GetEntityByNameRaw("FreeLookCamera");
    var avatarcameraentity = scene.GetEntityByNameRaw("AvatarCamera");
    var freecameralistener = freelookcameraentity.GetComponentRaw("EC_SoundListener");
    var avatarent = scene.GetEntityByNameRaw("Avatar" + client.GetConnectionID());
    var avatarlistener = avatarent.GetComponentRaw("EC_SoundListener");
    if ((freelookcameraentity == null) || (avatarcameraentity == null))
        return;
    var freelookcamera = freelookcameraentity.ogrecamera;
    var avatarcamera = avatarcameraentity.ogrecamera;
    avatarcamera.SetActive();
    avatarlistener.active = true;
    freecameralistener.active = false;
    returnButtonProxy.visible = false;
}

function ClientHandleToggleCamera() {
    // For camera switching to work, must have both the freelookcamera & avatarcamera in the scene
    var freelookcameraentity = scene.GetEntityByNameRaw("FreeLookCamera");
    var avatarcameraentity = scene.GetEntityByNameRaw("AvatarCamera");
    var freecameralistener = freelookcameraentity.GetComponentRaw("EC_SoundListener");
    var avatarent = scene.GetEntityByNameRaw("Avatar" + client.GetConnectionID());
    var avatarlistener = avatarent.GetComponentRaw("EC_SoundListener");
    if ((freelookcameraentity == null) || (avatarcameraentity == null))
        return;
    var freelookcamera = freelookcameraentity.ogrecamera;
    var avatarcamera = avatarcameraentity.ogrecamera;

    if (avatarcamera.IsActive()) {
        freelookcameraentity.placeable.transform = avatarcameraentity.placeable.transform;
        freelookcamera.SetActive();
        freecameralistener.active = true;
        avatarlistener.active = false;
        returnButtonProxy.visible = true;
    } else {
        avatarcamera.SetActive();
        avatarlistener.active = true;
        freecameralistener.active = false;
        returnButtonProxy.visible = false;
    }
    
    // Ask entity to check his camera state
    avatarent.Exec(1, "CheckState");
}

function ServerHandleUserAboutToConnect(connectionID, user) {
    // Uncomment to test access control
    //if (user.GetProperty("password") != "xxx")
    //    user.DenyConnection();
}

function ServerHandleUserConnected(connectionID, user) {
    var avatarEntityName = "Avatar" + connectionID;
    var avatarEntity = scene.CreateEntityRaw(scene.NextFreeId(), ["EC_Script", "EC_Placeable", "EC_AnimationController"]);
    avatarEntity.SetTemporary(true); // We never want to save the avatar entities to disk.
    avatarEntity.SetName(avatarEntityName);
    
    if (user != null) {
	avatarEntity.SetDescription(user.GetProperty("username"));
    }

    var script = avatarEntity.script;
    script.type = "js";
    script.runOnLoad = true;
    var r = script.scriptRef;
    r.ref = "local://simpleavatar.js";
    script.scriptRef = r;

    // Set random starting position for avatar
    var placeable = avatarEntity.placeable;
    var transform = placeable.transform;
    transform.pos.x = (Math.random() - 0.5) * avatar_area_size + avatar_area_x;
    transform.pos.y = (Math.random() - 0.5) * avatar_area_size + avatar_area_y;
    transform.pos.z = avatar_area_z;
    placeable.transform = transform;

    scene.EmitEntityCreatedRaw(avatarEntity);
    
    if (user != null) {
        print("[Avatar Application] Created avatar for " + user.GetProperty("username"));
    }
}

function ServerHandleUserDisconnected(connectionID, user) {
    var avatarEntityName = "Avatar" + connectionID;
    var avatartEntity = scene.GetEntityByNameRaw(avatarEntityName);
    if (avatartEntity != null) {
        var entityID = avatartEntity.id;
        scene.RemoveEntityRaw(entityID);

        if (user != null) {
        print("[Avatar Application] User " + user.GetProperty("username") + " disconnected, destroyed avatar entity.");
        }
    }
}

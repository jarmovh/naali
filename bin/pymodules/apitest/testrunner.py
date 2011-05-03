import rexviewer as r
import circuits
import naali
import time
import sys
import traceback

user, pwd, server = "Test Bot", "test", "world.realxtend.org:9000"
#user, pwd, server = "d d", "d", "world.evocativi.com:8002"

class TestRunner(circuits.BaseComponent):
    def __init__(self, cfsection=None):
        self.config = cfsection or dict()
        circuits.BaseComponent.__init__(self)
        self.testgen = self.run()
    

    @circuits.handler("on_sceneadded")
    def sceneadded(self, name):
        r.logInfo("base class sceneadded callback")

    @circuits.handler("update")
    def update(self, deltatime):
        prev = None
        try:
            status = self.testgen.next()
        except StopIteration:
            # print "Test finished"
            return 
        except:
            r.logInfo(traceback.format_exc())
            r.logInfo("unhandled exception in test")
            r.logInfo("Test state: failure")
            r.exit()
            return
        
        if status:
            r.logInfo("Test state: " + str(status))
            prev = status
        #else:
            #print "Test state", prev, "still running"

    def timer_start(self):
        self.timer_started = time.time()

    def elapsed(self, n):
        return (time.time() - self.timer_started) > n

class TestLoginLogoutExit(TestRunner):
    def run(self):
        self.wait_time = int(self.config.get("wait_time", 60))
        yield "login"
        self.timer_start()
        r.startLoginOpensim(user, pwd, server)
        print "ELAPSED:", self.elapsed(self.wait_time)
        while not self.elapsed(self.wait_time):
            #print "X",
            yield None

        yield "logging out"
        self.timer_start()
        r.logout()
        while not self.elapsed(self.wait_time):
            yield None

        yield "exiting"
        r.exit()

class TestCreateDestroy(TestRunner):
    def __init__(self, *args, **kw):
        self.finished = False
        self.scene = None
        self.select_test_ent = None
        TestRunner.__init__(self, *args, **kw)

    def run(self):
        self.wait_time = int(self.config.get("wait_time", 60))
        yield "doing login"
        self.timer_start()
        r.startLoginOpensim(user, pwd, server)
        yield "waiting for connection"
        conn = None
        while not self.elapsed(self.wait_time):
            conn = r.getServerConnection()
            if conn and conn.IsConnected():
                break
            else:
                yield None
        else:
            return
        yield "waiting for scene"
        while not self.scene and not self.elapsed(self.wait_time):
            yield None

        yield "waiting for Test_SelectTest"
        while (not self.select_test_ent) and (not self.elapsed(self.wait_time)):
            yield None

        if self.select_test_ent:
            self.add_extras(self.select_test_ent)
            yield "added extras"

        yield "done waiting for Test_SelectTest, continuing"

        yield "creating object"
        r.getServerConnection().SendObjectAddPacket(42, 42, 22)
        
        yield "waiting for EntityCreated"
        while (not self.finished) and (not self.elapsed(self.wait_time)):
            yield None

        yield "exiting"
        r.exit()
        if (not self.select_test_ent):
            if self.finished:
                yield "success"
            else:
                yield "failure"

    @circuits.handler("on_sceneadded")
    def sceneadded(self, name):
        #r.logInfo("CreateDestroy sceneadded called")
        self.scene = naali.getScene(name)

        self.scene.connect("EntityCreated(Scene::Entity*, AttributeChange::Type)", self.handle_entity_created)
        r.logInfo("EntityCreated callback registered")

    @circuits.handler("on_exit")
    def onexit(self):
        #yield "TestCreateDestroy exiting"
        r.logInfo("TestCreateDestroy exiting...")
        self.delete_extras(self.select_test_ent)
        r.logInfo("...done.")
        #yield "exit ready. done"

    # qt slot
    def handle_entity_created(self, ent, changetype):
        # fun fact: since we are called for every entity and
        # self.finished checked only every "update" event,
        # this often cleans up >1 test objects (in case any
        # are left over from failed tests)

        if ent.Id==2525429102:
            r.logInfo("##### found entity I need")
            self.select_test_ent = ent

        try:
            ec_netp = ent.network
        except AttributeError:
            if 0: print "skip entity without EC_NetworkPosition", dir(ent)
        else:
            netp = ec_netp.Position
            # for some reason z coord ends up as 22.25
            #r.logInfo("found entity with netpos %s %s %s" % (netp.x(), netp.y(), netp.z()))
            if netp.x() == 42.0 and netp.y() == 42.0 and int(netp.z()) == 22:
                r.logInfo("found created test prim - naming, moving and deleting (finished=%s)" % self.finished)
                ent.prim.Name = "Seppo"
                ent.prim.SendObjectNameUpdate()
                pos = ent.placeable.position
                pos.setX(netp.x() + 1) #change the x-coordinate
                ent.placeable.position = pos
                r.logInfo("Moving to move to pos: %s" % pos)
                
                r.getServerConnection().SendObjectDeRezPacket(
                    ent.Id, r.getTrashFolderId())
                self.finished = True

    # add_extras simulate object edit highlight and ruler usage
    def add_extras(self, ent):
        # add EC_Ruler
        ruler = ent.GetOrCreateComponentRaw("EC_Ruler")
        ruler.SetVisible(True)

        #add highlight
        try:
            ent.highlight
        except AttributeError:
            ent.GetOrCreateComponentRaw("EC_Highlight")

        h = ent.highlight

        if not h.IsVisible():
            h.Show()

    def delete_extras(self, ent):
        try:
            h = ent.highlight
        except AttributeError:
            try:
                r.logInfo(traceback.format_exc())
                r.logInfo("removing highlight, but it doesn't exist anymore: %d" % ent.Id)
            except:
                r.logInfo(traceback.format_exc())
                r.logInfo("Entity already removed")
        else:
            ent.RemoveComponentRaw(h)
            r.logInfo("highlight removed")

        try:
            ruler = ent.ruler
        except AttributeError:
            try:
                r.logInfo(traceback.format_exc())
                r.logInfo("removing ruler, but it doesn't exist anymore: %d" % ent.Id)
            except:
                r.logInfo(traceback.format_exc())
                r.logInfo("Entity already removed")
        else:
            ent.RemoveComponentRaw(ruler)
            r.logInfo("ruler removed")
            r.logInfo("Test state: success")

class TestDynamicProperties(TestRunner):
    def __init__(self, *args, **kw):
        self.scene = None
        TestRunner.__init__(self, *args, **kw)

    def run(self):
        self.wait_time = int(self.config.get("wait_time", 60))
        yield "doing login"
        self.timer_start()
        r.startLoginOpensim(user, pwd, server)
        yield "waiting for connection"
        conn = None
        while not self.elapsed(self.wait_time):
            conn = r.getServerConnection()
            if conn and conn.IsConnected():
                break
            else:
                yield None
        else:
            return
        yield "waiting for avatar to appear"
        ent = None
        while not self.elapsed(self.wait_time):
            try:
                ent = naali.getUserAvatar()
            except ValueError:
                yield None
            else:
                break
        
        if not ent:
            yield "failure, avatar didn't appear"
            return

        print 'dynamic property stuff:'
        ent.GetOrCreateComponentRaw("EC_DynamicComponent")
        print ent, type(ent)
        d = ent.qent.EC_DynamicComponent
        val = 42.0
        d.CreateAttribute("real", val)
        d.ComponentChanged(0)
        assert val == d.GetAttribute("real")

        val = 8.5
        d.SetAttribute("real", val)
        d.ComponentChanged(0)
        assert val == d.GetAttribute("real")
        d.RemoveAttribute("real")
        d.ComponentChanged(0)

        yield "created, changed and removed attribute"
        r.exit()
        yield "success"

    @circuits.handler("on_sceneadded")
    def sceneadded(self, name):
        #r.logInfo("CreateDestroy sceneadded called")
        self.scene = naali.getScene(name)

class TestApi(TestRunner):
    def __init__(self, *args, **kw):
        self.scene = None
        TestRunner.__init__(self, *args, **kw)

    def run(self):
        self.wait_time = int(self.config.get("wait_time", 60))
        yield "doing login"
        self.timer_start()
        r.startLoginOpensim(user, pwd, server)
        yield "waiting for connection"
        conn = None
        while not self.elapsed(self.wait_time):
            conn = r.getServerConnection()
            if conn and conn.IsConnected():
                break
            else:
                yield None
        else:
            return
        yield "waiting for avatar to appear"
        ent = None
        while not self.elapsed(self.wait_time):
            try:
                ent = naali.getUserAvatar()
            except ValueError:
                yield None
            else:
                break
        
        if not ent:
            yield "failure, avatar didn't appear"
            return

        for i in range(100):
            for x in self.do_api_calls():
                yield x
            break
        r.exit()
        yield "success"

    def do_api_calls(self):
        yield "createMeshEntity"
        e = naali.createMeshEntity("axes.mesh")

        from PythonQt.QtGui import QVector3D, QQuaternion
        e.placeable.position = QVector3D(128, 128, 60)
        e.placeable.Scale = QVector3D(5, 5, 5)
        e.placeable.Orientation = QQuaternion(0, 0, 0, 1)
        r.removeEntity(e.Id)

        yield "EC_Touchable & EC_Highlight"
        for longname, shortname in [("EC_Touchable", 'touchable'), ("EC_Highlight", 'highlight')]:
            e = naali.getUserAvatar()
            e.GetOrCreateComponentRaw(longname)
            x = getattr(e, shortname)
            x.Show()
            x.Hide()
            assert x.IsVisible() == False

        yield "naali.createEntity"
        ent = naali.createEntity()
        print "new entity created:", ent

        yield "get camera FOV"
        fov = naali.getCamera().camera.GetVerticalFov()
        
        yield "avatar position"
        p = naali.getUserAvatar().placeable.position

        yield "avatar animation controller"
        naali.getUserAvatar().animationcontroller.EnableAnimation("Walk")

        yield "test sendChat"
        r.sendChat("test chat")

        yield "test logInfo"
        r.logInfo("test log message")

        #XXX deprecate
        yield "test camera yaw/itch"
        r.setCameraYawPitch(.1, .5)
        r.getCameraYawPitch()
        
        yield "test webview"
        import PythonQt
        wv = PythonQt.QtWebKit.QWebView()
        wv.show()

        yield "test dotscene loading"
        from localscene import loader
        loader.load_dotscene("pymodules/localscene/test.scene")
    
        yield "test dynamic component"
        ent = naali.getUserAvatar()
        ent.GetOrCreateComponentRaw("EC_DynamicComponent")
        print ent, type(ent)
        d = ent.EC_DynamicComponent
        d.CreateAttribute("real", 42.0)
        d.ComponentChanged(0)
        d.SetAttribute("real", 8.5)
        d.ComponentChanged(0)
        d.RemoveAttribute("real")
        d.ComponentChanged(0)

        yield "test javascript"
        cam = naali.getCamera()
        naali.runjs('print("Hello from JS! " + x)', {'x': naali.renderer})
        naali.runjs('print("Another hello from JS! " + x)', {'x': naali.inputcontext})
        naali.runjs('print("Some camera! " + x)', {'x': cam.camera})
        #py objects are not qobjects. naali.runjs('print("Some camera, using naali :O ! " + x.getCamera())', {'x': naali})
        naali.runjs('print("Camera Entity " + x)', {'x': cam})
        naali.runjs('print("Camera placeable pos: " + pos)', {'pos': cam.placeable.position})
        #not exposed yet. naali.runjs('print("QVector3D: " + new QVector3D())', {})
        #naali.runjs('var a = {"a": true, "b": 2};')
        #naali.runjs('print(a.a + ", " + a.b)')
        #naali.runjs('print(JSON.stringify(a))')
        #naali.runjs('print("1 + 1 == " + 1 + 1)')
        #naali.runjs('print("1 - 1 == " + 1 - 1)')
        print ", done."

        if 0:
            runjs('var b = new QPushButton;')
            runjs('b.text = "hep";')
            runjs('b.show();')

    @circuits.handler("on_sceneadded")
    def sceneadded(self, name):
        #r.logInfo("CreateDestroy sceneadded called")
        self.scene = naali.getScene(name)
    

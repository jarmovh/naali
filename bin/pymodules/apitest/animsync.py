"""
A test / draft for defining Naali Entity-Components with Naali Attribute data.
This one synchs animation state with a GUI slider.
There is now also another test, door.py .
Both are registered as ComponentHandlers in componenthandler.py, and instanciated from there when components of interest are encountered in scene data.0

Originally this used the first test version of DynamicComponent that provided a single float attribute (Attribute<float>).
That can be useful when testing performance and robustness of an upcoming ECA sync mechanism.
The current implementation in Naali 0.2 uses a workaround that needed no protocol nor server changes to old ReX,
where all attributes of all components for a single entity are synched and stored as xml.

After this test, a Door test with two attributes: opened & locked, was written.
First as door.py here, and then as door.js that can be safely loaded from the net (the source is for convenience in Naali repo jsmodules).
To add support for multiple attributes of various types easily to py & js, that was made using a single Naali string attr to store json in v. 0.2.5
With the current ECA sync the performance makes no diff, so there are little drawbacks in this shortcut to get to experiment more.

Now finally for Naali 0.3.0 this system is quite full featured: the DynamicComponents are identified using names, so there can be several for different purposes in a single Entity, and the handler system works so that there can be any number of Entities with a certain component and the individual handlers for those work correctly.

"""

from __future__ import division
import time
import rexviewer as r
import naali

import PythonQt
from PythonQt import QtGui, QtCore

import circuits

INTERVAL = 0.2

class AnimationSync(circuits.BaseComponent):
    GUINAME = "Animation Sync"

    def __init__(self, entity, comp, changetype):
        self.comp = comp
        circuits.BaseComponent.__init__(self)
        self.inworld_inited = False #a cheap hackish substitute for some initin
        self.initgui()
        #todo: OnChanged() is deprecated
        comp.connect("OnChanged()", self.onChanged)

    def initgui(self):
        self.widget = QtGui.QSlider(QtCore.Qt.Horizontal)
        self.widget.connect('valueChanged(int)', self.sliderChanged)

        #naali proxywidget boilerplate
        uism = naali.ui
        self.proxywidget = r.createUiProxyWidget(self.widget)
        self.proxywidget.setWindowTitle(self.GUINAME)
        if not uism.AddWidgetToScene(self.proxywidget):
            print "Adding the ProxyWidget to the bar failed."
        # TODO: Due to core UI API refactor AddWidgetToMenu doesn't exist anymore.
        #uism.AddWidgetToMenu(self.proxywidget, self.GUINAME, "Developer Tools")

        #to not flood the network
        self.prev_sync = 0
        
    #json serialization to port to work with how DC works now with door.py & door.js
    def sliderChanged(self, guival):
        comp = self.comp
        if comp is not None:
            now = time.time()
            if self.prev_sync + INTERVAL < now:
                comp.SetAttribute("timepos", guival / 100)
                #ComponentChanged is no longer necessary to trigger network sync
                #comp.ComponentChanged("Local")
                self.prev_sync = now

    def onChanged(self):
        v = self.comp.GetAttribute('timepos')

        #copy-paste from door.py which also had a onClick handler
        if not self.inworld_inited:
            try:
                ent = self.comp.GetParentEntity()
            except ValueError:
                return

            try:
                t = ent.touchable
            except AttributeError:
                print "no touchable in animsynced obj? it doesn't persist yet? adding..", ent.id
                print ent.GetOrCreateComponentRaw("EC_Touchable")
                t = ent.touchable
            else:
                print "touchable pre-existed in animated character for animsync."
            t.connect('MousePressed()', self.showgui)
            self.inworld_inited = True        

        if self.proxywidget is None and self.widget is not None:
            #if self.widget is None:
            #    self.initgui()
            print "AnimSync DynamicComponent handler registering to GUI"
            self.registergui()

        #print "onChanged",
        comp = self.comp
        if comp is not None:
            #print comp.GetAttribute()
            try:
                ent = comp.GetParentEntity()
            except ValueError:
                print "No entity"
                return

            try:
                a = ent.animationcontroller
            except AttributeError:
                print "ent with DynamicComponent doesn't have animation"
                return

            #print a
            animname = self.comp.GetAttribute('animname') or "Wave"
            a.SetAnimationTimePosition(animname, v)
            self.widget.value = v * 100 #needs changetypes to work well, i guess
            
        else:
            print "- don't know what :o"

    def showgui(self):
        self.proxywidget.show()

    @circuits.handler("on_logout")
    def removegui(self, evid):
        self.proxywidget.hide()
        uism = naali.ui
        uism.RemoveWidgetFromScene(self.proxywidget)

COMPNAME = "animsync"
#now done in componenthandler 'cause this is not a circuits component / naali module
#componenthandler.register(COMPNAME, AnimationSync)

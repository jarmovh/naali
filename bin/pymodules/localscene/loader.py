import dotscene
import math
try:
    import naali
except ImportError:
    print "NOTE: dotscene loader not able to access Naali API, is running outiside Naali?"
    naali = None
else:
    import PythonQt.QtGui
    Vec = PythonQt.QtGui.QVector3D
    Quat = PythonQt.QtGui.QQuaternion

class OgreNode:
    """the equivalent of Ogre Node in Naali for this import now.
    perhaps this will wrap or just be the Naali .. placeable E-C?"""

    def __init__(self):
        #these are set by the parser directly
        self.position = None
        self.orientation = None
        self.scale = None
        self.object = None
        self.naali_ent = None
    
        self.id = None
        self.entityNode = None
        self.entityName = None
        self.entityMeshFile = None
        self.entityCollisionFile = None
        self.entityCollisionPrim = None
        self.entityStatic = None
        
    def createChildSceneNode(self):
        return OgreNode()

    def attachObject(self, ob):
        self.object = ob
        print self, "attached object", ob
        if naali is not None: 
            self.create_naali_meshentity()

    def create_naali_meshentity(self):
        self.naali_ent = naali.createMeshEntity(self.object.mesh, 100002) #XXX handle name too. what about id?

        e = self.naali_ent
        print "Created naali entity:", e, e.id
            
        """apply pos, rot & scale. 
        dotscene.py uses qt types now directly"""
        p = e.placeable
        mp = self.position
        p.Position = self.position
            
        o = self.orientation
        # XXX IMO this is wrong: content creators should export their assets correctly, instead of doing
        # this kind of rotations during import. But I guess this is old problem and similar rotations
        # are being done elsewhere too.
        # looks like objects are often exported wrong, so rotate along x axis
        #p.Orientation = o * Quat(1,1,0,0)
        # XXX if artists want rotate along x axis they can do it so now by checking flip z-y checkbox
        # at the moment it looks like, its needed to rotate each node 180 degrees along z axis to get it right
        #p.Orientation = o * Quat(1,0,0,1) * Quat(1,0,0,1)
        #p.Orientation = o * Quat(math.sqrt(0.5),0,0,math.sqrt(0.5)) * Quat(math.sqrt(0.5),0,0,math.sqrt(0.5))
        rotate90z = Quat(1,0,0,1)
        rotate90z.normalize() # ekvivalent of sqrt Quat above, (this fixes collisions)
        p.Orientation = o * rotate90z* rotate90z
        
        p.Scale = self.scale

"""
the server side importer in modrex does conversions like this:
                        Quaternion rot = new Quaternion(node.DerivedOrientation.X, node.DerivedOrientation.Y, node.DerivedOrientation.Z, node.DerivedOrientation.W);
                        if (m_swapAxes == true)
                        {
                            Vector3 temp = new Vector3(rot.X, rot.Y, rot.Z);
                            rot.X = -temp.X;
                            rot.Y = temp.Z;
                            rot.Z = temp.Y;
                        }
                        else
                        {
                            // Do the rotation adjust as in original importer
                            rot *= new Quaternion(1, 0, 0);
                            rot *= new Quaternion(0, 1, 0);
                        }
                        rot = sceneRotQuat * rot;
                        
                        SceneObjectGroup sceneObject = m_scene.AddNewPrim(m_scene.RegionInfo.EstateSettings.EstateOwner,
                            m_scene.RegionInfo.EstateSettings.EstateOwner, objPos, rot, PrimitiveBaseShape.CreateBox());
                        Vector3 newScale = new Vector3();
                        newScale.X = node.DerivedScale.X * m_objectScale;
                        newScale.Y = node.DerivedScale.Y * m_objectScale;
                        newScale.Z = node.DerivedScale.Z * m_objectScale;
                        if (m_swapAxes == true)
                        {
                            Vector3 temp = new Vector3(newScale);
                            newScale.X = temp.X;
                            newScale.Y = temp.Z;
                            newScale.Z = temp.Y;
                        } 
                        sceneObject.RootPart.Scale = newScale;
"""

class OgreEntity:
    def __init__(self, name, mesh):
        self.name = name
        self.mesh = mesh

class NaaliSceneManagerFacade:
    """
    implements the ogre SceneManager interface
    enough for the dotscene parser to work.

    kudos to mikkopa on the modrex side for coming up
    with this alternative SM impl trick in the c# loader there.
    """

    def __init__(self):
        self.rootnode = OgreNode()

    def getRootSceneNode(self):
        return self.rootnode

    def createEntity(self, name, mesh):
        print "Creating entity", name, mesh
        return OgreEntity(name, mesh)

    #def createLight(self, name):
    #    pass

    #def createCamera(

def load_dotscene(fname):
    sm = NaaliSceneManagerFacade()
    ds = dotscene.DotScene(fname, sm)
    ds.dotscenemanager.rotateScene90DegreesAlongAxis('x')
    return ds, ds.dotscenemanager

def unload_dotscene(ds):
    print "unload_dotscene"
    for k, oNode in ds.dotscenemanager.nodes.iteritems():
        # print "removing " + k
        # print oNode.naali_ent.id
        try:
            naali.removeEntity(oNode.naali_ent)
        except:
            print "failed in unload_dotscene"

    
if __name__ == '__main__':
    load_dotscene("test.scene")

    

# I want mat everywhere anyway.
from mat import *

PartsError = Exception

class Material(object):

    def __init__(self, density, color):
        """
        color -- a 32-bit hex ARGB value (or someday mixtures of
                 frequency (mean and std), reflectivity (including
                 specular vs. diffuse and opacity), ...?). Or emphasize
                 AHSV (i.e., opacity/absorption, frequency, inverse
                 deviation (down to zero), reflectivity?) Light itself
                 might be best defined in terms of mixtures of
                 frequency (Gaussian mean and std?) with radiant
                 intensity (W/sr) as the Y axis?
        """
        self.density = density
        self.color = color

class Part(object):
    """
    Represents a part of a robot, in the Mr. Potato Head, Lego, or Spore
    sense. Each individual part can stand alone or can be chained with other
    parts. Each has joint/socket positions (and preconfigured constraints
    and axes?) for easy composition.

    Parts are not necessarily rigid and may have complex, composite shapes.
    However, each individual part is considered a nondecomposable primitive.
    (I would like to get into decomposable, particle representations
    sometime, but that's not today.) Parts shouldn't have internally
    drivable motors. Rather motors should exist at joints between parts,
    but I may reconsider this style later.

    Parts should be cloneable, alone or in chains, to allow easy building
    of composite structures.

    TODO Or should composite structures be built separately from
    immutable parts? Still seems like parts ought to be paramterizable
    say for different sizes and so on. So go with copying them for now.
    Hmm. If you swap parts on a robot, you probably want the joint
    positions to change. Say the torso gets traded. You want the arms to
    attach to the new shoulder joints. So, we'd ideally want to have
    common names, but in any case, either we need to have primitive items
    with joints/sockets defined and a part absorbs that info, or we'd
    just need a function to relink everything and change out whole parts.
    The latter seems simpler (and more intuitive) at this point. So,
    parts are just parts and incorporate shape information and so on.
    """

    def __init__(self, material=None):
        self.joints = {}
        self.key = None
        self.material = material
        self.pos = A(0,0,0)
        self.rot = A(1,0,0,0)
        # Usually just one socket, but I can imagine other cases, even when
        # avoiding non-tree linkages.
        self.sockets = {}

    def add_joint(self, joint):
        joint.part = self
        # TODO Check for existing?
        self.joints[joint.name] = joint

    def add_socket(self, socket):
        socket.part = self
        # TODO Check for existing?
        self.sockets[socket.name] = socket

    def attach(self, part):
        """
        Equivalent to self.free_joint().attach(part.free_socket()).
        Any object really with a free_socket() method could be used.
        """
        self.free_joint().attach(part.free_socket())

    def empty_joints(self):
        return [joint for joint in self.joints.itervalues()
                if not joint.connected()]

    def empty_sockets(self):
        return [socket for socket in self.sockets.itervalues()
                if not socket.connected()]

    def fill_material(self, material):
        """
        Sets the material for this part and all connected parts without
        materials already specified. Won't cross material boundaries.
        That is, if a part with a material separates this part from one
        without a material, that other part will remain without a
        material. So this sort of works like a flood-fill.
        """
        # TODO Define general fill method with callbacks.
        # TODO Consider unifying treatment of joints and sockets for cases
        # like this?
        self.material = material
        for joint in self.joints.itervalues():
            if joint.connected():
                other = joint.socket.part
                if not other.material:
                    other.fill_material(material)
        for socket in self.sockets.itervalues():
            if socket.connected():
                other = socket.joint.part
                if not other.material:
                    other.fill_material(material)

    def free_joint(self):
        """
        Returns the single free joint if exactly one, else error.
        TODO Consider going along chain looking for all free joints.
        """
        empty_joints = self.empty_joints()
        if len(empty_joints) == 1:
            return empty_joints[0]
        raise PartsError("wrong number of empty joints: %d" %
                         len(empty_joints))

    def free_socket(self):
        """
        Returns the single free socket if exactly one, else error.
        TODO Consider going along chain looking for all free sockets.
        """
        empty_sockets = self.empty_sockets()
        if len(empty_sockets) == 1:
            return empty_sockets[0]
        raise PartsError("wrong number of empty sockets: %d" %
                         len(empty_sockets))

    def reset_all(self, parent=None):
        """
        Reset this joint and everything from its socket on. Assumes
        trees only.
        """
        for joint in self.joints.itervalues():
            joint.reset_all(parent)
        for socket in self.sockets.itervalues():
            socket.reset_all(parent)

class Capsule(Part):
    """
    A part defined by a capsule defined by two like spheres.

    TODO Could easily make two spheres of different radii then allow
    meshes that are still worked with as if capsules?
    """

    def __init__(
            self, radius, half_spread,
            material=None,
            joint_pos=0.5,
            joint_rot=A(1,0,0,0)):
        """
        Param 'joint_pos' indicates how far down the end spheres the
        default joint should be. The socket is at the positive Y end.
        A corresponding joint is placed on negative Y, with the same
        axis and a matching position.
        joint_rot -- The joint axis and angle (where angle 0 means
                     positive Y for (1,0,0)? and negative angles come
                     toward negative Z?)
        material -- assigned across linkages automatically if kept None
                    TODO Support multiple materials per part?
        """
        Part.__init__(self, material)
        self.radius = radius
        self.half_spread = half_spread
        # TODO Support multiple pos and axis values?
        if joint_pos is not None:
            self.add_joint(
                Joint(self.end_pos(radius_ratio=joint_pos), joint_rot))
            self.add_socket(
                Socket(
                    self.end_pos(axis=A(0,-1,0), radius_ratio=joint_pos),
                    joint_rot))

    def end_pos(self, axis=A(0,1,0), radius_ratio=1, half_spread_ratio=1):
        """
        Returns a position local to this capsule which is a certain
        ratio of the radius away from the center of an end sphere.
        In the axis, positive/zero or negative Y controls which end the
        point is on. To reverse the default, change half_spread_ratio.

        By default, end_pos returns the top tip of the capsule.
        """
        origin = half_spread_ratio * A(0, self.half_spread, 0)
        if axis[1] < 0:
            origin = -origin
        pos = origin + unitize(axis) * radius_ratio * self.radius
        return pos

class Joint(object):
    """
    Represents a hinge joint, but might extend this to 6-DOF joints.
    Positions and axes are relative to the part coordinate frame.
    TODO 6-DOF will need more than just a single axis.

    The idea is that the joint describes the type of motion as well as
    the drivable motor. The joint also describes DOF limits.
    Hmm. Should make Motor separate to allow coupled joints.
    """

    def __init__(self, pos, axis_angle, limits=None, name='joint'):
        if not name:
            raise PartsError("name undefined")
        self.axis_angle = axis_angle
        self.key = None
        self.limits = limits
        self.name = name
        self.part = None
        self.pos = pos
        self.socket = None

    def attach(self, socket_or_part):
        socket = socket_or_part.free_socket()
        self.socket = socket
        socket.joint = self
        self.reset()
        # TODO Rotate and position the socket's part to correspond to the
        # TODO joint object and the joint/socket transforms.
        # TODO
        # TODO Allow reverse attachment from socket, where the main
        # TODO receiver always coordinates the receivee's transform?
        pass

    def connected(self):
        return self.socket is not None

    def reset(self):
        """
        Transform the attached part (if any) to conform to this part's
        transform and the constraints of the joint and socket,
        resetting all angles and translations to 0.
        """
        socket = self.socket
        if not socket: return
        other = socket.part
        # TODO How to calculate other's rot in terms of self's?
        # TODO Then need to translate it. Or do as one big transform?

    def reset_all(self, parent=None):
        """
        Reset this joint and everything from its socket on. Assumes
        trees only.
        """
        if self.connected():
            self.reset()
            part = self.socket.part
            if part != parent:
                part.reset_all(self.part)

class Socket(object):
    """
    What you plug joints into. The socket is merely a static connector.
    Because this is simulation, it's easy to plug any kind of joint
    into a generic socket.

    TODO Spore has no sockets. Joints attach anywhere. Would we have to
    TODO create and move around sockets on the fly by our model?
    """

    def __init__(self, pos, axis_angle, name='socket'):
        self.joint = None
        self.name = name
        self.part = None
        self.pos = pos
        self.axis_angle = axis_angle

    def connected(self):
        return self.joint is not None

    def free_socket(self):
        if self.connected():
            raise PartsError("not free")
        return self

    def reset(self):
        """
        Transform the attached part (if any) to conform to this part's
        transform and the constraints of the joint and socket,
        resetting all angles and translations to 0.
        """
        joint = self.joint
        if not joint: return
        other = joint.part
        # TODO How to calculate other's rot in terms of self's?
        # TODO Then need to translate it. Or do as one big transform?

    def reset_all(self, parent=None):
        """
        Reset this joint and everything from its socket on. Assumes
        trees only.
        """
        if self.connected():
            self.reset()
            part = self.joint.part
            if part != parent:
                part.reset_all(self.part)

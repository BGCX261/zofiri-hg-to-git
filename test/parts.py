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

    def __init__(self, material=None, name=None):
        self.counted_joints = 0
        self.joints = {}
        self.key = None
        self.material = material
        self.name = name
        self.pos = A(0,0,0)
        self.rot = A(1,0,0,0)

    def add_joint(self, joint):
        if not joint.name:
            self.counted_joints += 1
            joint.name = 'joint' + str(self.counted_joints)
        joint.part = self
        # TODO Check for existing?
        self.joints[joint.name] = joint

    def attach(self, part):
        """
        Equivalent to self[part.name].attach(part[self.name]). That is,
        it matches part names to joint names.
        """
        self[part.name].attach(part[self.name])

    def empty_joints(self):
        return [joint for joint in self.joints.itervalues()
                if not joint.connected()]

    def empty_sockets(self):
        """
        TODO Broken right now. Don't use.
        TODO Still define sockets as fully constrained joints?
        """
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

    def __getitem__(self, key):
        """
        Convenience
        """
        # TODO Also go along chains?
        return self.joints[key]

    def reset_all(self, parent=None):
        """
        Reset this joint and everything from its socket on. Assumes
        trees only.
        """
        for joint in self.joints.itervalues():
            joint.reset_all(parent)

class Capsule(Part):
    """
    A part defined by a capsule defined by two like spheres.

    TODO Could easily make two spheres of different radii then allow
    meshes that are still worked with as if capsules?
    """

    def __init__(self, radius, half_spread, material=None, name=None):
        Part.__init__(self, material=material, name=name)
        self.radius = radius
        self.half_spread = half_spread

    def end_pos(self, radius_ratio=1, axis=A(0,1,0), half_spread_ratio=1):
        """
        Returns a position local to this capsule which is a certain
        ratio of the radius away from the center of an end sphere.
        In the axis, positive/zero or negative Y controls which end the
        point is on. To reverse the default, change half_spread_ratio.
        Negative radius_ratio also flips top or bottom (so yes, if you
        are serious about moving away from the ends, you need to negate
        the axis Y _and_ the radius_ratio -- just seems the uncommon
        case).

        By default, end_pos returns the top tip of the capsule.
        """
        if radius_ratio < 0:
            axis[1] = -axis[1]
        radius_ratio = abs(radius_ratio)
        origin = half_spread_ratio * A(0, self.half_spread, 0)
        if axis[1] < 0:
            origin = -origin
        pos = origin + unitize(axis) * radius_ratio * self.radius
        return pos

class Joint(object):
    """
    Represents a hinge joint, but might extend this to 6-DOF joints.
    Positions and axes are relative to the part coordinate frame.

    The idea is that the joint describes the type of motion as well as
    the drivable motor. The joint also describes DOF limits.
    TODO Hmm. Should make Motor separate to allow coupled joints.
    """

    def __init__(self, pos, rot=A(1,0,0,0), limits=zeros(6), name=None):
        self.rot = rot
        self.key = None
        self.limits = limits
        self.name = name
        self.part = None
        self.pos = pos
        self.socket = None

    def attach(self, joint_or_part):
        """
        Just joints for now, please.
        """
        socket = joint_or_part
        self.socket = socket
        socket.socket = self
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
        self_trans = transform_to_mat(self)
        print 'reset: self'
        print self.pos, self.rot
        print self_trans
        print 'reset: part'
        part = self.part
        part_trans = transform_to_mat(part)
        print part.pos, part.rot
        print part_trans
        print 'self abs'
        print dot(self_trans, part_trans)
        # TODO Transform part by self, then inverse transform by socket

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

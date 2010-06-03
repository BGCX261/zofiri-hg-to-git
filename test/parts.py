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
        self.key = None

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

    def bounds_abs(self, chain=True, parent=None):
        bounds = self.bounds_rel(False)
        # print self.name, bounds
        bounds = dot(transform_to_mat(self), cat((bounds, A((1,1)))))
        bounds = bounds[0:3,:]
        # print self.name, bounds
        if chain:
            for joint in self.joints.itervalues():
                if joint.connected():
                    other = joint.socket.part
                    if other is not parent:
                        other_bounds = other.bounds_abs(parent=self)
                        # print other.name, other_bounds
                        bounds[:,0] = minimum(bounds[:,0], other_bounds[:,0])
                        bounds[:,1] = maximum(bounds[:,1], other_bounds[:,1])
                        # print self.name, bounds
        return bounds

    def bounds_rel(self, chain=True):
        if chain:
            bounds = self.bounds_abs(True)
            bounds = cat((bounds, A((1,1))))
            bounds = solve(transform_to_mat(self), bounds)
            bounds = bounds[0:3,:]
            return bounds
        else:
            return self._bounds_rel()

    def _bounds_rel(self):
        # Override for own bounds. This is the core bounds definition.
        return zeros((3,2))

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

    def part(self):
        """
        Return a canonical primitive part associated with this
        potentially abstract part. By default, just returns self
        (thereby assuming that self is primitive).
        """
        return self

    def reset_all(self, parent=None, place_at_surface=True):
        """
        Resets the whole tree of parts attached to self, except through
        parent. This means that the joints are all reset to their 0
        points, and when no parent is specified, the chain is afterwards
        moved up to just above the surface (Y of 0).

        TODO That auto placement will need to be environmentally aware
        TODO at some point to be of any use.
        """
        for joint in self.joints.itervalues():
            joint.reset_all(parent=parent)
        if place_at_surface and parent is None:
            bounds = self.bounds_rel()
            # Move the whole chain up to the surface.
            self.pos[1] += 0.01 - bounds[1,0]
            # And reset again to make everything relative to the change.
            self.reset_all(place_at_surface=False)

    def traverse(self, handler, incoming_joint=None):
        """
        Traverses trees of parts starting at self and not going back
        through incoming_joint, calling handler at each point.

        Handling loopy graphs would require more work.
        """
        try:
            handler(self, joint=incoming_joint)
        except StopIteration:
            # Prevents going deeper down this path.
            # Doesn't stop the iteration down other branches.
            return
        for joint in self.joints.itervalues():
            if joint.connected() and joint is not incoming_joint:
                socket = joint.socket
                socket.part.traverse(handler, incoming_joint=socket)

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

    def _bounds_rel(self):
        radius = A(-self.radius, self.radius)
        bounds = A(radius, radius+(-self.half_spread,self.half_spread), radius)
        return bounds

    def end_pos(self, radius_ratio=1, axis=(0,1,0), half_spread_ratio=1):
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
        axis = AA(axis)
        if radius_ratio < 0:
            axis[1] = -axis[1]
        radius_ratio = abs(radius_ratio)
        origin = half_spread_ratio * A(0, self.half_spread, 0)
        if axis[1] < 0:
            origin = -origin
        pos = origin + unitize(axis) * radius_ratio * self.radius
        return pos

class Cylinder(Part):
    """
    A cylinder along the y axis.

    I'd like to have cylinders with rounded corners (different from
    capsules), too, but I'm not sure the best way to do that.
    """

    def __init__(self, radii, material=None, name=None):
        Part.__init__(self, material=material, name=name)
        self.radii = AA(radii)

    def _bounds_rel(self):
        # Should be the same as for a box.
        bounds = A(-self.radii, self.radii).T
        return bounds

class Joint(object):
    """
    Represents a hinge joint, but might extend this to 6-DOF joints.
    Positions and axes are relative to the part coordinate frame.

    The idea is that the joint describes the type of motion as well as
    the drivable motor. The joint also describes DOF limits.
    TODO Hmm. Should make Motor separate to allow coupled joints.
    """

    def __init__(self, pos, rot=(1,0,0,0), limits=None, name=None):
        """
        rot - Where the main axis becomes x,
              the angle denotes y (rotated from old y?), and
              z is orthogonal to both (right or left handed?)
              -- Or rather, what does Bullet do?
        limits - pos,rot by x,y,z by min,max
                 Defaults to Limits.rot_x().
        """
        self.rot = AA(rot)
        self.key = None
        self.limits = limits if limits is not None else Limits.rot_x()
        self.name = name
        self.part = None
        self.pos = AA(pos)
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
        rot = vecs_to_rot(socket.rot[0:3], self.rot[0:3])
        self_trans = pos_to_mat(self.pos, rot_to_mat(rot))
        part = self.part
        part_trans = transform_to_mat(part)
        joint_abs_trans = dot(part_trans, self_trans)
        socket_trans = pos_to_mat(socket.pos)
        other_trans = solve(socket_trans, joint_abs_trans)
        mat_to_transform(other_trans, other)
        if False and other.name.startswith('wheel'):
            print '\n\n===========================\n'
            print 'self_trans'
            print self_trans
            print 'part_trans'
            print part_trans
            print 'joint_abs_trans'
            print joint_abs_trans
            print 'socket_trans'
            print socket_trans
            print 'other_trans'
            print other_trans
            print '\n---------------------------'
            print 'part:', part.name, part.pos, part.rot
            print 'joint rel:', self.name, self.pos, self.rot
            print 'socket rel:', socket.name, socket.pos, socket.rot
            print 'other:', other.name, other.pos, other.rot
            print '\n===========================\n'

    def reset_all(self, parent=None):
        """
        Reset this joint and everything from its socket on. Assumes
        trees only.
        """
        if self.connected():
            self.reset()
            part = self.socket.part
            if part != parent:
                part.reset_all(parent=self.part)

class Limits(object):

    @staticmethod
    def rot_x(min_max=(-pi,pi)):
        """
        Limits to allow rotation only around the x axis.

        TODO Make (-inf,inf) the limits, and support inf to mean
        TODO unconstrained.
        """
        limits = Limits.zero()
        limits[1,0,:] = min_max
        return limits

    @staticmethod
    def zero():
        return zeros((2,3,2))

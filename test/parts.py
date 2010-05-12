PartsError = Exception

class Material(object):

    def __init__(self, density, color):
        """
        color -- a 32-bit hex ARGB value (or someday mixtures of frequency (mean and std), reflectivity (including specular vs. diffuse and opacity), ...?).
                 Or emphasize AHSV (i.e., opacity/absorption, frequency, inverse deviation (down to zero), reflectivity?)
                 Light itself might be best defined in terms of mixtures of frequency (Gaussian mean and std?) with radiant intensity (W/sr) as the Y axis?
        """
        self.density = density
        self.color = color

Material.DEFAULT = Material(1, 0xFFFFFF)

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
    parts are just parts and incorporat
    """

    def __init__(self):
        self.joints = {}
        self.key = None
        self.sockets = {}

    def add_joint(self, joint):
        joint.part = self
        # TODO Check for existing?
        self.joints[joint.name] = joint

    def attach(self, part):
        """
        Equivalent to self.free_joint().attach(part.free_socket()).
        Any object really with a free_socket() method could be used.
        """
        self.free_joint().attach(part.free_socket())

    def empty_joints(self):
        return [joint for joint in self.joints if not joint.connected()]

    def empty_sockets(self):
        return [socket for socket in self.sockets if not socket.connected()]

    def free_joint(self):
        """Returns the single free joint if exactly one, else error."""
        empty_joints = self.empty_joints()
        if len(empty_joints) == 1:
            return empty_joints[0]
        raise PartsError("wrong number of empty joints: %d" % len(empty_joints))

    def free_socket(self):
        """Returns the single free socket if exactly one, else error."""
        empty_sockets = self.empty_sockets()
        if len(empty_sockets) == 1:
            return empty_sockets[0]
        raise PartsError("wrong number of empty sockets: %d" % len(empty_sockets))

class Capsule(Part):
    """
    TODO Should make shapes separate from parts to allow swapping out shapes.
    """

    def __init__(self, radius, half_spread,
                 material=None,
                 joint_pos=0.5,
                 joint_rot=(1,0,0,0)):
        """
        Param 'joint_pos' indicates how far down the end spheres the
        default joint should be. The socket is at the positive Y end.
        A corresponding joint is placed on negative Y, with the same
        axis and a matching position.
        joint_rot - The joint axis and angle (where angle 0 means positive Y for (1,0,0)? and negative angles come toward negative Z?)

        TODO Support multiple materials per part?
        material -- assigned across linkages automatically if kept None
        """
        Part.__init__(self)
        self.radius = radius
        self.half_spread = half_spread
        # TODO Support multiple pos and axis values?
        if joint_pos is not None:
            self.add_joint(Joint(half_spread + joint_pos * radius, joint_rot))

    def place(self, axis, radius_ratio):
        """
        Returns a position local to this capsule which is a certain
        ratio of the radius away from the center of an end circle.
        In the axis, positive or negative Y controls which end the
        point is on.
        """

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
        self.limits = limits
        self.name = name
        self.part = None
        self.pos = pos
        self.socket = None;

    def attach(self, socket):
        # TODO Rotate and position the socket object to correspond to the joint object.
        pass

    def connected(self):
        return self.socket is not None

class Socket(object):
    """
    What you plug joints into. The socket is merely a
    static connector. Because this is simulation, it's easy to plug any
    kind of joint into a generic socket.
    """

    def __init__(self, pos, axis_angle, name='socket'):
        self.joint = None;
        self.part = None
        self.pos = pos
        self.axis_angle = axis_angle

    def connected(self):
        return self.joint is not None

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
    """

    def __init__(self):
        self.id = None
        self.joints = []
        self.sockets = []

    def add_joint(self, joint):
        joint.part = self
        self.joints.append(joint)

class Capsule(Part):

    def __init__(self, radius, half_spread, joint_pos=0.5, joint_axis=(1,0,0)):
        """
        Param 'joint_pos' indicates how far down the end spheres the
        default joint should be. The hinge is at the positive Y end.
        A corresponding socket is placed on negative Y, with the same
        axis and a matching position.
        """
        Part.__init__(self)
        self.radius = radius
        self.half_spread = half_spread
        self.add_joint(Joint(half_spread + joint_pos * radius, joint_axis))

class Joint(object):
    """
    Represents a hinge joint, but might extend this to 6-DOF joints.
    Positions and axes are relative to the part coordinate frame.
    """

    def __init__(self, pos, axis):
        self.next = None;
        self.part = None
        self.pos = pos
        self.axis = axis

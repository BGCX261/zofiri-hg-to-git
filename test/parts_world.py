class Arm(object):

    def __init__(self, facing_x):
        """
        facing_x - does the palm face positive X (1) or negative (-1)
        TODO Parameterize this some for length or width ratios, ...
        """
        from parts import Capsule
        shoulder = Capsule(0.05, 0, joint_pos=0, joint_rot=(0,0,facing_x,0))
        upper = Capsule(0.03, 0.125)
        shoulder.attach(upper)
        self.shoulder = shoulder

    def free_socket(self):
        # TODO Could actually look along the entire chain, but for now assume
        # at the shoulder.
        return self.shoulder.free_socket()

class Humanoid(object):
    """
    Creates a humanoid robot facing toward the positive Z axis.
    TODO Parameterize this some for height, build, ...
    """

    def __init__(self):
        from parts import Material
        torso = Torso()
        torso.attach(WheeledBase())
        # TODO Need a better automate resolve to Part methods.
        # TODO That would allow 'torso' instead of 'torso.chest'.
        # TODO Even awesomer would be torso['arm_right'].attach(...)
        torso.chest.joints['arm_right'].attach(Arm(-1))
        torso.chest.joints['arm_left'].attach(Arm(1))
        torso.chest.fill_material(Material(1, 0xFF808080))
        torso.chest.reset_all()
        # TODO Automatically position just above origin.
        self.torso = torso

class Torso(object):

    def __init__(self):
        from parts import Capsule, Joint
        chest = Capsule(0.1, 0.0725)
        abdomen = Capsule(0.08, 0.05)
        # TODO Most convenient way to name the joint?
        chest.attach(abdomen)
        abdomen.free_joint().axis_angle = (0,1,0,0)
        chest.add_joint(
            Joint(chest.end_pos((1,1,0),0.8), (0,0,-1,0), name='arm_right'))
        chest.add_joint(
            Joint(chest.end_pos((-1,1,0),0.8), (0,0,1,0), name='arm_left'))
        self.chest = chest
        self.abdomen = abdomen

    def attach(self, other):
        return self.abdomen.attach(other)

    def free_socket(self):
        # This is at the neck.
        return self.chest.free_socket()

class WheeledBase(object):

    def __init__(self):
        from parts import Capsule, Socket
        hips = Capsule(0.12, 0.02, joint_pos=None)
        hips.add_socket(Socket(hips.end_pos((0,1,0),0.8), (0,1,0,0), 'waist'))
        self.hips = hips
        # TODO Wheels
        # wheel_shape_id = tx.capsule(0.2,0.1)
        # tx.shape_scale(wheel_shape_id, (1,0.3,1))
        # tx.body(wheel_shape_id, tx.material(0.1, 0x303030), (0,3.5,0))

    def free_socket(self):
        return self.hips.free_socket()

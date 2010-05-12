class Arm(object):

    def __init__(self, facing_x):
        """
        facing_x - does the palm face positive X (1) or negative (-1)
        TODO Parameterize this some for length or width ratios, ...
        """
        from parts import Capsule
        shoulder = Capsule(0.05, 0, joint_pos=0, joint_rot=(0,0,facing_x*1,0))
        upper = Capsule(0.03, 0.125)
        shoulder.attach(upper)
        self.shoulder = shoulder

    def free_socket(self):
        # TODO Could actually look along the entire chain, but for now assume at the shoulder.
        return self.shoulder.free_socket()

class Humanoid(object):
    """
    Creates a humanoid robot facing toward the positive Z axis.
    TODO Parameterize this some for height, build, ...
    """

    def __init__(self):
        from parts import Capsule, Joint, Material
        chest = Capsule(0.1, 0.725)
        abdomen = Capsule(0.08, 0.05)
        # TODO Most convenient way to name the joint?
        chest.attach(abdomen)
        abdomen.free_joint().axis_angle = (0,1,0,0)
        abdomen.attach(WheeledBase())
        chest.add_joint(Joint(chest.place((1,1,0),0.8), (0,0,-1,0), name="arm_right"))
        chest.attach(Arm(-1))
        chest.add_joint(Joint(chest.place((-1,1,0),0.8), (0,0,1,0), name="arm_left"))
        chest.attach(Arm(1))
        chest.set_material(Material(1, 0xFF808080))

class Torso(object):

    def __init__(self):
        pass

class WheeledBase(object):

    def __init__(self):
        pass

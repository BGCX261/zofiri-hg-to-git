class Arm(object):

    def __init__(self, side_x):
        """
        side_x - is the arm on the left (-1) or right (1)
        TODO Parameterize this some for length or width ratios, ...
        """
        from parts import Capsule, Joint
        self.name = 'arm_' + side_name(side_x)
        shoulder = Capsule(0.05, 0, name='shoulder')
        shoulder.add_joint(Joint((0,0,0), rot=(0,0,-side_x,0), name='chest'))
        shoulder.add_joint(Joint((0,0,0), name='upper'))
        upper = Capsule(0.03, 0.1, name='upper')
        upper.add_joint(Joint(upper.end_pos(0.5), name='shoulder'))
        upper.add_joint(Joint(upper.end_pos(-1), rot=(0,1,0,0), name='elbow'))
        shoulder.attach(upper)
        self.shoulder = shoulder
        elbow = Capsule(0.04, 0, name='elbow')
        elbow.add_joint(Joint(elbow.end_pos(0.2), rot=(0,1,0,0), name='upper'))
        upper.attach(elbow)

    def __getitem__(self, key):
        # TODO Could actually look along the entire chain, but for now assume
        # at the shoulder.
        return self.shoulder[key]

class Head(object):

    def __init__(self):
        from parts import A, Capsule, Joint, Limits, pi
        self.name = 'head'
        neck = Capsule(0.04, 0, name='neck')
        neck.add_joint(Joint(
            neck.end_pos(-0.3),
            rot=(0,1,0,0),
            name='chest',
            limits=Limits.rot_x(A(-0.5,0.5)*pi)))
        neck.add_joint(Joint(
            neck.end_pos(0.3),
            name='skull',
            limits=Limits.rot_x(A(-0.5,0.1)*pi)))
        skull = Capsule(0.06, 0.01, name='skull')
        skull.add_joint(Joint(skull.end_pos(-1), name='neck'))
        neck.attach(skull)
        self.neck = neck
        self.skull = skull
        self._add_eye(-1)
        self._add_eye(1)

    def _add_eye(self, side_x):
        from parts import Capsule, Joint, Material
        name = 'eye_' + side_name(side_x)
        eye = Capsule(0.015, 0, name=name, material=Material(0.001,0xFF0060A0))
        eye.add_joint(Joint((0,0,0), name='skull'))
        skull = self.skull
        skull.add_joint(Joint(skull.end_pos(0.8,axis=(side_x*0.2,0,1)), name=name))
        skull.attach(eye)

    def __getitem__(self, key):
        # TODO Could actually look along the entire chain, but for now assume
        # at the neck.
        return self.neck[key]

class Humanoid(object):
    """
    Creates a humanoid robot facing toward the positive Z axis.
    TODO Parameterize this some for height, build, ...
    """

    def __init__(self):
        from mat import A, pi
        from parts import Material
        torso = Torso()
        # TODO Need a better inherit Part or resolve to Part methods.
        # TODO That would allow 'torso' instead of 'torso.abdomen' or
        # TODO 'torso.chest'.
        torso.abdomen.attach(WheeledBase())
        torso.chest.attach(Arm(-1))
        torso.chest.attach(Arm(1))
        torso.chest.attach(Head())
        torso.chest.fill_material(Material(1, 0xFF808080))

        # Pick an X,Z that we know are safe.
        # TODO Put a setter on pos that always float64-arrays the contents.
        torso.chest.pos = A(-0.2,0,0.2)
        # TODO Automate X,Z placement based on environment.
        # TODO Try to pick a nearby place that isn't occupied.

        # TODO Things aren't quite stable if I do this.
        # TODO Might be bugs somewhere in the rotation handling.
        # torso.chest.rot = A(0,1,0,0.25*pi)

        # Automatically position just above origin.
        torso.chest.reset_all()
        # print "torso bounds:", torso.chest.bounds_abs()

        self.torso = torso

    def part(self):
        return self.torso.chest

def side_name(side_x):
    return 'left' if side_x < 0 else 'right'

class Torso(object):

    def __init__(self):
        from mat import A, pi
        from parts import Capsule, Joint, Limits
        chest = Capsule(0.1, 0.0725, name='chest')
        chest.add_joint(Joint(chest.end_pos(0.5), name='head'))
        chest.add_joint(Joint(
            chest.end_pos(-0.5),
            name='abdomen',
            limits=Limits.rot_x(A(-0.1,0.5)*pi)
        ))
        abdomen = Capsule(0.08, 0.05, name='abdomen')
        abdomen.add_joint(Joint(abdomen.end_pos(0.5), name='chest'))
        abdomen.add_joint(
            Joint(abdomen.end_pos(-0.5), (0,1,0,0), name='base'))
        chest.attach(abdomen)
        chest.add_joint(
            Joint(chest.end_pos(1.4,(2.5,1,0)), (0,0,-1,0), name='arm_right'))
        chest.add_joint(
            Joint(chest.end_pos(1.4,(-2.5,1,0)), (0,0,1,0), name='arm_left'))
        chest.add_joint(Joint(chest.end_pos(), (0,1,0,0), name='head'))
        self.chest = chest
        self.abdomen = abdomen

    def free_socket(self):
        # This is at the neck.
        return self.chest.free_socket()

class WheeledBase(object):

    def __init__(self):
        from parts import Capsule, Joint
        hips = Capsule(0.12, 0.1, name='hips')
        hips.add_joint(Joint(hips.end_pos(0.8), (0,1,0,0), name='abdomen'))
        self.hips = hips
        self.name = 'base'
        # TODO Wheels
        self.wheel_right = self._add_wheel(-1)
        self.wheel_left = self._add_wheel(1)
        # wheel_shape_id = tx.capsule(0.2,0.1)
        # tx.shape_scale(wheel_shape_id, (1,0.3,1))
        # tx.body(wheel_shape_id, tx.material(0.1, 0x303030), (0,3.5,0))

    def _add_wheel(self, side_x):
        from parts import Cylinder, Joint, Material
        wheel = Cylinder(
            (0.2,0.025,0.2),
            material=Material(1, 0xFF202020),
            name='wheel_'+side_name(side_x))
        wheel.add_joint(Joint((0,0,0), rot=(0,1,0,0), name='hips'))
        hips = self.hips
        hips.add_joint(Joint(
            hips.end_pos(1.2,(side_x,0,0),-1),
            (-side_x,0,0,0),
            name=wheel.name))
        hips.attach(wheel)

    def __getitem__(self, key):
        return self.hips[key]

class Arm(object):

    def __init__(self, side_x):
        """
        side_x - is the arm on the left (-1) or right (1)
        TODO Parameterize this some for length or width ratios, ...
        """
        from parts import Capsule, Joint, pi
        self.name = 'arm_' + side_name(side_x)
        shoulder = Capsule(0.05, 0, name='shoulder')
        shoulder.add_joint(Joint((0,0,0), rot=(0,0,-side_x,0), name='chest'))
        shoulder.add_joint(Joint((0,0,0), name='upper'))
        upper = Capsule(0.03, 0.05, name='upper')
        upper.add_joint(Joint(upper.end_pos(0.5), name='shoulder'))
        upper.add_joint(Joint(upper.end_pos(-1), rot=(0,1,0,0), name='elbow'))
        shoulder.attach(upper)
        self.shoulder = shoulder
        elbow = Capsule(0.03, 0.01, name='elbow')
        elbow.add_joint(Joint(elbow.end_pos(0.2), rot=(0,1,0,0), name='upper'))
        elbow.add_joint(Joint(elbow.end_pos(-0.2), name='lower'))
        upper.attach(elbow)
        lower = Capsule(0.03, 0.05, name='lower')
        # TODO The angle on the joint rot doesn't currently do anything.
        lower.add_joint(Joint(
            lower.end_pos(), rot=(-side_x,0,0,0.5*pi), name='elbow'))
        lower.add_joint(Joint(lower.end_pos(-1), rot=(0,1,0,0), name='hand'))
        elbow.attach(lower)
        self.lower = lower
        lower.attach(Hand(side_x))

    def __getitem__(self, key):
        # TODO Could actually look along the entire chain, but for now assume
        # at the shoulder.
        return self.shoulder[key]

class Finger(object):

    def __init__(self, phalanx_count, parent='wrist'):
        from parts import A, Capsule, Joint, Limits, pi
        self.name = 'finger'
        # Phalanx 0 really is more of a metacarpal spread thing.
        # Just that the naming convention makes this code simpler.
        self.spread = current = Capsule(0.01, 0, name='spread')
        current.add_joint(Joint(
            current.end_pos(0.5),
            limits=Limits.rot_x(A(-1,1)*0.2*pi),
            name=parent))
        for n in xrange(phalanx_count):
            next = Capsule(0.01, 0.01, name='phalanx'+str(n))
            next.add_joint(Joint(
                next.end_pos(0.5),
                rot=(0,0,1,0),
                limits=Limits.rot_x(A(-0.01,0.5)*pi),
                name=current.name))
            current.add_joint(Joint(
                current.end_pos(-0.5), rot=(0,0,1,0), name=next.name))
            current.attach(next)
            current = next

    def __getitem__(self, key):
        # TODO Could actually look along the entire chain, but for now assume
        # at spread.
        return self.spread[key]

class Hand(object):

    def __init__(self, side_x):
        from parts import A, Capsule, Joint, Limits, pi
        self.name = 'hand'
        self.wrist = wrist = Capsule(0.03, 0, name='wrist')
        wrist.add_joint(Joint(
            wrist.end_pos(0.3), rot=(0,1,0,0), name='lower'))
        # Fingers
        for f in xrange(3):
            wrist.add_joint(Joint(
                wrist.end_pos(axis=(-side_x,-2,2*(f-1))),
                rot=(side_x,0,0,0),
                name='finger'+str(f)))
            finger = Finger(3)
            finger.name += str(f)
            wrist.attach(finger)
        # Thumb
        wrist.add_joint(Joint(
            wrist.end_pos(axis=(-side_x,0,1), half_spread_ratio=0),
            rot=(0,-side_x,0,0),
            limits=Limits.rot_x(A(-0.8,0)*pi),
            name='thumbTwist'))
        thumbTwist = Capsule(0.015, 0.005,  name='thumbTwist')
        thumbTwist.add_joint(Joint(
            thumbTwist.end_pos(0.2), rot=(0,0,-side_x,0), name='wrist'))
        thumbTwist.add_joint(Joint(
            thumbTwist.end_pos(-1, axis=(0,0,-1), half_spread_ratio=-1),
            rot=(side_x,0,0,0),
            name='thumb'))
        wrist.attach(thumbTwist)
        thumb = Finger(2, parent='thumbTwist')
        thumb.name = 'thumb'
        thumbTwist.attach(thumb)

    def __getitem__(self, key):
        # TODO Could actually look along the entire chain, but for now assume
        # at the wrist.
        return self.wrist[key]

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

def side_z_name(side_z):
    return 'back' if side_z < 0 else 'front'

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
        # Wheels.
        self.wheel_right = self._add_wheel(-1)
        self.wheel_left = self._add_wheel(1)
        # Casters.
        self._add_casters()

    def _add_casters(self):
        from parts import Capsule, Cylinder, Joint, Limits
        hips = self.hips
        wheel = self.wheel_left
        support = Cylinder(
            (0.9*self.hips.radius,0.02,wheel.radii[0]),
            name='support')
        support.add_joint(Joint(
            (0,0,0),
            rot=(0,1,0,0),
            limits=Limits.zero(),
            name='hips'))
        hips.add_joint(Joint(
            hips.end_pos(-1),
            rot=(0,1,0,0),
            limits=Limits.zero(),
            name='support'))
        hips.attach(support)
        # Automate caster position to same bottom level as wheels.
        offset_y = wheel.pos[1] - support.pos[1]
        for side_z in (-1,1):
            caster = Capsule(
                0.49*offset_y, 0,
                material=wheel.material,
                name='caster_'+side_z_name(side_z))
            support.add_joint(Joint(
                (0,0,side_z*support.radii[2]),
                name=caster.name))
            caster.add_joint(Joint((0,0,0), name=support.name))
            support.attach(caster)

    def _add_wheel(self, side_x):
        from parts import Cylinder, Joint, Material
        wheel = Cylinder(
            (0.2,0.04,0.2),
            material=Material(1, 0xFF202020),
            name='wheel_'+side_name(side_x))
        wheel.add_joint(Joint((0,0,0), rot=(0,1,0,0), name='hips'))
        hips = self.hips
        hips.add_joint(Joint(
            hips.end_pos(1,(side_x,0,0),-1) + side_x*wheel.radii[1],
            rot=(-side_x,0,0,0),
            name=wheel.name))
        hips.attach(wheel)
        return wheel

    def __getitem__(self, key):
        return self.hips[key]

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
    say for different sizes and so on.
    """

    def __init__(self):
        # TODO Pass in a shape or let shapes subclass parts?
        pass

class Capsule(Part):
    pass

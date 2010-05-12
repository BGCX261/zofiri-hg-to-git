"""
For math utilities, simplifying things from numpy and math.
"""

def A(*vals):
    """
    Short for "array". It's simpler for numpy array creation to say
    A(x,y,z) than array((x,y,z)) or to say A((a,b),(c,d)) than
    array(((a,b),(c,d))).

    It abusively uses an uppercase 'A' to avoid collisions with
    easily imagined variable names such as 'a'.
    """
    return array(vals)

# Full form handily available.
from numpy import array

# Make e easily available.
from math import e

# Because there is no built-in constant or literal for infinity.
inf = float('inf')

# Make pi easily available.
from math import pi

def unitize(vector):
    """
    TODO Support unitizing all vectors along dimension d.
    """
    from numpy.linalg import norm
    return vector / norm(vector)

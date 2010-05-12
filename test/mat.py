"""
For math utilities, simplifying things from numpy and math.
"""

# Import handy things from elsewhere for easier importing from here.
from math import e, pi
from numpy import array, sign
from numpy.linalg import norm

def A(*vals):
    """
    Short for "array". It's simpler for numpy array creation to say
    A(x,y,z) than array((x,y,z)) or to say A((a,b),(c,d)) than
    array(((a,b),(c,d))).

    It abusively uses an uppercase 'A' to avoid collisions with
    easily imagined variable names such as 'a'.
    """
    return array(vals)

# Because there is no built-in constant or literal for infinity.
# Numpy defines various names for infinity, based on a definition in native code.
# This will do for me here. And it seems the preferred name for Python.
inf = float('inf')

def unitize(vector):
    """
    TODO Support unitizing all vectors along dimension d.
    """
    return vector / norm(vector)

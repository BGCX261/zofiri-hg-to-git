"""
For math utilities, simplifying things from numpy and math.
"""

# Import handy things from elsewhere for easier importing from here.
from math import e, pi
from numpy import \
    abs, append, arccos as acos, arcsin as asin, arctan2 as atan2, argmax, \
    argmin, array, concatenate as cat, cos, cross, dot, eye, float64, min, \
    max, outer, real, sign, sin
from numpy.linalg import eig, norm

def A(*vals):
    """
    Short for "array". It's simpler for numpy array creation to say
    A(x,y,z) than array((x,y,z)) or to say A((a,b),(c,d)) than
    array(((a,b),(c,d))). 'A' also assumes float64.

    It abusively uses an uppercase 'A' to avoid collisions with
    easily imagined variable names such as 'a'.
    """
    return array(vals, dtype=float64)

# Because there is no built-in constant or literal for infinity.
# Numpy defines various names for infinity, based on a definition in native code.
# This will do for me here. And it seems the preferred name for Python.
inf = float('inf')

def mat_to_rot(mat):
    """
    Given an rotation matrix, return an (x, y, z, angle) rotation. See
    also: http://en.wikipedia.org/wiki/Rotation_matrix

    TODO Support a fast batch form?
    """

    # TODO Is there a faster way than eigen calculation?
    # TODO Wikipedia also gives the following:
    #
    # A partial approach is as follows.
    # x = Qzy-Qyz
    # y = Qxz-Qzx
    # z = Qyx-Qxy
    # r = norm(x,norm(y,z))
    # t = Qxx+Qyy+Qzz
    # theta = atan2(r,t-1)
    # 
    # The x, y, and z components of the axis would then be divided by r. A fully robust approach will use different code when t is negative, as with quaternion extraction. When r is zero because the angle is zero, an axis must be provided from some source other than the matrix.

    # Find the eigenvector with eigenvalue 1.
    val, vec = eig(mat)
    i = argmin(1 - val)
    axis = real(vec[:,i])
    # Find the angle as Wikipedia suggests.
    # TODO Surely a faster way exists.
    # Find any orthogonal vector.
    a = (1,0,0)
    if min(abs(axis - a)) < 0.1:
        # Just need any non-parallel
        a = (0,0,1)
    a = cross(axis, a)
    # Find the signed angle between a and rotated a.
    b = dot(mat, a)
    angle = atan2(dot(cross(a,b),axis), dot(a,b))
    return append(axis, angle)

def rot_to_mat(rot):
    """
    Given an (x, y, z, angle) rotation, create a rotation matrix fit
    for column vector coordinates. See also:
    http://en.wikipedia.org/wiki/Rotation_matrix

    TODO Support a fast batch form?
    """
    axis = unitize(rot[0:-1])
    angle = rot[3]
    out = outer(axis, axis)
    skew = A(
        (0, -axis[2], axis[1]),
        (axis[2], 0, -axis[0]),
        (-axis[1], axis[0], 0))
    mat = out + cos(angle)*(eye(3)-out) + sin(angle)*skew
    return mat

def transform_to_mat(transform):
    """
    Turns an object with pos and rot members to a 4x4 transformation
    matrix.
    """
    mat = rot_to_mat(transform.rot)
    mat = cat((mat, A(*(transform.pos,)).T), axis=1)
    mat = cat((mat, A((0,0,0,1))))
    return mat

def unitize(vector):
    """
    TODO Support unitizing all vectors along dimension d.
    """
    return vector / norm(vector)

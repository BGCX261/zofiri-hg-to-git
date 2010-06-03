"""
For math utilities, simplifying things from numpy and math.
"""

# Import handy things from elsewhere for easier importing from here.
from math import e, pi
from numpy import \
    abs, any, append, arccos as acos, arcsin as asin, arctan2 as atan2, argmax, \
    argmin, array, concatenate as cat, cos, cross, dot, eye, float64, isnan, \
    minimum, maximum, ones, outer, real, sign, sin, zeros
from numpy.linalg import eig, norm, solve

def A(*vals):
    """
    Short for "array". It's simpler for numpy array creation to say
    A(x,y,z) than array((x,y,z)) or to say A((a,b),(c,d)) than
    array(((a,b),(c,d))). 'A' also assumes float64.

    It abusively uses an uppercase 'A' to avoid collisions with
    easily imagined variable names such as 'a'.
    """
    return array(vals, dtype=float64)

def AA(seq_or_array):
    """
    For keeping an array as an array or turning another sequence into
    one.
    """
    return array(seq_or_array, copy=False, dtype=float64)

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
    a = cross(axis, (1,0,0))
    if any(isnan(a)):
        # Just need any non-parallel
        a = cross(axis, (0,0,1))
    # Find the signed angle between a and rotated a.
    b = dot(mat, a)
    angle = atan2(dot(cross(a,b),axis), dot(a,b))
    return append(axis, angle)

def mat_to_transform(mat, transform=None):
    """
    Turns an object with pos and rot members to a 4x4 transformation
    matrix.
    """
    if transform is None:
        transform = {}
    transform.rot = mat_to_rot(mat[0:3,0:3])
    transform.pos = mat[0:3,3]
    return transform

def pos_to_mat(pos, rot_mat=None):
    """
    Given an (x, y, z) position, create a 4x4 homogeneous transformation matrix.
    """
    mat = eye(3) if rot_mat is None else rot_mat
    mat = cat((mat, A(*(pos,)).T), axis=1)
    mat = cat((mat, A((0,0,0,1))))
    return mat

def rot_to_mat(rot):
    """
    Given an (x, y, z, angle) rotation, create a 3x3 rotation matrix fit
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
    return pos_to_mat(transform.pos, mat)

def unitize(vector):
    """
    TODO Support unitizing all vectors along dimension d.
    """
    return vector / norm(vector)

def vecs_to_rot(a, b):
    """
    Finds the rotation from vector a to vector b using as axis an
    orthogonal vector.
    """
    axis = unitize(cross(a,b))
    if any(isnan(axis)):
        # Parallel vectors -> No rotation, arbitrary axis.
        return (1,0,0,0)
    angle = atan2(dot(cross(a,b),axis), dot(a,b))
    return append(axis, angle)

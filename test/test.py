def build_arm(tx, material_id, side, chest_id, chest_pos):
    from math import pi
    from numpy.core import array
    # TODO side should affect axis, too.
    shoulder_offset = (0,0.125,side*0.13)
    shoulder_pos = chest_pos + shoulder_offset
    shoulder_id = tx.body(tx.capsule(0.05,0), material_id, shoulder_pos)
    shoulder_hinge_id = tx.hinge(shoulder_id,(0,0,0),(1,0,0), chest_id,shoulder_offset,(1,0,0))
    upper_arm_id = tx.body(tx.capsule(0.03,0.25), material_id, shoulder_pos-(0,0.14,0))
    upper_arm_hinge_id = tx.hinge(upper_arm_id,(0,0.14,0),(0,0,1), shoulder_id,(0,0,0),(0,0,1))

def build_base(tx, material_id, abdomen_id, abdomen_pos):
    from math import pi
    from numpy.core import array
    hips_pos = abdomen_pos - (0,0.23,0)
    hips_id = tx.body(tx.capsule(0.12,0.04), material_id, hips_pos)
    hips_hinge_id = tx.hinge(hips_id,(0,0.1,0),(0,1,0), abdomen_id,(0,-0.13,0),(0,1,0))

def build_body(tx):
    from math import pi
    from numpy.core import array
    metal_id = tx.material(1, 0x808080)
    abdomen_pos = array((0.5,0.4,-0.5))
    abdomen_id = tx.body(tx.capsule(0.08,0.1), metal_id, abdomen_pos)
    build_base(tx, metal_id, abdomen_id, abdomen_pos)
    # TODO Automate linkage displacement by size.
    chest_pos = abdomen_pos + (0,0.324,0)
    chest_id = tx.body(tx.capsule(0.1,0.15), metal_id, chest_pos)
    waist_id = tx.hinge(abdomen_id,(0,0.09,0),(0,0,1), chest_id,(0,-0.125,0),(0,0,1))
    tx.hinge_limit(waist_id, array((-0.5,0.1))*pi)
    build_arm(tx, metal_id, 1, chest_id, chest_pos)
    build_arm(tx, metal_id, -1, chest_id, chest_pos)
    build_hand(tx, metal_id)

def build_hand(tx, material_id):
    from math import pi
    from numpy.core import array
    # TODO Support sending all before reading.
    origin = array((0,2,0)) # + random noise in pos/rot?
    carpal_id = tx.body(tx.capsule(0.01,0.05), material_id, origin, (0,0,1,0.5*pi))
    # 4 fingers.
    for f in range(4):
        build_finger(tx, carpal_id, material_id, 4, origin, array((0.022*(f-1.5),-0.01,0)))

def build_finger(tx, carpal_id, material_id, digit_count, origin, position):
    """
    Builds a "finger" chain. Probably base this on general limb linkage
    function later.
    """
    from math import pi
    from numpy.core import array
    # The carpal is rotated, so we need to transform the finger position.
    # TODO Use less hackish transforming.
    carpal_hinge_pos = (position[1], position[0], position[2])
    radius = 0.008
    spread = 0.025
    shape_id = tx.capsule(radius, spread)
    pos_b = array((0, 0.5*(radius+spread), 0))
    pos_a = -pos_b
    position += origin + pos_a
    finger_id = tx.body(shape_id, material_id, position)
    hinge_id = tx.hinge(carpal_id, carpal_hinge_pos, (0,0,1), finger_id, pos_b, (0,0,1))
    tx.hinge_limit(hinge_id, array((-0.3,0.3))*pi)
    # Values common to remaining grip hinges.
    axis = (1,0,0)
    limit = array((-0.4,0.05))*pi
    for d in range(1,digit_count):
        position += 2*pos_a
        finger_id2 = tx.body(shape_id, material_id, position)
        hinge_id = tx.hinge(finger_id, pos_a, axis, finger_id2, pos_b, axis)
        tx.hinge_limit(hinge_id, limit)
        finger_id = finger_id2

def main():
    from zofiri import Connection
    conn = Connection()
    tx = conn.transaction()
    try:
        build_body(tx)
    finally:
        tx.close()

if __name__ == '__main__':
    main()

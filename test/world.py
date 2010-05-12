# TODO Better robot model structures to automate some of these offsets.
# TODO Should have easy 'attach' features.
# TODO Then that also gives the model for inverse kinematics/dynamics, too.
# TODO Should be able to change scales, etc., much easier that way, too.

def build_arm(tx, material_id, side, chest_id, chest_pos):
    # TODO side should affect axis, too.
    shoulder_offset = (0,0.125,side*0.13)
    shoulder_pos = chest_pos + shoulder_offset
    shoulder_id = tx.body(tx.capsule(0.05,0), material_id, shoulder_pos)
    shoulder_hinge_id = tx.hinge(shoulder_id,(0,0,0),(1,0,0), chest_id,shoulder_offset,(1,0,0))
    upper_arm_id = tx.body(tx.capsule(0.03,0.25), material_id, shoulder_pos-(0,0.14,0))
    upper_arm_hinge_id = tx.hinge(upper_arm_id,(0,0.14,0),(0,0,1), shoulder_id,(0,0,0),(0,0,1))

def build_base(tx, material_id, abdomen_id, abdomen_pos):
    hips_pos = abdomen_pos - (0,0.23,0)
    hips_id = tx.body(tx.capsule(0.12,0.04), material_id, hips_pos)
    hips_hinge_id = tx.hinge(hips_id,(0,0.1,0),(0,1,0), abdomen_id,(0,-0.13,0),(0,1,0))
    wheel_shape_id = tx.capsule(0.2,0.1)
    tx.shape_scale(wheel_shape_id, (1,0.3,1))
    tx.body(wheel_shape_id, tx.material(0.1, 0x303030), (0,3.5,0))

def build_body(tx):
    from mat import A, pi
    metal_id = tx.material(1, 0x808080)
    abdomen_pos = A(0.5,0.4,-0.5)
    abdomen_id = tx.body(tx.capsule(0.08,0.1), metal_id, abdomen_pos)
    build_base(tx, metal_id, abdomen_id, abdomen_pos)
    # TODO Automate linkage displacement by size.
    chest_pos = abdomen_pos + (0,0.324,0)
    chest_id = tx.body(tx.capsule(0.1,0.15), metal_id, chest_pos)
    waist_id = tx.hinge(abdomen_id,(0,0.09,0),(0,0,1), chest_id,(0,-0.125,0),(0,0,1))
    tx.hinge_limit(waist_id, A(-0.5,0.1)*pi)
    build_arm(tx, metal_id, 1, chest_id, chest_pos)
    build_arm(tx, metal_id, -1, chest_id, chest_pos)
    build_hand(tx, metal_id)

def build_hand(tx, material_id):
    from mat import A, pi
    # TODO Support sending all before reading.
    origin = A(0,2,0) # + random noise in pos/rot?
    carpal_id = tx.body(tx.capsule(0.01,0.05), material_id, origin, (0,0,1,0.5*pi))
    # 4 fingers.
    for f in range(4):
        build_finger(tx, carpal_id, material_id, 4, origin, A(0.022*(f-1.5),-0.01,0))

def build_finger(tx, carpal_id, material_id, digit_count, origin, position):
    """
    Builds a "finger" chain. Probably base this on general limb linkage
    function later.
    """
    from mat import A, pi
    # The carpal is rotated, so we need to transform the finger position.
    # TODO Use less hackish transforming.
    carpal_hinge_pos = (position[1], position[0], position[2])
    radius = 0.008
    spread = 0.025
    shape_id = tx.capsule(radius, spread)
    pos_b = A(0, 0.5*(radius+spread), 0)
    pos_a = -pos_b
    position += origin + pos_a
    finger_id = tx.body(shape_id, material_id, position)
    hinge_id = tx.hinge(carpal_id, carpal_hinge_pos, (0,0,1), finger_id, pos_b, (0,0,1))
    tx.hinge_limit(hinge_id, A(-0.3,0.3)*pi)
    # Values common to remaining grip hinges.
    axis = (1,0,0)
    limit = A(-0.4,0.05)*pi
    for d in range(1,digit_count):
        position += 2*pos_a
        finger_id2 = tx.body(shape_id, material_id, position)
        hinge_id = tx.hinge(finger_id, pos_a, axis, finger_id2, pos_b, axis)
        tx.hinge_limit(hinge_id, limit)
        finger_id = finger_id2

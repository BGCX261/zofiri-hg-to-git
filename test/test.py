def hand(tx):
    from math import pi
    from numpy.core import array
    # TODO Support sending all before reading.
    metal_id = tx.material(1, 0x808080)
    origin = array((0,2,0)) # + random noise in pos/rot?
    carpal_id = tx.body(tx.capsule(0.01,0.05), metal_id, origin, (0,0,1,0.5*pi))
    # 4 fingers.
    for f in range(4):
        finger(tx, carpal_id, metal_id, 4, origin, array((0.022*(f-1.5),-0.01,0)))

def finger(tx, carpalId, material_id, digit_count, origin, position):
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
    shapeId = tx.capsule(radius, spread)
    pos_b = array((0, 0.5*(radius+spread), 0))
    pos_a = -pos_b
    position += origin + pos_a
    finger_id = tx.body(shapeId, material_id, position)
    hinge_id = tx.hinge(carpalId, carpal_hinge_pos, (0,0,1), finger_id, pos_b, (0,0,1))
    tx.hinge_limit(hinge_id, array((-0.3,0.3))*pi)
    # TODO Set hinge limits.
    axis = (1,0,0)
    for d in range(1,digit_count):
        position += 2*pos_a
        finger_id2 = tx.body(shapeId, material_id, position)
        hinge_id = tx.hinge(finger_id, pos_a, axis, finger_id2, pos_b, axis)
        tx.hinge_limit(hinge_id, array((-0.3,0.3))*pi)
        finger_id = finger_id2

def main():
    from zofiri import Connection
    conn = Connection()
    tx = conn.transaction()
    try:
        hand(tx)
    finally:
        tx.close()

if __name__ == '__main__':
    main()

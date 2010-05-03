def hand(tx):
    from math import pi
    from numpy.core import array
    # TODO Support sending all before reading.
    carpalShapeId = tx.capsule(0.01, 0.05)
    metalId = tx.material(1, 0x808080)
    origin = array((0,2,0))
    carpalId = tx.body(carpalShapeId, metalId, origin, (0,0,1,0.5*pi))
    # 4 fingers.
    for f in range(4):
        finger(tx, carpalId, metalId, 4, origin, 0.01*array((2.2*(f-1.5),-1.1,0)))

def finger(tx, carpalId, materialId, digitCount, origin, position):
    from numpy.core import array
    # The carpal is rotated, so we need to transform the finger position.
    # TODO Use less hackish transforming.
    carpalHingePos = (position[1], position[0], position[2])
    radius = 0.01
    spread = 0.015
    shapeId = tx.capsule(radius, spread)
    axis = (1,0,0)
    posB = array((0,radius+spread/2+0.01,0))
    posA = -posB
    position += origin + posA
    fingerId = tx.body(shapeId, materialId, position)
    tx.hinge(carpalId, carpalHingePos, (0,1,0), fingerId, posB, axis)
    for d in range(1,digitCount):
        position += 2*posA
        fingerId2 = tx.body(shapeId, materialId, position)
        tx.hinge(fingerId, posA, axis, fingerId2, posB, axis)
        fingerId = fingerId2

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

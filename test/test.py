def main():
    from math import pi
    from zofiri import Connection
    conn = Connection()
    tx = conn.transaction()
    try:
        # TODO Support sending all before reading.
        carpalShapeId = tx.capsule(0.01, 0.05)
        metalId = tx.material(1, 0x808080)
        carpalId = tx.body(carpalShapeId, metalId, (0, 2, 0), (0, 0, 1, 0.5*pi))
    finally:
        tx.close()
    print carpalId

if __name__ == '__main__':
    main()

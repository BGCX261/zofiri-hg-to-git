def main():
    # TODO How to reference current module for reloading?
    # TODO And introduce an optional param on main to avoid infinit recursion?
    # Always reload our modules for easy testing.
    import parts, world, zofiri
    for module in [parts, world, zofiri]:
        reload(module)
    # Now get down to business.
    from world import build_body
    from zofiri import Connection
    conn = Connection()
    tx = conn.transaction()
    try:
        build_body(tx)
    finally:
        tx.close()
    # TODO Introduce clear, resume, and pause features?
    # TODO We could just leave the sim running the whole time if so.

if __name__ == '__main__':
    main()

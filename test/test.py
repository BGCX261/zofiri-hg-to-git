"""
Just kicks off tests after reloading everything. Does it this way
because numpy can be slow to reload, so I don't kick off new processes
completely. But the rest of the code reloads fast, and I want it fresh.
"""

from __future__ import with_statement 

def main(reload=True):
    if reload:
        # Always reload our modules for easy testing.
        current = reload_modules()
        current.main(False)
        return
    from parts_world import Humanoid
    humanoid = Humanoid()
    # Now get down to business.
    from world import build_body
    from zofiri import Connection
    conn = Connection()
    with conn.transaction() as tx:
        build_body(tx)
    # TODO Introduce clear, resume, and pause features?
    # TODO We could just leave the sim running the whole time if so.

def reload_modules():
    from os import listdir
    from os.path import abspath, basename, dirname
    current = split_ext(basename(__file__))[0]
    for name in listdir(dirname(abspath(__file__))):
        name, ext = split_ext(name)
        if ext == 'py':
            module = __import__(name, level=1)
            # TODO How to know if stale?
            reload(module)
            if name == current:
                current = module
    return current

def split_ext(name):
    from re import match
    m = match(r'(.*)\.(.*)', name)
    if m is None:
        return name, ''
    else:
        return m.group(1), m.group(2)

if __name__ == '__main__':
    main()

# TODO Move this into main Zofiri src dir?

from __future__ import with_statement 

class Connection(object):
    """
    Represents a connection to a Zofiri server. From here, open transactions
    for individual operations.
    """

    def __init__(self, host='127.0.0.1', port=2041):
        from socket import AF_INET, SOCK_STREAM, socket
        zsocket = socket(AF_INET, SOCK_STREAM)
        try:
            zsocket.connect((host, port))
            # TODO Using file mode requires blocking. Maybe reconsider later.
            self._file = zsocket.makefile('rw')
        finally:
            # They say that you need to close the socket in addition to the file.
            zsocket.close()

    def __del__(self):
        self.close()

    def close(self):
        if self._file:
            self._file.close()
        self._file = None

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.close()

    def read(self):
        """
        Reads one result line from the server and returns it as a string,
        whitespace-stripped on both sides.
        """
        # TODO Support quoted args.
        # TODO Standardize error responses so we can coordinate them here.
        line = self._file.readline().strip()
        # print "Received:", line
        return line

    def send(self, line):
        """
        Sends the command line, adding '\n' automatically. Returns the
        result line split into individual result args.
        """
        # print "Sending:", line
        file = self._file
        file.write(line + '\n')
        file.flush()

    def transaction(self):
        return Transaction(self)

class Result:
    """
    Zofiri has variable support that allows for weaving multiple commands
    in a single transmission. This allows for graph structures rather than
    just trees as well as allowing a command-oriented protocol, while
    staying simple and avoiding as much need for round trips. The Result
    class allows storing temporary var names later to be replaced by the
    actual result value from the server.
    """

    def __init__(self, value):
        """
        The initial value should begin with a '$' and is used as the variable name.
        """
        self.value = value

    def __repr__(self):
        return repr(self.value)

    def __str__(self):
        return str(self.value)

class PartsBinder(object):
    """
    Binds a set of parts to a Zofiri server, attempting to keep the two
    sides in sync. Or at least it can serialize (and deserialize soon?).

    TODO Move this to a separate module from zofiri or parts?
    """

    def __init__(self, connection):
        self._connection = connection

    def build(self, part):
        """
        Send out the commands to build the parts and everything
        attached to it.

        TODO This needs replaced with more serious two-way binding.
        """
        from mat import minimum, maximum

        def build_capsule(tx, capsule):
            return tx.capsule(capsule.radius, 2*capsule.half_spread)

        def build_material_if_needed(tx, material):
            if not material.key:
                key = tx.material(material.density, material.color)
                material.key = key

        def build_part_and_joints(tx, part, joint):
            from parts import Capsule
            # The following could be handled by the visitor pattern, but
            # that's stupid, too.
            # TODO Consider a 'type' field in each part.
            # TODO That way it's at least not tied to inheritance.
            shape_key = {
                Capsule: build_capsule
            }[type(part)](tx, part)
            # TODO I still haven't resolved needing angle*pi or not.
            build_material_if_needed(tx, part.material)
            body_key = tx.body(
                shape_key, part.material.key, part.pos, part.rot)
            part.key = body_key
            if joint:
                socket = joint.socket
                parent = joint.socket.part
                # TODO For now, the server wants the axis only.
                # TODO When the server is changes, send the whole rot.
                joint_key = tx.hinge(
                    parent.key, socket.pos, socket.rot[0:-1],
                    body_key, joint.pos, joint.rot[0:-1])
                joint.key = socket.key = joint_key

                # Now apply joint limits as the extreme of the two.
                limits = joint.limits.copy()
                other_limits = socket.limits
                limits[:,:,0] = maximum(limits[:,:,0], other_limits[:,:,0])
                limits[:,:,1] = minimum(limits[:,:,1], other_limits[:,:,1])
                # TODO Pull out full limits once we support them.
                tx.hinge_limit(joint_key, limits[1,0,:])

        with self._connection.transaction() as tx:
            part.part().traverse(
                lambda part, joint: build_part_and_joints(tx, part, joint))

class Transaction(object):
    """
    A transaction with a Zofiri server. All commands sent on a single transaction
    are executed between simulator time steps by Zofiri.
    """

    def __init__(self, connection):
        self._connection = connection
        self._results = []
        self._var_count = 0
        
    def __del__(self):
        """
        Closes the transaction.
        """
        self.close()

    def body(self, shape_id, material_id, position, axis_angle=None):
        if axis_angle is not None:
            message = 'body %s %s %f %f %f %f %f %f %f' % (
                (shape_id, material_id) + tuple(position) + tuple(axis_angle)
            )
        else:
            message = 'body %s %s %f %f %f' % (
                (shape_id, material_id) + tuple(position)
            )
        return self._send(message)

    # TODO box (from multisphere)?

    def capsule(self, radius, spread):
        return self._send('capsule %f %f' % (radius, spread))

    def close(self):
        """
        Finishes the transaction. For now, that means committing it.
        Zofiri doesn't support aborting transactions at the moment.
        """
        if self._connection:
            # print "Going down!"
            # TODO Could check response.
            self._connection.send(';')
            self._read_results()
            self._connection = None

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.close()

    def hinge(self, body_id1, position1, axis1, body_id2, position2, axis2):
        # TODO Automate body2 axis at the server?
        # TODO Should always be the same axes in global frame or not?
        return self._send('hinge %s %f %f %f %f %f %f %s %f %f %f %f %f %f' % (
            (body_id1,) + tuple(position1) + tuple(axis1) + (body_id2,) + tuple(position2) + tuple(axis2)
        ))

    def hinge_limit(self, hinge_id, low_high):
        return self._send('hinge-limit %s %f %f' % ((hinge_id,) + tuple(low_high)))

    def material(self, density, color):
        return self._send('material %f %X' % (density, color))

    # TODO round_box (from multisphere)?

    def shape_scale(self, shape_id, scale):
        return self._send('shape-scale %s %f %f %f' % ((shape_id,) + tuple(scale)))

    def _read_results(self):
        # print "Results size:", len(self._results)
        for result in self._results:
            # Replace the var name with the actual response.
            result.value = self._connection.read()
        # Read the commit status, too.
        self._connection.read()

    def _send(self, line):
        """
        Send the line and return a result to be lazily filled.
        """
        self._var_count += 1;
        result = Result('$' + str(self._var_count))
        self._connection.send('%s = %s' % (result, line))
        self._results.append(result)
        return result

class TestConnection(Connection):
    """
    Just prints lines instead of sending them, and replies with random
    integer values.
    """

    def __init__(self):
        from random import Random
        self._random = Random()

    def close(self):
        pass

    def read(self):
        return str(self._random.randint(0, int(1e9)))

    def send(self, line):
        print line

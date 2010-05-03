class Connection:
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

    def send(self, line):
        """
        Sends the command line, adding '\n' automatically. Returns the
        result line split into individual result args.
        """
        file = self._file
        file.write(line + '\n')
        file.flush()
        # TODO Support quoted args.
        # TODO Support sending multiple before reading.
        results = file.readline().split()
        # TODO Standardize error responses so we can coordinate them here.
        return results

    def transaction(self):
        return Transaction(self)

class Transaction:
    """
    A transaction with a Zofiri server. All commands sent on a single transaction
    are executed between simulator time steps by Zofiri.
    """

    def __init__(self, connection):
        self._connection = connection
        
    def __del__(self):
        """
        Closes the transaction.
        """
        self.close()

    def body(self, shapeId, materialId, position, axisAngle=None):
        if axisAngle:
            message = 'body %d %d %f %f %f %f %f %f %f' % (
                (shapeId, materialId) + tuple(position) + tuple(axisAngle)
            )
        else:
            message = 'body %d %d %f %f %f' % (
                (shapeId, materialId) + tuple(position)
            )
        return self.sendForInt(message)

    def capsule(self, radius, spread):
        return self.sendForInt('capsule %f %f' % (radius, spread))

    def close(self):
        """
        Finishes the transaction. For now, that means committing it.
        Zofiri doesn't support aborting transactions at the moment.
        """
        if self._connection:
            # TODO Could check response.
            self._connection.send(';')
            self._connection = None

    def hinge(self, bodyId1, position1, axis1, bodyId2, position2, axis2):
        return self.sendForInt('hinge %d %f %f %f %f %f %f %d %f %f %f %f %f %f' % (
            (bodyId1,) + tuple(position1) + tuple(axis1) + (bodyId2,) + tuple(position2) + tuple(axis2)
        ))

    def material(self, density, color):
        return self.sendForInt('material %f %x' % (density, color))

    def sendForInt(self, line):
        """
        Send the line and return a single int.
        """
        return int(self._connection.send(line)[0])

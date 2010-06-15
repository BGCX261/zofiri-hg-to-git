#include "etc.h"
#include "mat.h"
#include "pub.h"
#include "sim.h"
#include <iostream>
#include <string>
#include <sstream>

namespace zof {

void log(const btVector3& vector) {
	if (false) cerr <<
		"btVector3(" <<
		vector.x() << "," << vector.y() << "," << vector.z() <<
		")" << endl;
}

void logHingeBody(int bodyId, const btVector3& position, const btVector3& axis) {
	if (false) cerr <<
		"hinge on " << bodyId <<
		" at " << position.x() << "," << position.y() << "," << position.z() <<
		" around " << axis.x() << "," << axis.y() << "," << axis.z() <<
		endl;
}

struct BodyCommand: Command {
	virtual std::string perform(Transaction* tx) {
		if (!(tx->args.size() == 6 || tx->args.size() == 10)) {
			throw "wrong number of args for body (should be 'body $shape $material $x $y $z [$ax $ay $az $a]')";
		}
		int shapeId;
		tx->args[1] >> shapeId;
		int materialId;
		tx->args[2] >> materialId;
		btVector3 position;
		tx->args[3] >> position.m_floats[0];
		tx->args[4] >> position.m_floats[1];
		tx->args[5] >> position.m_floats[2];
		btQuaternion orientation;
		if (tx->args.size() == 6) {
			orientation = btQuaternion::getIdentity();
		} else {
			// Read the orientation as axis-angle.
			btVector3 axis;
			tx->args[6] >> axis.m_floats[0];
			tx->args[7] >> axis.m_floats[1];
			tx->args[8] >> axis.m_floats[2];
			// TODO Pi support function.
			std::string& angleArg = tx->args[9];
			btScalar angle;
			bool usePi = false;
			std::string::size_type piIndex = angleArg.rfind("pi");
			if (piIndex == angleArg.size() - 2) {
				angleArg.erase(angleArg.end() - 2);
				usePi = true;
			}
			angleArg >> angle;
			if (usePi) {
				angle = pi(angle);
			}
 			orientation = btQuaternion(axis, angle);
		}
		Sim* sim = tx->pub->viz->sim;
		Material* material = sim->getMaterial(materialId);
		if (!material) {
			throw "no such material for body";
		}
		btCollisionShape* shape = sim->getShape(shapeId);
		if (!shape) {
			throw "no such shape for body";
		}
		btRigidBody* body = sim->createBody(
			shape,
			btTransform(orientation, sim->m(position)),
			material
		);
		int id = sim->addBody(body);
		stringstream result;
		result << id;
		return result.str();
	}
};

struct BoxCommand: Command {
	virtual std::string perform(Transaction* tx) {
		if (tx->args.size() < 4) {
			throw "too few args for box";
		}
		// Parse stuff out to build the material.
		btVector3 halfExtents;
		tx->args[1] >> halfExtents.m_floats[0];
		tx->args[2] >> halfExtents.m_floats[1];
		tx->args[3] >> halfExtents.m_floats[2];
		// Build and store the shape.
		Sim* sim = tx->pub->viz->sim;
		btBoxShape* shape = new btBoxShape(sim->m(halfExtents));
		int id = sim->addShape(shape);
		// Return the ID.
		stringstream result;
		result << id;
		return result.str();
	}
};

struct CapsuleCommand: Command {
	virtual std::string perform(Transaction* tx) {
		if (tx->args.size() < 3) {
			throw "too few args for capsule";
		}
		// Parse stuff out to build the material.
		btScalar radius, spread;
		tx->args[1] >> radius;
		tx->args[2] >> spread;
		// Build and store the shape.
		Sim* sim = tx->pub->viz->sim;
		btCapsuleShape* shape = new btCapsuleShape(sim->m(radius), sim->m(spread));
		int id = sim->addShape(shape);
		// Return the ID.
		stringstream result;
		result << id;
		return result.str();
	}
};

struct CylinderCommand: Command {
	virtual std::string perform(Transaction* tx) {
		if (tx->args.size() < 3) {
			throw "too few args for capsule";
		}
		// Parse stuff out to build the material.
		btVector3 halfExtents;
		tx->args[1] >> halfExtents.m_floats[0];
		tx->args[2] >> halfExtents.m_floats[1];
		if (tx->args.size() < 4) {
			halfExtents.m_floats[2] = halfExtents.m_floats[0];
		} else {
			tx->args[3] >> halfExtents.m_floats[2];
		}
		// Build and store the shape.
		Sim* sim = tx->pub->viz->sim;
		btCylinderShape* shape = new btCylinderShape(sim->m(halfExtents));
		int id = sim->addShape(shape);
		// Return the ID.
		stringstream result;
		result << id;
		return result.str();
	}
};

struct HingeCommand: Command {
	virtual std::string perform(Transaction* tx) {
		if (tx->args.size() != 15) {
			throw "wrong number of args for hinge (should be 'hinge $body1 $x1 $y1 $z1 $ax1 $ay1 $az1 $body2 $x2 $y2 $z2 $ax2 $ay2 $az2')";
		}
		int bodyId1;
		tx->args[1] >> bodyId1;
		btVector3 position1;
		tx->args[2] >> position1.m_floats[0];
		tx->args[3] >> position1.m_floats[1];
		tx->args[4] >> position1.m_floats[2];
		btVector3 axis1;
		tx->args[5] >> axis1.m_floats[0];
		tx->args[6] >> axis1.m_floats[1];
		tx->args[7] >> axis1.m_floats[2];
		logHingeBody(bodyId1, position1, axis1);
		int bodyId2;
		tx->args[8] >> bodyId2;
		btVector3 position2;
		tx->args[9] >> position2.m_floats[0];
		tx->args[10] >> position2.m_floats[1];
		tx->args[11] >> position2.m_floats[2];
		btVector3 axis2;
		tx->args[12] >> axis2.m_floats[0];
		tx->args[13] >> axis2.m_floats[1];
		tx->args[14] >> axis2.m_floats[2];
		logHingeBody(bodyId2, position2, axis1);
		// Load data.
		Sim* sim = tx->pub->viz->sim;
		btRigidBody* body1 = sim->getBody(bodyId1);
		if (!body1) {
			throw "no such body1 for hinge";
		}
		btRigidBody* body2 = sim->getBody(bodyId2);
		if (!body2) {
			throw "no such body2 for hinge";
		}
		btTypedConstraint* constraint = new btHingeConstraint(
			*body1, *body2,
			sim->m(position1), sim->m(position2),
			axis1, axis2,
			false
		);
		int id = sim->addConstraint(constraint);
		stringstream result;
		result << id;
		return result.str();
	}
};

struct HingeLimitCommand: Command {
	virtual std::string perform(Transaction* tx) {
		if (tx->args.size() != 4) {
			throw "wrong number of args for hinge-limit (should be 'hinge-limit $hinge $low $high')";
		}
		int hingeId;
		tx->args[1] >> hingeId;
		btScalar low, high;
		tx->args[2] >> low;
		tx->args[3] >> high;
		// Load data.
		Sim* sim = tx->pub->viz->sim;
		btTypedConstraint* constraint = sim->getConstraint(hingeId);
		if (!constraint) {
			throw "no such constraint";
		}
		if (constraint->getObjectType() != HINGE_CONSTRAINT_TYPE) {
			throw "constraint not a hinge";
		}
		btHingeConstraint* hinge = dynamic_cast<btHingeConstraint*>(constraint);
		hinge->setLimit(low, high);
		// Just non-error case here.
		return "0";
	}
};

struct MaterialCommand: Command {
	virtual std::string perform(Transaction* tx) {
		if (tx->args.size() < 3) {
			throw "too few args for material";
		}
		// Parse stuff out to build the material.
		// TODO Make helper functions for converting types?
		btScalar density;
		tx->args[1] >> density;
		zof_color color;
		stringstream args;
		args << tx->args[2];
		args >> hex >> color;
		Material* material = new Material(color);
		material->density = density;
		// Store the material in the sim.
		Sim* sim = tx->pub->viz->sim;
		int id = sim->addMaterial(material);
		// Return the ID.
		stringstream result;
		result << id;
		return result.str();
	}
};

struct ShapeScaleCommand: Command {
	virtual std::string perform(Transaction* tx) {
		if (tx->args.size() != 5) {
			throw "wrong number of args for shape-scale (should be 'shape-scale $shape $x $y $z')";
		}
		int shapeId;
		tx->args[1] >> shapeId;
		btVector3 scale;
		tx->args[2] >> scale.m_floats[0];
		tx->args[3] >> scale.m_floats[1];
		tx->args[4] >> scale.m_floats[2];
		// Load data.
		Sim* sim = tx->pub->viz->sim;
		btCollisionShape* shape = sim->getShape(shapeId);
		if (!shape) {
			throw "no such shape";
		}
		shape->setLocalScaling(scale);
		// Just non-error case here.
		return "0";
	}
};

void initCommands(Pub* pub) {
	pub->commands["body"] = new BodyCommand();
	pub->commands["box"] = new BoxCommand();
	pub->commands["capsule"] = new CapsuleCommand();
	pub->commands["cylinder"] = new CylinderCommand();
	pub->commands["hinge"] = new HingeCommand();
	pub->commands["hinge-limit"] = new HingeLimitCommand();
	pub->commands["material"] = new MaterialCommand();
	pub->commands["shape-scale"] = new ShapeScaleCommand();
}

Pub::Pub(Viz* viz, int port) {
	server = new Server(port);
	mod_uri = zof_null;
	this->viz = viz;
	viz->pub = this;
	initCommands(this);
}

Pub::~Pub() {
	delete server;
	for (std::map<std::string,Command*>::iterator c = commands.begin(); c != commands.end(); c++) {
		delete c->second;
	}
}

void Pub::update() {

	if (mod_uri) {
		zof_mod mod = zof_mod_new(mod_uri);
		if (!mod) {
			exit(EXIT_FAILURE);
		}
		if (!zof_mod_sim_init(mod, (zof_sim)viz->sim)) {
			exit(EXIT_FAILURE);
		}
		zof_ref_free(mod);
		mod_uri = zof_null;
	}

	vector<std::string> results;
	vector<Socket*> sockets;
	server->select(&sockets);
	for(vector<Socket*>::iterator s = sockets.begin(); s < sockets.end(); s++) {
		Socket* socket = *s;
		//cerr << "Found a socket: " << socket << endl;
		// TODO We need this non-blocking and caching between sessions.
		// TODO For now we could get hung still.
		// TODO Create transaction object!!!
		Transaction tx(this);
		results.clear();
		std::string line;
		while (socket->readLine(&line) && line != ";") {
			//cerr << "Processing: " << line << endl;
			std::string result = tx.processLine(line);
			// TODO Manual buffer here, but could we just send and let it buffer elsewhere?
			// TODO Do we need to support reading before ';'?
			results.push_back(result);
		}
		// cerr << "Results size: " << results.size() << endl;
		for (vector<std::string>::iterator r = results.begin(); r < results.end(); r++) {
			socket->writeLine(r->c_str());
		}
		if (line == ";") {
			// Give a final status response.
			//cerr << "Processing: " << line << endl;
			socket->writeLine("0");
		}
	}
	if (!sockets.empty()) {
		//cerr << "Done with that!" << endl;
	}
}

Transaction::Transaction(Pub* pub) {
	this->pub = pub;
}

std::string Transaction::processCommand(const vector<std::string>& args) {
	std::string result;
	this->args = args;
	if (args.empty()) {
		return result;
	}
	std::string commandName = args.front();
	std::string varName;
	if (commandName[0] == '$') {
		varName = commandName;
		if (args.size() < 2 || args[1] != "=") {
			throw "var assignment without =";
		}
		this->args.erase(this->args.begin(), this->args.begin() + 2);
		if (this->args.empty()) {
			return result;
		}
		commandName = this->args.front();
	}
	std::map<std::string,Command*>::iterator c = pub->commands.find(commandName);
	if (c != pub->commands.end()) {
		// Found the command.
		// Perform var substitution among the args.
		for (vector<std::string>::iterator a = this->args.begin(); a < this->args.end(); a++) {
			const std::string& arg = *a;
			if (arg[0] == '$') {
				const std::map<std::string,std::string>::iterator v = vars.find(arg);
				if (v == vars.end()) {
					throw "undefined var";
				} else {
					*a = v->second;
				}
			}
		}
		// Call the command.
		result = c->second->perform(this);
	}
	if (!varName.empty()) {
		vars[varName] = result;
	}
	return result;
}

std::string Transaction::processLine(const std::string& line) {
	const char* delims = " \t";
	bool last = false;
	// TODO Handle quotes. Double and single per bash?
	std::string::size_type pos = line.find_first_not_of(delims);
	if (pos == std::string::npos) {
		// Whitespace only.
		return "";
	}
	vector<std::string> args;
	while (!last) {
		std::string::size_type nextPos = line.find_first_of(delims, pos);
		if (nextPos == std::string::npos) {
			nextPos = line.length();
			last = true;
		}
		std::string part = line.substr(pos, nextPos - pos);
		args.push_back(part);
		if (!last) {
			pos = line.find_first_not_of(delims, nextPos);
			if (pos == std::string::npos) {
				last = true;
			}
		}
	}
	return processCommand(args);
}

}

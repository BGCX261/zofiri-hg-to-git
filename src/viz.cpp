#include "zofiri.h"

namespace zofiri {

void buildPlaneVertices(
	const vector3df& halfExtents,
	Material* material,
	const vector3df& normal,
	S3DVertex* vertices, u32& v,
	u16* indices, u32& i
) {
	//cout << "indices: " << indices << ", vertices: " << vertices << " at " << v << "\n";
	// Figure out the orientation.
	// TODO Is there some simpler way?
	S3DVertex vertex;
	vector3df a, b;
	s32 dir;
	if (normal.X) {
		dir = -normal.X;
		a = vector3df(0,1,0);
		b = vector3df(0,0,1);
	} else if (normal.Y) {
		dir = normal.Y;
		a = vector3df(1,0,0);
		b = vector3df(0,0,1);
	} else {
		dir = -normal.Z;
		a = vector3df(1,0,0);
		b = vector3df(0,1,0);
	}
	// Set indices before vertices, because v will change.
	if (dir > 0) {
		indices[i++] = v+0;
		indices[i++] = v+1;
		indices[i++] = v+2;
		indices[i++] = v+1;
		indices[i++] = v+3;
		indices[i++] = v+2;
	} else {
		indices[i++] = v+0;
		indices[i++] = v+2;
		indices[i++] = v+1;
		indices[i++] = v+1;
		indices[i++] = v+2;
		indices[i++] = v+3;
	}
	// Vertices.
	for(s32 k=-1; k <= 1; k += 2) {
		for(s32 j = -1; j <= 1; j += 2, v++) {
			S3DVertex vertex;
			vertex.Pos = (k*a + j*b + normal) * halfExtents;
			//cout << v << ": " << vertex.Pos.X << ", " << vertex.Pos.Y << ", " << vertex.Pos.Z << "\n";
			vertex.Color = material->color;
			vertex.Normal = normal;
			vertices[v] = vertex;
		}
	}
}

Viz::Viz(Sim* s): sim(s) {
	// TODO Support multi-viz per sim and vice versa.
	sim->viz = this;
}

IMesh* Viz::buildBoxMesh(btBoxShape* shape, Material* material) {
	// TODO Make this a general rectangle thing? Still need to reuse vertices from the two sides.
	const btVector3& btHalfExtents = shape->getHalfExtentsWithMargin();
	//cout << "btBoxShape btHalfExtents: " << btHalfExtents.x() << " " << btHalfExtents.y() << " " << btHalfExtents.z() << endl;
	vector3df halfExtents(btHalfExtents.x(), btHalfExtents.y(), btHalfExtents.z());
	//cout << "btBoxShape halfExtents: " << halfExtents.X << " " << halfExtents.Y << " " << halfExtents.Z << endl;
	S3DVertex vertices[24];
	u16 indices[36];
	u32 v = 0;
	u32 i = 0;
	//cout << "START indices: " << indices << ", vertices: " << vertices << " at " << v << endl;
	buildPlaneVertices(halfExtents, material, vector3df(-1,0,0), vertices, v, indices, i);
	buildPlaneVertices(halfExtents, material, vector3df(1,0,0), vertices, v, indices, i);
	buildPlaneVertices(halfExtents, material, vector3df(0,-1,0), vertices, v, indices, i);
	buildPlaneVertices(halfExtents, material, vector3df(0,1,0), vertices, v, indices, i);
	buildPlaneVertices(halfExtents, material, vector3df(0,0,-1), vertices, v, indices, i);
	buildPlaneVertices(halfExtents, material, vector3df(0,0,1), vertices, v, indices, i);
	SMeshBuffer* buffer = new SMeshBuffer();
	buffer->append(vertices, v, indices, i);
	SMesh* mesh = new SMesh();
	mesh->addMeshBuffer(buffer);
	return mesh;
}

	// TODO Swap y for z and figure out LH vs. RH.
IMesh* Viz::createCapsuleMesh(btCapsuleShape* shape, Material* material, u32 latCount, u32 longCount) {
	f64 distHalf = shape->getHalfHeight();
	f64 radius = shape->getRadius();
	// TODO How much can I reuse code between this and createSphereMesh?
	// TODO Can I trust the int math on the doubles?
	// TODO I basically double the resolution of lat vs. long. Better to keep them the same?
	latCount = floor(latCount / 2);
	s32 latCountSigned = latCount;
	// Extra ring in the middle for the capsule.
	u32 vertexCount = longCount * (2 * latCount + 2);
	// TODO Need a macro for _malloca on MSVC?
	S3DVertex* vertices = reinterpret_cast<S3DVertex*>(alloca(sizeof(S3DVertex) * vertexCount));
	// 2 triangles per quad.
	// Extra ring in the middle for the capsule.
	u32 indexCount = 2 * 3 * longCount * (2 * latCount + 1);
	u16* indices = reinterpret_cast<u16*>(alloca(sizeof(u16) * indexCount));
	u32 v = 0;
	u32 i = 0;
	for(s32 t = -latCountSigned - 1; t <= latCountSigned; t++) {
		// TODO Optimize single point at bottom and top? Or generally?
		f64 latRatio = t;
		f64 heightOffset = distHalf;
		if (t < 0) {
			latRatio++;
			heightOffset = -heightOffset;
		}
		f64 latAngle = (latRatio / latCount) * pi(0.5);
		f32 y = sin(latAngle) * radius + heightOffset;
		f64 sectionRadius = cos(latAngle) * radius;
		for(u32 g = 0; g < longCount; g++, v++) {
			f64 longAngle = (g / f64(longCount)) * pi(2);
			f32 x = cos(longAngle) * sectionRadius;
			f32 z = sin(longAngle) * sectionRadius;
			S3DVertex vertex;
			switch(shape->getUpAxis()) {
			case 0:
				vertex.Pos = vector3df(y,x,z);
				vertex.Normal = vector3df(-heightOffset,0,0);
				break;
			case 1:
				vertex.Pos = vector3df(x,y,z);
				vertex.Normal = vector3df(0,-heightOffset,0);
				break;
			case 2:
				vertex.Pos = vector3df(x,z,y);
				vertex.Normal = vector3df(0,0,-heightOffset);
				break;
			}
			vertex.Color = material->color;
			// TODO Alternatively could calculate normal from latAngle and longAngle.
			vertex.Normal += vertex.Pos;
			vertex.Normal /= radius;
			vertices[v] = vertex;
			if(t < latCountSigned) {
				u32 v1 = v + 1;
				if(!(v1 % longCount)) {
					// Wrapped around the circle.
					v1 -= longCount;
				}
				// Triangle 1.
				indices[i++] = v;
				switch(shape->getUpAxis()) {
				case 0:
					indices[i++] = v1 + longCount;
					indices[i++] = v + longCount;
					break;
				case 1:
				case 2:
					indices[i++] = v + longCount;
					indices[i++] = v1 + longCount;
					break;
				}
				// Triangle 2.
				indices[i++] = v;
				switch(shape->getUpAxis()) {
				case 0:
					indices[i++] = v1;
					indices[i++] = v1 + longCount;
					break;
				case 1:
				case 2:
					indices[i++] = v1 + longCount;
					indices[i++] = v1;
					break;
				}
			}
		}
	}
	SMeshBuffer* buffer = new SMeshBuffer();
	buffer->append(vertices, vertexCount, indices, indexCount);
	SMesh* mesh = new SMesh();
	mesh->addMeshBuffer(buffer);
	return mesh;
}

IMesh* Viz::createPlaneMesh() {
	// TODO Figure out LH vs. RH.
	// TODO This really isn't infinite.
	// TODO A forum discussion indicates shaders might work for infinite.
	// Lighting only affects vertices with gradients between.
	// Resolution for more nodes, so local lighting works better.
	u32 res = 4;
	f32 extent = sim->m(10);
	S3DVertex vertices[(res+1)*(res+1)];
	u16 indices[6*res*res];
	u32 i = 0;
	u32 v = 0;
	for(u32 ix = 0; ix <= res; ix++) {
		for(u32 iz = 0; iz <= res; iz++, v++) {
			f32 x = ix*extent/res - extent/2;
			f32 z = iz*extent/res - extent/2;
			S3DVertex vertex;
			vertex.Pos = vector3df(x,0,z);
			vertex.Color = SColor(0xFFFFFFFF);
			vertex.Normal = vector3df(0,1,0);
			vertices[v] = vertex;
			if (ix && iz) {
				indices[i++] = v - res - 2;
				indices[i++] = v - res - 1;
				indices[i++] = v - 1;
				indices[i++] = v - res - 1;
				indices[i++] = v;
				indices[i++] = v - 1;
			}
		}
	}
	SMeshBuffer* buffer = new SMeshBuffer();
	buffer->append(vertices, v, indices, i);
	SMesh* mesh = new SMesh();
	mesh->addMeshBuffer(buffer);
	return mesh;
}

IMesh* Viz::createSphereMesh(btSphereShape* shape, u32 latCount, u32 longCount) {
	// TODO Swap y for z and figure out LH vs. RH.
	f64 radius = shape->getRadius();
	// TODO Can I trust the int math on the doubles?
	// TODO I basically double the resolution of lat vs. long. Better to keep them the same?
	latCount = floor(latCount / 2);
	s32 latCountSigned = latCount;
	u32 vertexCount = longCount * (2 * latCount + 1);
	// TODO Need a macro for _malloca on MSVC?
	S3DVertex* vertices = reinterpret_cast<S3DVertex*>(alloca(sizeof(S3DVertex) * vertexCount));
	// 2 triangles per quad.
	u32 indexCount = 2 * 3 * longCount * (2 * latCount);
	u16* indices = reinterpret_cast<u16*>(alloca(sizeof(u16) * indexCount));
	u32 v = 0;
	u32 i = 0;
	for(s32 t = -latCountSigned; t <= latCountSigned; t++) {
		// TODO Optimize single point at bottom and top? Or generally?
		f64 latAngle = (t / latCount) * pi(0.5);
		f32 y = sin(latAngle) * radius;
		f32 sectionRadius = cos(latAngle) * radius;
		for(u32 g = 0; g < longCount; g++, v++) {
			f64 longAngle = (g / longCount) * pi(2);
			f32 x = cos(longAngle) * sectionRadius;
			f32 z = sin(longAngle) * sectionRadius;
			S3DVertex vertex;
			vertex.Pos = vector3df(x,y,z);
			vertex.Color = SColor(0xFF0000FF);
			vertex.Normal = vertex.Pos / radius;
			vertices[v] = vertex;
			if(t < latCountSigned) {
				u32 v1 = v + 1;
				if(!(v1 % longCount)) {
					// Wrapped around the circle.
					v1 -= longCount;
				}
				// Triangle 1.
				indices[i++] = v;
				indices[i++] = v1 + longCount;
				indices[i++] = v + longCount;
				// Triangle 2.
				indices[i++] = v;
				indices[i++] = v1;
				indices[i++] = v1 + longCount;
			}
		}
	}
	SMeshBuffer* buffer = new SMeshBuffer();
	buffer->append(vertices, vertexCount, indices, indexCount);
	SMesh* mesh = new SMesh();
	mesh->addMeshBuffer(buffer);
	return mesh;
}

void Viz::run() {
	// TODO We are leaking about 2 KB from irrlicht/gl/x. Simple program does the same, though.
	SIrrlichtCreationParameters irrParams;
	irrParams.DriverType = EDT_OPENGL;
	// This createDeviceEx() call actually shows a window. TODO Are there ways to avoid that at first?
	device = createDeviceEx(irrParams);
	if(!device) {
		throw "failed to start irrlicht device";
	}
	device->setWindowCaption(L"Zofiri Viz");

	btCollisionObjectArray& bodies = sim->dynamics->getCollisionObjectArray();
	for(int b = 0; b < bodies.size(); b++) {
		btCollisionObject* object = bodies[b];
		Material* material = BodyInfo::of(object)->material;
		//cout << "Color " << hex << material->color.color << dec;
		IMesh* mesh;
		btCollisionShape* shape = object->getCollisionShape();
		switch(shape->getShapeType()) {
		case BOX_SHAPE_PROXYTYPE:
			//cout << " box ";
			mesh = buildBoxMesh(reinterpret_cast<btBoxShape*>(shape), material);
			break;
		case CAPSULE_SHAPE_PROXYTYPE:
			//cout << " capsule ";
			mesh = createCapsuleMesh(reinterpret_cast<btCapsuleShape*>(shape), material, 10, 10);
			break;
		case STATIC_PLANE_PROXYTYPE:
			//cout << " plan ";
			mesh = createPlaneMesh();
			break;
		case SPHERE_SHAPE_PROXYTYPE:
			//cout << " sphere ";
			mesh = createSphereMesh(reinterpret_cast<btSphereShape*>(shape), 10, 10);
			break;
		default:
			throw "unsupported shape kind";
		}
		IMeshSceneNode* node = scene()->addMeshSceneNode(mesh);
		BodyInfo::of(object)->sceneNode = node;
		btVector3& origin = object->getWorldTransform().getOrigin();
		//cout << " at: " << origin.x() << " " << origin.y() << " " << origin.z() << endl;
		node->setPosition(vector3df(origin.x(), origin.y(), origin.z()));
	}

	// ICameraSceneNode* camera =
	// Normal:
	scene()->addCameraSceneNode(0, sim->m(vector3df(-0.8,1.2,0.8)), sim->m(vector3df(-0.05,0.5,-0.05)));
	// Close-up on hand:
	//ICameraSceneNode* camera = scene()->addCameraSceneNode(0, 1e-2*vector3df(-10,140,10), 1e-2*vector3df(10,90,-10));
	//camera->setPosition(camera->getPosition() - 0.5 * (camera->getPosition() - camera->getTarget()));
	//camera->setUpVector(vector3df(0,0,1));
	//camera->setViewMatrixAffector(matrix4().setScale(vector3df(-1,1,1)));
	// Prepare a brighter sky color (for less painful printing).
	IImage* sky = video()->createImage(ECF_A8R8G8B8, dimension2du(1,1));
	sky->setPixel(0, 0, SColor(0xFFB0B0FF), false);
	scene()->addSkyDomeSceneNode(video()->addTexture("sky", sky));
	// Set up lighting.
	scene()->setAmbientLight(SColor(0xFF505050));
	// Primary directional lighting.
	ILightSceneNode* light = scene()->addLightSceneNode();
	light->getLightData().DiffuseColor = SColor(0xFFB0B0B0);
	light->setRotation(vector3df(0, -60, 150));
	light->setLightType(ELT_DIRECTIONAL);
	light->setRadius(4000);
	// And a point light for nice effect.
	scene()->addLightSceneNode(0, sim->m(vector3df(0.5,3.0,1.0)), SColor(0xFFFFFFFF), sim->m(2.0));
	while(device->run()) {
		if(device->isWindowActive()) {
			video()->beginScene();
			//camera->setProjectionMatrix(projection);
			try {
				if(sim) {
					sim->dynamics->stepSimulation(btScalar(1)/btScalar(60));
				}
			} catch(void* err) {
				// TODO Log? Could get tedious.
				// TODO I did already have a case of errors, but I didn't see them.
				// TODO I wonder why. (Sarcasm there.)
				// TODO Maybe still throttle logging somewhere at the logging layer?
				// This is the top of a loop and a good place to catch errors.
			}
			scene()->drawAll();
			video()->endScene();
			//char name[100];
			//static u32 screenCount = 0;
			//snprintf(name, 99, "shots/shot-%05d.png", screenCount++);
			//video()->writeImageToFile(video()->createScreenShot(), name, 0);
		} else {
			// TODO Run the simulation anyway?
			device->yield();
		}
	}

	device->drop();
	device = NULL;
}

void Viz::update(btRigidBody* body) {
	btTransform transform;
	body->getMotionState()->getWorldTransform(transform);
	ISceneNode* node = BodyInfo::of(body)->sceneNode;
	// TODO Build auto-conversions between these vector types.
	btVector3& origin = transform.getOrigin();
	node->setPosition(vector3df(origin.x(), origin.y(), origin.z()));
	btScalar yaw, pitch, roll;
	transform.getBasis().getEulerYPR(yaw, pitch, roll);
	// TODO Vectorize the radToDeg transform?
	node->setRotation(vector3df(radToDeg(roll), radToDeg(pitch), radToDeg(yaw)));
}

}

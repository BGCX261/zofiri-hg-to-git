include_directories(
  .
  ${BULLET_INCLUDE_DIR}
  ${IRRLICHT_INCLUDE_DIR}
  ../test
)

# Use the base dir (in case they are installed there)
# or subdirs in case of a bullet build.
link_directories(
	${BULLET_LIB_BASE_DIR}
	${BULLET_LIB_BASE_DIR}/BulletCollision
	${BULLET_LIB_BASE_DIR}/BulletDynamics
	${BULLET_LIB_BASE_DIR}/LinearMath
	${IRRLICHT_LIB_DIR}
)

add_executable(
	zofiri
	zofiri.cpp
	net.cpp
	pub.cpp
	sim.cpp
	viz.cpp
	../test/world.cpp
)

target_link_libraries(
	zofiri
	${OPENGL_gl_LIBRARY}
	BulletDynamics
	BulletCollision
	LinearMath
	Irrlicht
	Xxf86vm
)

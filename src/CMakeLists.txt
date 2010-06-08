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
	mod.cpp
	net.cpp
	pub.cpp
	sim.cpp
	viz.cpp
	zof.cpp
	../test/world.cpp
)


if(CMAKE_COMPILER_IS_GNUCC)
	# TODO Mac-friendly?
	target_link_libraries(
		zofiri
		${OPENGL_gl_LIBRARY}
		BulletDynamics
		BulletCollision
		LinearMath
		Irrlicht
		Xxf86vm
	)
else(CMAKE_COMPILER_IS_GNUCC)
	target_link_libraries(
		zofiri
		${OPENGL_gl_LIBRARY}
		BulletDynamics
		BulletCollision
		LinearMath
		Irrlicht
		wsock32
	)
endif(CMAKE_COMPILER_IS_GNUCC)

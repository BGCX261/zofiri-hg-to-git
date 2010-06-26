link_directories(
	${BULLET_LIB_BASE_DIR}
	${BULLET_LIB_BASE_DIR}/BulletCollision
	${BULLET_LIB_BASE_DIR}/BulletDynamics
	${BULLET_LIB_BASE_DIR}/LinearMath
	${IRRLICHT_LIB_DIR}
)

add_library(humanoid MODULE
  humanoid.c
)

target_link_libraries(humanoid
	zof
)

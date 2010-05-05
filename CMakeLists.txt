cmake_minimum_required(VERSION 2.4)

project(ZOFIRI)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
		"Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
		FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set(IRRLICHT_INCLUDE_DIR "${ZOFIRI_SOURCE_DIR}/../irrlicht/include" CACHE STRING "The directory for Irrlicht header files.")
set(IRRLICHT_LIB_DIR "${ZOFIRI_SOURCE_DIR}/../irrlicht/lib/Linux" CACHE STRING "The directory for the Irrlicht link library.")

set(BULLET_INCLUDE_DIR "${ZOFIRI_SOURCE_DIR}/../bullet/src" CACHE STRING "The directory for Bullet Physics header files.")
set(BULLET_LIB_BASE_DIR "${ZOFIRI_SOURCE_DIR}/../bullet/build/src" CACHE STRING "The base directory for Bullet Physics link libraries.")

# OpenGL CMake scripting taken from Bullet then shrunk a lot when moving to Irrlicht.
find_package(OpenGL)
if(OPENGL_FOUND)
	# Good for us.
	# message("OPENGL FOUND")
	# message(${OPENGL_LIBRARIES})
else(OPENGL_FOUND)
	message("OPENGL NOT FOUND")
	set(OPENGL_gl_LIBRARY opengl32)
endif(OPENGL_FOUND)

# TODO Direct3D on Windows? We'll want to link with it at some point.

set(ZOFIRI_INCLUDE_DIR ${ZOFIRI_SOURCE_DIR}/include)

include_directories(
	${ZOFIRI_INCLUDE_DIR}
)

if(CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_CXX_FLAGS "-Wall")
  # set(CMAKE_CXX_FLAGS "-pg") # for profiling -- tie to debug, too?
endif(CMAKE_COMPILER_IS_GNUCC)

subdirs(
	src
)

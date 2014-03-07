#-------------------------------------------------------------------------------
# Checking CMAKE version
cmake_minimum_required( VERSION 2.8 FATAL_ERROR )

#-------------------------------------------------------------------------------
# Changing compiler and options
set( CMAKE_C_COMPILER "clang" )
set( CMAKE_CXX_COMPILER "clang++" )
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}-O3 -msse4.2 -m64 -Wclang-long-long -Wsign-compare -Wextra -Wall -pedantic" CACHE STRING "" )
set( CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/Users/claesnl" CACHE STRING "" )

#-------------------------------------------------------------------------------
# Defining the project
project( pd_clamp )
set( CMAKE_BUILD_TYPE RELEASE )
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake-modules
	${CMAKE_MODULE_PATH} )

#-------------------------------------------------------------------------------
# Find the package ...
find_package( MINC REQUIRED )

#-------------------------------------------------------------------------------
# Setup include directory for this project
include_directories( ${MINC_INCLUDE_DIR} )
include_directories( ${PROJECT_SOURCE_DIR}/include )

#-------------------------------------------------------------------------------
# Locate sources and headers for this project
file( GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cpp )
file( GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh )

#-------------------------------------------------------------------------------
# Add the executable, and link to libraries
add_executable( pd_clamp main.cpp ${sources} ${headers} )
target_link_libraries( pd_clamp ${MINC_minc2_LIBRARY} ${MINC_volume_io2_LIBRARY} )

#-------------------------------------------------------------------------------
# Install the executable to 'bin' directory under CMAKE_INSTALL_PREFIX
install( TARGETS pd_clamp DESTINATION bin )

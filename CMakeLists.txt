PROJECT(kinect)
cmake_minimum_required(VERSION 2.6)

SET (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/.orogen/config")
INCLUDE(kinectBase)

add_subdirectory(tools)

# FIND_PACKAGE(KDL)
# FIND_PACKAGE(OCL)


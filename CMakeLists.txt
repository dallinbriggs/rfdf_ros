cmake_minimum_required(VERSION 2.8.3)
project(rfdf)


find_package(catkin REQUIRED COMPONENTS
  roscpp
  rospy
  std_msgs
  geometry_msgs
)

catkin_package(
#  INCLUDE_DIRS include
#  LIBRARIES rfdf
#  CATKIN_DEPENDS roscpp rospy std_msgs
#  DEPENDS system_lib
)


include_directories(
include
src
  ${catkin_INCLUDE_DIRS}
)

add_executable(rfdf_heading_node
    src/heading.cpp)

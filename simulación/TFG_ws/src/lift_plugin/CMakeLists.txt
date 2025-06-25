cmake_minimum_required(VERSION 3.8)
project(lift_plugin)

# Dependencias
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(gazebo_ros REQUIRED)

# Incluye los headers
include_directories(
  include
)

# Plugin como biblioteca compartida
add_library(${PROJECT_NAME} SHARED src/lift_plugin.cpp)

ament_target_dependencies(${PROJECT_NAME}
  rclcpp
  std_msgs
  gazebo_ros
)

# Instala el plugin
install(TARGETS ${PROJECT_NAME}
  LIBRARY DESTINATION lib
)

ament_package()


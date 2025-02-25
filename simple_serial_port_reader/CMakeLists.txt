if(DEFINED ENV{AMENT_PREFIX_PATH})
  # ==============================
  # ROS2 用設定 (ament_cmake)
  # ==============================
  cmake_minimum_required(VERSION 3.5)
  project(simple_serial_port_reader)

  if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 14)
  endif()

  find_package(ament_cmake REQUIRED)
  find_package(rclcpp REQUIRED)
  find_package(simple_serial_port_reader_msgs REQUIRED)
  find_package(Boost REQUIRED COMPONENTS regex system)

  include_directories(include)

  add_executable(simple_serial_port_reader_node
    src/simple_serial_port_reader_node.cpp
  )
  target_link_libraries(simple_serial_port_reader_node
    rclcpp::rclcpp
    ${Boost_LIBRARIES}
  )
  # ament_target_dependencies() には、Boost は含めず、ament 対象のパッケージのみ指定
  ament_target_dependencies(simple_serial_port_reader_node
    simple_serial_port_reader_msgs
  )

  install(TARGETS simple_serial_port_reader_node
    DESTINATION lib/${PROJECT_NAME}
  )
  install(
  DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)


  ament_package()

else()
  # ==============================
  # ROS1 用設定 (catkin)
  # ==============================
  cmake_minimum_required(VERSION 2.8.3)
  project(simple_serial_port_reader)

  add_definitions(-std=c++11)

  find_package(catkin REQUIRED COMPONENTS
    nodelet
    pluginlib
    roscpp
    simple_serial_port_reader_msgs
  )
  find_package(Boost REQUIRED COMPONENTS regex system)

  include_directories(
    include
    ${catkin_INCLUDE_DIRS}
  )

  add_library(simple_serial_port_reader_plugin
    src/simple_serial_port_reader_plugin.cpp
  )
  add_dependencies(simple_serial_port_reader_plugin
    ${catkin_EXPORTED_TARGETS}
  )
  target_link_libraries(simple_serial_port_reader_plugin
    ${catkin_LIBRARIES}
    ${Boost_LIBRARIES}
  )

  catkin_package(
    CATKIN_DEPENDS nodelet pluginlib roscpp simple_serial_port_reader_msgs
  )
endif()

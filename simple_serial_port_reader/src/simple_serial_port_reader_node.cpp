#include "rclcpp/rclcpp.hpp"
#include "simple_serial_port_reader/simple_serial_port_reader_ros2.hpp"

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<simple_serial_port_reader::SimpleSerialPortReader>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}

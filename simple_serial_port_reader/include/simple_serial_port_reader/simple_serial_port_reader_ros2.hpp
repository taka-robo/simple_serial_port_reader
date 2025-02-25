#ifndef SIMPLE_SERIAL_PORT_READER_SIMPLE_SERIAL_PORT_READER_HPP
#define SIMPLE_SERIAL_PORT_READER_SIMPLE_SERIAL_PORT_READER_HPP

#include <stdexcept>
#include <string>
#include <thread>

#include <rclcpp/rclcpp.hpp>
#include <simple_serial_port_reader_msgs/msg/string_stamped.hpp>

#include <boost/algorithm/string/replace.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/regex.hpp>

namespace simple_serial_port_reader {

class SimpleSerialPortReader : public rclcpp::Node {
public:
  SimpleSerialPortReader() : Node("simple_serial_port_reader"), serial_(io_service_) {
    // Declare parameters with default values
    std::string device;
    int baud_rate;
    std::string start_command;
    std::string match_expression;
    std::string format_expression;
    std::string stop_command;
    bool verbose;

    this->declare_parameter<std::string>("device", "/dev/ttyUSB0");
    this->declare_parameter<int>("baud_rate", 9600);
    this->declare_parameter<std::string>("start_command", "");
    this->declare_parameter<std::string>("match_expression", "(.+)\r?\n");
    this->declare_parameter<std::string>("format_expression", "$1");
    this->declare_parameter<std::string>("stop_command", "");
    this->declare_parameter<bool>("verbose", false);

    this->get_parameter("device", device);
    this->get_parameter("baud_rate", baud_rate);
    this->get_parameter("start_command", start_command);
    this->get_parameter("match_expression", match_expression);
    this->get_parameter("format_expression", format_expression);
    this->get_parameter("stop_command", stop_command);
    this->get_parameter("verbose", verbose);

    start_cmd_ = replaceEscapeSequence(start_command);
    match_expr_ = boost::regex(match_expression);
    format_expr_ = format_expression;
    stop_cmd_ = replaceEscapeSequence(stop_command);
    verbose_ = verbose;

    // Open the serial port
    namespace ba = boost::asio;
    try {
      serial_.open(device);
      serial_.set_option(ba::serial_port::baud_rate(baud_rate));
    } catch (const std::exception &error) {
      RCLCPP_FATAL(this->get_logger(), "Error opening \"%s\": %s", device.c_str(), error.what());
      return;
    }
    if (verbose_) {
      RCLCPP_INFO(this->get_logger(), "Opened \"%s\"", device.c_str());
    }

    // Create publisher (ROS2ではQoSの指定が必要)
    pub_ =
        this->create_publisher<simple_serial_port_reader_msgs::msg::StringStamped>("formatted", 10);

    // スレッドを起動してblockingな読み出し処理を実行
    reader_thread_ = std::thread(&SimpleSerialPortReader::main, this);
  }

  ~SimpleSerialPortReader() {
    if (reader_thread_.joinable()) {
      reader_thread_.join();
    }
  }

private:
  void main() {
    namespace ba = boost::asio;
    try {
      // write the start command (if any)
      if (!start_cmd_.empty()) {
        ba::write(serial_, ba::buffer(start_cmd_));
        if (verbose_) {
          RCLCPP_INFO(this->get_logger(), "Wrote start command: \"%s\"", start_cmd_.c_str());
        }
      }

      // reading loop
      ba::streambuf buffer;
      while (rclcpp::ok()) {
        // read until the buffer contains the match expression
        const std::size_t bytes = ba::read_until(serial_, buffer, match_expr_);
        rclcpp::Time stamp = this->now();

        // search matched sequence in the buffer
        const char *buffer_begin = boost::asio::buffer_cast<const char *>(buffer.data());
        const char *buffer_end = buffer_begin + bytes;
        if (verbose_) {
          std::string read_str(buffer_begin, bytes);
          RCLCPP_INFO(this->get_logger(), "Read: \"%s\"", read_str.c_str());
        }
        boost::cmatch match;
        boost::regex_search(buffer_begin, buffer_end, match, match_expr_);
        if (verbose_) {
          RCLCPP_INFO(this->get_logger(), "Matched: \"%s\"", match.str().c_str());
        }

        // Format the matched sequence
        simple_serial_port_reader_msgs::msg::StringStamped formatted;
        formatted.header.stamp = stamp;
        formatted.data = match.format(format_expr_);
        if (verbose_) {
          RCLCPP_INFO(this->get_logger(), "Formatted: \"%s\"", formatted.data.c_str());
        }

        // Publish the formatted string
        pub_->publish(formatted);

        // Consume the processed data from the buffer
        buffer.consume(bytes);
      }

      // write the stop command (if any)
      if (!stop_cmd_.empty()) {
        ba::write(serial_, ba::buffer(stop_cmd_));
        if (verbose_) {
          RCLCPP_INFO(this->get_logger(), "Wrote stop command: \"%s\"", stop_cmd_.c_str());
        }
      }
    } catch (const std::exception &error) {
      RCLCPP_ERROR(this->get_logger(), "%s", error.what());
    }
  }

  static std::string replaceEscapeSequence(std::string str) {
    static const std::pair<std::string, std::string> replace_map[] = {
        {R"(\a)", "\a"}, {R"(\b)", "\b"}, {R"(\f)", "\f"}, {R"(\n)", "\n"},
        {R"(\r)", "\r"}, {R"(\t)", "\t"}, {R"(\v)", "\v"}, {R"(\\)", "\\"},
        {R"(\?)", "\?"}, {R"(\')", "\'"}, {R"(\")", "\""}, {R"(\0)", "\0"}};
    for (const auto &entry : replace_map) {
      boost::algorithm::replace_all(str, entry.first, entry.second);
    }
    return str;
  }

  // メンバ変数
  std::string start_cmd_;
  boost::regex match_expr_;
  std::string format_expr_;
  std::string stop_cmd_;
  bool verbose_;

  rclcpp::Publisher<simple_serial_port_reader_msgs::msg::StringStamped>::SharedPtr pub_;
  boost::asio::io_service io_service_;
  boost::asio::serial_port serial_;
  std::thread reader_thread_;
};

} // namespace simple_serial_port_reader

#endif

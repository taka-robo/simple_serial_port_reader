#ifndef SIMPLE_SERIAL_PORT_READER_SIMPLE_SERIAL_PORT_READER_HPP
#define SIMPLE_SERIAL_PORT_READER_SIMPLE_SERIAL_PORT_READER_HPP

#include <stdexcept>
#include <string>

#include <nodelet/nodelet.h>
#include <ros/node_handle.h>
#include <ros/publisher.h>
#include <ros/subscriber.h>
#include <ros/timer.h>
#include <std_msgs/String.h>
#include <simple_serial_port_reader_msgs/StringStamped.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/regex.hpp>

namespace simple_serial_port_reader {

class SimpleSerialPortReader : public nodelet::Nodelet {
public:
  SimpleSerialPortReader() : serial_(io_service_) {}

private:
  virtual void onInit() override {
    namespace ba = boost::asio;
    namespace sspr_msgs = simple_serial_port_reader_msgs;

    ros::NodeHandle nh = getNodeHandle(), pnh = getPrivateNodeHandle(), mnh = getMTNodeHandle();

    // load parameters
    const std::string device = pnh.param<std::string>("device", "/dev/ttyUSB0");
    const int baud_rate = pnh.param("baud_rate", 9600);
    start_cmd_ = replaceEscapeSequence(pnh.param<std::string>("start_command", ""));
    match_expr_ = pnh.param<std::string>("match_expression", "(.+)\r?\n");
    format_expr_ = pnh.param<std::string>("format_expression", "$1");
    stop_cmd_ = replaceEscapeSequence(pnh.param<std::string>("stop_command", ""));
    verbose_ = pnh.param("verbose", false);

    // open the serial port
    try {
      serial_.open(device);
      serial_.set_option(ba::serial_port::baud_rate(baud_rate));
    } catch (const std::exception &error) {
      NODELET_FATAL_STREAM("error on opening \"" << device << "\": " << error.what());
      return;
    }
    if (verbose_) {
      NODELET_INFO_STREAM("opened \"" << device << "\"");
    }

    // create the publisher
    pub_ = nh.advertise<sspr_msgs::StringStamped>("formatted", 1);

    // create the subscriber
    sub_ = nh.subscribe<std_msgs::String>("input_string", 1, &SimpleSerialPortReader::stringCallback, this);

    // call main function (blocking) on multi-threading queue via one-shot timer
    timer_ = mnh.createTimer(ros::Duration(0.1), &SimpleSerialPortReader::main, this,
                             /* one_shot = */ true);
  }

  void main(const ros::TimerEvent &) {
    namespace ba = boost::asio;
    namespace sspr_msgs = simple_serial_port_reader_msgs;

    try {
      // write the start command (if any)
      if (!start_cmd_.empty()) {
        ba::write(serial_, ba::buffer(start_cmd_));
        if (verbose_) {
          NODELET_INFO_STREAM("wrote as the start command: \"" << start_cmd_ << "\"");
        }
      }

      // reading loop
      ba::streambuf buffer;
      while (ros::ok()) {
        // read until the buffer contains the match_expression
        const std::size_t bytes = ba::read_until(serial_, buffer, match_expr_);
        const ros::Time stamp = ros::Time::now();

        // search matched sequence in the buffer
        const char *const buffer_begin = ba::buffer_cast<const char *>(buffer.data());
        const char *const buffer_end = buffer_begin + bytes;
        if (verbose_) {
          NODELET_INFO_STREAM("read: \"" << std::string(buffer_begin, bytes) << "\"");
        }
        boost::cmatch match;
        boost::regex_search(buffer_begin, buffer_end, match, match_expr_);
        if (verbose_) {
          NODELET_INFO_STREAM("matched: \"" << match.str() << "\"");
        }

        // format the matched sequence
        sspr_msgs::StringStampedPtr formatted(new sspr_msgs::StringStamped());
        formatted->header.stamp = stamp;
        formatted->data = match.format(format_expr_);
        if (verbose_) {
          NODELET_INFO_STREAM("formatted: \"" << formatted->data << "\"");
        }

        // publish the formatted string
        pub_.publish(formatted);

        // consume the buffer processed in this loop
        buffer.consume(bytes);
      }

      // write the stop command (if any)
      if (!stop_cmd_.empty()) {
        ba::write(serial_, ba::buffer(stop_cmd_));
        if (verbose_) {
          NODELET_INFO_STREAM("wrote as the stop command: \"" << stop_cmd_ << "\"");
        }
      }
    } catch (const std::exception &error) {
      NODELET_ERROR_STREAM(error.what());
    }
  }

  void stringCallback(const std_msgs::String::ConstPtr &msg) {
    namespace ba = boost::asio;
    try {
      if (verbose_) {
        NODELET_INFO_STREAM("received string: \"" << msg->data << "\"");
      }
      ba::write(serial_, ba::buffer(msg->data));
      if (verbose_) {
        NODELET_INFO_STREAM("sent string via serial: \"" << msg->data << "\"");
      }
    } catch (const std::exception &error) {
      NODELET_ERROR_STREAM("error sending string via serial: " << error.what());
    }
  }

  static std::string replaceEscapeSequence(std::string str) {
    static const std::string replace_map[][2] = {{R"(\a)", "\a"}, {R"(\b)", "\b"}, {R"(\f)", "\f"},
                                                 {R"(\n)", "\n"}, {R"(\r)", "\r"}, {R"(\t)", "\t"},
                                                 {R"(\v)", "\v"}, {R"(\\)", "\\"}, {R"(\?)", "\?"},
                                                 {R"(\')", "\'"}, {R"(\")", "\""}, {R"(\0)", "\0"}};
    for (const std::string(&entry)[2] : replace_map) {
      boost::algorithm::replace_all(str, entry[0], entry[1]);
    }
    return str;
  }

private:
  std::string start_cmd_;
  boost::regex match_expr_;
  std::string format_expr_;
  std::string stop_cmd_;
  bool verbose_;

  ros::Timer timer_;
  ros::Publisher pub_;
  ros::Subscriber sub_;

  boost::asio::io_service io_service_;
  boost::asio::serial_port serial_;
};

} // namespace simple_serial_port_reader

#endif

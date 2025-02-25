
if(DEFINED ENV{AMENT_PREFIX_PATH})
  # ===== ROS2用設定 =====
  cmake_minimum_required(VERSION 3.5)
  project(simple_serial_port_reader_msgs)

  # ROS2用パッケージの検索
  find_package(ament_cmake REQUIRED)
  find_package(rosidl_default_generators REQUIRED)
  find_package(std_msgs REQUIRED)

  # メッセージファイルの指定（パスはパッケージ直下のmsgフォルダ内）
  set(msg_files
    "msg/StringStamped.msg"
  )

  # インターフェース生成（メッセージ定義）
  rosidl_generate_interfaces(${PROJECT_NAME}
    ${msg_files}
    DEPENDENCIES std_msgs
  )

  # 依存情報のエクスポート
  ament_export_dependencies(rosidl_default_runtime)
  ament_package()

else()
  # ===== ROS1用設定 =====
  cmake_minimum_required(VERSION 3.0.2)
  project(simple_serial_port_reader_msgs)

  # ROS1用パッケージの検索
  find_package(catkin REQUIRED COMPONENTS
    message_generation
    std_msgs
  )

  # メッセージファイルの指定（パッケージ直下のmsgフォルダ内）
  add_message_files(
    FILES
    StringStamped.msg
  )

  # メッセージ生成の実行
  generate_messages(
    DEPENDENCIES
    std_msgs
  )

  # catkinパッケージの設定（メッセージランタイムなどの依存情報をエクスポート）
  catkin_package(
    CATKIN_DEPENDS message_runtime std_msgs
  )

  # includeディレクトリの追加（必要に応じて）
  include_directories(
    ${catkin_INCLUDE_DIRS}
  )
endif()

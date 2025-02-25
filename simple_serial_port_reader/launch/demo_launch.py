#!/usr/bin/env python3
import launch
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='simple_serial_port_reader',
            executable='simple_serial_port_reader_node',
            name='demo_reader',
            output='screen',
            remappings=[('formatted', 'o2/state')],
            parameters=[{
                'device': '/dev/ttyUSB0',
                'baud_rate': 9600,
                'match_expression': '% (\\d+\\.\\d+) e \\d+\\r\\n',
                'format_expression': 'O2 = $1 %',
                'verbose': True
            }]
        )
    ])

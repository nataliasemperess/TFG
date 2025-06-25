import sys
if sys.prefix == '/usr':
    sys.real_prefix = sys.prefix
    sys.prefix = sys.exec_prefix = '/TFG_ws/src/install/mqtt_ros_bridge'

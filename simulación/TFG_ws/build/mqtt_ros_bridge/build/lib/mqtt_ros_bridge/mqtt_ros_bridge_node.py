import rclpy
from rclpy.node import Node
from std_msgs.msg import String
import paho.mqtt.client as mqtt

class MQTTBridge(Node):
    def __init__(self):
        super().__init__('mqtt_ros_bridge_node')
        self.publisher_ = self.create_publisher(String, 'ascensor_planta', 10)

        self.mqtt_client = mqtt.Client()
        self.mqtt_client.on_connect = self.on_connect
        self.mqtt_client.on_message = self.on_message

        self.mqtt_client.connect("localhost", 1883, 60)
        self.mqtt_client.loop_start()

    def on_connect(self, client, userdata, flags, rc):
        self.get_logger().info('Conectado a MQTT broker')
        client.subscribe("ascensor/planta")

    def on_message(self, client, userdata, msg):
        payload = msg.payload.decode()
        self.get_logger().info(f'Recibido por MQTT: {payload}')
        ros_msg = String()
        ros_msg.data = payload
        self.publisher_.publish(ros_msg)

def main(args=None):
    rclpy.init(args=args)
    node = MQTTBridge()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


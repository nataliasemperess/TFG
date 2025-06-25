import rclpy
from rclpy.node import Node
from std_msgs.msg import String
from gazebo_msgs.srv import SetEntityState
from gazebo_msgs.msg import EntityState
import yaml
import os

class ElevatorMover(Node):
    def __init__(self):
        super().__init__('elevator_mover_node')

        # Ruta absoluta al archivo YAML
        base_path = os.path.dirname(os.path.abspath(__file__))
        yaml_path = os.path.join(base_path, '../../../rmf_nayar/maps/nayar/nayar.building.yaml')

        # Leer alturas desde el YAML
        self.plantas = {}
        try:
            with open(yaml_path, 'r') as f:
                data = yaml.safe_load(f)
                if 'levels' in data:
                    for nivel, props in data['levels'].items():
                        self.plantas[nivel.lower()] = props.get('elevation', 0.0)
                self.get_logger().info(f'Plantas cargadas: {self.plantas}')
        except Exception as e:
            self.get_logger().error(f'Error leyendo el archivo YAML: {e}')

        # Cliente al servicio de Gazebo
        self.client = self.create_client(SetEntityState, '/gazebo/set_entity_state')
        while not self.client.wait_for_service(timeout_sec=1.0):
            self.get_logger().info('Esperando al servicio /gazebo/set_entity_state...')

        # Suscripción al tópico
        self.sub = self.create_subscription(String, '/ascensor_planta', self.planta_callback, 10)
        self.get_logger().info('ElevatorMover listo y escuchando plantas (ej: planta2)')

    def planta_callback(self, msg):
        nombre_planta = msg.data.strip().lower()
        if nombre_planta not in self.plantas:
            self.get_logger().warn(f'Planta desconocida: {nombre_planta}')
            return

        altura = self.plantas[nombre_planta]
        state = EntityState()
        state.name = 'lift1'
        state.pose.position.x = 0.0
        state.pose.position.y = 0.0
        state.pose.position.z = altura
        state.pose.orientation.w = 1.0

        req = SetEntityState.Request()
        req.state = state

        self.client.call_async(req)
        self.get_logger().info(f'Moviendo a {nombre_planta} (z = {altura})')

def main(args=None):
    rclpy.init(args=args)
    node = ElevatorMover()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


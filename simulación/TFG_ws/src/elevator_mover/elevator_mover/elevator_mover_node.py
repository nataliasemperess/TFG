import rclpy
from rclpy.node import Node
from std_msgs.msg import String
from gazebo_msgs.srv import SetModelState
from gazebo_msgs.msg import ModelState
from geometry_msgs.msg import Pose, Twist

class ElevatorMover(Node):
    def __init__(self):
        super().__init__('elevator_mover_node')

        # 1. Crear el cliente para el servicio de Gazebo
        self.client = self.create_client(SetModelState, '/gazebo/set_model_state')
        while not self.client.wait_for_service(timeout_sec=1.0):
            self.get_logger().info('Esperando al servicio /gazebo/set_model_state...')

        # 2. Suscribirse al topic /ascensor_planta (tipo String)
        self.subscription = self.create_subscription(
            String,
            '/ascensor_planta',
            self.listener_callback,
            10
        )

    def listener_callback(self, msg):
        planta = msg.data.strip()
        self.get_logger().info(f'Recibida solicitud para mover a planta: {planta}')

        # 3. Convertir planta a altura en el eje Z (por ejemplo, planta 0 = 0.0, planta 1 = 2.5, etc.)
        try:
            planta_num = int(planta)
            altura_z = planta_num * 2.5  # Ajusta este valor según tu simulación
        except ValueError:
            self.get_logger().warn('El mensaje no es un número de planta válido.')
            return

        # 4. Crear el mensaje ModelState
        state = ModelState()
        state.model_name = 'lift1'
        state.pose = Pose()
        state.pose.position.x = 0.0
        state.pose.position.y = 0.0
        state.pose.position.z = altura_z
        state.pose.orientation.x = 0.0
        state.pose.orientation.y = 0.0
        state.pose.orientation.z = 0.0
        state.pose.orientation.w = 1.0
        state.twist = Twist()
        state.reference_frame = 'world'

        # 5. Enviar la solicitud al servicio
        request = SetModelState.Request()
        request.model_state = state
        future = self.client.call_async(request)

        self.get_logger().info(f'Moviendo el ascensor a planta {planta_num} (Z={altura_z})')

def main(args=None):
    rclpy.init(args=args)
    node = ElevatorMover()
    rclpy.spin(node)
    rclpy.shutdown()


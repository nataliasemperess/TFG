import paho.mqtt.client as mqtt
import subprocess
import json

# Mapeo de las plantas MQTT a los nombres de planta en ROS 2
floor_mapping = {
    "planta0": "floor0",  # planta0 se mapea a floor0
    "planta1": "floor1",  # planta1 se mapea a floor1
    "planta2": "floor2",  # planta2 se mapea a floor2
    # Agrega más plantas según sea necesario
}

# Esta función se ejecuta cuando recibimos un mensaje MQTT
def on_message(client, userdata, message):
    try:
        # Convertir el mensaje de MQTT a un diccionario Python
        msg = json.loads(message.payload.decode())
        
        # Obtener la planta de destino desde el mensaje
        destination_floor = msg.get("destination_floor")

        # Si la planta no está en el mapeo, mostramos un error
        if destination_floor not in floor_mapping:
            print(f"Planta desconocida: {destination_floor}")
            return

        # Mapeamos la planta a su nombre en ROS 2
        ros_floor_name = floor_mapping[destination_floor]

        # Construir el comando de ROS 2 para ejecutar la tarea de patrol en la planta correcta
        command = f"ros2 run rmf_demos_tasks dispatch_patrol -p {ros_floor_name} -n 1 --use_sim_time"

        # Ejecutar el comando en el sistema
        subprocess.run(command, shell=True)
        print(f"Ejecutando comando para mover a: {ros_floor_name}")

    except Exception as e:
        print(f"Error al procesar el mensaje: {e}")

# Configuración del cliente MQTT
client = mqtt.Client()
client.connect("localhost", 1883, 60)  # Asegúrate de que el broker MQTT esté corriendo

# Suscríbete al tópico donde se recibe el mensaje de destino
client.subscribe("/ascensor/move_to")  # Este es el tópico donde el robot recibe los mensajes de planta

# Asignamos la función que se ejecutará cuando recibamos un mensaje
client.on_message = on_message

# Mantener el cliente MQTT en ejecución para recibir los mensajes
client.loop_forever()


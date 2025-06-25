## Natalia Sempere - TFG SIMULACIÓN NAYAR

### DOCUMENTACIÓN DEL MODELO OPEN-RMF - EDIFICIO DE NAYAR SYSTEMS

Este documento describe de forma detallada el procedimiento para construir un modelo funcional en Open-RMF, en este caso basado en un modelo del edificio de Nayar Systems.

#### Solo si no funciona el Docker

1. Clonar el repositori https://github.com/open-rmf/rmf
   
2. Editar l'arxiu rmf.repos, canviant "main" per "jazzy" a les línies 33 i 45 (versions dels repositoris rmf_simulation i rmf_traffic_editor, ho expliquen ací: https://github.com/open-rmf/rmf_demos/issues/306)
   
3. Generar la imatge de Docker amb l'ordre
 
 ```bash
docker build -t rmf_demos .
```

```bash
cd ~/Documentos/GitHub/TFG/simulación/
docker build -t rmf_demos_humble -f Dockerfile.rmf_fixed .
```

4. Comprovar que funciona bé amb la demo de l'office:
   
```bash
rocker --nvidia --x11 --name rmf_demos   -e ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST   --network host   rmf_demos bash

sudo cp -R /root/.gazebo .
ros2 launch rmf_demos_gz  office.launch.xml
```


5. Provar el teu codi:
```bash
rocker --nvidia --x11 --name rmf_demos   -e ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST   --network host --user   --volume `pwd`/rmf_ws:/home/usuario/rmf_ws --  rmf_demos  bash
sudo cp -R /root/.gazebo .
cd rmf_ws
colcon build
source install/setup.bash
ros2 launch rmf_demos_gz TI.launch.xml
```

#### 1) Docker container with : 


```bash
cd /home/usuario/Documentos/GitHub/TFG/simulación/
rocker --nvidia --x11 --name rmf_nayar \
  -e ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST \
  --network host \
  --user \
  --volume `pwd`/TFG_ws:/TFG_ws \
  --mode interactive \
  -- rmf_demos bash

```

#### Abrir otro docker

```bash
docker ps
docker exec -it rmf_nayar bash

```

#### 2 ) Traffic-editor.

Desde dentro del docker, abriremos el traffic-editor y comenzaremos a construir el modelo:

```bash
cd /home/usuario/Documentos/GitHub/TFG/simulación/TFG_ws/src/rmf_nayar/maps
traffic-editor
```

#### 3) Generate the Gazebo world 

```bash
source /opt/ros/jazzy/setup.bash
sudo cp -R /root/.gazebo .	
cd ../../TFG_ws

rm -rf build/ install/ log/
colcon build
source install/setup.bash
```

#### Terminal 1 : Lanzar Gazebo y RViz

```bash 
ros2 launch rmf_nayar nayar.launch.xml 
```

#### Terminal 1 : Lanzar Tasks por plantas

```bash
docker exec -it rmf_nayar bash
cd ../../TFG_ws/
source /TFG_ws/install/setup.bash
source /opt/ros/jazzy/setup.bash
ros2 run rmf_demos_tasks dispatch_patrol -p floor1 -n 1 --use_sim_time

```



#### Terminal 2 : MQTT-ROS


```bash
docker exec -it rmf_nayar bash
cd ../../TFG_ws/
source /TFG_ws/install/setup.bash
source /opt/ros/jazzy/setup.bash
sudo apt update && sudo apt install -y mosquitto
mosquitto -d
ros2 run mqtt_ros_bridge mqtt_ros_bridge_node
```
#### Terminal 3 : Prueba envio por MQTT


```bash
docker exec -it rmf_nayar bash
cd ../../TFG_ws/
source /TFG_ws/install/setup.bash
source /opt/ros/jazzy/setup.bash

sudo apt update
sudo apt install mosquitto-clients

mosquitto_sub -h localhost -t "ascensor/planta"
mosquitto_pub -h localhost -t "ascensor/planta" -m "1" 
```
![imagen](https://github.com/user-attachments/assets/06220a7f-9963-491d-a5cf-5ed0301cc5b8)

#### Terminal 4 : Mover el ascensor 


```bash
docker exec -it rmf_nayar bash
cd ../../TFG_ws/
source /TFG_ws/install/setup.bash
source /opt/ros/jazzy/setup.bash

```

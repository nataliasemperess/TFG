## Natalia Sempere - TFG SIMULACIÓN NAYAR


```bash
cd ~/Documentos/GitHub/TFG/simulación
docker build -t rmf_demos_humble -f Dockerfile.rmf_fixed .

```

#### 1) Docker container with : 


```bash
rocker --nvidia --x11 \
  --name rmf_demos_humble \
  -v $(pwd)/TFG_ws:/TFG_ws \
  -e ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST \
  --network host \
  rmf_demos_humble

```

```bash
apt update
apt install -y ros-humble-rmf-building-map-tools
apt update
apt install -y ros-humble-gazebo-ros-pkgs ros-humble-gazebo-ros2-control

```

#### Generate the Gazebo world 

```bash
source /opt/ros/humble/setup.bash
rm -rf build/ install/ log/


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

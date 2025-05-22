## Natalia Sempere - TFG SIMULACIÓN NAYAR

### DOCUMENTACIÓN DEL MODELO OPEN-RMF - EDIFICIO DE NAYAR SYSTEMS

Este documento describe de forma detallada el procedimiento para construir un modelo funcional en Open-RMF, en este caso basado en un modelo del edificio de Nayar Systems.

![imagen](https://github.com/user-attachments/assets/b9e2384d-8048-41b2-9bd1-63e3500742e2)

#### 1) Create a ROS workspace named "TFG_ws" with a README.md file in it. 

En primer lugar, creamos la estructura básica del workspace de ROS 2, donde se alojará el paquete del examen:

```bash
mkdir -p /home/usuario/Documentos/GitHub/TFG/simulación/TFG_ws/src
cd /home/usuario/Documentos/GitHub/TFG/simulación/TFG_ws/
touch README.md
```

#### 2) Run the Open-RMF Docker container with : 

En este paso iniciamos el entorno Docker para facilitar el desarrollo : 

```bash
cd /home/usuario/Documentos/GitHub/TFG/simulación/

rocker --nvidia --x11 --name rmf_nayar -e ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST --network host --user --volume `pwd`/TFG_ws:/TFG_ws -- ghcr.io/open-rmf/rmf/rmf_demos:latest bash
```
Este contenedor nos ofrece un entorno gráfico, conectividad de red y monta el workspace exam_ws en /exam_ws dentro del contenedor.

#### 3) Create a ROS package named "rmf_library" and the folders : "launch", "config" and "maps".

A continuación, se crea el paquete principal del proyecto utilizando el comando ros2 pkg create, especificando el sistema de construcción ament_cmake, y la licencia Apache-2.0. Luego, se generan manualmente 3 carpetas básicas que formarán parte de la estructura estándar del paquete: 

```bash

cd /home/usuario/Documentos/GitHub/TFG/simulación/TFG_ws/src
source /opt/ros/humble/setup.bash
ros2 pkg create rmf_nayar --build-type ament_cmake --license Apache-2.0
cd rmf_nayar
mkdir launch config maps
```
Organizaré mis carpetas de la siguiente manera : 

- maps/ : contendrá los planos de la biblioteca (en PNG) y el archivo del modelo del edificio "library.building.yaml" (se creará en los siguientes pasos).

- launch/ : contendrá los archivos de lanzamiento XML utlizados para iniciar los distitnos componentes del sistema, en mi caso contaré con :

  	- common.launch.xml (inicialización común para Gazebo y RViz)
  	- library.launch.xml (archivo principal de lanzar la simulación)
  	- simulation.launch.xml (control de los recursos de simulación)
  	- visualization.launch.xml (inicializa la visualización en RViz)
 
- config/ : aquí se encontrarán los archivos de configuración para los robots y RViz, tendremos :

  	- cleanerBotA_config.yaml y tinyRobot_config.yaml (configuración de los robots)
  	- library.rviz (configuración personalizada del entorno RViz.

Además fuera de las carpetas, en "exam_ws/src/rmf_library/" contamos con dos archivos fundamentales para cualquier paquete en ROS2: 

- CMakeList.txt : Se trata de un archivo obligatorio para la compilación con colcon. Este archivo define cómo se construye el paquete, incluyendo dependencias, bibliotecas, ejecutables, etc. Además en mi caso, lo usaremos también para lanzar el mundo Gazebo y la visualización de los grafos (en los pasos siguientes se verá como se lanza una vez configurado).

- package.xml : Contiene el nombre del paquete, autor, licencia, dependencias, descripciones y metadatos para que ROS 2 lo reconozca.

#### 4) Download the floor plans of the library and convert the PDFs to PNGs and store in "map" folder:

Los planos se descargan desde la web oficial de la UJI y se convertirán para ser usados en traffic-editor.

```bash
cd /home/usuario/Documentos/GitHub/TFG/simulación/TFG_ws/src/rmf_nayar/maps

pdftoppm CD-n0-1.pdf CD-n0-1 -png -singlefile
pdftoppm CD-n1-1.pdf CD-n1-1 -png -singlefile
pdftoppm CD-n2-1.pdf CD-n2-1 -png -singlefile
pdftoppm CD-n3-1.pdf CD-n3-1 -png -singlefile
pdftoppm CD-n4-1.pdf CD-n4-1 -png -singlefile
pdftoppm CD-n5-1.pdf CD-n5-1 -png -singlefile
```

#### 5) Create a building model named "library.building.yaml" in the "maps" folder with traffic-editor.

Desde dentro del docker, abriremos el traffic-editor y comenzaremos a construir el modelo:

```bash
cd /home/usuario/Documentos/GitHub/TFG/simulación/TFG_ws/src/rmf_nayar/maps
traffic-editor
```
![imagen](https://github.com/user-attachments/assets/92b1493a-998f-43d0-8b7d-8e479c3e49ee)

#### 6) Generate the Gazebo world and navigation graphs.

```bash
source /opt/ros/jazzy/setup.bash
sudo cp -R /root/.gazebo .	
cd ../../exam_ws

rm -rf build/ install/ log/
colcon build
source install/setup.bash
```

#### 7) Create a launch file named "library.launch.xml" for running the simulation and visualization (you can create additional launch files if needed)

#### Terminal 1 : Lanzar Gazebo y RViz

```bash 
ros2 launch rmf_nayar nayar.launch.xml 
```
Una vez nos ubiquemos en el Gazebo y en el RViz, hay que tener en cuenta que el edificio lo he diseñado para que empiece por la planta "L2_entrada" (así lo deberás indicar en el RViz para poder visualizarla), el resto de plantas corresponden a los nombres : "L1", "L3", "L4", "L5" Y "L6".

![imagen](https://github.com/user-attachments/assets/9e64da1b-04bd-49c7-b4c2-5352f8899778)

#### Terminal 2 : API Server (Solo en el caso de lanzar las tasks por la Dashboard)

Abrimos un segundo terminal donde ejecutaremos el servidor API para la interacción con los servicios:

```bash
docker run --network host -it \
  -e ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST \
  -e RMW_IMPLEMENTATION=rmw_cyclonedds_cpp \
	ghcr.io/open-rmf/rmf-web/api-server:latest
```
#### Terminal 3 : Dashboard (Solo en el caso de lanzar las tasks por la Dashboard)

En otro terminal, ejecutamos el Dashboard para tener una visualización de las tareas y el estado de los robots:

```bash
docker run --network host -it \
  -e RMF_SERVER_URL=http://localhost:8000 \
  -e TRAJECTORY_SERVER_URL=ws://localhost:8006 \
	ghcr.io/open-rmf/rmf-web/dashboard:latest
```
URL del Dashboard : http://localhost:3000

#### 8) Add the instructions for running several patrol and clean tasks in the command line.

#### POR TERMINAL 

PATROL TASK 1 : 

```bash
docker exec -it rmf_library bash
cd ../../exam_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run rmf_demos_tasks dispatch_patrol -p entrada_ascensor -n 1 --use_sim_time
```
![task1](https://github.com/user-attachments/assets/a41f0fa2-5ba7-4055-a647-92c02b6cea38)


https://github.com/user-attachments/assets/a06aa358-01c8-41d4-9312-4615f683fb65


PATROL TASK 2 : 

```bash
docker exec -it rmf_library bash
cd ../../exam_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run rmf_demos_tasks dispatch_patrol -p salon_L3 -n 1 --use_sim_time
```
![imagen](https://github.com/user-attachments/assets/69fe00e7-8a90-4589-be70-a0feb356a3b8)

https://github.com/user-attachments/assets/3dfe7d25-b6d3-47ee-9089-054ae113bca7


CLEAN TASK 1 : 

```bash
docker exec -it rmf_library bash
cd ../../exam_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run rmf_demos_tasks dispatch_clean -cs  clean_zona2 --use_sim_time
```
![imagen](https://github.com/user-attachments/assets/e4f9dd0f-23d7-4003-aeeb-95d2699019c2)

https://github.com/user-attachments/assets/06f76fd7-66c0-4858-893d-d1fbc0d071e9


CLEAN TASK 2 : 

```bash
docker exec -it rmf_library bash
cd ../../exam_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run rmf_demos_tasks dispatch_clean -cs  clean_zona3  --use_sim_time
```

![imagen](https://github.com/user-attachments/assets/37744c75-f644-49ac-ba5b-ea15184a292b)


https://github.com/user-attachments/assets/f1903293-113d-4f70-8c6f-135488ea1be6


#### POR DASHBOARD (EXTRA)

PATROL TASK 1 : entrada_ascensores -> salon_L3

https://github.com/user-attachments/assets/49701f72-eb95-48b0-8db6-e80f5587ea66

CLEAN TASK 1 : clean_zona2

https://github.com/user-attachments/assets/960d060b-0275-464e-9a47-25a340e08052

----------------------------------------------------------------------------------
### EXTRAS PARA PRUEBAS CON GAZEBO INDEPENDIENTES

```bash
# Se hace dentro del root
 cd /exam_ws/src/rmf_library/maps
ros2 run rmf_building_map_tools building_map_generator gazebo library.building.yaml library.world ./library_world
```

```bash
ros2 run rmf_building_map_tools building_map_model_downloader library.building.yaml -e ./models

export GZ_SIM_RESOURCE_PATH=`pwd`/library_world:`pwd`/models

// Usamos este export para cuando tengamos ya el robot, sino solo usamos el primer export
export GZ_SIM_RESOURCE_PATH=`pwd`/library_world:`pwd`/models:/rmf_demos_ws/install/rmf_demos_assets/share/rmf_demos_assets/models

gz sim -r -v 3 library.world

```

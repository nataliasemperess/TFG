#include <Wire.h>
#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <VL53L1X.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <LoRa.h>

// Pines + Tamaño OLED
#define SDA 4
#define SCL 15
#define OLED_RST 16
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Pines para VL53L1X en I2C secundario (Wire1)
#define SDA_VL53 21
#define SCL_VL53 22

// Pines LoRa
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26
#define BAND 866E6

Adafruit_MPU6050 mpu;
TwoWire WireVL53 = TwoWire(1);
VL53L1X sensorPuerta;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
HardwareSerial LaserSerial(2);

String estado = "";

int ciclosSinMovimiento = 0;

bool ascensorEnMovimiento = false;
bool estabaEnMovimiento = true;
bool sensorVL53L1X_OK = false;
volatile bool movimientoDetectado = false; // Interrupciones

QueueHandle_t colaEnvioLoRa;

enum TipoDato
{
  DATOS_MOV,
  DISTANCIA_PUERTA,
  DISTANCIA_TECHO
};

struct MensajeLoRa
{
  TipoDato tipo;
  union
  {
    struct
    {
      float ax;
      float ay;
      float az;
      float gx;
      float gy;
      float gz;
    } datos_mov;

    float distancia_puerta;
    float distancia_techo;
  };
};

struct Medicion
{
  float ax;
  float ay;
  float az;
  float gx;
  float gy;
  float gz;
};

void IRAM_ATTR detectarMovimiento();
void inicializarMPU();
void inicializarPuerta();
void encenderLaser();
void distanciaPuerta();
void recogerDatosAc();
void estadoAscensor();
void distanciaPuertaTask(void *pvParameters);
void distanciaTecho(void *pvParameters);
void envioLoRA(void *pvParameters);

// Variables para mostrar en pantalla
float ultimaDistanciaTecho = -1;
float ultimaDistanciaPuerta = -1;
void actualizarDisplay()
{
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.print("Estado: ");
  display.println(estado);

  display.setCursor(0, 20);
  display.print("Techo: ");
  if (ultimaDistanciaTecho >= 0)
  {
    display.print(ultimaDistanciaTecho, 2);
    display.println(" m");
  }
  else
  {
    display.println("N/A");
  }

  display.setCursor(0, 40);
  display.print("Puerta: ");
  if (ultimaDistanciaPuerta >= 0)
  {
    display.print(ultimaDistanciaPuerta, 2);
    display.println(" m");
  }
  else
  {
    display.println("N/A");
  }

  display.display();
}

void IRAM_ATTR detectarMovimiento()
{
  movimientoDetectado = true;
}

void inicializarMPU()
{
  if (!mpu.begin())
  {
    Serial.println("Failed to find MPU6050 chip");
  }
  Serial.println("MPU6050 Found!");

  mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ); // Elimina vibraciones bajas (útil en parado)

  // mpu.setHighPassFilter(MPU6050_HIGHPASS_DISABLE);
  mpu.setMotionDetectionThreshold(0); // Umbral mínimo de mov (0 es demasiado bajo, cambiarlo)
  mpu.setMotionDetectionDuration(49); // Tiempo que el mov debe durar para generar interrupción
  mpu.setInterruptPinLatch(false);    // Señal de interrupción activa hasta "limpiar" señal
  mpu.setInterruptPinPolarity(true);  // Polaridad del pin de interrupción (True -> Nivel Alto)
  mpu.setMotionInterrupt(true);       // Activa la interrupción por detección de mov
}

void inicializarPuerta()
{
  sensorPuerta.setBus(&WireVL53);
  Serial.print("Iniciando puerta...");
  if (!sensorPuerta.init())
  {
    Serial.println("VL53L1X ERROR");
  }
  else
  {
    delay(6000);
    sensorPuerta.setDistanceMode(VL53L1X::Long);
    sensorPuerta.setMeasurementTimingBudget(100000);
    Serial.println("VL53L1X OK");
    // Iniciamos medición continua con un intervalo de 50 ms entre lecturas.
    // sensorPuerta.startContinuous(50);
  }
}

void encenderLaser()
{
  LaserSerial.println("O");
  Serial.println("He enviado O para encender el laser");
  delay(500);
}

void distanciaPuerta()
{
  Serial.println("[Puerta] Intentando medir...");

  // Medición simple
  int distancia = sensorPuerta.readSingle();
  Serial.printf("[Puerta] Resultado: %d mm\n", distancia);

  // Validación simple (más tolerante)
  if (distancia > 0 && distancia < 3000)
  {
    float distancia_m = distancia / 1000.0;
    MensajeLoRa msg;
    msg.tipo = DISTANCIA_PUERTA;
    msg.distancia_puerta = distancia_m;

    ultimaDistanciaPuerta = distancia_m;
    actualizarDisplay();

    if (xQueueSend(colaEnvioLoRa, &msg, 10) != pdTRUE)
    {
      Serial.println("¡Cola llena! No se pudo enviar mensaje LoRa");
    }
  }
  else
  {
    Serial.println("[Puerta] Valor inválido. Reiniciando sensor...");
    inicializarPuerta();
  }
}

void recogerDatosAc()
{
  if (movimientoDetectado)
  {
    movimientoDetectado = false;
    ascensorEnMovimiento = true;

    sensors_event_t a, g, temp;

    mpu.getEvent(&a, &g, &temp);

    MensajeLoRa msg;
    msg.tipo = DATOS_MOV;
    msg.datos_mov.ax = a.acceleration.x;
    msg.datos_mov.ay = a.acceleration.y;
    msg.datos_mov.az = a.acceleration.z;
    msg.datos_mov.gx = g.gyro.x;
    msg.datos_mov.gy = g.gyro.y;
    msg.datos_mov.gz = g.gyro.z;
    if (xQueueSend(colaEnvioLoRa, &msg, 0) != pdTRUE)
    {
      Serial.println("¡Cola llena! No se pudo enviar mensaje LoRa");
    }

    Serial.printf("Estado pin INT: %d\n", digitalRead(25));
    Serial.println("----- Movimiento detectado -----");
    Serial.printf("[Estado Ascensor] %s\n", estado);
    Serial.printf("Acelerómetro - X: %.3f, Y: %.3f, Z: %.3f\n", a.acceleration.x, a.acceleration.y, a.acceleration.z);
    Serial.printf("Giroscopio   - X: %.3f, Y: %.3f, Z: %.3f\n", g.gyro.x, g.gyro.y, g.gyro.z);
    Serial.println("--------------------------------");
  }
  else
  {
    Serial.println("Esperando una interrupción...");
    Serial.printf("[Estado Ascensor] %s\n", estado);
    ascensorEnMovimiento = false;
  }
}

void estadoAscensor()
{
  if (movimientoDetectado)
  {
    movimientoDetectado = false;
    ascensorEnMovimiento = true;
    estado = "EN MOVIMIENTO";
    ciclosSinMovimiento = 0;
  }

  else
  {
    ciclosSinMovimiento++;

    if (ciclosSinMovimiento >= 5 && ascensorEnMovimiento)
    {
      ascensorEnMovimiento = false;
      estado = "PARADO";
      Serial.println("[Ascensor] Ha entrado en estado PARADO");
    }
  }
}
void distanciaPuertaTask(void *pvParameters)
{
  static bool estabaMoviendose = true;
  static int medicionesRealizadas = 0;

  while (true)
  {
    // Si ha pasado de MOVIMIENTO a PARADO, reiniciamos el contador
    if (estado == "PARADO" && estabaMoviendose)
    {
      medicionesRealizadas = 0;
      estabaMoviendose = false;
    }

    // Hacer hasta 3 mediciones solo si realmente está en estado PARADO
    if (estado == "PARADO" && medicionesRealizadas < 3)
    {
      sensorPuerta.stopContinuous();
      delay(100);
      distanciaPuerta();
      medicionesRealizadas++;

      // Esperar entre mediciones (ajustable)
      vTaskDelay(1500 / portTICK_PERIOD_MS);
    }

    // Si ha vuelto a moverse, permitimos futuras mediciones cuando vuelva a parar
    if (estado == "EN MOVIMIENTO")
    {
      estabaMoviendose = true;
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void distanciaTecho(void *pvParameters)
{
  while (true)
  {
    if (estado == "PARADO" && estabaEnMovimiento)
    {
      Serial.println("Ascensor se ha detenido. Solicitando medición de techo...");
      LaserSerial.flush(); // probarlo para vaciar el buffer para evitar lecturas antiguas
      LaserSerial.println("D");
      delay(300);

      while (LaserSerial.available())
      {
        String data = LaserSerial.readStringUntil('\n');
        data.trim();
        Serial.println("Respuesta: " + data);

        int idx1 = data.indexOf(':');
        int idx2 = data.indexOf('m');

        if (idx1 != -1 && idx2 != -1)
        {
          String valorStr = data.substring(idx1 + 1, idx2);
          valorStr.trim();
          float distanciaTecho_valor = valorStr.toFloat();

          ultimaDistanciaTecho = distanciaTecho_valor; // Actualiza valor mostrado en pantalla
          actualizarDisplay();                         // Refresca la pantalla OLED

          MensajeLoRa msg;
          msg.tipo = DISTANCIA_TECHO;
          msg.distancia_techo = distanciaTecho_valor;
          if (xQueueSend(colaEnvioLoRa, &msg, 0) != pdTRUE)
          {
            Serial.println("¡Cola llena! No se pudo enviar mensaje LoRa");
          }
        }
      }
    }
    estabaEnMovimiento = ascensorEnMovimiento;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void envioLoRA(void *pvParameters)
{
  MensajeLoRa msg;
  while (true)
  {
    if (xQueueReceive(colaEnvioLoRa, &msg, portMAX_DELAY) == pdTRUE)
    {
      LoRa.beginPacket();
      switch (msg.tipo)
      {
      case DATOS_MOV:
        LoRa.printf("Ascensor: %s | ax:%.3f | ay:%.3f | az:%.3f | gx:%.3f | gy:%.3f| gz:%.3f",
                    estado,
                    msg.datos_mov.ax,
                    msg.datos_mov.ay,
                    msg.datos_mov.az,
                    msg.datos_mov.gx,
                    msg.datos_mov.gy,
                    msg.datos_mov.gz);
        break;

      case DISTANCIA_PUERTA:
        LoRa.printf("Puerta: %.3f", msg.distancia_puerta);
        break;

      case DISTANCIA_TECHO:
        LoRa.printf("Techo: %.3f", msg.distancia_techo);
        break;
      }
      LoRa.endPacket();
    }
  }
}

void setup()
{
  LaserSerial.begin(19200, SERIAL_8N1, 3, 1);

  Serial.begin(115200);
  Wire.begin(SDA, SCL);               // MPU + OLED
  WireVL53.begin(SDA_VL53, SCL_VL53); // VL53L1X

  // Definición Pin para Interrupciones
  pinMode(25, INPUT); // GPIO que usas para INT del MPU6050
  attachInterrupt(digitalPinToInterrupt(25), detectarMovimiento, RISING);

  // Reinició pantalla OLED
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    Serial.println("Error al inicializar la pantalla OLED");

  // Inicialización de sensores
  inicializarMPU();

  inicializarPuerta();

  // LoRa
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(25, 10);
  display.print("LORA SENDER");
  display.display();

  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(BAND))
    Serial.println("Error al iniciar LoRa");

  display.setCursor(0, 30);
  display.print("LoRa Inicializado OK!");
  display.display();

  // Definición de las Colas
  colaEnvioLoRa = xQueueCreate(300, sizeof(MensajeLoRa));

  encenderLaser();

  // Task
  // xTaskCreate(distanciaPuertaTask, "distanciaPuertaTask", 4096, NULL, 2, NULL);
  xTaskCreate(distanciaTecho, "distanciaTecho", 4096, NULL, 2, NULL);
  xTaskCreate(envioLoRA, "envioLoRa", 4096, NULL, 2, NULL);
}

void loop()
{
  recogerDatosAc();
  estadoAscensor();
  actualizarDisplay();

  delay(1500);
}
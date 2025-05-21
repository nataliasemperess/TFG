#include <list>
#include <vector>
#include <map>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP_Mail_Client.h>
#include <LittleFS.h>
#include <Crypto.h>
#include <ChaCha.h>
#include <base64.h>

// Configuración OLED
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Configuración LoRa
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26
#define BAND 866E6

// Tiempos
#define ALERTA_1_MIN 60000
#define ALERTA_3_MIN 30000
#define ALERTA_1_HORA 3600000
#define ALERTA_3_HORAS 10800000
#define ALERTA_12_HORAS 43200000
#define ALERTA_24_HORAS 86400000
#define ALERTA_48_HORAS 172800000
#define MQTT_MAX_PACKET_SIZE 512

unsigned long tiempoInicioParada = 0;
bool ascensorEnMovimiento = false;
bool correoEnviado = false;
bool alerta1minEnviada = false;
bool alerta3minEnviada = false;
bool alerta1horaEnviada = false;
bool alerta3horasEnviada = false;
bool alerta12horasEnviada = false;
bool alerta24horasEnviada = false;
bool alerta48horasEnviada = false;
bool alertaDesnivelEnviada = false;

String estado = "";
String puerta = "Desconocido";
String techo_formateado = "";

float ax, ay, az, gx, gy, gz;
float distancia_min_planta = 3300.0;
float distancia_max_planta = 0.0;
float valor_techo;
float distancia_max_puerta;
float distancia_min_puerta = 1000.0;
float distancia_anterior = 0.0;
float valor_puerta;

const float DISTANCIA_MINIMA_ENTRE_PLANTAS = 3.4;
const float UMBRAL_DESNIVEL = 0.6;

int planta_actual = 0;
int planta_anterior = 0;
int estado_anterior = -1;
int planta_anterior_viaje = -1;

// MQTT
const char *ssid = "NAYAR SYSTEMS";
const char *password = "nayarsystems07";
const char *mqttServer = "192.168.90.206";
const int mqttPort = 1883;

std::list<float> lista_planta;
std::vector<float> alturas_plantas_detectadas;
std::vector<int> contador_por_planta;
std::map<int, float> distanciaEsperadaPlanta = {
    {0, 19.42}, {1, 16}, {2, 11.83}, {3, 4.80}, {4, 8.20}, {5, 1.12}};

// CIFRADO
uint8_t key[32] = {
    0x01, 0x06, 0x07, 0x08, 0x02, 0x03, 0x04, 0x05,
    0x10, 0x20, 0x30, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0, 0x00,
    0x09, 0x0A, 0x0B, 0x40, 0x50, 0x60, 0x70, 0x80};

uint8_t nonce[12] = {
    0x11, 0x22, 0xA0, 0xB0, 0xC0, 0xD0,
    0x00, 0x11, 0xAA, 0xBB, 0x44, 0x66};

WiFiClient espClient;
PubSubClient client(espClient);
SMTPSession smtp;
Session_Config config;
SMTP_Message message;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

void configurar_correo();
void enviar_correo(String titulo, String contenido);
void estado_puerta(float valor_actual);
void estado_planta(float valor_actual);

void configurar_correo()
{

  config.server.host_name = "smtp.gmail.com";
  config.server.port = 465;
  config.secure.startTLS = false;
  config.login.email = "n.sempere@nayarsystems.com";
  config.login.password = "fwiz iahu rsdn fuqe";
  config.time.ntp_server = "pool.ntp.org";
  config.time.gmt_offset = 1;
  config.time.day_light_offset = 1;

  message.sender.name = "Natalia Sempere";
  message.sender.email = "n.sempere@nayarsystems.com";
  message.addRecipient("Administrador", "nataliasemperess@gmail.com");
  smtp.debug(1);
}

void enviar_correo(String titulo, String contenido)
{

  message.subject = titulo;
  message.text.content = contenido;

  if (!smtp.connect(&config))
  {
    Serial.println("Error conectando al servidor SMTP.");
    return;
  }

  if (!MailClient.sendMail(&smtp, &message))
  {
    Serial.printf("Error enviando el correo: %s\n", smtp.errorReason().c_str());
  }
  else
  {
    Serial.println("Correo enviado correctamente.");
  }

  smtp.closeSession();
}

void estado_puerta(float valor_actual)
{
  const float TOL_CERRADA = 0.02, TOL_ABIERTA = 0.02, UMBRAL_CAMBIO = 0.01;
  if (valor_actual > distancia_max_puerta)
    distancia_max_puerta = valor_actual;
  if (valor_actual < distancia_min_puerta)
    distancia_min_puerta = valor_actual;

  if (abs(valor_actual - distancia_min_puerta) <= TOL_CERRADA)
    puerta = "PUERTA CERRADA";
  else if (abs(valor_actual - distancia_max_puerta) <= TOL_ABIERTA)
    puerta = "PUERTA ABIERTA";
  else
    puerta = (valor_actual > distancia_anterior) ? "ABRIENDO" : "CERRANDO";

  distancia_anterior = valor_actual;
}

void estado_planta(float valor_actual)
{
  // Normalizamos la lectura antes de usarla
  float valor_normalizado = round(valor_actual * 10.0) / 10.0;

  if (valor_normalizado < distancia_min_planta)
    distancia_min_planta = valor_normalizado;
  if (valor_normalizado > distancia_max_planta)
    distancia_max_planta = valor_normalizado;

  bool encontrada = false;
  for (size_t i = 0; i < alturas_plantas_detectadas.size(); i++)
  {
    if (abs(valor_normalizado - alturas_plantas_detectadas[i]) < DISTANCIA_MINIMA_ENTRE_PLANTAS)
    {
      planta_actual = i;
      encontrada = true;
      break;
    }
  }

  if (!encontrada)
  {
    alturas_plantas_detectadas.push_back(valor_normalizado); // << usamos el valor redondeado
    std::sort(alturas_plantas_detectadas.begin(), alturas_plantas_detectadas.end(), std::greater<float>());

    // Aseguramos que haya un contador para cada planta
    contador_por_planta.resize(alturas_plantas_detectadas.size(), 0);

    for (size_t i = 0; i < alturas_plantas_detectadas.size(); i++)
    {
      if (abs(valor_normalizado - alturas_plantas_detectadas[i]) < DISTANCIA_MINIMA_ENTRE_PLANTAS)
      {
        planta_actual = i;
        break;
      }
    }
  }

  // Solo aumentar contador si ha cambiado de planta
  if (planta_actual != planta_anterior)
  {
    if (planta_actual >= 0 && planta_actual < contador_por_planta.size())
    {
      contador_por_planta[planta_actual]++;
      Serial.printf("Contador planta %d = %d\n", planta_actual, contador_por_planta[planta_actual]);

      char topic[50];
      sprintf(topic, "ascensor/contador/planta_%d", planta_actual);

      char msg[200];
      sprintf(msg, "Planta %d - Paradas : %d", planta_actual, contador_por_planta[planta_actual]);

      client.publish(topic, msg);
    }
    planta_anterior = planta_actual;
  }

  /*
  char msg[200];
  sprintf(msg, "Planta actual: %d - Distancia: %.2f", planta_actual, valor_normalizado); // 👈 mostramos el valor limpio
  client.publish("ascensor/estado_planta", msg);*/
}

void setup()
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println(WiFi.localIP());

  while (WiFi.status() != WL_CONNECTED)
    delay(500);
  client.setServer(mqttServer, mqttPort);
  while (!client.connected())
    client.connect("ESP32Client");

  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("LoRa Receiver");
  display.display();

  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
  LoRa.begin(BAND);

  if (!LittleFS.begin())
  {
    Serial.println("Error al montar LittleFS");
    while (true)
      ; // detiene el programa si falla
  }

  File f = LittleFS.open("/tze.txt", "w");
  if (f)
  {
    f.println("log");
    f.close();
  }
  else
  {
    Serial.println("Error creando /tze.txt");
  }

  configurar_correo();
}

void loop()
{
  client.loop();
  int packetSize = LoRa.parsePacket();
  if (packetSize)
  {
    String mensaje = LoRa.readStringUntil('\n');
    mensaje.trim();
    Serial.print("Recibido por LoRa: ");
    Serial.println(mensaje);

    client.loop();

    if (mensaje.startsWith("Ascensor:"))
    {
      int estado_end = mensaje.indexOf('|');
      String estadoStr = mensaje.substring(10, estado_end);
      estadoStr.trim();

      if (estadoStr == "EN MOVIMIENTO")
      {
        estado = "M";
        ascensorEnMovimiento = true;
        client.publish("ascensor/movimiento", "MOVING");
      }
      else if (estadoStr == "PARADO")
      {
        estado = "P";
        ascensorEnMovimiento = false;
        client.publish("ascensor/movimiento", "STOPPED");
      }

      // ax
      int ax_start = mensaje.indexOf("ax:") + 3;
      int ax_end = mensaje.indexOf(',', ax_start);
      float ax_recibido = mensaje.substring(ax_start, ax_end).toFloat();
      ax = ax_recibido;

      // ay
      int ay_start = mensaje.indexOf("ay:") + 3;
      int ay_end = mensaje.indexOf(',', ay_start);
      float ay_recibido = mensaje.substring(ay_start, ay_end).toFloat();
      ay = ay_recibido;

      // az
      int az_start = mensaje.indexOf("az:") + 3;
      int az_end = mensaje.indexOf(',', az_start);
      float az_recibido = mensaje.substring(az_start, az_end).toFloat();
      az = az_recibido;

      // gx
      int gx_start = mensaje.indexOf("gx:") + 3;
      int gx_end = mensaje.indexOf(',', gx_start);
      float gx_recibido = mensaje.substring(gx_start, gx_end).toFloat();
      gx = gx_recibido;

      // gy
      int gy_start = mensaje.indexOf("gy:") + 3;
      int gy_end = mensaje.indexOf(',', gy_start);
      float gy_recibido = mensaje.substring(gy_start, gy_end).toFloat();
      gy = gy_recibido;

      // gz
      int gz_start = mensaje.indexOf("gz:") + 3;
      int gz_end = mensaje.indexOf(',', gz_start);
      float gz_recibido = mensaje.substring(gz_start, gz_end).toFloat();
      gz = gz_recibido;

      Serial.printf("ax: %.3f - ay: %.3f - az: %.3f - gx: %.3f - gy: %.3f - gz: %.3f",
                    ax, ay, az, gx, gy, gz);
    }

    else if (mensaje.startsWith("Puerta:"))
    {
      float valor = mensaje.substring(7).toFloat();
      valor_puerta = valor;
      estado_puerta(valor);

      Serial.println();
      Serial.print("Valor de la puerta: ");
      Serial.println(valor);
      Serial.print("Estado puerta: ");
      Serial.println(puerta);
    }

    else if (mensaje.startsWith("Techo:"))
    {
      float valor = mensaje.substring(6).toFloat();
      valor_techo = valor;

      char techoMsg[20];
      snprintf(techoMsg, sizeof(techoMsg), "%.2f", valor_techo);
      client.publish("ascensor/techo", techoMsg);

      lista_planta.push_back(valor);
      if (lista_planta.size() > 100)
        lista_planta.pop_front();

      estado_planta(valor);

      char buffer[30];
      snprintf(buffer, sizeof(buffer), "Techo: %.2f", valor);
      techo_formateado = String(buffer);

      char plantaMsg[10];
      sprintf(plantaMsg, "%d", planta_actual);
      client.publish("ascensor/planta", plantaMsg);
    }

    // Publicar estado de la puerta simplificado
    if (puerta == "PUERTA ABIERTA")
    {
      client.publish("ascensor/puerta", "OPEN");
    }
    else if (puerta == "PUERTA CERRADA" || puerta == "CERRANDO" || puerta == "ABRIENDO")
    {
      client.publish("ascensor/puerta", "CLOSED");
    }

    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("Estado: %s", estado.c_str());
    display.setCursor(0, 20);
    display.printf("Puerta: %s", puerta.c_str());
    display.setCursor(0, 35);
    display.print(techo_formateado);
    display.setCursor(0, 50);
    display.printf("Planta: %d", planta_actual);
    display.display();

    char msg[400];
    sprintf(msg, "%s -  Puerta: %s (%.2f m) - Piso: %d (%.2f m) - az: %.2f - gz: %.2f ",
            estado.c_str(), puerta.c_str(), valor_puerta, planta_actual, valor_techo, az, gz);

    int mensajeLen = strlen(msg);   // Leemos el valor real de tamaño del buffer
    uint8_t ciphertext[mensajeLen]; // Ciframos el mensaje con chacha20

    ChaCha chacha;
    chacha.setKey(key, 32);
    chacha.setIV(nonce, 12);
    chacha.encrypt(ciphertext, (const uint8_t *)msg, mensajeLen);

    const int totalLen = 12 + mensajeLen;
    uint8_t paquete[totalLen];

    memcpy(paquete, nonce, 12);                   // primero el nonce
    memcpy(paquete + 12, ciphertext, mensajeLen); // luego el mensaje cifrado

    String mensajeBase64 = base64::encode(paquete, totalLen);

    client.publish("ascensor/datos", mensajeBase64.c_str());

    // Solo detectar viajes si el ascensor se ha detenido
    if (estado == "P")
    {
      // Comprobar si venía de movimiento y cambió de planta
      if (estado_anterior == 1 && planta_actual != planta_anterior_viaje)
      {
        Serial.println(" VIAJE DETECTADO");
        client.publish("ascensor/viaje", "1"); // Puedes usar "1" o enviar más datos
        planta_anterior_viaje = planta_actual;
      }
      estado_anterior = 0;

      if (tiempoInicioParada == 0)
      {
        tiempoInicioParada = millis();

        correoEnviado = false;
      }
      Serial.println(tiempoInicioParada);

      if (!alertaDesnivelEnviada)
      {
        auto it = distanciaEsperadaPlanta.find(planta_actual);
        if (it != distanciaEsperadaPlanta.end())
        {
          float esperada = it->second;
          float desviacion = abs(valor_techo - esperada);

          if (desviacion > UMBRAL_DESNIVEL)
          {
            char contenido[300];
            snprintf(contenido, sizeof(contenido),
                     "Se ha detectado un desnivel de la cabina del ascensor"
                     "Alerta de desnivel:\n"
                     "Planta detectada: %d\n"
                     "Distancia esperada: %.2f m\n"
                     "Distancia medida: %.2f m\n"
                     "Diferencia: %.2f m",
                     planta_actual, esperada, valor_techo, desviacion);

            enviar_correo("ALERTA : Desnivel detectado en planta", contenido);

            alertaDesnivelEnviada = true;
          }
        }
      }
    }

    else if (estado == "M")
    {
      estado_anterior = 1;
      tiempoInicioParada = 0;
      correoEnviado = false;
      alertaDesnivelEnviada = false;
    }
  }

  // Enviar correo si ha estado parado más de 24h

  if (tiempoInicioParada > 0 && !correoEnviado && millis() - tiempoInicioParada >= ALERTA_3_MIN)
  {
    enviar_correo("ALERTA: Inactividad de 3 minutos",
                  "El ascensor ha estado parado más de 3 minutos. Esto puede indicar un fallo. Revíselo cuanto antes. ");
    correoEnviado = true;
  }

  if (tiempoInicioParada > 0 && !correoEnviado && millis() - tiempoInicioParada >= ALERTA_3_HORAS)
  {
    enviar_correo("ALERTA: Inactividad de 3 horas",
                  "El ascensor ha estado parado más de 3 horas. Esto puede indicar un fallo. Revíselo cuanto antes. ");
    correoEnviado = true;
  }

  if (tiempoInicioParada > 0 && !correoEnviado && millis() - tiempoInicioParada >= ALERTA_12_HORAS)
  {
    enviar_correo("ALERTA: Inactividad de 12 horas",
                  "El ascensor ha estado parado más de 12 horas. Esto puede indicar un fallo. Revíselo cuanto antes. ");
    correoEnviado = true;
  }
  if (tiempoInicioParada > 0 && !correoEnviado && millis() - tiempoInicioParada >= ALERTA_24_HORAS)
  {
    enviar_correo("ALERTA: Inactividad de 24 horas",
                  "El ascensor ha estado parado más de 24 horas. Esto puede indicar un fallo. Revíselo cuanto antes. ");
    correoEnviado = true;
  }
  if (tiempoInicioParada > 0 && !correoEnviado && millis() - tiempoInicioParada >= ALERTA_48_HORAS)
  {
    enviar_correo("ALERTA: Inactividad de 48 horas",
                  "El ascensor ha estado parado más de 48 horas. Esto puede indicar un fallo. Revíselo cuanto antes. ");
    correoEnviado = true;
  }
}

// Wio LTE/M1 SORACOM beamを使ってみる
// MQTTでAWS IoTにアクセス
// 温湿度情報をMQTTでイベントがAWS IoTに送信されます。

#include <WioCellLibforArduino.h>
#include <WioCellularClient.h>
#include <PubSubClient.h>		// https://github.com/SeeedJP/pubsubclient
#include <stdio.h>

#define APN               "soracom.io"
#define USERNAME          "sora"
#define PASSWORD          "sora"

#define MQTT_SERVER_HOST  "beam.soracom.io"
#define MQTT_SERVER_PORT  (1883)

#define ID                "wioltem"
#define OUT_TOPIC         "topic/Test"
#define IN_TOPIC          "topic/Test/#"

#define INTERVAL          (10000)

#define DHT11 WIO_D20

boolean setup_flag = false;

WioCellular Wio;
WioCellularClient WioClient(&Wio);
PubSubClient MqttClient;

void setup() {
  // デバッグ用シリアル初期化
  SerialUSB.begin(115200);
  SerialUSB.println("");
  SerialUSB.println("--- START ---------------------------------------------------");

  SerialUSB.println("### I/O Initialize.");
  Wio.Init();

  SerialUSB.println("### Power supply ON.");
  Wio.PowerSupplyCellular(true);
  delay(1000);

  SerialUSB.println("### Turn on or reset.");
  if (!Wio.TurnOnOrReset()) {
    SerialUSB.println("### ERROR! ###1");
    return;
  }

  #ifdef ARDUINO_WIO_LTE_M1NB1_BG96
    SerialUSB.println("### SetSelectNetwork MANUAL_IMSI ###");
    Wio.SetSelectNetwork(WioCellular::SELECT_NETWORK_MODE_MANUAL_IMSI);
  #endif

  #ifdef ARDUINO_WIO_3G
    SerialUSB.println("### SetSelectNetwork AUTOMATIC ###");
    Wio.SetSelectNetwork(WioCellular::SELECT_NETWORK_MODE_AUTOMATIC);
  #endif

  SerialUSB.println("### Connecting to \"" APN "\".");

  if (!Wio.Activate(APN, USERNAME, PASSWORD)) {
    SerialUSB.println("### ERROR! ###2");
    return;
  }

  // MQTTサーバにアクセスする
  SerialUSB.println("### Connecting to MQTT server \"" MQTT_SERVER_HOST "\"");
  MqttClient.setServer(MQTT_SERVER_HOST, MQTT_SERVER_PORT);
  MqttClient.setCallback(callback);
  MqttClient.setClient(WioClient);

  if (!MqttClient.connect(ID)) {
    SerialUSB.println("### ERROR! ###3");
    return;
  }

  // 受信するトピックを要求する
  MqttClient.subscribe(IN_TOPIC);

  // GROVE端子へ電源供給を行う(D38以外向け）
  Wio.PowerSupplyGrove(true);

  TemperatureAndHumidityBegin(DHT11);

  SerialUSB.println("### Setup completed.");

  setup_flag = true;
}

void loop() {
  if(setup_flag == false)
  {
    delay(5000);
    setup();
    return;
  }

  float temp;
  float humi;

  if (!TemperatureAndHumidityRead(&temp, &humi)) {
    SerialUSB.println("ERROR!");
    goto err;
  }

  SerialUSB.print("Current humidity = ");
  SerialUSB.print(humi);
  SerialUSB.print("%  ");
  SerialUSB.print("temperature = ");
  SerialUSB.print(temp);
  SerialUSB.println("C");

    char data[100];
    sprintf(data, "{\"temp\": %2.2f, \"humi\": %2.2f}", temp, humi);
    SerialUSB.print("Publish:");
    SerialUSB.print(data);
    SerialUSB.println("");
    MqttClient.publish(OUT_TOPIC, data);

err:
  // 休憩（ただし、受信できるように、delay関数は使わないようにする）
  unsigned long next = millis();
  while (millis() < next + INTERVAL)
  {
    MqttClient.loop();
  }
}

// IN_TOPICを受信した時に呼ばれるコールバック関数
void callback(char* topic, byte* payload, unsigned int length) {
  // 受信したトピックを表示する
  SerialUSB.print("(");
  SerialUSB.print(topic);
  SerialUSB.print(")Subscribe:");
  for (int i = 0; i < (int)length; i++) {
    SerialUSB.print((char)payload[i]);
  }
  SerialUSB.println("");
}

int TemperatureAndHumidityPin;

void TemperatureAndHumidityBegin(int pin) {
  TemperatureAndHumidityPin = pin;
  DHT11Init(TemperatureAndHumidityPin);
}

bool TemperatureAndHumidityRead(float* temperature, float* humidity) {
  byte data[5];

  DHT11Start(TemperatureAndHumidityPin);
  for (int i = 0; i < 5; i++) data[i] = DHT11ReadByte(TemperatureAndHumidityPin);
  DHT11Finish(TemperatureAndHumidityPin);

  if(!DHT11Check(data, sizeof (data))) return false;
  if (data[1] >= 10) return false;
  if (data[3] >= 10) return false;

  *humidity = (float)data[0] + (float)data[1] / 10.0f;
  *temperature = (float)data[2] + (float)data[3] / 10.0f;

  return true;
}

void DHT11Init(int pin) {
  digitalWrite(pin, HIGH);
  pinMode(pin, OUTPUT);
}

void DHT11Start(int pin) {
  // Host the start of signal
  digitalWrite(pin, LOW);
  delay(18);

  // Pulled up to wait for
  pinMode(pin, INPUT);
  while (!digitalRead(pin)) ;

  // Response signal
  while (digitalRead(pin)) ;

  // Pulled ready to output
  while (!digitalRead(pin)) ;
}

byte DHT11ReadByte(int pin) {
  byte data = 0;

  for (int i = 0; i < 8; i++) {
    while (digitalRead(pin)) ;

    while (!digitalRead(pin)) ;
    unsigned long start = micros();

    while (digitalRead(pin)) ;
    unsigned long finish = micros();

    if ((unsigned long)(finish - start) > 50) data |= 1 << (7 - i);
  }

  return data;
}

void DHT11Finish(int pin) {
  // Releases the bus
  while (!digitalRead(pin)) ;
  digitalWrite(pin, HIGH);
  pinMode(pin, OUTPUT);
}

bool DHT11Check(const byte* data, int dataSize) {
  if (dataSize != 5) return false;

  byte sum = 0;
  for (int i = 0; i < dataSize - 1; i++) {
    sum += data[i];
  }

  return data[dataSize - 1] == sum;
}

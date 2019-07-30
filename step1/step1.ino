// Soracom管理画面、 「Sim 管理」 -> Sim選択 ->「SORACOM Harvest 設定」より設定後
// Soracom管理画面、 「Sim 管理」 -> Sim選択 -> 「操作」-> 「データを確認」より送信データを確認することができます
#include <WioCellLibforArduino.h>

#define DHT11 WIO_D20

#define INTERVAL        (10000)
#define RECEIVE_TIMEOUT (10000)

WioCellular Wio;
int counter;

void setup() {
  delay(200);

  SerialUSB.begin(115200);
  SerialUSB.println("");
  SerialUSB.println("--- START ---");

  SerialUSB.println("--- I/O Initialize. ---");
  Wio.Init();

  SerialUSB.println("--- Power supply ON. ---");
  Wio.PowerSupplyCellular(true);
  // GROVE端子へ電源供給を行う(D38以外向け）
  Wio.PowerSupplyGrove(true);
  delay(500);

  TemperatureAndHumidityBegin(DHT11);

  SerialUSB.println("--- Turn on or reset. ---");
  if (!Wio.TurnOnOrReset()) {
    SerialUSB.println("### ERROR! ###");
    return;
  }

  SerialUSB.println("--- Connecting to \"soracom.io\". ---");
#ifdef ARDUINO_WIO_LTE_M1NB1_BG96
  Wio.SetSelectNetwork(WioCellular::SELECT_NETWORK_MODE_MANUAL_IMSI);
#endif
  if (!Wio.Activate("soracom.io", "sora", "sora")) {
    SerialUSB.println("### ERROR! ###");
    return;
  }

  SerialUSB.println("--- Setup completed. ---");
}

void loop() {
  float temp;
  float humi;
  char data[100];

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

  // create payload
  sprintf(data, "{\"temp\": %2.2f, \"humi\": %2.2f}", temp, humi);

  SerialUSB.println("--- Open socket. ---");

  // Open harvest connection
  int connectId;
  connectId = Wio.SocketOpen("harvest.soracom.io", 8514, WIO_UDP);
  if (connectId < 0) {
    SerialUSB.println("### ERROR! ###");
    goto err;
  }

  // Send data.
  SerialUSB.println("--- Send data. ---");
  SerialUSB.print("Send:");
  SerialUSB.println(data);
  if (!Wio.SocketSend(connectId, data)) {
    SerialUSB.println("### ERROR! ###");
    goto err_close;
  }

  // Receive data.
  SerialUSB.println("-- Receive data. ---");
  int length;
  length = Wio.SocketReceive(connectId, data, sizeof (data), RECEIVE_TIMEOUT);
  if (length < 0) {
    SerialUSB.println("### ERROR! ###");
    goto err_close;
  }

  if (length == 0) {
    SerialUSB.println("### RECEIVE TIMEOUT! ###");
    goto err_close;
  }

  SerialUSB.print("Receive:");
  SerialUSB.print(data);
  SerialUSB.println("");

err_close:
  SerialUSB.println("### Close.");
  if (!Wio.SocketClose(connectId)) {
    SerialUSB.println("### ERROR! ###");
    goto err;
  }

err:
  delay(INTERVAL);
  counter++;
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
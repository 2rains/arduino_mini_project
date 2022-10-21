#include <DHT.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <Stepper.h>

#define DHT_PIN 2       // 습도센서
#define DHT_TYPE DHT11  // 습도센서
#define DUST_PIN A0     // 미세먼지센서
#define DUST_LED_PIN 12
#define DUST_SAMPLING 280
#define DUST_WAITING 40
#define DUST_STOPTIME 9680

#define SS_PIN 9   // SDA - RFID
#define RST_PIN 8  // RST - RFID

#define no_dust 0.35                     // 초기값
const int stepvalue = 2048;              //1024는 180도 회전 / 2048은 1바퀴, 4096은 2바퀴
Stepper stepper(stepvalue, 3, 5, 4, 6);  // 스텝모터 객체 핀 번호
int state = 0, now_state = 0;            // 창문이 닫혀있으면 0, 열려있으면 1
// boolean setdir = LOW;                    // Set Direction


int Relaypin = 8;  // 릴레이 핀 3번

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

DHT dht(DHT_PIN, DHT_TYPE);
long dhtTimer = -60000;

float dustValue = 0;
float dustDensityung = 0;
long dustTimer = -60000;



char serialBuffer[128];
int serialBufferIndex = 0;

bool ledStatus = false;
long ledTimer = 0;

long time;
long DUST_time;


void setup() {
  stepper.setSpeed(5);  // 모터 속도 / 분당 회전수

  pinMode(Relaypin, OUTPUT);

  time = millis();
  DUST_time = time;

  Serial.begin(9600);
  while (!Serial) {
    ;
  }

  // Start Init LED PIN
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Start Init Serial Buffer
  memset(serialBuffer, 0, 128);

  // Start Init DHT
  dht.begin();

  pinMode(DUST_LED_PIN, OUTPUT);

  // Start Init RFID Module
  SPI.begin();
  rfid.PCD_Init();
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] == 0xFF;
  }
}

void loop() {

  checkSerial();
  checkDHT();
  checkDust();
  checkLEDStatus();
}

void checkSerial() {
  if (Serial.available()) {
    serialBuffer[serialBufferIndex] = (char)Serial.read();

    if (serialBuffer[serialBufferIndex] == '\n') {
      handleSerialData();
      serialBufferIndex = 0;
    } else {
      serialBufferIndex++;
    }
  }
}

void handleSerialData() {

  DynamicJsonDocument doc(128);
  deserializeJson(doc, serialBuffer);

  if (doc["type"] == "rfid") {
    if (doc["result"] == 1) {
      digitalWrite(LED_BUILTIN, HIGH);
      ledStatus = true;
      ledTimer = millis();
    } else {
    }
  }
}

void checkLEDStatus() {
  if (ledStatus == true) {
    if (millis() > ledTimer + 5000) {
      // digitalWrite(LED_BUILTIN, LOW);
      ledStatus = false;
      ledTimer = 0;
    }
  }
}

void checkDHT() {
  if (millis() > dhtTimer + 60000) {
    dhtTimer = millis();
    float humidity = dht.readHumidity();

    DynamicJsonDocument doc(256);
    doc["messageType"] = "readHumidity";
    doc["humidity"] = humidity;
    serializeJson(doc, Serial);
    Serial.println();
    delay(1);

    if (humidity > 40) {
      digitalWrite(Relaypin, HIGH);
      delay(100);
    } else if (humidity < 40) {
      digitalWrite(Relaypin, LOW);
      delay(100);
    }
  }
}

void checkDust() {  // 미세먼지센서 값 받아온 후 2초 뒤에 다시 실행시킴. 그래야 쓰레기값 안 나옴!
  time = millis();  //아두이노 켜진시간 확인
  if (DUST_time + 2000 <= time) {
    DUST_time = time;

    digitalWrite(DUST_LED_PIN, LOW);
    delayMicroseconds(280);
    dustValue = analogRead(DUST_PIN);
    delayMicroseconds(40);
    digitalWrite(DUST_LED_PIN, HIGH);
    delayMicroseconds(9680);
    dustDensityung = (0.17 * (dustValue * (5.0 / 1024)) - 0.1) * 1000;

    Serial.println("Dust Density [ug/m3]: " + String(dustDensityung));

    if (dustDensityung < 100) {  // 창문이 닫힘 0, 열림 1
      state = 1;                 // 100이하면 state에 1대입
    }

    if (now_state == 1) {  // 창문이 열려 있는 상태에서 미세먼지가 높으면 state 0대입
      if (dustDensityung > 100) {
        state = 0;  // 5초가 측정한 데이터가 기준에 맞으면 state에 0대입
      }
    }

    if (state != now_state) {  // 상태가 바뀌었을 때
      if (state == 1) {        // 창문을 열기 위해서 시계 방향으로 회전
        stepper.step(stepvalue);
        Serial.println("state = 1");  // state = 열림
        Serial.println("open");
      } else {  // 창문을 닫기 위해서 반시계 방향으로 회전
        stepper.step(-stepvalue);
        Serial.println("close");
        Serial.println("state = 0");  // state =  닫힘
      }
      delay(1000);
    }
    now_state = state;
    delay(500);
  }
}
#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <SPI.h>
#include <DHT.h>

#define PINMQ2 6
#define LEDMQ2 5

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define pinPM25 7
#define pinPM1 8
#define sampleTime 5000  // mSec   -> 5..30 sec datasheet recommends


#define SEALEVELPRESSURE_HPA (1013.25)

typedef struct {
  float humidade;
  float temperatura;
  float pressureo;

} Sensor;

DHT dht(7,DHT22);
// Adafruit_BME280 bme;
// Adafruit_BME280 bme(BME_CS); // hardware SPI
Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

void printBME280() {
  Serial.print(bme.readTemperature());
  Serial.println(" *C");

  Serial.print("Pressure = ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.println(" hPa");
  
  Serial.print("Approx. Altitude = ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");
  
  Serial.print("Humidity = ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");
}

float calc_low_ratio(float lowPulse) {
  return lowPulse / sampleTime * 100.0;  // low ratio in %
}

float calc_c_mgm3(float lowPulse) {
  float r = calc_low_ratio(lowPulse);
  float c_mgm3 = 0.00258425 * pow(r, 2) + 0.0858521 * r - 0.01345549;
  return max(0, c_mgm3);
}

float calc_c_pcs283ml(float lowPulse) {
  float r = calc_low_ratio(lowPulse);
  float c_pcs283ml =  625 * r;
  return min(c_pcs283ml, 12500);
}

void print_DSM501() {
  static unsigned long t_start = millis();
  static float lowPM25, lowPM1 = 0;

  lowPM25 += pulseIn(pinPM25, LOW) / 1000.0;
  lowPM1 += pulseIn(pinPM1, LOW) / 1000.0;

  if ((millis() - t_start) >= sampleTime) {
    Serial.print("low_%  PM25    : ");
    Serial.println(calc_low_ratio(lowPM25));
    Serial.print("c_mgm3 PM25    : ");
    Serial.println(calc_c_mgm3(lowPM25));
    Serial.print("c_pcs283ml PM25: ");
    Serial.println(calc_c_pcs283ml(lowPM25));
    Serial.print("low_%  PM1: ");
    Serial.println(calc_low_ratio(lowPM1));
    Serial.print("c_mgm3 PM1    : ");
    Serial.println(calc_c_mgm3(lowPM1));
    Serial.print("c_pcs283ml PM1: ");
    Serial.println(calc_c_pcs283ml(lowPM1));
    Serial.println();
    lowPM25 = 0;
    lowPM1 = 0;
    t_start = millis();
  }
}

void LDR_READ() {
  int analogValue = analogRead(A0);

  Serial.print("Analog Read = ");
  Serial.print(analogValue);

  if(analogValue < 100) Serial.println(" - Very Bright\n");
  else if(analogValue < 200) Serial.println(" - Bright\n");
  else if(analogValue < 500) Serial.println(" - Light\n");
  else if(analogValue < 800) Serial.println(" - Dim\n");
  else Serial.println(" - Dark");
}

void setup() {
  Serial.begin(9600);

  //DSM501
  pinMode(pinPM25, INPUT);
  pinMode(pinPM1, INPUT);
  Serial.println("Warming up DSM501");
  delay(60000);  // 1 minute until become stable

  //MQ2 SENSOR
  pinMode(PINMQ2, INPUT);
  pinMode(LEDMQ2, OUTPUT);
  digitalWrite(LEDMQ2, LOW);

  //BME280 SENSOR
  Serial.println("BME280 TEST");
  bool status;
  status = bme.begin();
  if(!status) {
    Serial.println("BME280 nao funfou");
  }
  else Serial.println("TUDO SHOW");
  
  //Start SPI
  digitalWrite(SS, HIGH);
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV8);
  
}

void loop() {
  digitalWrite(SS, LOW);
  delay(2000);
  Sensor sensores;
  sensores.humidade = dht.readHumidity();
  sensores.temperatura = dht.readTemperature();

  uint8_t* bytes = (uint8_t*)&sensores;
  size_t bytes_len = sizeof(sensores);

  if(isnan(sensores.humidade) || isnan(sensores.temperatura)) {
    Serial.println("Failed DHT");
    return;
  }

  //Serial.print("SIZEOF STRUCT ");
  //Serial.println(sizeof(Sensor));
  //Serial.print("BYTES:");
  //Serial.println(bytes_len);
  //Serial.print("Humidity:");
  //Serial.println(sensores.humidade);
  //Serial.print("Temperature(Celsius):");
  //Serial.println(sensores.temperatura);

  Serial.println("TESTE: ");
  for (int i = 0; i < bytes_len; i++) Serial.print(bytes[i], 6), Serial.print(" ");
  Serial.println("\n");

  char msg[256] = "Nao sabo";
  size_t length = strlen(msg);
  SPI.transfer(msg, length);
  //SPI.transfer(bytes, bytes_len); //(bytes, bytes_len);
  digitalWrite(SS,HIGH);
  
  //MQ2 SENSOR
  if(digitalRead(PINMQ2) == LOW) {
    digitalWrite(LEDMQ2, HIGH);
  }
  else digitalWrite(LEDMQ2, LOW);

  Serial.print("======================\n");
  print_DSM501();
  printBME280();
  LDR_READ();

  delay(3000);
}

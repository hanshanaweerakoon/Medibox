#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

unsigned long lastLDRReading = 0;
const int DEFAULT_SAMPLE_INTERVAL = 5000;  
const int DEFAULT_SEND_INTERVAL = 120000;  
int sampleInterval = DEFAULT_SAMPLE_INTERVAL;
int sendInterval = DEFAULT_SEND_INTERVAL;
unsigned long lastSendTime = 0;
unsigned long lastServoUpdate = 0;
const unsigned long SERVO_UPDATE_INTERVAL = 10000; 

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor;
const String dayNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const String monthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
                             "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

#define BUZZER 5
#define SERVO_PIN 13  
#define DHTPIN 12   
#define LDR_PIN 32   

float ldrReadings[24];  
int readingCount = 0;
float currentAvgLight = 0;
char tempAr[8];  

float theta_offset = 30.0; 
float control_factor = 0.75;  
float T_med = 30.0;        
float lastTheta = theta_offset;  
Servo windowServo;         

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
WiFiClient espClient;
PubSubClient mqttClient(espClient);

int days = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;
unsigned long epochTime = 0;
unsigned long timeNow = 0;
unsigned long timeLast = 0;
bool isScheduledON = false;
unsigned long scheduledOnTime;

void update_time() {
  timeClient.update();
  hours = timeClient.getHours();
  minutes = timeClient.getMinutes();
  seconds = timeClient.getSeconds();
  days = timeClient.getDay();
  epochTime = timeClient.getEpochTime();
}

String getFormattedDate() {
  time_t rawtime = epochTime;
  struct tm * ti;
  ti = localtime(&rawtime);
  int year = ti->tm_year + 1900;
  int month = ti->tm_mon + 1;
  int day = ti->tm_mday;
  return String(monthNames[month-1]) + " " + String(day);
}

void print_line(String text, int column, int row, int text_size) {
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row);
  display.println(text);
}

void print_time_now() {
  display.clearDisplay();
  String dayDateStr = dayNames[days] + " " + getFormattedDate();
  print_line(dayDateStr, 0, 0, 1);
  String timeStr = String(hours < 10 ? "0" : "") + String(hours) + ":" + 
                  String(minutes < 10 ? "0" : "") + String(minutes) + ":" + 
                  String(seconds < 10 ? "0" : "") + String(seconds);
  print_line(timeStr, 10, 15, 2);
  String envStr = "L:" + String(currentAvgLight, 2) + " T:" + String(tempAr);
  print_line(envStr, 0, 50, 1);
  display.display();
}

void update_time_with_check_alarm() {
  static int lastSecond = -1;
  update_time();
  if (seconds != lastSecond) {
    lastSecond = seconds;
    print_time_now();
  }
}

void updateTemperature() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  float T = data.temperature;
  
  if (isnan(T)) {
    strcpy(tempAr, "NaN");
    return;
  }
  if (T < -40 || T > 80) {
    strcpy(tempAr, "ERR");
    return;
  }
  
  snprintf(tempAr, sizeof(tempAr), "%.2f", T);
}

void setupWifi() {
  WiFi.begin("Wokwi-GUEST", "");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    display.clearDisplay();
    print_line("Connecting to WIFI", 0, 0, 2);
    display.display();
  }
}

void buzzerOn(bool on) {
  if (on) {
    tone(BUZZER, 256);
  } else {
    noTone(BUZZER);
  }
}

void controlServo() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  float T = data.temperature;
  
  if (isnan(T) || T < -40 || T > 80) {
    return;
  }
  if (isnan(currentAvgLight) || currentAvgLight < 0 || currentAvgLight > 1) {
    return;
  }
  if (control_factor < 0 || T_med <= 0) {
    return;
  }
  
  float factor = log(1.0 + abs(T - T_med) / T_med);
  float theta = theta_offset + (180.0 - theta_offset) * currentAvgLight * control_factor * factor;
  
  theta = constrain(theta, theta_offset, 180.0);
  
  if (abs(theta - lastTheta) > 0.5) {
    Serial.printf("[Servo] Debug: T=%.1f, T_med=%.1f, I=%.3f, γ=%.2f, factor=%.3f\n", 
                  T, T_med, currentAvgLight, control_factor, factor);
    windowServo.write(theta);\
    lastTheta = theta;
    
    char angleStr[8];
    dtostrf(theta, 1, 1, angleStr);
    bool published = mqttClient.publish("ADMIN-SERVO-ANGLE", angleStr);
    Serial.printf("[MQTT] Sent [ADMIN-SERVO-ANGLE]: %s (%s)\n", 
                  angleStr, published ? "Success" : "Failed");
    Serial.printf("[Servo] Angle changed: %.1f°\n", theta);
  }
}

void receiveCallback(char* topic, byte* payload, unsigned int length) {
  char payloadStr[length + 1];
  strncpy(payloadStr, (char*)payload, length);
  payloadStr[length] = '\0';
  
  if (strcmp(topic, "ADMIN-MAIN-ON-OFF") == 0) {
    Serial.printf("[MQTT] Received [%s]: %s\n", topic, payloadStr);
    buzzerOn(payloadStr[0] == '1');
  }
  else if (strcmp(topic, "ADMIN-SCH-ON") == 0) {
    Serial.printf("[MQTT] Received [%s]: %s\n", topic, payloadStr);
    if (payloadStr[0] == 'N') {
      isScheduledON = false;
    }
    else {
      uint64_t millisTimestamp = strtoull(payloadStr, NULL, 10);
      if (millisTimestamp == 0) {
        return;
      }
      scheduledOnTime = millisTimestamp / 1000;
      isScheduledON = true;
    }
  }
  else if (strcmp(topic, "ADMIN-SAMPLE-INTERVAL") == 0) {
    Serial.printf("[MQTT] Received [%s]: %s\n", topic, payloadStr);
    sampleInterval = atoi(payloadStr) * 1000;
  }
  else if (strcmp(topic, "ADMIN-SEND-INTERVAL") == 0) {
    Serial.printf("[MQTT] Received [%s]: %s\n", topic, payloadStr);
    sendInterval = atoi(payloadStr) * 1000;
  }
  else if (strcmp(topic, "ADMIN-THETA-OFFSET") == 0) {
    Serial.printf("[MQTT] Received [%s]: %s\n", topic, payloadStr);
    theta_offset = atof(payloadStr);
  }
  else if (strcmp(topic, "ADMIN-GAMMA") == 0) {
    Serial.printf("[MQTT] Received [%s]: %s\n", topic, payloadStr);
    control_factor = atof(payloadStr);
  }
  else if (strcmp(topic, "ADMIN-T-MED") == 0) {
    Serial.printf("[MQTT] Received [%s]: %s\n", topic, payloadStr);
    T_med = atof(payloadStr);
  }
}

void setupMqtt() {
  mqttClient.setServer("test.mosquitto.org", 1883);
  mqttClient.setCallback(receiveCallback);
}

void connectToBroker() {
  while (!mqttClient.connected()) {
    Serial.print("[MQTT] Connecting...");
    String clientId = "ESP32-" + String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("Connected");
      mqttClient.subscribe("ADMIN-MAIN-ON-OFF");
      mqttClient.subscribe("ADMIN-SCH-ON");
      mqttClient.subscribe("ADMIN-SAMPLE-INTERVAL");
      mqttClient.subscribe("ADMIN-SEND-INTERVAL");
      mqttClient.subscribe("ADMIN-THETA-OFFSET");
      mqttClient.subscribe("ADMIN-GAMMA");
      mqttClient.subscribe("ADMIN-T-MED");
    } else {
      Serial.printf("Failed, rc=%d, retrying in 5s\n", mqttClient.state());
      delay(5000);
    }
  }
}

unsigned long getTime() {
  timeClient.update();
  return timeClient.getEpochTime();
}

void checkSchedule() {
  if (isScheduledON) {
    unsigned long currentTime = getTime();
    if (currentTime > scheduledOnTime) {
      buzzerOn(true);
      isScheduledON = false;
    }
  }
}

float readLDR() {
  const int samples = 5;
  int sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(LDR_PIN);
    delay(2);
  }
  float normalized = (sum / (float)samples) / 4095.0;
  return constrain(normalized, 0, 1);
}

void handleLDR() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastLDRReading >= sampleInterval) {
    lastLDRReading = currentMillis;
    if (readingCount < 24) {
      ldrReadings[readingCount++] = readLDR();
    }
  }
  
  if (currentMillis - lastSendTime >= sendInterval && readingCount > 0) {
    lastSendTime = currentMillis;
    currentAvgLight = 0;
    for (int i = 0; i < readingCount; i++) {
      currentAvgLight += ldrReadings[i];
    }
    currentAvgLight /= readingCount;
    
    char lightStr[8];
    dtostrf(currentAvgLight, 1, 3, lightStr);
    mqttClient.publish("ADMIN-LIGHT", lightStr);
    readingCount = 0;
    
    controlServo();
  }
}

void setup() {
  pinMode(BUZZER, OUTPUT);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  pinMode(LDR_PIN, INPUT);
  
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  windowServo.setPeriodHertz(50);
  windowServo.attach(SERVO_PIN, 500, 2400);
  windowServo.write(theta_offset);
  
  Serial.begin(9600);
  dhtSensor.setup(DHTPIN, DHTesp::DHT22);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    for(;;);
  }
  display.display();
  delay(2000);
  
  setupWifi();
  setupMqtt();
  timeClient.begin();
  timeClient.setTimeOffset(5.5 * 3600);
  
  display.clearDisplay();
  print_line("Welcome ", 30, 5, 2);
  print_line("to", 50, 27, 2);
  print_line("Medibox!", 24, 49, 2);
  display.display();
  delay(2000);
  display.clearDisplay();
}

void loop() {
  update_time_with_check_alarm();
  if (!mqttClient.connected()) {
    connectToBroker();
  }
  mqttClient.loop();
  
  updateTemperature();
  if (strcmp(tempAr, "NaN") != 0 && strcmp(tempAr, "ERR") != 0) {
    mqttClient.publish("ADMIN-TEMP", tempAr);
  }
  
  checkSchedule();
  handleLDR();
  
  unsigned long currentMillis = millis();
  if (currentMillis - lastServoUpdate >= SERVO_UPDATE_INTERVAL) {
    lastServoUpdate = currentMillis;
    controlServo();
  }
  
  delay(100);
}

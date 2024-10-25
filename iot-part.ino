#include <WiFi.h>
#include <DHTesp.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define DHT_PIN 15
#define BUZZER 19


DHTesp dhtSensor;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);


char tempAr[6];
bool isSheduledON = false;
unsigned long sheduledOnTime;


void setup() {
  Serial.begin(115200);

  setupWiFi();
  setupMqtt();

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  timeClient.begin();
  timeClient.setTimeOffset(5.5 * 3600);

  pinMode(BUZZER,OUTPUT);
  digitalWrite(BUZZER, LOW);

}

void loop() {

  if (!mqttClient.connected()) {
    connectToBroker();
  }
  mqttClient.loop();

  updateTemperature();
  Serial.println(tempAr);
  mqttClient.publish("210433-TEMP", tempAr);

  checkShedule()
  
  delay(1000);

}


void checkShedule(){
  if(isSheduledON){
    unsigned long currentTime = getTime();
    if(currentTime > sheduledOnTime){
      buzzerOn(true);
      isSheduledON = false;
      mqttClient.publish("210433-MAIN-ON-OFF-ESP","1");
      mqttClient.publish("210433-SCH-ON-ESP","0");
      Serial.println("Sheduled ON");
    }
  }
}

unsigned long getTime(){
  timeClient.update();
  return timeClient.getEpochTime();
}

void receiveCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]");

  char payloadCharAr[length];

  for (int i=0; i < length; i++) {
    Serial.print((char)payload[i]);
    payloadCharAr[i] = payload[i];
  }

  Serial.println();

  if ( strcmp(topic, "210433-MAIN-ON-OFF") == 0 ) {  //compare 2 strings. if same, value should be 0.
    buzzerOn(payloadCharAr[0]=='1');
  }else if ( strcmp(topic, "210433-SCH-ON") == 0 ){
    if(payloadCharAr[0]=='N'){
      isSheduledON = false;
    } else {
      isSheduledON = true;
      sheduledOnTime = atol(payloadCharAr); //array to long convertion = atol
    }
  }
}

void setupWiFi() {

  WiFi.begin("Wokwi-GUEST", "");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void updateTemperature() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  String(data.temperature, 2).toCharArray(tempAr, 6);
}

void setupMqtt() {
  mqttClient.setServer("test.mosquitto.org", 1883);
  mqttClient.setCallback(receiveCallback);
}

void connectToBroker() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect("ESP32-210433")) {
      Serial.println("Connected");
      mqttClient.subscribe("210433-ON-OFF");
      mqttClient.subscribe("210433-SCH-ON");
    } else {
      Serial.println("Failed");
      Serial.println(mqttClient.state());
      delay(5000);

    }
  }
}

void buzzerOn(bool isOn){
  if(isOn){
    tone(BUZZER,256);
  } else{
    noTone(BUZZER);
  }
}

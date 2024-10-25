
// E.M.A.R Niroshan
//210433R

//Libraries
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>

//Definitions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#define BUZZER 19
#define LED1 5
#define CANCEL 15
#define OK 16
#define UP 18
#define DOWN 17

#define DHT_PIN 12
#define LED2 23

#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET     19800
#define UTC_OFFSET_DST 0

#define LDR_LEFT 32
#define LDR_RIGHT 33


//Variables
int seconds = 0;
int minutes = 0;
int hours = 0;
int days = 0;

int offset=0;

unsigned long timeNow = 0;
unsigned long timeLast = 0;

bool alarmEnabled = true;
int n_alarms =3;
int alarmHours[] = {0,0,0};
int alarmMinutes[] = {1,2,3};
bool alarmTriggered[] = {false,false,false};

int nNotes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int CH = 523;
int notes[] = {C, D, E, F, G, A, B, CH};

int currentMode = 0;
int maxModes = 4;
String options[] = {"1 - Set Alarm 1","2 - Set Alarm 2","3 -set alarm 3","4 - disable all alarms"};

char tempAr[6];
char humAr[6];
bool isSheduledON = false;
unsigned long sheduledOnTime;

char IdrLArr[41];
char IdrRArr[4];

Servo servo;
int t_off = 30;
float gamma_i = 0.75;

//Declare Objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dht;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);


void setup() {
  
  Serial.begin(115200);

  pinMode(BUZZER,OUTPUT);
  pinMode(LED1,OUTPUT);
  pinMode(CANCEL,INPUT);
  pinMode(OK,INPUT);
  pinMode(UP,INPUT);
  pinMode(DOWN,INPUT);
  pinMode(LED2,OUTPUT);


  dht.setup(DHT_PIN, DHTesp::DHT22);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)){
    Serial.println(F("SSD1306 allocation failed."));
  }

  delay(2000);
  display.clearDisplay();

  setupWiFi();
  setupMqtt();

  timeClient.begin();
  timeClient.setTimeOffset(5.5 * 3600);

  servo.attach(2);

  display.clearDisplay();
  printLine("Welcome to Medibox!",12,10,1);
  delay(2000);
  display.clearDisplay();
  
}

void loop() {

  if (!mqttClient.connected()) {
    connectToBroker();
  }
  mqttClient.loop();

  printTimeNow();
  
  updateTimeWithCheckAlarm();
  display.clearDisplay();
  if (digitalRead(OK)==LOW){
    delay(200);
    goToMenu();
  }

  printTimeNow();

  checkTemp();
  
  printTimeNow();

  updateTempHumid();
  Serial.println(tempAr);
  mqttClient.publish("210433-TEMP", tempAr);
  Serial.println(humAr);
  mqttClient.publish("210433-HUMIDITY", humAr);

  checkShedule();

  delay (50);
  updateLight();
  // Serial.println(IdrLArr);
  // Serial.println(IdrRArr);
  mqttClient.publish("210433-LIGHT-L_NI", IdrLArr);
  delay(50);
   mqttClient.publish("210433-LIGHT-R_NI", IdrRArr);
  delay(100);

  delay(10); // this speeds up the simulation
}


void printLine(String text, int column, int row, int font_size){
  display.setTextSize(font_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row);
  display.println(text);
  display.display();
}

void printTimeNow(void){
  timeClient.update();
  printLine(timeClient.getFormattedTime(), 20, 15, 2);
}

void updateTime(void){
  timeClient.update();
  hours = timeClient.getHours();
  minutes = timeClient.getMinutes();
  seconds = timeClient.getSeconds();

}

void updateTimeWithCheckAlarm(void){
  updateTime();
  printTimeNow();

  if (alarmEnabled == true){
    for (int i=0; i<=n_alarms-1 ; i++){
    if (alarmTriggered[i]==false && alarmHours[i] == hours && alarmMinutes[i] == minutes){
      ringAlarm();
      alarmTriggered[i] = true;
    }
    }
  }
}

void ringAlarm(void){
  display.clearDisplay();
  printLine("Medicine Time",20,20,1);

  digitalWrite(LED1, HIGH);

  bool breakHappen = false;
  while ( digitalRead(CANCEL)==HIGH && breakHappen == false ){
    for (int i=0; i<=nNotes; i++){
      if (digitalRead(CANCEL)==LOW){
        delay(200);
        breakHappen = true;
        break;
      }
      tone(BUZZER,notes[i]);
      delay(500);
      noTone(BUZZER);
      delay(2);
    }
  }

  digitalWrite(LED1, LOW);
  display.clearDisplay();

}

int waitForButtonPress() {
  while (true){
    if (digitalRead(UP) == LOW){
      delay(200);
      return UP;
    }
    else if (digitalRead(DOWN) == LOW){
      delay(200);
      return DOWN;
    }
    else if (digitalRead(OK) == LOW){
      delay(200);
      return OK;
    }
    else if (digitalRead(CANCEL) == LOW){
      delay(200);
      return CANCEL;
    }

    updateTime();
  }
}


void goToMenu(void){
  while(digitalRead(CANCEL)==HIGH){
    display.clearDisplay();
    printLine(options[currentMode],10,10,1);

    int pressed = waitForButtonPress();

    if(pressed==UP){
      currentMode += 1;
      currentMode %= maxModes;
    }

    else if(pressed==DOWN){
      currentMode -= 1;
      if (currentMode < 0){
        currentMode = maxModes - 1;
      }
    }

    else if (pressed==OK){
      Serial.println(currentMode);
      delay(200);
      runMode(currentMode);
    }

  }
}


void runMode(int mode){

  if(mode==0 || mode==1 || mode==2){
    setAlarm(mode);
  }

  else if(mode==3){
    alarmEnabled=false;
    display.clearDisplay();
    printLine("All alarms disabled ",0,0,2);
    delay(2000);
  }

}


//Set alarms function
void setAlarm(int alarm){
  display.clearDisplay();
  printLine("Menu >> Set Alarm >> Hours",0,0,1);
  delay(1000);

  int tempHour = hours;
  while (true){
    display.clearDisplay();
    printLine("Enter hour : "+String(tempHour),0,30,1);

    int pressed = waitForButtonPress();

    if (pressed == UP){
      delay(200);
      tempHour += 1;
      tempHour %= 24;
    }
    else if (pressed == DOWN){
      delay(200);
      tempHour -= 1;
      if (tempHour < 0){
        tempHour = 23;
      }
    }
    else if (pressed == OK){
      delay(200);
      alarmHours[alarm] = tempHour;
      break;
    }
    else if (pressed == CANCEL){
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  printLine("Menu >> Set Alarm >> Hours",0,0,1);
  delay(1000);
  int tempMinute = minutes;
  while (true){
    display.clearDisplay();
    printLine("Enter minute : "+String(tempMinute),0,30,1);

    int pressed = waitForButtonPress();

    if (pressed == UP){
      delay(200);
      tempMinute += 1;
      tempMinute %= 60;
    }
    else if (pressed == DOWN){
      delay(200);
      tempMinute -= 1;
      if (tempMinute < 0){
        tempMinute = 59;
      }
    }
    else if (pressed == OK){
      delay(200);
      alarmMinutes[alarm] = tempMinute;
      break;
    }
    else if (pressed == CANCEL){
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  printLine("Alarm "+ String(alarm+1) +" is Set.",10,30,1);
  delay(1000);
  display.clearDisplay();

}

void checkTemp(void){
  TempAndHumidity data = dht.getTempAndHumidity();
  bool allGood = true;

  if (data.temperature >35){
    allGood = false;
    digitalWrite(LED2,HIGH);
    printLine("TEMP HIGH",32,40,1);
  }
  else if (data.temperature <25){
    allGood = false;
    digitalWrite(LED2,HIGH);
    printLine("TEMP LOW",32,40,1);
  }

  if (data.humidity >85){
    allGood = false;
    digitalWrite(LED2,HIGH);
    printLine("HUMD HIGH",32,50,1);
  }
  else if (data.humidity <35){
    allGood = false;
    digitalWrite(LED2,HIGH);
    printLine("HUMD HIGH",32,50,1);
  }

  if (allGood ){
    digitalWrite(LED2,LOW);
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

void setupMqtt() {
  mqttClient.setServer("test.mosquitto.org", 1883);
  mqttClient.setCallback(receiveCallback);
}

void updateTempHumid() {
  TempAndHumidity data = dht.getTempAndHumidity();
  String(data.temperature, 2).toCharArray(tempAr, 6);
  String(data.humidity, 2).toCharArray(humAr, 6);
}

void connectToBroker() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect("ESP32-210433")) {
      Serial.println("Connected");
      mqttClient.subscribe("210433-MAIN-ON-OFF");
      mqttClient.subscribe("210433-SCH-ON");
      mqttClient.subscribe("210433-SERVO-MINA");
      mqttClient.subscribe("210433-SERVO-CF");
    } else {
      Serial.println("Failed");
      Serial.println(mqttClient.state());
      delay(5000);

    }
  }
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
    if (payloadCharAr[0]=='1'){
      buzzerOn(true);
      Serial.print("Main Switch ON");
      
    }else{
      buzzerOn(false);
      Serial.print("Main Switch OFF");
    }
    
  }else if ( strcmp(topic, "210433-SCH-ON") == 0 ){
    if(payloadCharAr[0]=='N'){
      isSheduledON = false;
    } else if (atol(payloadCharAr)>getTime()) {
      isSheduledON = true;
      sheduledOnTime = atol(payloadCharAr); //array to long convertion = atol
    }
    
  } else if (strcmp(topic, "210433-SERVO-MINA") == 0) {
    t_off = String(payloadCharAr).toInt();

  } else if (strcmp(topic, "210433-SERVO-CF") == 0) {
    gamma_i = String(payloadCharAr).toFloat();

  }
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
  return timeClient.getEpochTime(); // Get raw time in seconds since Jan 1, 1970
}

void buzzerOn(bool isOn){
  if(isOn){
    tone(BUZZER,256);
    printLine("Medicine Time",20,3,1);
  } else{
    noTone(BUZZER);
    display.clearDisplay();
  }
}

void updateLight() {
  float lsv = analogRead(LDR_LEFT) * 1.00;
  float rsv = analogRead(LDR_RIGHT) * 1.00;
  float lsv_cha = (lsv - 4063.00) / (32.00 - 4063.00);
  float rsv_cha = (rsv - 4063.00) / (32.00 - 4063.00);
  Serial.println(String(lsv_cha) + " " + String(rsv_cha));
  updateAngle(lsv_cha, rsv_cha);
  String(lsv_cha).toCharArray(IdrLArr, 4);
  String(rsv_cha).toCharArray(IdrRArr, 4);
}



void updateAngle(float lsv, float rsv) {
  float max_I = lsv;
  float D = 1.5;
  if (rsv > max_I) {
    max_I = rsv;
    D = 0.5;
  }
  int theta = t_off * D + (180 - t_off) * max_I * gamma_i ;
  theta = min(theta, 180);
  servo.write(theta);
}

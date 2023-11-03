#include <ESP32Servo.h>
#include <WiFi.h>

//////////////////////////////
// servo section
//////////////////////////////

int minUs = 200;
int maxUs = 2700;

Servo servo[4];
int servoPin[4] = {22, 21, 17, 16};
int servoPos[4][10] = {
  {166, 150, 129, 112, 94, 77, 59, 42, 24, 7},
  {164, 147, 129, 112, 94, 77, 64, 48, 32, 20},
  {160, 142, 121, 104, 90, 75, 62, 42, 24, 7},
  {164, 147, 129, 112, 94, 77, 63, 44, 26, 9},
};

//////////////////////////////
// WiFi and NTP section
//////////////////////////////

// switch between 24H (12 rotors) / 12H (10 rotors)
#define HOUR12 false

// NTP settings
#define TIMEZONE 9 // timezone (GMT = 0, Japan = 9)
#define NTP_SERVER "pool.ntp.org"

#define WIFI_SMARTCONFIG true

#if !WIFI_SMARTCONFIG
// if you do not use smartConfifg, please specify SSID and password here
#define WIFI_SSID "SSID" // your WiFi's SSID
#define WIFI_PASS "PASS" // your WiFi's password
#endif

void getNTP(void) {
  for(int i = 0; WiFi.status() != WL_CONNECTED; i++) {
    if(i > 30) {
      ESP.restart();
    }
    Serial.println("Waiting for WiFi connection..");
    delay(1000);
  }

  configTime(TIMEZONE * 3600L, 0, NTP_SERVER);
  printLocalTime();
}

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %Y-%m-%d %H:%M:%S");
}

void wifiSetup() {
  int wifiMotion = 400; // while wainting for wifi, large motion
  int smatconfigMotion = 100; // while wainting for smartConfig, small motion

  WiFi.mode(WIFI_STA);
#if WIFI_SMARTCONFIG
  WiFi.begin();
#else
  WiFi.begin(WIFI_SSID, WIFI_PASS);
#endif

  for (int i = 0; ; i++) {
    Serial.println("Connecting to WiFi...");
    delay(1000);
    if (WiFi.status() == WL_CONNECTED) {
      break;
    }
#if WIFI_SMARTCONFIG
  if(i > 6)
    break;
#endif    
  }

#if WIFI_SMARTCONFIG
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.beginSmartConfig();

    //Wait for SmartConfig packet from mobile
    Serial.println("Waiting for SmartConfig.");
    while (!WiFi.smartConfigDone()) {
      Serial.print(".");
      for(int i = 0; i <= 3; i++) {
        servo[i].write(servoPos[i][7]);
      } 
      delay(1000);
    }

    Serial.println("");
    Serial.println("SmartConfig received.");

    //Wait for WiFi to connect to AP
    Serial.println("Waiting for WiFi");
    for(int i = 0; i <= 3; i++) {
      servo[i].write(servoPos[i][9]);
    } 
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(",");
    }
  }
  Serial.println("WiFi Connected.");
#endif

  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}



void setup() {
  Serial.begin(115200);
  Serial.println("start");

  for(int i = 0; i < 4; i++) {
    servo[i].setPeriodHertz(50);
    servo[i].attach(servoPin[i], minUs, maxUs);
  }
  for(int i = 0; i <= 3; i++) {
    servo[i].write(servoPos[i][8]);
  } 


  wifiSetup();
  getNTP(); // get current time
}
  

void loop() {
  static int prevhour = -1;
  static int prevMin = -1;
  struct tm tmtime;
  int digit[4];

  getLocalTime(&tmtime);

  if(prevMin == tmtime.tm_min)
    return;
  prevMin = tmtime.tm_min;

#if HOUR12
  tmtime.tm_hour %= 12;
  if(tmtime.tm_hour == 0)
    tmtime.tm_hour = 12;
#endif

  digit[0] = tmtime.tm_hour / 10;
  digit[1] = tmtime.tm_hour % 10;
  digit[2] = tmtime.tm_min / 10;
  digit[3] = tmtime.tm_min % 10;
  
  for (int pos = 0; pos < 10; pos++) {
    for(int i = 0; i <= 3; i++) {
      servo[i].write(servoPos[i][digit[i]]+1);
    } 
  }
  delay(500);
  for (int pos = 0; pos < 10; pos++) {
    for(int i = 0; i <= 3; i++) {
      servo[i].write(servoPos[i][digit[i]]-1);
    } 
  }
  delay(500);
  for (int pos = 0; pos < 10; pos++) {
    for(int i = 0; i <= 3; i++) {
      servo[i].write(servoPos[i][digit[i]]);
    } 
  }


  if(tmtime.tm_hour != prevhour) {
    if(tmtime.tm_hour % 6 == 0)
      getNTP();
    prevhour = tmtime.tm_hour;
  }
}

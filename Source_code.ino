#include <Keypad.h>
#include <Wire.h>
#include <math.h>
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <string.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#define BLYNK_PRINT Serial
float lat,lon;
char latlon[100];


char auth[] = "************";   // Auth Token from the Blynk App.

/************************* WiFi Access Point *********************************/
#define WLAN_SSID       "****" //Write Wifi UserID
#define WLAN_PASS       "****" //Write Wifi Password
/************************* Adafruit.io Setup *********************************/
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "***" //Write adafruit username
#define AIO_KEY         "***" //Write adafruit IO key

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/
Adafruit_MQTT_Publish photocell = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/photocell");



int safety_check(int16_t x,int16_t y,int16_t z);
void MQTT_connect();
void mqtt_publish();
void mqtt_publish2();
void mqtt_publish3();
void accident_alert();
void check(int16_t AcX,int16_t AcY,int16_t AcZ,int16_t GyX,int16_t GyY,int16_t GyZ);
int pass_check();
void blink(int n);


const int MPU=0x68;
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {D0, D3, D4, D0}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {D5, D6, D7, D5}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

int i=0,pass_verified=0,pass_attempt=0,pass_right=0;
char pass[3]={'9','8','7'};

void setup(){
  Wire.begin(D1,D2);
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  Serial.begin(9600);

  
	pinMode(10, OUTPUT);
	digitalWrite(10, LOW);
    delay(1000);

  Serial.println(F("Adafruit MQTT"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());
  Blynk.begin(auth, WLAN_SSID, WLAN_PASS);

}

uint32_t x=0;
int16_t safety_var=0;

void loop(){

  MQTT_connect();



  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU,14,true);


  int AcXoff,AcYoff,AcZoff,GyXoff,GyYoff,GyZoff;
  int temp,toff;
  double t,tx,tf;


//Gyro correction
  GyXoff = -16000;
  GyYoff = -200;
  GyZoff = -800;
//Acc correction
  GyXoff = -16000;
  GyYoff = -200;
  GyZoff = -800;


//read gyro data
GyX=(Wire.read()<<8|Wire.read()) + GyXoff;
GyY=(Wire.read()<<8|Wire.read()) + GyYoff;
GyZ=(Wire.read()<<8|Wire.read()) + GyZoff;


//read temperature data
temp=(Wire.read()<<8|Wire.read()) + toff;
tx=temp;
t = tx/340 + 36.53;
tf = (t * 9/5) + 32;

  //read accel data
  AcX=(Wire.read()<<8|Wire.read()) + AcXoff;
  AcY=(Wire.read()<<8|Wire.read()) + AcYoff;
  AcZ=(Wire.read()<<8|Wire.read()) + AcZoff;


if(pass_verified==0 && safety_var==0)
{
 if( safety_check(GyX,GyY,GyZ)==1){
  Serial.println("Safety Alert");
   pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
    mqtt_publish3();
  safety_var=1;
 }
}
  
if(pass_verified==0 && pass_attempt<3)
{
  if(pass_check()==1) pass_verified=1;
  if(pass_attempt>=3 && pass_verified!=1)
  {
     pinMode(10, OUTPUT);
    Serial.println("Thief");
    mqtt_publish2();
    digitalWrite(10, HIGH);
  }
  if(pass_verified==1) {Serial.println("Corect");  blink(2);}
}
else if(pass_verified==1)
{ 
check(AcX,AcY,AcZ,GyX,GyY,GyZ);
}
}

int pass_check()
{
  char key = keypad.getKey(); 
  if (key){
    Serial.println(key);
    if(key!=pass[i]) pass_right=2;
    i++;
  }
  if(i==3 && pass_right==0) {  i=0;pass_attempt++;pass_right=0;return 1;}
  else if(i==3 && pass_right==2){  i=0;pass_attempt++;pass_right=0;return 2;}
  return 5;
}

void check(int16_t AcX,int16_t AcY,int16_t AcZ,int16_t GyX,int16_t GyY,int16_t GyZ)
{

    if(AcX <= -19728.0  && AcZ <= 14894.0) { accident_alert(); return; }
    if(AcX <= -19728.0  && AcZ > 14894.0 && GyZ <= 474.0 && GyZ <= -4701.0) { accident_alert(); return; }
    if(AcX <= -19728.0  && AcZ > 14894.0 && GyZ > 474.0) { accident_alert(); return; }
    if(AcX > -19728.0  && AcX <= 16767.5 && AcZ <= -21158.0) { accident_alert(); return; }
    if(AcX > -19728.0  && AcX > 16767.5) { accident_alert(); return; }
    if(AcX > -19728.0  && AcX <= 16767.5 && AcZ > -21158.0 && AcX <= -14164.0 && AcZ <= 8532.0) { accident_alert(); return; }
}

void blink(int n)
{
  for(int i=0;i<n;i++)
  {
    digitalWrite(10, HIGH);
   delay(300);
   digitalWrite(10, LOW); 
   delay(300);
  }
}

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
  blink(2);
}


void mqtt_publish()
{
  Serial.print(F("\nSending  val "));
  Serial.print(x);
  Serial.print("Sending message with mqtt");
  if (! photocell.publish("\n\nAccident happened!!!!\n")) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

sprintf (latlon,"\nLatitude: %d.%02u \nLongitude: %d.%02u", (int) lat, (int) ((lat - (int) lat ) * 7),(int) lon, (int) ((lon - (int) lon ) * 7) );
  if (! photocell.publish(latlon)) {
    Serial.println(F("lat Failed"));
  } else {
    Serial.println(F("lat OK!"));
  }
}





void accident_alert()
{
  pinMode(10, OUTPUT);
        digitalWrite(10, HIGH);
        Serial.print("\n\n Accelerated. \n\n");
        mqtt_publish();
        Blynk.run();
        delay(1000);
        digitalWrite(10, LOW);
}


void mqtt_publish2()
{
  Serial.print(F("\nSending"));
  Serial.print(x);
  Serial.print("Sending message with mqtt");
  if (! photocell.publish("\n\n Thief ! Thief ! Thief !!!!! ")) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }
}


void mqtt_publish3()
{
  Serial.print(F("\nSending"));
  Serial.print(x);
  Serial.print("Sending message with mqtt");
  if (! photocell.publish("\n\n Safety Alert !!!!! ")) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }
}


BLYNK_WRITE(V1) {
  GpsParam gps(param);

  // Print 6 decimal places for Lat, Lon
  Serial.print("Lat: ");
  lat=gps.getLat();
  Serial.println(lat, 7);

  Serial.print("Lon: ");
  lon=gps.getLon();
  Serial.println(lon, 7);

  // Print 2 decimal places for Alt, Speed
  Serial.print("Altitute: ");
  Serial.println(gps.getAltitude(), 2);

  Serial.print("Speed: ");
  Serial.println(gps.getSpeed(), 2);

  Serial.println();
}


int safety_check(int16_t x,int16_t y,int16_t z)
{

    if(x<0) x=-x;
    if(y<0) y=-y;
    if(z<0) z=-z;
    if(x>14500 || y>14500 || z>14500)
    {
        delay(600);
        return 1;
    }
    return 0;
}
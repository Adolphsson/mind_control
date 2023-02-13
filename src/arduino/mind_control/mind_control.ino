
/* 
 *  Mind Control Server - for ESP8266
 *  
 *  
 * License - GPL v3
 * (C) 2021 Copyright - Gene Ruebsamen
 * ruebsamen.gene@gmail.com
 *
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <arduino-timer.h> // https://github.com/contrem/arduino-timer

#define DDRIVE_MIN -100 //The minimum value x or y can be.
#define DDRIVE_MAX 100  //The maximum value x or y can be.
#define MOTOR_MIN_PWM -1023 //The minimum value the motor output can be.
#define MOTOR_MAX_PWM 1023 //The maximum value the motor output can be.

// wired connections
#define L9110_A_IA 0 // D7 --> Motor B Input A --> MOTOR A +
#define L9110_A_IB 2 // D6 --> Motor B Input B --> MOTOR A -
 
// functional connections
#define MOTOR_A_PWM L9110_A_IA // Motor A PWM Speed
#define MOTOR_A_DIR L9110_A_IB // Motor A Direction

#define MIN_THRESHOLD 50

// Set these two variables if you want the UDP Server to be available on your local network.
// Will instead use AP mode if the network can't be connected to during start.
const char ssid[]="";
const char pass[]="";

IPAddress local_IP(192,168,1,205);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

WiFiUDP udp;
unsigned int localPort = 10000;
const int PACKET_SIZE = 256;
char packetBuffer[PACKET_SIZE];
int status = WL_IDLE_STATUS;
auto timer = timer_create_default();

int rlen;
int  x,y;
bool debug = false;

void coast() {
  digitalWrite( MOTOR_A_DIR, LOW );
  digitalWrite( MOTOR_A_PWM, LOW );
}

void setup() {
  
  Serial.begin(115200);
  Serial.begin(115200);
  Serial.println();

  setupWifi();

  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());

  pinMode( MOTOR_A_DIR, OUTPUT );
  pinMode( MOTOR_A_PWM, OUTPUT );
  coast();

  Serial.println("start udp read");
}

void setupWifi() {
  if (ssid[0] != '\0') {
    // Try to connect to LOCAL AP
    WiFi.mode(WIFI_STA);
    if (!WiFi.config(local_IP, gateway, subnet)) {
      Serial.println("STA Failed to configure");
    }
    WiFi.begin(ssid,pass);
    Serial.print("Connecting");

    int wifiConnectWait = 0;
    while (WiFi.status() != WL_CONNECTED && wifiConnectWait < 10)
    {
      delay(500);
      Serial.print(".");
      wifiConnectWait++;
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println();
      Serial.print("Connected, IP address: ");
      Serial.println(WiFi.localIP());
      return;
    }
    WiFi.disconnect();
  }

  WiFi.mode(WIFI_AP);
  // Setup Soft AP
  Serial.print("Setting soft-AP configuration ... ");
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");

  Serial.print("Setting soft-AP ... ");
  Serial.println(WiFi.softAP("MindControl_AP") ? "Ready" : "Failed!");

  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  rlen = udp.parsePacket();

  if (rlen) {
    //Serial.printf("Received %d bytes from %s, port %d\n", rlen, udp.remoteIP().toString().c_str(), udp.remotePort());
  
    int len = udp.read(packetBuffer, 255);
    if (len > 0)
    {
      packetBuffer[len] = 0;
    }
    //Serial.printf("UDP packet contents: %s\n", packetBuffer);
    if (debug) {
      udp.beginPacket(udp.remoteIP(), udp.remotePort());
      udp.write(packetBuffer);
      udp.endPacket();
    }


    char *val;
    val = strtok(packetBuffer,":");
    while (val != NULL) {
      if (val[0] == 'X') {
        x = atoi(&val[1]);
      }
      else if (val[0] == 'Y') {
        y = atoi(&val[1]);
      }
      else if (val[0] == 'D') {
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        debug = !debug;
        if (debug)
          udp.write("Debug ON\n");
         else 
          udp.write("Debug OFF\n");
        udp.endPacket();
      }
      else if (val[0] == 'P') {
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.write("PONG");
        udp.endPacket();
      }
      val = strtok(NULL,":");
    }

    yield();  
    //Serial.printf("L: %d, R: %d\n",LeftMotorOutput,RightMotorOutput);
    int mapx = map(x, -100, 100, -1023, 1023);
    Serial.printf("x_val: %d\n",mapx);
    if (mapx > MIN_THRESHOLD) {
      digitalWrite( MOTOR_A_DIR, LOW ); // direction = forward (HIGH)
      analogWrite( MOTOR_A_PWM, mapx ); // PWM speed = slow      
    } else if (mapx < MIN_THRESHOLD) {
      analogWrite( MOTOR_A_DIR, -mapx ); // direction = forward (HIGH)
      digitalWrite( MOTOR_A_PWM, LOW ); // PWM speed = slow            
    } else {
      digitalWrite( MOTOR_A_DIR, LOW ); // direction = forward (HIGH)
      digitalWrite( MOTOR_A_PWM, LOW ); // PWM speed = slow                  
    }      
  }
  
  timer.tick();
}

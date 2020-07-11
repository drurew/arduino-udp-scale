/*
 * this is a simple scale for weighing beehives
 * it utilizes udp to send values over a wifi network.
 * It can be adapted to be used with probably any arduino by associating the correct libraries.
 */
/**************************LIBRARIES********************************/
#include <ESP8266WiFi.h>
#include "HX711.h"
#include <WiFiUdp.h>
#include "Arduino.h"
HX711 scale;                          
/**************************VARIABLES********************************/


#define calibration_factor 23.90      //This value is obtained using the SparkFun_HX711_Calibration sketch 
const int LOADCELL_DOUT_PIN = D7;     // which pin DOUT connects to on the controller 
const int LOADCELL_SCK_PIN = D8;      // Which pin SCK connects to on the controller

const char* ssid = "xxxxxxxx";        //set your wifi ssid here
const char* password = "xxxxxxxxxx";  //password

float DMS_value = 0;
char str_DMS[5];

long offset = 0;

float reading;
long GewichtFaktor;
boolean tariere = false;
boolean HX711_OK;

// Wifi setting
WiFiUDP Udp;

// Parameters of the logging device
IPAddress ip(xxx, xxx, xx, xxx);      // Destination device IP (server)
int recPort = xxxx;                   // Reciever Port on your server

// Port on which the device should send on
unsigned int localUdpPort = xxxx;     // Local port to listen d1 mini

const int UDP_PACKET_SIZE = 7;
byte packetBuffer[UDP_PACKET_SIZE];   //buffer to hold incoming and outgoing packets
char incomingPacket[255];             // buffer for incoming packets

/*******************************************************************
++++++++++++++++++++++++++SETUP++++++++++++++++++++++++++++++++++++
********************************************************************/

void setup()
{
  Serial.begin(115200);
 
  // Initialize serial interface
  #ifndef ESP8266
   while (!Serial);                   
  #endif

  delay(2000);

  // Set Wifi values
  Serial.printf("Wifi connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");

  // UDP parameters
  Udp.begin(localUdpPort);
  Serial.printf("Sending data from IP %s to UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);

  Serial.println("Using HX711...");

  HX711_OK = (digitalRead(LOADCELL_DOUT_PIN) == 0) ;
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  
  if (HX711_OK)
  {
    scale.set_scale(GewichtFaktor);
    Serial.println(GewichtFaktor);
    scale.tare();
  }
  else Serial.println("HX711 nicht angeschlossen");

  scale.set_scale(calibration_factor);              //This value is obtained by using the SparkFun_HX711_Calibration sketch
  Serial.println("scale setup complete");
  scale.tare();                                     //Assuming there is no weight on the scale at start up, reset the scale to 0
  Serial.println("scale has been set to zero");
}

void loop()
{
  DMS_value =  scale.get_units(5);
  Serial.println(DMS_value);
  dtostrf(DMS_value, 4, 2,str_DMS);
  // ++++++++++++++++++++++++++++++++++++++++++++++++
  // +++ Send data to defined port and IP address +++
  // ++++++++++++++++++++++++++++++++++++++++++++++++
  Serial.println("Sending Data");
  sprintf((char*)packetBuffer,"%s",str_DMS);   //Reset the buffer to 0
  Udp.beginPacket(ip, recPort);
  Udp.print("Gewicht=\\");                     //This can be modified to identify the string on the server side
  Udp.write(packetBuffer, UDP_PACKET_SIZE);     
  Udp.endPacket();
  memset(packetBuffer, 0, UDP_PACKET_SIZE);
  Serial.println("Done");
  delay(10000);                                // Change this to a higher number to delay resending the udp string 

  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // +++ Recieve data from defined port and IP address ++++
  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++

  /* any non interpretable serial commands the device recieves will cause the scale to reset itself to zero.*/   
  
  int packetSize = Udp.parsePacket();
 
  if (packetSize)
  {
   Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
   int len = Udp.read(incomingPacket, 255);

   /* should a reset be undesiarable, comment out the following from below;
    *  Serial.printf("UDP packet contents: %s\n", incomingPacket);
    *  Serial.println("Taring...");
    *  scale.tare();   
    *  offset = scale.get_offset();
    *  Serial.printf("New offset: %i\n", offset);
    */
   
   if (len > 0)
   {
    incomingPacket[len] = 0;
   }
   
   Serial.printf("UDP packet contents: %s\n", incomingPacket);
   Serial.println("Taring...");
   scale.tare();                               //Assuming there is no weight on the scale at start up, reset the scale to 0
   offset = scale.get_offset();
   Serial.printf("New offset: %i\n", offset);
  }       
}

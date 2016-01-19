#include <dht.h>
#include <PubSubSslClient.h>
#include <DateTimeLibrary.h>
#include <WiFi.h>
#include <Wire.h>
#include <driverlib/timer.h>

#include "config.h"

time_t   myEpochRTC;
boolean isConnected;
boolean timeSetFromNtp;

WiFiClient client;
PubSubSslClient mqttClient(client);
DateTime myRTC;

boolean seenMovement;
unsigned long lastMovement;
unsigned long loadEnabled;

/* *****************************************************************
 * Callback for MQTT and Interrupt
 * *****************************************************************/
void callback(char* topic, byte* payload, unsigned int length) 
{
  // We are only interested in one topic
  if(strcmp(topic, "sensor/cc3200/cmd") != 0)
    return;
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) 
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  
  // Show notification, if less that 2 minutes have been passes since last movement
  if( seenMovement )
  {
    digitalWrite(LOAD_PIN, HIGH);
    loadEnabled = millis();
  }
  
} 

void pirDetectedMovement() 
{
  lastMovement = millis();
  seenMovement = true;
  
   #ifdef DEBUG_PIN
     digitalWrite(DEBUG_PIN, HIGH);
   #endif
  
}

void Timer0IntHandler()
{
  // Close email notifier after 3 minutes
  if( loadEnabled != 0 && ( millis() - loadEnabled ) > 180000 )
  {
    digitalWrite(LOAD_PIN, LOW);
    loadEnabled = 0;
  }
  
  // If more than 120 seconds has passend since last movement detection, set flag to low
  if ( seenMovement && ( millis() - lastMovement ) > 120000 )
  {
    seenMovement = false;
    #ifdef DEBUG_PIN
      digitalWrite(DEBUG_PIN, LOW);
     #endif
  }

  //  
  unsigned long ulStatus = MAP_TimerIntStatus(TIMERA0_BASE, false);
  MAP_TimerIntClear(TIMERA0_BASE,ulStatus);
}

/* *****************************************************************
 * Mainloop
 * *****************************************************************/
void loop()
{     
  int16_t l1, temp_int=0, hum_int=0;
  char buffer[500];
  float temperature = 0.0,
        humidity = 0.0;
  
  // Read time from RTC  
  myEpochRTC = myRTC.getLocalTime();

  // Read temperature and humidity
  if( !dht::readFloatData(DHT22_PIN, &temperature, &humidity, true) == 0 )
  {
    Serial.println("DHT22 error..");
  } else {
    temp_int=(int)temperature*100.0;
    hum_int=(int)humidity*100.0;
  }
    
  //  
  Serial.print("Value of movement detector: ");
  Serial.println(lastMovement);
    
  //    
  if(mqttClient.connect("IoTClient"))
  {
    Serial.println("Connected to AWS MQTT");
      
    if( !mqttClient.subscribe("sensor/cc3200/cmd") )
    {
      Serial.println("Subscribe failed");
    }
      
    mqttClient.loop();
      
    snprintf(buffer,500, "{\"temp\":%d, \"hum\":%d, \"time\":\"%s\"}", 
      temp_int, hum_int, stringDateTime(myEpochRTC).c_str() );
      
    mqttClient.publish("sensor/cc3200/values",buffer);
    
    // Try to read response for 60 seconds (every 5 seconds)
    for(l1=0; l1<12; l1++)
    {
      mqttClient.loop();
      delay( 5000 );
    }

    Serial.println("Disconnecting from MQTT...");
    mqttClient.disconnect();
  } else {
    Serial.println("Unable to connect to MQTT..");
    sleep(60000);
  }  
  
  // Sleep for 4 minutes ( 240 seconds ; 240000 ms)
  Serial.println("Sleeping for 4 minutes...");
  sleep( 240000 );
}

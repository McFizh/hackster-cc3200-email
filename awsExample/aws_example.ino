#include <dht.h>
#include <PubSubSslClient.h>
#include <DateTimeLibrary.h>
#include <WiFi.h>
#include <Wire.h>

#include "config.h"

time_t   myEpochRTC;
boolean isConnected;
boolean timeSetFromNtp;

WiFiClient client;
PubSubSslClient mqttClient(client);
DateTime myRTC;

unsigned long lastMovement;

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
  if( ( millis() - lastMovement ) < 120000 )
  {
    
  }
  
} 

void pirDetectedMovement() 
{
  lastMovement = millis();
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
    Serial.println("Connected to AWS mtqq");
      
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

    mqttClient.disconnect();
  } else {
    Serial.println("Unable to connect to mtqq");
    delay(60000);
  }  
  
  // Delay for 4 minutes ( 240 seconds ; 240000 ms)
  delay( 240000 );
}

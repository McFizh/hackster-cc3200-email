#include "config.h"

/* *****************************************************************
 * Setup board
 * *****************************************************************/
void setup()
{  
  unsigned long t_time;
  byte counter;
  
  // Setup led pin, serial and rtc
  pinMode(29, OUTPUT);  
  pinMode(PIR_PIN, INPUT_PULLDOWN);
  lastMovement=0;
  attachInterrupt(PUSH2, pirDetectedMovement, RISING);
  
  Serial.begin(9600);
  Serial.println("Init begin..");
  
  myRTC.begin();
  myRTC.setTimeZone(tz_CEST);
    
  // Try to connect to WiFi network
  counter=0;
  while( setup_wifi() == false )
  {
    counter++;
    if(counter==2)
      failure_state();
  }

  // Try to get local IP     
  Serial.print("Waiting for IP..");
  
  counter = 0;
  t_time = millis();
  while( WiFi.localIP() == INADDR_NONE )
  {
    Serial.print(".");
    delay(500);
    
    if(counter==2)
      failure_state();
    
    if( ( millis() - t_time ) > 5000 )
    {
      Serial.println("Unable to get IP..");
      setup_wifi();
      Serial.print("Waiting for IP..");
      delay(10000);
      t_time = millis();
      counter++;
    }
    
  }

  // Print out local IP
  Serial.print("\nMy IP is: ");
  Serial.println( WiFi.localIP() );

  // Verify that the board has certificates installed
  if( !client.useRootCA() )
  {
    Serial.println("RootCA not found .. aborting ..");
    failure_state();
  }

  if( !client.useClientCert() )
  {
    Serial.println("Client cert files not found .. aborting ..");
    failure_state();
  }
  
  //
  timeSetFromNtp = setupTimeFromNtp();
  
  //
  mqttClient.setServer( aws_endpoint , 8883 );
  mqttClient.setCallback( callback );  
}

/* *****************************************************************
 * Try to connect to wifi network
 * *****************************************************************/
boolean setup_wifi()
{
  byte counter = 0;

  // Led red off
  digitalWrite(29, LOW);

  // Disconnect first, if we are currently connected
  if(isConnected)
  {
    Serial.println("Disconnecting from WiFi..");
    WiFi.disconnect();
    isConnected = false;
    delay(500);
  }
  
  Serial.print("Attempting to connect to WiFi network..");
  
  // Open connection to WiFi
  WiFi.begin(ap_ssid, ap_key);
  while ( WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
    counter++;
    if(counter==10)
    {
      break;
    }
  }
  
  if(counter==10)
  {
    Serial.println("\nUnable to connect to wifi");
    isConnected = false;
    digitalWrite(29, HIGH);
    return false;
  }
  
  //
  Serial.println("\nConnected to wifi");
  isConnected = true;  
  return true;
}

/* *****************************************************************
 * Try to get time from NTP
 * *****************************************************************/
boolean setupTimeFromNtp()
{
  uint8_t status;
  
  // Get IP for ntp server.. 
  IPAddress ntpIP(0,0,0,0);
  if( !WiFi.hostByName(ntp_pool, ntpIP) )
  {
    return false;
  }
  
  // Try to set time from NTP server 
  Serial.print( "Trying to get NTP time... " ); 
  status =  getTimeNTP( myEpochRTC, ntpIP );
  if( status == 0 )
  {
    Serial.println( "success" );
    myRTC.setTime(myEpochRTC);
    return true;
  } 
  
  //
  Serial.print( "failed with code:" );
  Serial.println(status);
  return false;
}

/* *****************************************************************
 * Stop application and blink red led
 * *****************************************************************/
void failure_state()
{
  while(true)
  {
    delay(500);
    digitalWrite(29, HIGH);
    delay(500);
    digitalWrite(29, LOW);
  }
}


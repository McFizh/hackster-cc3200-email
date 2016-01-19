#include "config.h"

/* *****************************************************************
 * Setup board
 * *****************************************************************/
void setup()
{  
  unsigned long t_time;
  byte counter;
  
  // Setup led pins (red off, yellow on)
  pinMode(RED_LED, OUTPUT);
    digitalWrite(RED_LED, LOW);
  pinMode(YELLOW_LED, OUTPUT);
    digitalWrite(YELLOW_LED, HIGH);
  pinMode(LOAD_PIN, OUTPUT);
    digitalWrite(LOAD_PIN, LOW);
  
  #ifdef DEBUG_PIN
    pinMode(DEBUG_PIN, OUTPUT);
      digitalWrite(DEBUG_PIN, LOW);
  #endif
  
  // Setup PIR sensor
  lastMovement=0;
  seenMovement=false;
  loadEnabled=0;
  pinMode(PIR_PIN, INPUT_PULLDOWN);
  attachInterrupt(PUSH2, pirDetectedMovement, RISING);
  
  // Enable serial
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
  
  // Setup timer interrupt
  setup_timer();  
  
  // Setup completed.. shutdown yellow led
  digitalWrite(YELLOW_LED, LOW);
}

/* *****************************************************************
 * Try to connect to wifi network
 * *****************************************************************/
boolean setup_wifi()
{
  byte counter = 0;

  // Led red off
  digitalWrite(29, LOW);

  // Disconnect first, if currently connected
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

void setup_timer() {
  
  MAP_PRCMPeripheralClkEnable(PRCM_TIMERA0, PRCM_RUN_MODE_CLK);
  MAP_PRCMPeripheralReset(PRCM_TIMERA0);

  TimerIntRegister(TIMERA0_BASE, TIMER_A, &Timer0IntHandler);
  
  MAP_TimerConfigure(TIMERA0_BASE, TIMER_CFG_PERIODIC);  
  MAP_TimerIntEnable(TIMERA0_BASE, TIMER_TIMA_TIMEOUT);  
  MAP_TimerLoadSet(TIMERA0_BASE, TIMER_A, F_CPU );  
  MAP_TimerEnable(TIMERA0_BASE, TIMER_A);  
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
    sleep(500);
    digitalWrite(RED_LED, HIGH);
    sleep(500);
    digitalWrite(RED_LED, LOW);
  }
}


#ifndef __CONFIG_H
#define __CONFIG_H

// Enter your WiFi settings here 
char ap_ssid[] = "";
char ap_key[] = "";

// NTP pool to use
char ntp_pool[] = "europe.pool.ntp.org";

// AWS endpoint.. should look something like this:
// {random characters}.iot.{service zone}.amazonaws.com
// for example: abcd1234.iot.eu-west-1.amazonaws.com
char aws_endpoint[] = "xxxxxxxxxxxx.iot.eu-west-1.amazonaws.com";

// 
#define DHT22_PIN 32
#define PIR_PIN 11

#define LOAD_PIN 8

// Comment this out, if you don't have debug led connected
#define DEBUG_PIN 5

#endif

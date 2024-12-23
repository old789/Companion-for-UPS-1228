#define DEBUG_SERIAL  // because just "DEBUG" defined in PZEM004Tv30.h ( legacy :)
#define DBG_WIFI    // because "DEBUG_WIFI" defined in a WiFiClient library

#if defined ( DBG_WIFI ) && not defined ( DEBUG_SERIAL )
#define DEBUG_SERIAL
#endif

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#include <EEPROM.h>

#include <SimpleCLI.h>  // https://github.com/SpacehuhnTech/SimpleCLI

#include "TickTwo.h"    // https://github.com/sstaub/TickTwo

#include "uptime.h"     // https://github.com/YiannisBourkelis/Uptime-Library


#define PIN_LED1        D1
#define PIN_LED2        D2
#define PIN_PG          D3
#define PIN_MBSW        D5
#define BUTTON          D6

#define MAX_ALLOWED_INPUT 127

// Create CLI Object
SimpleCLI cli;

void count_uptime();
void usual_report();
void check_ups_status();

// Create timers object
TickTwo timer1( count_uptime, 1000);
TickTwo timer2( check_ups_status, 100);
TickTwo timer3( usual_report, 60000);

byte external_power_state = HIGH;
byte external_power_state_prev = HIGH;
byte led1_state = HIGH;
byte led1_state_prev = HIGH;
byte led2_state = HIGH;
byte led2_state_prev = HIGH;
byte mbsw_state = HIGH;
byte mbsw_state_prev = HIGH;
// unsigned int last_change_state = 0;
bool first_report = true;
bool enable_cli = false;
bool eeprom_bad = false;
int httpResponseCode = 0;
char str_uptime[17] = "0d0h0m0s";
char in_str[128] = {0};
char str_post[1024];

// EEPROM data
uint16_t mark = 0x55aa;
uint8_t standalone = 0;
char ups_name[33] = {0};
char ssid[33] = {0};
char passw[65] = {0};
char host[65] = {0};
uint16_t port = 443;
char uri[128] = {0};
uint8_t http_auth = 0;
char http_user[33] = {0};
char http_passw[33] = {0};
uint8_t wifi_tries = 0;
// uint8_t after_party = 0;

#define PT_STANDALONE       sizeof(mark)
#define PT_UPS_NAME         PT_STANDALONE + sizeof(standalone)
#define PT_SSID             PT_UPS_NAME + sizeof(ups_name)
#define PT_PASSW            PT_SSID + sizeof(ssid)
#define PT_HOST             PT_PASSW + sizeof(passw)
#define PT_PORT             PT_HOST + sizeof(host)
#define PT_URI              PT_PORT + sizeof(port)
#define PT_AUTH             PT_URI + sizeof(uri)
#define PT_HUSER            PT_AUTH + sizeof(http_auth)
#define PT_HPASSW           PT_HUSER + sizeof(http_user)
#define PT_WIFI_TRIES       PT_HPASSW + sizeof(http_passw)
#define PT_CRC              PT_WIFI_TRIES + sizeof(wifi_tries)
#define SIZE_EEPROM         PT_WIFI_TRIES + sizeof(wifi_tries) - 1 // PT_CRC d'not count
// #define PT_AFTER_PARTY      PT_WIFI_TRIES + sizeof(wifi_tries)
// #define PT_CRC              PT_AFTER_PARTY + sizeof(after_party)
// #define SIZE_EEPROM         PT_AFTER_PARTY + sizeof(after_party) - 1 // PT_CRC d'not count

// Commands
Command cmdStandalone;
Command cmdUpsName;
Command cmdSsid;
Command cmdPassw;
Command cmdShow;
Command cmdHost;
Command cmdPort;
Command cmdUri;
Command cmdHauth;
Command cmdHuser;
Command cmdHpassw;
Command cmdSave;
Command cmdReboot;
Command cmdHelp;


void setup() {
#ifdef DEBUG_SERIAL
  Serial.begin(115200,SERIAL_8N1);
  delay(50);
  Serial.println(".\nStart debugging serial");
#endif

  EEPROM.begin(2048);
#ifdef DEBUG_SERIAL
  Serial.println("Init EEPROM");
#endif

  if ( ! eeprom_read() ) {
    eeprom_bad = true;
  } else {
    if ( ! is_conf_correct() ) {
      eeprom_bad = true;
    }
  }

  enable_cli = is_button_pressed();

  if ( enable_cli ) {
    // Command line mode
#if not defined DEBUG_SERIAL
    Serial.begin(115200);
    delay(50);
#endif
    SetSimpleCli();
    Serial.println("Usage:");
    Serial.println(cli.toString());
    if ( eeprom_bad ){
      Serial.println("\nEEPROM error or bad config");
    }
  }else{
    pinMode(PIN_PG, INPUT);
    pinMode(PIN_LED1,  INPUT);
    pinMode(PIN_LED2,  INPUT);
    pinMode(PIN_MBSW,  INPUT);
    pinMode(BUTTON,  INPUT);

    if ( eeprom_bad ) {
      standalone = 1;
    }
 
    if ( standalone == 0 ) {
#ifdef DEBUG_SERIAL
      Serial.println("Enter to network mode");
#endif
      wifi_init();
    }
#ifdef DEBUG_SERIAL
    else{
      Serial.println("Enter to standalone mode");
    }
#endif
    timer1.start();
    timer2.start();
    if ( standalone == 0 ) {
      timer3.start();
    }
  }
}

void loop(){
  if (enable_cli) {
    loop_cli_mode();
  }else{
    loop_usual_mode();
  }
}

void loop_usual_mode() {
  timer1.update();
  timer2.update();
  if ( standalone == 0 ) {
    timer3.update();
  }
}

void check_ups_status(){
#if defined ( DEBUG_SERIAL )
  PGM_P msg_pwr_fail = PSTR("External power failed");
  PGM_P msg_pwr_restore = PSTR("External power restored");
  PGM_P msg_battery_low = PSTR("Low battery");
  PGM_P msg_battery_ok = PSTR("Battery is Ok");
#endif

  // read and logging external power state
  external_power_state = digitalRead(PIN_PG);   
  if (external_power_state_prev != external_power_state) {
    external_power_state_prev = external_power_state;
    if (external_power_state == LOW) {
      send_alarm_ab_input( false );
#ifdef DEBUG_SERIAL
      Serial.println(FPSTR(msg_pwr_fail));
#endif
    } else {
      send_alarm_ab_input( true );
#ifdef DEBUG_SERIAL
      Serial.println(FPSTR(msg_pwr_restore));
#endif
    }
  }

  led1_state = digitalRead(PIN_LED1);   
  if (led1_state_prev != led1_state) {
    led1_state_prev = led1_state;
    if (led1_state == LOW) {
#ifdef DEBUG_SERIAL
      Serial.println("Led1 ON");
#endif
    } else {
#ifdef DEBUG_SERIAL
      Serial.println("Led1 off");
#endif
    }
  }

  led2_state = digitalRead(PIN_LED2);   
  if (led2_state_prev != led2_state) {
    led2_state_prev = led2_state;
    if (led2_state == LOW) {
#ifdef DEBUG_SERIAL
      Serial.println("Led2 ON");
#endif
    } else {
#ifdef DEBUG_SERIAL
      Serial.println("Led2 off");
#endif
    }
  }

  mbsw_state = digitalRead(PIN_MBSW);   
  if (mbsw_state_prev != mbsw_state) {
    mbsw_state_prev = mbsw_state;
    if (mbsw_state == LOW) {
      send_alarm_last_breath();
#ifdef DEBUG_SERIAL
      Serial.println(FPSTR(msg_battery_low));
#endif
    } 
  }


}

void count_uptime() {
  uptime::calculateUptime();
  memset(str_uptime, 0, sizeof(str_uptime));
  sprintf( str_uptime, "%ud%uh%um%us", uptime::getDays(), uptime::getHours(), uptime::getMinutes(), uptime::getSeconds() );
}

bool is_button_pressed() {
  if ( digitalRead(BUTTON) == HIGH ) {
    return(false);
  }
  delay(2000);
  if ( digitalRead(BUTTON) == LOW ) {
    return(true);
  }
  return(false);
}

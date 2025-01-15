#define DBG_SERIAL  // because just "DBG" defined in PZEM004Tv30.h ( legacy :)
#define DBG_WIFI    // because "DEBUG_WIFI" defined in a WiFiClient library

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#include <EEPROM.h>

#include <SimpleCLI.h>  // https://github.com/SpacehuhnTech/SimpleCLI

#include "TickTwo.h"    // https://github.com/sstaub/TickTwo

#include "uptime.h"     // https://github.com/YiannisBourkelis/Uptime-Library

#include <microDS18B20.h>   // https://github.com/GyverLibs/microDS18B20/

#define PIN_MBSW        D1
#define PIN_PG          D2
#define BUTTON          D6

#define MAX_ALLOWED_INPUT 127
#define START_DELAY     10000

#define NO_BATTERY_VOLTAGE_THRESHOLD  3.0
#define LOW_BATTERY_COUNT_THRESHOLD   6
#define NO_BATTERY_COUNT_THRESHOLD    3

// DS18B20 sensor
MicroDS18B20<0> thermometer;   // D3

// Create CLI Object
SimpleCLI cli;

void count_uptime();
void usual_report();
void check_ups_status();
void check_wifi();
void check_battery_voltage();

// Create timers object
TickTwo timer1( count_uptime, 1000);
TickTwo timer2( check_ups_status, 100);
TickTwo timer3( check_battery_voltage, 20000);
TickTwo timer4( usual_report, 60000);
TickTwo timer5( check_wifi, 3600000);

byte external_power_state = HIGH;
byte external_power_state_prev = HIGH;
byte mbsw_state = LOW;
byte mbsw_state_prev = LOW;
bool last_breath_taken = false;
bool first_report = true;
bool enable_cli = false;
bool eeprom_bad = false;
bool wifi_not_connected = false;
bool standalone_mode = false;
uint8_t wifi_fail_check = 0;
float battery_voltage = 0;
bool no_battery = false;
int no_battery_count = 0;
bool low_battery = false;
int low_battery_count = 0;
int httpResponseCode = 0;
char str_uptime[17] = "0d0h0m0s";
char in_str[128] = {0};
char str_post[1024];

// EEPROM data
uint16_t mark = 0x55aa;
uint8_t standalone = 0;
char ups_name[33] = {0};
char ups_model[33] = {0};
char ssid[33] = {0};
char passw[65] = {0};
char host[65] = {0};
uint16_t port = 443;
char uri[128] = {0};
uint8_t http_auth = 0;
char http_user[33] = {0};
char http_passw[33] = {0};
float R1 = 1;
float R2 = 1;
float correction_value = 1;
float low_battery_voltage_threshold = 12.5;

#define PT_STANDALONE       sizeof(mark)
#define PT_UPS_NAME         PT_STANDALONE + sizeof(standalone)
#define PT_UPS_MODEL        PT_UPS_NAME + sizeof(ups_name)
#define PT_SSID             PT_UPS_MODEL + sizeof(ups_model)
#define PT_PASSW            PT_SSID + sizeof(ssid)
#define PT_HOST             PT_PASSW + sizeof(passw)
#define PT_PORT             PT_HOST + sizeof(host)
#define PT_URI              PT_PORT + sizeof(port)
#define PT_AUTH             PT_URI + sizeof(uri)
#define PT_HUSER            PT_AUTH + sizeof(http_auth)
#define PT_HPASSW           PT_HUSER + sizeof(http_user)
#define PT_R1               PT_HPASSW + sizeof(http_passw)
#define PT_R2               PT_R1 + sizeof(R1)
#define PT_CORR             PT_R2 + sizeof(R2)
#define PT_LV               PT_CORR + sizeof(correction_value)
#define PT_CRC              PT_LV + sizeof(low_battery_voltage_threshold)
#define SIZE_EEPROM         PT_LV + sizeof(low_battery_voltage_threshold) - 1 // PT_CRC d'not count

// Commands
Command cmdStandalone;
Command cmdUpsName;
Command cmdUpsModel;
Command cmdSsid;
Command cmdPassw;
Command cmdShow;
Command cmdHost;
Command cmdPort;
Command cmdUri;
Command cmdR1;
Command cmdR2;
Command cmdCorrection;
Command cmdLowVolt;
Command cmdHauth;
Command cmdHuser;
Command cmdHpassw;
Command cmdSave;
Command cmdReboot;
Command cmdHelp;


void setup() {
  PGM_P msg_bad_eeprom = PSTR("\nEEPROM error or bad config");

  Serial.begin(115200);
  delay(50);
#ifdef DBG_SERIAL
  Serial.println(".\nStart debugging serial");
#endif

  EEPROM.begin(2048);
#ifdef DBG_SERIAL
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
    SetSimpleCli();
    Serial.println("Usage:");
    Serial.println(cli.toString());
    if ( eeprom_bad ){
      Serial.println(FPSTR(msg_bad_eeprom));
    }
  }else{
    pinMode(PIN_PG, INPUT);
    pinMode(PIN_MBSW,  INPUT);
    pinMode(BUTTON,  INPUT);

    if ( eeprom_bad ) {
      Serial.println(FPSTR(msg_bad_eeprom));
      standalone = 1;
    }
 
    if ( standalone == 0 ) {
#ifdef DBG_SERIAL
      Serial.println("Enter to network mode");
#endif
      delay(START_DELAY);
      wifi_init();
    } else {
      standalone_mode = true;
#ifdef DBG_SERIAL
      Serial.println("Enter to standalone mode");
#endif
    }
    thermometer.requestTemp();
    timer1.start();
    timer2.start();
    timer3.start();
    timer4.start();
    if ( standalone == 0 ) {
      timer5.start();
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
  timer3.update();
  timer4.update();
  if ( standalone == 0 ) {
    timer5.update();
  }
}

void check_ups_status(){
  PGM_P msg_pwr_fail = PSTR("External power failed");
  PGM_P msg_pwr_restore = PSTR("External power restored");
  PGM_P msg_battery_disch = PSTR("Battery discharged");
  PGM_P msg_battery_chrg = PSTR("Battery is charging");

  // read and logging external power state
  external_power_state = digitalRead(PIN_PG);   
  if (external_power_state_prev != external_power_state) {
    external_power_state_prev = external_power_state;
    if (external_power_state == LOW) {
      send_alarm_ab_input( false );
      Serial.println(FPSTR(msg_pwr_fail));
    } else {
      send_alarm_ab_input( true );
      Serial.println(FPSTR(msg_pwr_restore));
      if ( last_breath_taken ) {
        Serial.println(FPSTR(msg_battery_chrg));
        last_breath_taken = false;
      }
    }
  }

  // read state of a MB_SW pin
  mbsw_state = digitalRead(PIN_MBSW);   
  if (mbsw_state_prev != mbsw_state) {
    mbsw_state_prev = mbsw_state;
    if (mbsw_state == HIGH) {
      if ( ! last_breath_taken ) {
        send_alarm_last_breath();
        Serial.println(FPSTR(msg_battery_disch));
        last_breath_taken = true;
      }
    } 
  }
}

void check_wifi() {
  PGM_P msg_conn_fail = PSTR("Reboot because WiFi is not connected");
#ifdef DBG_WIFI
  Serial.println("Check WiFi");
#endif
  if ( wifi_not_connected ) {
#ifdef DBG_WIFI
    Serial.println("Wifi not connected after boot - system reset");
#endif
    Serial.println(FPSTR(msg_conn_fail));
    ESP.reset();
  }
  if ( WiFi.status() != WL_CONNECTED ) {
#ifdef DBG_WIFI
    Serial.print("Wifi lost connection, check attempt ");  Serial.println(wifi_fail_check);
#endif
    if ( ++wifi_fail_check > 3 ) {
#ifdef DBG_WIFI
      Serial.print("Reset system because Wifi lost connection for hours");
#endif
      Serial.println(FPSTR(msg_conn_fail));
      ESP.reset();
    }
  } else {
    wifi_fail_check = 0;
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

void check_battery_voltage(){
  PGM_P msg_battery_low = PSTR("Battery low");
  PGM_P msg_battery_good = PSTR("Battery good");
  PGM_P msg_no_battery = PSTR("Battery disconnected");
  PGM_P msg_battery_conn = PSTR("Battery connected");
  if ( ( R2 > 0 ) and ( correction_value > 0 ) ) {
    battery_voltage = analogRead(A0) * (( R1 + R2 ) / R2 / correction_value ) / 1024;
  } else {
    return;
  }

  if ( no_battery ) { 
    if ( battery_voltage < low_battery_voltage_threshold ) {  // it's not mistake
      return;
    } else {
      no_battery_count--;
      if ( no_battery_count <= 0 ) {
        // send_alarm_ab_battery(3);
        Serial.println(FPSTR(msg_battery_conn));
        no_battery = false;
        no_battery_count = 0;
      }
    }
  } else {
    if ( battery_voltage < NO_BATTERY_VOLTAGE_THRESHOLD ) {
      no_battery_count++;
      if ( no_battery_count > NO_BATTERY_COUNT_THRESHOLD ) {
        // send_alarm_ab_battery(2);
        Serial.println(FPSTR(msg_no_battery));
        no_battery = true;
      }
    }
  }

  if ( low_battery_voltage_threshold > 0 ) {
    if ( low_battery ) {
     if ( battery_voltage > low_battery_voltage_threshold ) {
        low_battery_count--;
        if ( low_battery_count <= 0 ) {
          // send_alarm_ab_battery(1);
          Serial.println(FPSTR(msg_battery_good));
          low_battery = false;
          low_battery_count = 0;
        }
      }
    } else {
      if ( battery_voltage > low_battery_voltage_threshold ) {
        low_battery_count = 0;
      } else {
        low_battery_count++;
        if ( low_battery_count >= LOW_BATTERY_COUNT_THRESHOLD ) {
          send_alarm_ab_battery(0);
          Serial.println(FPSTR(msg_battery_low));
          low_battery = true;
        }
      }
    }
  }
}

void usual_report(){
  char str_batt[21] = {0};
  char str_power[10] = {0};
  char str_batt_volt[10] = {0};
  char str_degrees[10] = {0};
  char str_tmp[128];
  float temperature = -99;

  if ( thermometer.readTemp() ) { 
    temperature = thermometer.getTemp();
  } 
  dtostrf(temperature,1,1,str_degrees);
  thermometer.requestTemp();
  
  if ( external_power_state == HIGH ) {
    strncpy(str_power, "powerOk", sizeof(str_power)-1);
  } else {
    strncpy(str_power, "nopower", sizeof(str_power)-1);
  }
 
  if ( mbsw_state == HIGH ) {
    strncpy(str_batt, "batteryDischarged", sizeof(str_batt)-1);
  } else {
    if ( no_battery ) {
      strncpy(str_batt, "noBattery", sizeof(str_batt)-1);
    } else if ( low_battery ) {
      strncpy(str_batt, "batteryLow", sizeof(str_batt)-1);
    } else {
      strncpy(str_batt, "batteryOk", sizeof(str_batt)-1);
    }
  }
  
  dtostrf(battery_voltage,1,2,str_batt_volt);
  
  sprintf(str_tmp, "%s,%s,%s,%s", str_power, str_batt, str_degrees, str_batt_volt );
  Serial.println( str_tmp );
  
  if ( standalone_mode ) {
    return;
  }

  memset(str_tmp,0,sizeof(str_tmp));
  sprintf(str_tmp, "&data=%s,%s,%s,%d,%s,%s", str_power, str_batt, WiFi.localIP().toString().c_str(), WiFi.RSSI(), str_degrees, str_batt_volt );
  
  make_post_header();
  strncat(str_post, str_tmp, sizeof(str_post)-1);

#ifdef DBG_WIFI
  Serial.print("Prepared data: \""); Serial.print(str_post); Serial.println("\"");
#endif
  send_data();
}


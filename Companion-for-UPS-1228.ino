#define DBG_SERIAL  // because just "DBG" defined in PZEM004Tv30.h ( legacy :)
#define DBG_WIFI    // because "DEBUG_WIFI" defined in a WiFiClient library

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>  // https://github.com/hmueller01/pubsubclient3

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
#define UNSUCCESSFUL_ATTEMPTS_COUNT 30

#define NO_BATTERY_VOLTAGE_THRESHOLD  3.0
#define LOW_BATTERY_COUNT_THRESHOLD   6
#define NO_BATTERY_COUNT_THRESHOLD    3

WiFiClient espClient;
PubSubClient client(espClient);

// DS18B20 sensor
MicroDS18B20<0> thermometer;   // D3

// Create CLI Object
SimpleCLI cli;

WiFiEventHandler on_wifi_connect_handler;
WiFiEventHandler on_wifi_got_IP_handler;
WiFiEventHandler on_wifi_disconnect_handler;

void count_uptime();
void usual_report();
void check_ups_status();
void check_battery_voltage();

// Create timers object
TickTwo timer1( count_uptime, 1000);
TickTwo timer2( check_ups_status, 100);
TickTwo timer3( check_battery_voltage, 20000);
TickTwo timer4( usual_report, 60000);

byte external_power_state = HIGH;
byte external_power_state_prev = HIGH;
byte mbsw_state = LOW;
byte mbsw_state_prev = LOW;
bool dying_gasp_taken = false;
unsigned int unsucessfull_attempt = 0;
bool first_message_after_boot = true;
bool enable_cli = false;
bool eeprom_bad = false;
bool wifi_is_ok = false;
bool standalone_mode = false;
IPAddress mqtt_host_ip(IPADDR_NONE);
IPAddress prev_mqtt_host_ip(IPADDR_NONE);
uint16_t prev_mqtt_port = 1883;
float battery_voltage = 0;
bool no_battery = false;
int no_battery_count = 0;
bool low_battery = false;
int low_battery_count = 0;
char str_uptime[17] = "0d0h0m0s";
char in_str[128] = {0};
char topic_header[65] = {0};

// EEPROM data
uint16_t mark = 0x55aa;
uint8_t standalone = 0;
char ups_name[33] = {0};
char ups_model[33] = {0};
char ssid[33] = {0};
char passw[65] = {0};
unsigned int mqtt_host_resolving = 0; // Resolving mode: 0 - mDNS, 1 - DNS
char mqtt_host[33] = {0};     // hostname ( DNS or mDNS mode ) or IP ( DNS only mode ) of a MQTT server
uint16_t mqtt_port = 1883;
char mqtt_user[33] = {0};     // MQTT authentification parameters
char mqtt_passw[33] = {0};    // MQTT authentification parameters
char mqtt_prefix[33] ={0};  // MQTT topic beginning
float R1 = 1;
float R2 = 1;
float correction_value = 1;
float low_battery_voltage_threshold = 12.5;

#define PT_STANDALONE       sizeof(mark)
#define PT_UPS_NAME         PT_STANDALONE + sizeof(standalone)
#define PT_UPS_MODEL        PT_UPS_NAME + sizeof(ups_name)
#define PT_SSID             PT_UPS_MODEL + sizeof(ups_model)
#define PT_PASSW            PT_SSID + sizeof(ssid)
#define PT_DNS              PT_PASSW + sizeof(passw)
#define PT_HOST             PT_DNS + sizeof(mqtt_host_resolving)
#define PT_PORT             PT_HOST + sizeof(mqtt_host)
#define PT_MUSER            PT_PORT + sizeof(mqtt_port)
#define PT_MPASSW           PT_MUSER + sizeof(mqtt_user)
#define PT_MPREF            PT_MPASSW + sizeof(mqtt_passw)
#define PT_R1               PT_MPREF + sizeof(mqtt_prefix)
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
Command cmdDns;
Command cmdHost;
Command cmdPort;
Command cmdMuser;
Command cmdMpassw;
Command cmdPref;
Command cmdR1;
Command cmdR2;
Command cmdCorrection;
Command cmdLowVolt;
Command cmdShow;
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
      on_wifi_connect_handler = WiFi.onStationModeConnected(on_wifi_connect);
      on_wifi_got_IP_handler = WiFi.onStationModeGotIP(on_wifi_got_IP);
      on_wifi_disconnect_handler = WiFi.onStationModeDisconnected(on_wifi_disconnect);
      wifi_init();
      sprintf( topic_header, "%s/%s/", mqtt_prefix, ups_name );
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
      publish_msg_ab_input( false );
      Serial.println(FPSTR(msg_pwr_fail));
    } else {
      publish_msg_ab_input( true );
      Serial.println(FPSTR(msg_pwr_restore));
      if ( dying_gasp_taken ) {
        Serial.println(FPSTR(msg_battery_chrg));
        dying_gasp_taken = false;
      }
    }
  }

  // read state of a MB_SW pin
  mbsw_state = digitalRead(PIN_MBSW);   
  if (mbsw_state_prev != mbsw_state) {
    mbsw_state_prev = mbsw_state;
    if (mbsw_state == HIGH) {
      if ( ! dying_gasp_taken ) {
        send_dying_gasp();
        Serial.println(FPSTR(msg_battery_disch));
        dying_gasp_taken = true;
      }
    } 
  }
}

void count_uptime() {
  uptime::calculateUptime();
  memset(str_uptime, 0, sizeof(str_uptime));
  sprintf( str_uptime, "%ud%uh%um%us", uptime::getDays(), uptime::getHours(), uptime::getMinutes(), uptime::getSeconds() );
  if ( ( standalone_mode == 0 ) and wifi_is_ok ) {
    MDNS.update();
  }
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

// check is battery connected
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

// check battery level
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
          // send_alarm_ab_battery(0);
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
  
  sprintf(str_tmp, "%s,%s,%s,%s,%s", str_power, str_batt, str_degrees, str_batt_volt, str_uptime );
  Serial.println( str_tmp );
  
  if ( standalone_mode ) {
    return;
  }

  unsucessfull_attempt++;
  if ( wifi_is_ok and mqtt_setserver() and mqtt_publish(str_power, str_batt, str_degrees, str_batt_volt) ) {
    unsucessfull_attempt=0;
    return;
  }
  if ( unsucessfull_attempt > UNSUCCESSFUL_ATTEMPTS_COUNT ) {
    Serial.println(F("Too many unsucessfull attempts to publish, restart"));
    ESP.restart();
  }
}


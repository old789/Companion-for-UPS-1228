bool mqtt_setserver(){
  char mqtt_serv[] = "mqtt\0";
  char mqtt_proto[] = "tcp\0";
  unsigned int rc = 0;

  if ( mqtt_host_resolving == 1 ) {  
    // using a hostname as a MQTT server address
    client.setServer(mqtt_host, mqtt_port);
    return(true);
  } 
  // looking for a MQTT server IP in mDNS 
  rc = mdns_resolving(mqtt_serv,mqtt_proto,mqtt_host,&mqtt_host_ip,&mqtt_port);
  if ( rc != 1 ) {
    Serial.println(F("MQTT host not found"));
  }
  if (mqtt_host_ip == IPADDR_NONE){
    return(false);
  }
  if (mqtt_port == 0){
    return(false);
  }
  if ( mqtt_host_ip != prev_mqtt_host_ip or mqtt_port != prev_mqtt_port ) {
    client.setServer(mqtt_host_ip, mqtt_port);
    prev_mqtt_host_ip = mqtt_host_ip;
    prev_mqtt_port = mqtt_port;
    Serial.print(F("MQTT host "));
    Serial.print(mqtt_host_ip);
    Serial.print(":");
    Serial.println(mqtt_port);
  }
  return(true);
}

void publish_msg_ab_input( bool context ){
  char topic[256] = {0};
  char msg[256] = {0};
  if (!client.connect(ups_name, mqtt_user, mqtt_passw, NULL, 0, false, NULL, true)) {
    Serial.println(F("MQTT server not connected"));
    return;
  }
  if ( context ) {
    strncpy(msg, "powerOk", sizeof(msg)-1);
  } else {
    strncpy(msg, "nopower", sizeof(msg)-1);
  }
 
  sprintf( topic, "%sexternal_power", topic_header);
  if (!client.publish(topic, msg)) {
    Serial.print(F("Error publishing to "));
    Serial.println(topic);
  }
}

void send_dying_gasp(){
  char topic[256] = {0};
  char msg[256] = {0};
  if (!client.connect(ups_name, mqtt_user, mqtt_passw, NULL, 0, false, NULL, true)) {
    Serial.println(F("MQTT server not connected"));
    return;
  }
  sprintf( topic, "%s/system_events", mqtt_prefix);
  sprintf( msg, "%s dying gasp", ups_name );
  if (!client.publish(topic, msg)) {
    Serial.print(F("Error publishing to "));
    Serial.println(topic);
  }
}

bool mqtt_publish(char *str_power, char *str_batt, char *str_degrees, char *str_batt_volt){
  char topic[256] = {0};
  char msg[256] = {0};
  if (!client.connect(ups_name, mqtt_user, mqtt_passw, NULL, 0, false, NULL, true)) {
    Serial.println(F("MQTT server not connected"));
    return(false);
  }

  if ( first_message_after_boot ) {
    sprintf( topic, "%s/system_events", mqtt_prefix);
    sprintf( msg, "%s booted", ups_name );
    if (!client.publish(topic, msg)) {
      Serial.print(F("Error publishing to "));
      Serial.println(topic);
        return(false);
    }
    first_message_after_boot = false;
    memset( topic, 0, sizeof(topic) );
  }

  sprintf( topic, "%sexternal_power", topic_header);
  if (!client.publish(topic, str_power)) {
    Serial.print(F("Error publishing to "));
    Serial.println(topic);
    return(false);
  }
  
  memset( topic, 0, sizeof(topic) );
  sprintf( topic, "%sbattery/status", topic_header);
  if (!client.publish(topic, str_batt)) {
    Serial.print(F("Error publishing to "));
    Serial.println(topic);
    return(false);
  }
  
  memset( topic, 0, sizeof(topic) );
  sprintf( topic, "%stemp", topic_header);
  if (!client.publish(topic, str_degrees)) {
    Serial.print(F("Error publishing to "));
    Serial.println(topic);
    return(false);
  }

  memset( topic, 0, sizeof(topic) );
  sprintf( topic, "%sbattery/voltage", topic_header);
  if (!client.publish(topic, str_batt_volt)) {
    Serial.print(F("Error publishing to "));
    Serial.println(topic);
    return(false);
  }
  
  client.disconnect();
  return(true);
}


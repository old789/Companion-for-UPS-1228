void wifi_init(){
  Serial.print(F("Connecting to ")); Serial.print(ssid); Serial.println(F(" ..."));

  if ( WiFi.getMode() != WIFI_STA )
    WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, passw);             // Connect to the network
}

void on_wifi_connect(const WiFiEventStationModeConnected& event) {
  Serial.print(F("Connected to Wi-Fi sucessfully, "));
  Serial.print(F("RSSI: "));
  Serial.println(WiFi.RSSI());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}

void on_wifi_got_IP(const WiFiEventStationModeGotIP& event) {
  Serial.print(F("Obtained IP address: "));
  Serial.println(WiFi.localIP());
  wifi_is_ok = true;
  // Start mDNS
  if (!MDNS.begin(ups_name)) {             
    Serial.println(F("Error starting mDNS"));
  }else{
    Serial.println(F("mDNS started"));
    MDNS.addService(ups_name, "tcp", 8080);  // Announce tcp service on port 8080 ( just for informaton )
  }
}

void on_wifi_disconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.print(F("Disconnected from Wi-Fi, reason: "));
  Serial.println(event.reason);
  wifi_is_ok = false;
  WiFi.disconnect();
  wifi_init();
}

unsigned int mdns_resolving(char *service, char *proto, char *hostname, IPAddress *ip, uint16_t *port) {
  unsigned int rc = 0;
  int n = MDNS.queryService(service, proto);
  if (n > 0) {
    for (int i = 0; i < n; ++i) {
    /*  
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(MDNS.hostname(i));
      Serial.print(" (");
      Serial.print(MDNS.IP(i));
      Serial.print(":");
      Serial.print(MDNS.port(i));
      Serial.println(")");
    */  
      if ( strcmp(MDNS.hostname(i).c_str(), hostname) == 0 ) {
        *ip=MDNS.IP(i);
        *proto=MDNS.port(i);
        rc = 1;
        break;
      }
    }
  }
  MDNS.removeQuery();
  return(rc);
}

void make_post_header(){
  memset(str_post,0,sizeof(str_post));
  strncpy(str_post, "uptime=", sizeof(str_post)-1);
  strncat(str_post, str_uptime, sizeof(str_post)-1);
  
  if (strlen(ups_name) > 0) {
    strncat(str_post, "&name=", sizeof(str_post)-1);
    strncat(str_post, ups_name, sizeof(str_post)-1);
  } 
 
 if (strlen(ups_model) > 0) {
    strncat(str_post, "&model=", sizeof(str_post)-1);
    strncat(str_post, ups_model, sizeof(str_post)-1);
  }
  
  if ( first_report ) {
    strncat(str_post, "&boot=1", sizeof(str_post)-1);
  }
}  

void send_alarm_ab_input( bool wtf ){
  if ( standalone_mode ) {
      return;
  }
  make_post_header();
  if ( wtf ) {
    strncat(str_post, "&alarm=power restored", sizeof(str_post)-1);
  } else {
    strncat(str_post, "&alarm=no input voltage", sizeof(str_post)-1);
  }

#ifdef DBG_WIFI
  Serial.print("Alarm: \""); Serial.print(str_post); Serial.println("\"");
#endif
  send_data();
}

void send_alarm_last_breath() {
  if ( standalone_mode ) {
      return;
  }
  make_post_header();
  strncat(str_post, "&alarm=battery discharged, shutdown now", sizeof(str_post)-1);
  send_data();
#ifdef DBG_WIFI
  Serial.print("Alarm: \""); Serial.print(str_post); Serial.println("\"");
#endif
}

void send_data(){
  if ( standalone_mode ) {
#ifdef DBG_WIFI
    Serial.println("Standalone mode, do nothing");
#endif
      return;
  }

  if ( strlen(str_post) == 0 ){
#ifdef DBG_WIFI
    Serial.println("Nothing to send");
#endif
    return;
  }

  //Check WiFi connection status
  if(WiFi.status() != WL_CONNECTED){
#ifdef DBG_WIFI
    Serial.println("WiFi Disconnected");
#endif
    wifi_init();
  }
  
#ifdef DBG_WIFI
  Serial.println("Send data");
#endif

  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  // Ignore SSL certificate validation
  client->setInsecure();
  
  //Serial.println("HTTP client");
  HTTPClient http;

  //Serial.println("http begin");
  // Your Domain name with URL path or IP address with path
  // http.begin(client, serverName);
  http.begin(*client, host, port, uri);

  if ( http_auth > 0 ) {
    http.setAuthorization(http_user, http_passw);
  }
  
  //Serial.println("http header");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  //http.addHeader("Content-Type", "text/plain");
  //Serial.println("http post");
  httpResponseCode = http.POST(str_post);

#ifdef DBG_WIFI
  Serial.print("HTTP Response code: "); Serial.println(httpResponseCode);
#endif

  // Free resources
  http.end();
  
  if ( httpResponseCode == 200 ) {
    if ( first_report ) {
      first_report = false;
    }
  }
}

void wifi_init(){
  PGM_P msg_conn_fail = PSTR("Connection not established, falling to standalone mode");
  PGM_P msg_conn = PSTR("Connection established! ");
  PGM_P msg_ip = PSTR("IP address: ");
  PGM_P msg_rssi = PSTR("RSSI: ");
#ifdef DBG_WIFI
  Serial.print("Connecting to "); Serial.print(ssid); Serial.println(" ...");
#endif

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, passw);             // Connect to the network

  uint16_t i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(100);
    i++;
#ifdef DBG_WIFI
    Serial.print(i); Serial.print(' ');
#endif
    if ( i > 1500 ) {  // if don't connect - continue
      continue;
    }
  }
  
  if ( WiFi.status() != WL_CONNECTED ) {
    standalone_mode = true;
    wifi_not_connected = true;
#ifdef DBG_WIFI
    Serial.println('\n');
#endif
    Serial.println(FPSTR(msg_conn_fail));
    return;
  }

  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

#ifdef DBG_WIFI
  Serial.println('\n');
#endif
  Serial.print(FPSTR(msg_conn));
  Serial.print(FPSTR(msg_ip)); Serial.println(WiFi.localIP());
  Serial.print(FPSTR(msg_rssi)); Serial.println(WiFi.RSSI());
}

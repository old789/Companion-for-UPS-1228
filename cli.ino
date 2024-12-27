
void SetSimpleCli(){

  cmdStandalone = cli.addSingleArgCmd("standalone");
  cmdStandalone.setDescription(" Set standalone mode (0/1, 1=standlone)");

  cmdUpsName = cli.addSingleArgCmd("name");
  cmdUpsName.setDescription(" Set UPS name");

  cmdUpsModel = cli.addSingleArgCmd("model");
  cmdUpsModel.setDescription(" Set UPS model");

  cmdSsid = cli.addSingleArgCmd("ssid");
  cmdSsid.setDescription(" Set WiFi SSID");

  cmdPassw = cli.addSingleArgCmd("passw");
  cmdPassw.setDescription(" Set WiFi password");

  cmdHost = cli.addSingleArgCmd("host");
  cmdHost.setDescription(" Set destination host ( hostname or IPv4 )");

  cmdPort = cli.addSingleArgCmd("port");
  cmdPort.setDescription(" Set destination port");

  cmdUri = cli.addSingleArgCmd("uri");
  cmdUri.setDescription(" Set destination URI");

  cmdHauth = cli.addSingleArgCmd("auth");
  cmdHauth.setDescription(" Set HTTP(S) authorization (0/1, 1=enable)");

  cmdHuser = cli.addSingleArgCmd("huser");
  cmdHuser.setDescription(" Set HTTP(S) username");

  cmdHpassw = cli.addSingleArgCmd("hpassw");
  cmdHpassw.setDescription(" Set HTTP(S) password");

  cmdR1 = cli.addSingleArgCmd("R1");
  cmdR1.setDescription(" Set the resistance of a R1 resistor in a divider (kOhm, float)");

  cmdR2 = cli.addSingleArgCmd("R2");
  cmdR2.setDescription(" Set the resistance of a R2 resistor in a divider (kOhm, float)");

  cmdCorrection = cli.addSingleArgCmd("corr");
  cmdCorrection.setDescription(" Set the correction coefficient for the battery voltage (float)");

  cmdShow = cli.addSingleArgCmd("show");
  cmdShow.setDescription(" Show configuration");

  cmdSave = cli.addSingleArgCmd("save");
  cmdSave.setDescription(" Save configuration to EEPROM");

  cmdReboot = cli.addSingleArgCmd("reboot");
  cmdReboot.setDescription(" Reboot hard | soft");

  cmdHelp = cli.addSingleArgCmd("help");
  cmdHelp.setDescription(" Get help");

}


void  loop_cli_mode(){
  String input;
  const char emptyArg[] = "Argument is empty, do nothing";
  // uint8_t argNum = 0;
  uint8_t argLen = 0;

  Serial.print("> ");
  readStringWEcho(input, MAX_ALLOWED_INPUT);

  if (input.length() > 0) {
    cli.parse(input);
  }

  if (cli.available()) {
    Command c = cli.getCmd();
    // argNum = c.countArgs();
    argLen = c.getArg(0).getValue().length();
    unsigned int i = 0;
    float j = 0;

    if (c == cmdStandalone) {
      if ( argLen == 0 ) {
        Serial.println(emptyArg);
      }else{
        i = c.getArg(0).getValue().toInt();
        if ( i > 1 ) {
          Serial.println("Argument must be 0 or 1");
        }else{
          standalone = (uint8_t)i;
          if ( standalone == 1 )
            Serial.println("Standalone mode enabled");
          else
            Serial.println("Standalone mode disabled");
        }
      }
    } else if (c == cmdUpsName) {
      if ( argLen == 0 ) {
        Serial.println(emptyArg);
      }else{
        memset(ups_name, 0, sizeof(ups_name));
        c.getArg(0).getValue().toCharArray(ups_name, sizeof(ups_name)-1 );
        Serial.println("UPS name set to \"" + c.getArg(0).getValue() + "\"");
      }
    } else if (c == cmdUpsModel) {
      if ( argLen == 0 ) {
        Serial.println(emptyArg);
      }else{
        memset(ups_model, 0, sizeof(ups_model));
        c.getArg(0).getValue().toCharArray(ups_model, sizeof(ups_model)-1 );
        Serial.println("UPS model set to \"" + c.getArg(0).getValue() + "\"");
      }
    } else if (c == cmdSsid) {
      if ( argLen == 0 ) {
        Serial.println(emptyArg);
      }else{
        memset(ssid, 0, sizeof(ssid));
        c.getArg(0).getValue().toCharArray(ssid, sizeof(ssid)-1 );
        Serial.println("SSID set to \"" + c.getArg(0).getValue() + "\"");
      }
    } else if (c == cmdPassw) {
      if ( argLen == 0 ) {
        Serial.println(emptyArg);
      }else{
        memset(passw, 0, sizeof(passw));
        c.getArg(0).getValue().toCharArray(passw, sizeof(passw)-1 );
        Serial.println("WiFi password set to \"" + c.getArg(0).getValue() + "\"");
      }
    } else if (c == cmdHost) {
      if ( argLen == 0 ) {
        Serial.println(emptyArg);
      }else{
        memset(host, 0, sizeof(host));
        c.getArg(0).getValue().toCharArray(host, sizeof(host)-1 );
        Serial.println("Host is \"" + c.getArg(0).getValue() + "\"");
      }
    } else if (c == cmdPort) {
      if ( argLen == 0 ) {
        Serial.println(emptyArg);
      }else{
        port = c.getArg(0).getValue().toInt();
        Serial.println("Port set to \"" + c.getArg(0).getValue() + "\"");
      }
    } else if (c == cmdUri) {
      if ( argLen == 0 ) {
        Serial.println(emptyArg);
      }else{
        memset(uri, 0, sizeof(uri));
        c.getArg(0).getValue().toCharArray(uri, sizeof(uri)-1 );
        Serial.println("URI set to \"" + c.getArg(0).getValue() + "\"");
      }
    } else if (c == cmdHauth) {
      if ( argLen == 0 ) {
        Serial.println(emptyArg);
      }else{
        i = c.getArg(0).getValue().toInt();
        if ( i > 1 ) {
          Serial.println("Argument must be 0 or 1");
        }else{
          http_auth = (uint8_t)i;
          if ( http_auth == 1 )
            Serial.println("HTTP(S) authorization enabled");
          else
            Serial.println("HTTP(S) authorization disabled");
        }
      }
    } else if (c == cmdHuser) {
      if ( argLen == 0 ) {
        Serial.println(emptyArg);
      }else{
        memset(http_user, 0, sizeof(http_user));
        c.getArg(0).getValue().toCharArray(http_user, sizeof(http_user)-1 );
        Serial.println("HTTP(S) username set to \"" + c.getArg(0).getValue() + "\"");
      }
    } else if (c == cmdHpassw) {
      if ( argLen == 0 ) {
        Serial.println(emptyArg);
      }else{
        memset(http_passw, 0, sizeof(http_passw));
        c.getArg(0).getValue().toCharArray(http_passw, sizeof(http_passw)-1 );
        Serial.println("HTTP(S) password set to \"" + c.getArg(0).getValue() + "\"");
      }
    } else if (c == cmdR1) {
      if ( argLen == 0 ) {
        Serial.println(emptyArg);
      }else{
        j = c.getArg(0).getValue().toDouble();
        if ( j <= 0 ) {
          Serial.println("Argument must be greater then 0");
        }else{
          R1 = j;
          Serial.print("R1 set to \""); Serial.print(R1); Serial.println("\"");
        }
      }
    } else if (c == cmdR2) {
      if ( argLen == 0 ) {
        Serial.println(emptyArg);
      }else{
        j = c.getArg(0).getValue().toDouble();
        if ( j <= 0 ) {
          Serial.println("Argument must be greater then 0");
        }else{
          R2 = j;
          Serial.print("R2 set to \""); Serial.print(R1); Serial.println("\"");
        }
      }
    } else if (c == cmdCorrection) {
      if ( argLen == 0 ) {
        Serial.println(emptyArg);
      }else{
        j = c.getArg(0).getValue().toDouble();
        if ( j <= 0 ) {
          Serial.println("Argument must be greater then 0");
        }else{
          correction_value = j;
          Serial.print("Correction value set to \""); Serial.print(correction_value, 4); Serial.println("\"");
        }
      }
    } else if (c == cmdSave) {
      eeprom_save();
      Serial.println("Configuration saved to EEPROM");
    } else if (c == cmdShow) {
      if ( standalone == 1 )
        Serial.println("Standalone mode enabled");
      else
        Serial.println("Standalone mode disabled");
      Serial.print("UPS name = \"");Serial.print(ups_name);Serial.println("\"");
      Serial.print("UPS model = \"");Serial.print(ups_model);Serial.println("\"");
      Serial.print("WiFi SSID = \"");Serial.print(ssid);Serial.println("\"");
      Serial.print("WiFi password = \"");Serial.print(passw);Serial.println("\"");
      Serial.print("Host = \"");Serial.print(host);Serial.println("\"");
      Serial.print("Port = \"");Serial.print(port);Serial.println("\"");
      Serial.print("URI = \"");Serial.print(uri);Serial.println("\"");
      Serial.print("R1 = \"");Serial.print(R1);Serial.println("\"");
      Serial.print("R2 = \"");Serial.print(R2);Serial.println("\"");
      Serial.print("Corection = \"");Serial.print(correction_value, 4);Serial.println("\"");
      if ( http_auth > 0 )
        Serial.println("HTTP(S) authorization enabled");
      else
        Serial.println("HTTP(S) authorization disabled");
      Serial.print("HTTP(S) username = \"");Serial.print(http_user);Serial.println("\"");
      Serial.print("HTTP(S) password = \"");Serial.print(http_passw);Serial.println("\"");
    } else if (c == cmdReboot) {
      if ( ( argLen == 0 ) || c.getArg(0).getValue().equalsIgnoreCase("soft") ) {
        Serial.println("Reboot...");
        delay(3000);
        ESP.restart();
      }else if ( c.getArg(0).getValue().equalsIgnoreCase("hard") ){
        Serial.println("Reset...");
        delay(3000);
        ESP.reset();
      }else{
        Serial.println("Unknown argument, allowed only \"hard\" or \"soft\"");
      }
    } else if (c == cmdHelp) {
      Serial.println("Help:");
      Serial.println(cli.toString());
    }

  }

  if (cli.errored()) {
    CommandError cmdError = cli.getError();

    Serial.print("ERROR: ");
    Serial.println(cmdError.toString());

    if (cmdError.hasCommand()) {
      Serial.print("Did you mean \"");
      Serial.print(cmdError.getCommand().toString());
      Serial.println("\"?");
    }
  }
}


void readStringWEcho(String& input, size_t char_limit) { // call with char_limit == 0 for no limit
  for(;;) {
    if (Serial.available()) {
      char c = Serial.read();
      if ((uint8_t)c == 8) {
        if ( input.length() ) {
          clearString(input.length());
          input.remove(input.length()-1);
          Serial.print(input);
        }
        continue;
      }
      if ( ((uint8_t)c == 10) || ((uint8_t)c == 13) ){
        Serial.println();
        return;
      }
      if ( ((uint8_t)c < 32) || ((uint8_t)c > 126)) {
        Serial.print((char)7);
        continue;
      }
      input += c;
      Serial.print(c);
      if (char_limit && (input.length() >= char_limit)) {
        return;
      }
    }
  }
}

void clearString( uint16_t len ){
  char stmp[MAX_ALLOWED_INPUT+7];
  memset(stmp+1,' ',len+2);
  stmp[0]='\r';
  stmp[len+3]='\r';
  stmp[len+4]='>';
  stmp[len+5]=' ';
  stmp[len+6]=0;
  Serial.print(stmp);
}

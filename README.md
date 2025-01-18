# Smart companion for a dumb UPS module
 Node MCU smart companion for the module UPS-1228 and similar

### Features
- Based on a ESP8266 Node MCU.
- Compatible with the the module UPS-1228 and similar like Proline EF6012/EF9012 or Mean Well PSC-35/60/100
- Monitor an external power state, battery voltage, and temperature 
- Send data to the remote server via HTTPS ( configurable ).
- Send events over the serial interface to inform a local device.
- Command line interface for the configuring.
- An example of the remote server is in a repository 'APC_UPS_controller'

### Command line
To enter Commadline mode, you need to press the button for 2 seconds during booting.
Communication parameters for terminal: 115200,8N1.

| Command *arg* | Explanation |
| --- | --- |
| standalone *digit* | Set standalone mode (0/1, 1=standlone). In this mode network disabled |
| name *word* | Set UPS name, used as a device identifier on the remote server |
| ssid *word* | Set WiFi SSID |
| passw *word* | Set WiFi password |
| host *word* | Set destination host ( hostname or IPv4 ) |
| port *number* | Set destination port |
| uri *word* | Set destination URI ( like "/cgi-bin/ups.cgi" ) |
| auth *word* | Set HTTP(S) authorization (0/1, 1=enable) |
| huser *word* | Set HTTP(S) username |
| hpassw *word* | Set HTTP(S) password|
| R1 *float*| Set *full* resistance of a resistor R1 in a voltage divider for ADC (kOhm)|
| R2 *float*| Set *full* resistance of a resistor R2 in a voltage divider for ADC (kOhm)|
| corr *float*| Set the correction coefficient for the battery voltage (like 1.033)|
| low *float* | Set low battery voltage ( 0 - disable, volts, ~12.55 for LiFePo, ~11.2 for LeadAcid ) |
| show | Show current configuration |
| save | Save configuration to EEPROM |
| reboot [ *hard* or *soft* ] | Reboot, *hard* doing ESP.reset(), *soft* doing ESP.restart(), default is *soft* |
| help | Get help |

### Transmitting data to the remote side
Data are transmitted in the POST request.

| Field | Value |
| --- | --- |
| uptime | Uptime in form "1d1h01m1s", not very accurate |
| name | Name of the device which set via the command line |
| model | UPS model which UPS reported |
| boot | always "1", this is 1st  message after boot |
| alarm | Emergency messages like a main power failure |
| msg | Informational messages |
| data | Comma separated lists of main parameters: external power state, battery state, battery voltage (float), temperature (float)|




/****************************************************************************************************************************
 * BlynkSimpleESP32_WFM.h
 * For ESP32 using WiFiManager and WiFi along with BlueTooth / BLE
 *
 * Library for inclusion of both ESP32 Blynk BT and WiFi libraries. Then select one at runtime.
 * Forked from Blynk library v0.6.1 https://github.com/blynkkk/blynk-library/releases
 * Built by Khoi Hoang https://github.com/khoih-prog/BlynkGSM_ESPManager
 * Licensed under MIT license
 * Version: 1.0.2
 *
 * Original Blynk Library author:
 * @file       BlynkSimpleESP32.h
 * @author     Volodymyr Shymanskyy
 * @license    This project is released under the MIT License (MIT)
 * @copyright  Copyright (c) 2015 Volodymyr Shymanskyy
 * @date       Oct 2016
 * @brief
 *
 * Version Modified By   Date      Comments
 * ------- -----------  ---------- -----------
 *  1.0.0   K Hoang      25/01/2020 Initial coding
 *  1.0.1   K Hoang      27/01/2020 Enable simultaneously running BT/BLE and WiFi
 *  1.0.2   K Hoang      04/02/2020 Add Blynk WiFiManager support similar to Blynk_WM library
 *****************************************************************************************************************************/

#ifndef BlynkSimpleEsp32_WFM_h
#define BlynkSimpleEsp32_WFM_h

#ifndef ESP32
#error This code is intended to run on the ESP32 platform! Please check your Tools->Board setting.
#endif

#define BLYNK_SEND_ATOMIC

// KH
#define BLYNK_TIMEOUT_MS     30000UL

#include <BlynkApiArduino.h>
#include <Blynk/BlynkProtocol.h>
#include <Adapters/BlynkArduinoClient.h>
#include <WiFi.h>

#include <WebServer.h>

//default to use EEPROM, otherwise, use SPIFFS
#if USE_SPIFFS
#include <FS.h>
#include "SPIFFS.h"
#else
#include <EEPROM.h>
#endif

#include <esp_wifi.h>
#define ESP_getChipId()   ((uint32_t)ESP.getEfuseMac())

// Configurable items besides fixed Header
#define NUM_CONFIGURABLE_ITEMS    8
typedef struct Configuration 
{
    char header         [16];
    char wifi_ssid      [32];
    char wifi_pw        [32];
    char blynk_server   [32];
    int  blynk_port;
    char blynk_token    [36];
    char blynk_bt_tk    [36];
    char blynk_ble_tk   [36];
    char board_name     [24];
} Blynk_WF_Configuration;

// Currently CONFIG_DATA_SIZE  =   248
uint16_t CONFIG_DATA_SIZE = sizeof(Blynk_WF_Configuration);

#define root_html_template " \
<!DOCTYPE html> \
<meta name=\"robots\" content=\"noindex\"> \
<html> \
<head> \
<meta charset=\"utf-8\"> \
<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> \
<title>Blynk_Esp32_BT_BLE_WF</title> \
</head> \
<body> \
<div align=\"center\"> \
<table> \
<tbody> \
<tr> \
<th colspan=\"2\">WiFi</th> \
</tr> \
<tr> \
<td>SSID</td> \
<td><input type=\"text\" value=\"[[wf_id]]\" size=20 maxlength=64 id=\"wf_id\"></td> \
</tr> \
<tr> \
<td>Password</td> \
<td><input type=\"text\" value=\"[[wf_pw]]\" size=20 maxlength=64 id=\"wf_pw\"></td> \
</tr> \
<tr> \
<th colspan=\"2\">Blynk</th> \
</tr> \
<tr> \
<td>Server</td> \
<td><input type=\"text\" value=\"[[b_svr]]\" size=20 maxlength=64 id=\"b_svr\"></td> \
</tr> \
<tr> \
<td>Port</td> \
<td><input type=\"text\" value=\"[[b_pt]]\" id=\"b_pt\"></td> \
</tr> \
<tr> \
<td>Token</td> \
<td><input type=\"text\" value=\"[[b_tok]]\" size=20 maxlength=32 id=\"b_tok\"></td> \
</tr> \
<tr> \
<td>BT Token</td> \
<td><input type=\"text\" value=\"[[b_bt_tk]]\" size=20 maxlength=32 id=\"b_bt_tk\"></td> \
</tr> \
<tr> \
<td>BLE Token</td> \
<td><input type=\"text\" value=\"[[b_ble_tk]]\" size=20 maxlength=32 id=\"b_ble_tk\"></td> \
</tr> \
<tr> \
<th colspan=\"2\">Hardware</th> \
</tr> \
<tr> \
<td>Name</td> \
<td><input type=\"text\" value=\"[[bd_nm]]\" size=20 maxlength=32 id=\"bd_nm\"></td> \
</tr> \
<tr> \
<td colspan=\"2\" align=\"center\"> \
<button onclick=\"save()\">Save</button> \
</td> \
</tr> \
</tbody> \
</table> \
</div> \
<script id=\"jsbin-javascript\"> \
function udVal(key, value) { \
var request = new XMLHttpRequest(); \
var url = '/?key=' + key + '&value=' + value; \
console.log('call ' + url + '...'); \
request.open('GET', url, false); \
request.send(null); \
} \
function save() { \
udVal('wf_id', document.getElementById('wf_id').value); \
udVal('wf_pw', document.getElementById('wf_pw').value); \
udVal('b_svr', document.getElementById('b_svr').value); \
udVal('b_pt', document.getElementById('b_pt').value); \
udVal('b_tok', document.getElementById('b_tok').value); \
udVal('b_bt_tk', document.getElementById('b_bt_tk').value); \
udVal('b_ble_tk', document.getElementById('b_ble_tk').value); \
udVal('bd_nm', document.getElementById('bd_nm').value); \
alert('Updated. Reset'); \
} \
</script> \
</body> \
</html>"

#define BLYNK_SERVER_HARDWARE_PORT    8080

#define BOARD_TYPE      "ESP32_WFM"
#define NO_CONFIG       "nothing"

class BlynkWifi
    : public BlynkProtocol<BlynkArduinoClient>
{
    typedef BlynkProtocol<BlynkArduinoClient> Base;
public:
    BlynkWifi(BlynkArduinoClient& transp)
        : Base(transp)
    {}

    void connectWiFi(const char* ssid, const char* pass)
    {
        BLYNK_LOG2(BLYNK_F("Connecting to "), ssid);
        WiFi.mode(WIFI_STA);
        
        // New from Blynk_WM v1.0.5
        if (static_IP != IPAddress(0, 0, 0, 0))
        {
          BLYNK_LOG1(BLYNK_F("Use static IP"));
          WiFi.config(static_IP, static_GW, static_SN, static_DNS1, static_DNS2);         
        }        
        setHostname();
        
        if (pass && strlen(pass)) {
            WiFi.begin(ssid, pass);
        } else {
            WiFi.begin(ssid);
        }
        while (WiFi.status() != WL_CONNECTED) {
            BlynkDelay(500);
        }
        BLYNK_LOG1(BLYNK_F("Connected to WiFi"));

        BLYNK_LOG6(BLYNK_F("IP = "), WiFi.localIP().toString(), BLYNK_F(", GW = "), WiFi.gatewayIP().toString(), 
                   BLYNK_F(", SN = "), WiFi.subnetMask().toString());
        BLYNK_LOG4(BLYNK_F("DNS1 = "), WiFi.dnsIP(0).toString(), BLYNK_F(", DNS2 = "), WiFi.dnsIP(1).toString());
    }

    void config(const char* auth,
                const char* domain = BLYNK_DEFAULT_DOMAIN,
                uint16_t    port   = BLYNK_DEFAULT_PORT)
    {
        Base::begin(auth);
        this->conn.begin(domain, port);
    }

    void config(const char* auth,
                IPAddress   ip,
                uint16_t    port = BLYNK_DEFAULT_PORT)
    {
        Base::begin(auth);
        this->conn.begin(ip, port);
    }

    void begin(const char* auth,
               const char* ssid,
               const char* pass,
               const char* domain = BLYNK_DEFAULT_DOMAIN,
               uint16_t    port   = BLYNK_DEFAULT_PORT)
    {
        connectWiFi(ssid, pass);
        config(auth, domain, port);
        while(this->connect() != true) {}
    }

    void begin(const char* auth,
               const char* ssid,
               const char* pass,
               IPAddress   ip,
               uint16_t    port   = BLYNK_DEFAULT_PORT)
    {
        connectWiFi(ssid, pass);
        config(auth, ip, port);
        while(this->connect() != true) {}
    }
   
    #ifndef LED_BUILTIN
    #define LED_BUILTIN       2         // Pin D2 mapped to pin GPIO2/ADC12 of ESP32, control on-board LED
    #endif
        
		void begin(const char *iHostname = "")
    {
        #define TIMEOUT_CONNECT_WIFI			30000

        //Turn OFF
        pinMode(LED_BUILTIN, OUTPUT);
        digitalWrite(LED_BUILTIN, LOW);
        
				if (iHostname[0] == 0)
				{
					#ifdef ESP8266
						String _hostname = "ESP8266-" + String(ESP.getChipId(), HEX);
					#else		//ESP32
						String _hostname = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
					#endif
					_hostname.toUpperCase();

					getRFC952_hostname(_hostname.c_str());		
					
				}
				else
				{
					// Prepare and store the hostname only not NULL
					getRFC952_hostname(iHostname);
				}

				BLYNK_LOG2(BLYNK_F("RFC925 Hostname = "), RFC952_hostname);        
               
        if (getConfigData())
        {
          hadConfigData = true;

          Base::begin(BlynkESP32_WM_config.blynk_token);
          this->conn.begin(BlynkESP32_WM_config.blynk_server, BlynkESP32_WM_config.blynk_port);

          if (connectToWifi(TIMEOUT_CONNECT_WIFI)) 
          {
            BLYNK_LOG1(BLYNK_F("bg: WiFi connected. Try Blynk"));
            
            int i = 0;
            while ( (i++ < 10) && !this->connect() )
            {
            }
            
            if  (this->connected())
            {
              BLYNK_LOG1(BLYNK_F("bg: WiFi+Blynk connected"));
            }
            else 
            {
              BLYNK_LOG1(BLYNK_F("bg: WiFi connected, Blynk not"));
              // failed to connect to Blynk server, will start configuration mode
              startConfigurationMode();
            }
          } 
          else 
          {
              BLYNK_LOG1(BLYNK_F("bg: Fail2connect WiFi+Blynk"));
              // failed to connect to Blynk server, will start configuration mode
              startConfigurationMode();
          }
        }
        else
        {
            BLYNK_LOG1(BLYNK_F("bg: No configdat. Stay forever in config portal"));
            // failed to connect to Blynk server, will start configuration mode
            hadConfigData = false;
            startConfigurationMode();                  
        }        
    }    

#ifndef TIMEOUT_RECONNECT_WIFI
  #define TIMEOUT_RECONNECT_WIFI   10000L
#else
  // Force range of user-defined TIMEOUT_RECONNECT_WIFI between 10-60s
  #if (TIMEOUT_RECONNECT_WIFI < 10000L)
    #warning TIMEOUT_RECONNECT_WIFI too low. Reseting to 10000
    #undef TIMEOUT_RECONNECT_WIFI
    #define TIMEOUT_RECONNECT_WIFI   10000L
  #elif (TIMEOUT_RECONNECT_WIFI > 60000L)
    #warning TIMEOUT_RECONNECT_WIFI too high. Reseting to 60000
    #undef TIMEOUT_RECONNECT_WIFI
    #define TIMEOUT_RECONNECT_WIFI   60000L
  #endif
#endif

#ifndef RESET_IF_CONFIG_TIMEOUT
  #define RESET_IF_CONFIG_TIMEOUT   true
#endif

#ifndef CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET
  #define CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET          10
#else
  // Force range of user-defined TIMES_BEFORE_RESET between 2-100
  #if (CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET < 2)
    #warning CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET too low. Reseting to 2
    #undef CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET
    #define CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET   2
  #elif (CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET > 100)
    #warning CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET too high. Reseting to 100
    #undef CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET
    #define CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET   100
  #endif
#endif
   
    void run()
    {
      static int retryTimes = 0;
      
      // Lost connection in running. Give chance to reconfig.
      if ( WiFi.status() != WL_CONNECTED || !connected() )
      {   
        // If configTimeout but user hasn't connected to configWeb => try to reconnect WiFi / Blynk. 
        // But if user has connected to configWeb, stay there until done, then reset hardware
		    if ( configuration_mode && ( configTimeout == 0 ||  millis() < configTimeout ) )
		    {
		      retryTimes = 0;
		      
		      if (server)
			      server->handleClient();		
			      
			    return;
		    }
		    else
		    {
		      #if RESET_IF_CONFIG_TIMEOUT
		      // If we're here but still in configuration_mode, permit running TIMES_BEFORE_RESET times before reset hardware
		      // to permit user another chance to config.
		      if ( configuration_mode && (configTimeout != 0) )
		      {	        
		        if (++retryTimes <= CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET)
		        {
		          BLYNK_LOG2(BLYNK_F("run: WiFi lost & config Timeout. Connect WiFi+Blynk. Retry# : "), retryTimes);
		        }
		        else
		        {
              ESP.restart();
		        }		        
		      }
		      #endif
		      		    
			    // Not in config mode, try reconnecting before force to config mode
			    if ( WiFi.status() != WL_CONNECTED )
			    {
				    BLYNK_LOG1(BLYNK_F("run: WiFi lost. Reconnect WiFi+Blynk"));
				    if (connectToWifi(TIMEOUT_RECONNECT_WIFI)) 
				    {
				      // turn the LED_BUILTIN OFF to tell us we exit configuration mode.
              digitalWrite(LED_BUILTIN, LOW);

				      BLYNK_LOG1(BLYNK_F("run: WiFi reconnected. Connect to Blynk"));
				      
				      if (connect())
				      {
					      BLYNK_LOG1(BLYNK_F("run: WiFi+Blynk reconnected"));
					    }
				    }
			    }
			    else
			    {
				    BLYNK_LOG1(BLYNK_F("run: Blynk lost. Connect Blynk"));
				    if (connect()) 
				    {
				      // turn the LED_BUILTIN OFF to tell us we exit configuration mode.
              digitalWrite(LED_BUILTIN, LOW);
              
					    BLYNK_LOG1(BLYNK_F("run: Blynk reconnected"));
				    }
			    }
					
			    //BLYNK_LOG1(BLYNK_F("run: Lost connection => configMode"));
			    //startConfigurationMode();
        }
      }
      else if (configuration_mode)
      {
      	configuration_mode = false;
      	BLYNK_LOG1(BLYNK_F("run: got WiFi+Blynk back, great"));
      	// Turn the LED_BUILTIN OFF when out of configuration mode. ESP32 LED_BUILDIN is correct polarity, LOW to turn OFF
        digitalWrite(LED_BUILTIN, LOW);      	
      }

      if (connected())
      {
        Base::run();
      }
    }
        
		void setHostname(void)
		{
			if (RFC952_hostname[0] != 0)
			{
        // See https://github.com/espressif/arduino-esp32/issues/2537
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
        WiFi.setHostname(RFC952_hostname);
			}
		}    

		void setConfigPortalIP(IPAddress portalIP = IPAddress(192, 168, 4, 1))
		{
			portal_apIP = portalIP;
		}
		
		void setConfigPortal(String ssid = "", String pass = "")
		{
			portal_ssid = ssid;
			portal_pass = pass;
		}		

    void setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn = IPAddress(255, 255, 255, 0), 
                              IPAddress dns_address_1 = IPAddress(0, 0, 0, 0), 
                              IPAddress dns_address_2 = IPAddress(0, 0, 0, 0))
    {
      static_IP     = ip;
      static_GW     = gw;
      static_SN     = sn;
      
      // Default to local GW
      if (dns_address_1 == IPAddress(0, 0, 0, 0))
        static_DNS1   = gw;
      else
        static_DNS1   = dns_address_1;
        
      // Default to Google DNS (8, 8, 8, 8)  
      if (dns_address_2 == IPAddress(0, 0, 0, 0))
        static_DNS2   = IPAddress(8, 8, 8, 8);
      else
        static_DNS2   = dns_address_2;
    }
    
    String getServerName()
    {
      if (!hadConfigData)
        getConfigData();

      return (String(BlynkESP32_WM_config.blynk_server));
    }
                       
    String getToken()
    {
      if (!hadConfigData)
        getConfigData();
            
      return (String(BlynkESP32_WM_config.blynk_token));
    }
    
    String getBlynkBTToken(void)
    {
      if (!hadConfigData)
        getConfigData();
               
      return (String(BlynkESP32_WM_config.blynk_bt_tk));
    }

    String getBlynkBLEToken(void)
    {
      if (!hadConfigData)
        getConfigData();
        
      return (String(BlynkESP32_WM_config.blynk_ble_tk));
    }
    
    String getBoardName()
    {
      if (!hadConfigData)
        getConfigData();

      return (String(BlynkESP32_WM_config.board_name));
    }
    
    int getHWPort()
    {
      if (!hadConfigData)
        getConfigData();

      return (BlynkESP32_WM_config.blynk_port);
    }    
          
    
    Blynk_WF_Configuration* getFullConfigData(Blynk_WF_Configuration *configData)
    {
      if (!hadConfigData)
        getConfigData();
        
      // Check if NULL pointer
      if (configData)
        memcpy(configData, &BlynkESP32_WM_config, sizeof(Blynk_WF_Configuration));

      return (configData);
    }
                              
private:
    WebServer *server;
    bool configuration_mode = false;
    
    unsigned long configTimeout;
    bool hadConfigData = false; 
    
    Blynk_WF_Configuration BlynkESP32_WM_config;

    // For Config Portal, from Blynk_WM v1.0.5     
    IPAddress portal_apIP = IPAddress(192, 168, 4, 1);
    
    String portal_ssid = "";
    String portal_pass = "";
    
    // For static IP, from Blynk_WM v1.0.5 
    IPAddress static_IP   = IPAddress(0, 0, 0, 0);
    IPAddress static_GW   = IPAddress(0, 0, 0, 0);
    IPAddress static_SN   = IPAddress(255, 255, 255, 0);
    IPAddress static_DNS1 = IPAddress(0, 0, 0, 0);
    IPAddress static_DNS2 = IPAddress(0, 0, 0, 0);
   
		#define RFC952_HOSTNAME_MAXLEN      24
		char RFC952_hostname[RFC952_HOSTNAME_MAXLEN + 1];

		char* getRFC952_hostname(const char* iHostname)
		{ 
			memset(RFC952_hostname, 0, sizeof(RFC952_hostname));
			
			size_t len = ( RFC952_HOSTNAME_MAXLEN < strlen(iHostname) ) ? RFC952_HOSTNAME_MAXLEN : strlen(iHostname);
			
			size_t j = 0;
			
			for (size_t i = 0; i < len - 1; i++)
			{
				if ( isalnum(iHostname[i]) || iHostname[i] == '-' )
				{
				  RFC952_hostname[j] = iHostname[i];
				  j++;
				}  
			}
			// no '-' as last char
			if ( isalnum(iHostname[len - 1]) || (iHostname[len - 1] != '-') )
				RFC952_hostname[j] = iHostname[len - 1];

			return RFC952_hostname;
		}  
		
		void displayConfigData(void)
		{
		    BLYNK_LOG6(BLYNK_F("Header = "), BlynkESP32_WM_config.header, BLYNK_F(", SSID = "), BlynkESP32_WM_config.wifi_ssid, 
                   BLYNK_F(", PW = "),     BlynkESP32_WM_config.wifi_pw);
        BLYNK_LOG6(BLYNK_F("Server = "), BlynkESP32_WM_config.blynk_server, BLYNK_F(", Port = "), BlynkESP32_WM_config.blynk_port, 
                   BLYNK_F(", Token = "),  BlynkESP32_WM_config.blynk_token);
        BLYNK_LOG4(BLYNK_F("BT Token = "), BlynkESP32_WM_config.blynk_bt_tk, BLYNK_F(", BLE Token = "), BlynkESP32_WM_config.blynk_ble_tk);       
        BLYNK_LOG2(BLYNK_F("Board Name = "), BlynkESP32_WM_config.board_name);   
		}     

#if USE_SPIFFS     

    #define  CONFIG_FILENAME         BLYNK_F("/wfm_config.dat")
    #define  CONFIG_FILENAME_BACKUP  BLYNK_F("/wfm_config.bak")

    void loadConfigData(void)
    {
      File file = SPIFFS.open(CONFIG_FILENAME, "r");
      BLYNK_LOG1(BLYNK_F("Load configfile "));
      
      if (!file) 
      {
        BLYNK_LOG1(BLYNK_F("failed"));
        
        // Trying open redundant config file
        file = SPIFFS.open(CONFIG_FILENAME_BACKUP, "r");
        BLYNK_LOG1(BLYNK_F("Load backup configfile ")); 
        
        if (!file)
        {
          BLYNK_LOG1(BLYNK_F("also failed"));
          return;
        }
      }
        
      file.readBytes((char *) &BlynkESP32_WM_config, sizeof(BlynkESP32_WM_config));
           
      BLYNK_LOG1(BLYNK_F("OK"));
      file.close();    
    }

    void saveConfigData(void)
    {
      File file = SPIFFS.open(CONFIG_FILENAME, "w");
      BLYNK_LOG1(BLYNK_F("Save configfile "));
      
      if (file) 
      {
        file.write((uint8_t*) &BlynkESP32_WM_config, sizeof(BlynkESP32_WM_config));  
        file.close();     
        BLYNK_LOG1(BLYNK_F("OK"));
      }
      else
      {
        BLYNK_LOG1(BLYNK_F("failed"));        
      }  
      
      // Trying open redundant Auth file
      file = SPIFFS.open(CONFIG_FILENAME_BACKUP, "w");
      BLYNK_LOG1(BLYNK_F("Save backup configfile ")); 
      
      if (file)
      {
        file.write((uint8_t *) &BlynkESP32_WM_config, sizeof(BlynkESP32_WM_config));
        file.close();  
        BLYNK_LOG1(BLYNK_F("OK"));      
      }
      else
      {
        BLYNK_LOG1(BLYNK_F("failed"));       
      }           
    }
    
    // Return false if init new EEPROM or SPIFFS. No more need trying to connect. Go directly to config mode 
    bool getConfigData()
    {     
      if (!SPIFFS.begin()) 
      {
        BLYNK_LOG1(BLYNK_F("SPIFFS failed! Pls use EEPROM."));
        return false;
      }
      
      if ( SPIFFS.exists(CONFIG_FILENAME) || SPIFFS.exists(CONFIG_FILENAME_BACKUP) )
      { 
        // if config file exists, load
        loadConfigData();
      }

      displayConfigData();
      
      if (strncmp(BlynkESP32_WM_config.header, BOARD_TYPE, strlen(BOARD_TYPE)) != 0) 
      {
          memset(&BlynkESP32_WM_config, 0, sizeof(BlynkESP32_WM_config));
                                   
          BLYNK_LOG2(BLYNK_F("Init new configfile, size = "), sizeof(BlynkESP32_WM_config));          
          // doesn't have any configuration
          strcpy(BlynkESP32_WM_config.header,           BOARD_TYPE);
          strcpy(BlynkESP32_WM_config.wifi_ssid,        NO_CONFIG);
          strcpy(BlynkESP32_WM_config.wifi_pw,          NO_CONFIG);
          strcpy(BlynkESP32_WM_config.blynk_server,     NO_CONFIG);
          BlynkESP32_WM_config.blynk_port = BLYNK_SERVER_HARDWARE_PORT;
          strcpy(BlynkESP32_WM_config.blynk_token,      NO_CONFIG);
          strcpy(BlynkESP32_WM_config.blynk_bt_tk,      NO_CONFIG);
          strcpy(BlynkESP32_WM_config.blynk_ble_tk,     NO_CONFIG);
          strcpy(BlynkESP32_WM_config.board_name,       NO_CONFIG);
          
          saveConfigData();
          
          return false;        
      }
      else if ( !strncmp(BlynkESP32_WM_config.wifi_ssid,        NO_CONFIG, strlen(NO_CONFIG))   ||
                !strncmp(BlynkESP32_WM_config.wifi_pw,          NO_CONFIG, strlen(NO_CONFIG) )  ||
                !strncmp(BlynkESP32_WM_config.blynk_server,     NO_CONFIG, strlen(NO_CONFIG) )  ||
                !strncmp(BlynkESP32_WM_config.blynk_token,      NO_CONFIG, strlen(NO_CONFIG) )  ||
                !strncmp(BlynkESP32_WM_config.blynk_bt_tk,      NO_CONFIG, strlen(NO_CONFIG) )  ||
                !strncmp(BlynkESP32_WM_config.blynk_ble_tk,     NO_CONFIG, strlen(NO_CONFIG) ) ) 
      {
        // If SSID, PW, Server,Token ="nothing", stay in config mode forever until having config Data.
        return false;
      }  
      else
      {
        displayConfigData();        
      }
      
      return true;
    }
     

#else

#ifndef EEPROM_SIZE
  #define EEPROM_SIZE     512
#else
  #if (EEPROM_SIZE > 2048)
    #warning EEPROM_SIZE must be <= 2048. Reset to 2048
    #undef EEPROM_SIZE
    #define EEPROM_SIZE     2048
  #endif
  #if (EEPROM_SIZE < CONFIG_DATA_SIZE)
    #warning EEPROM_SIZE must be > CONFIG_DATA_SIZE. Reset to 512
    #undef EEPROM_SIZE
    #define EEPROM_SIZE     512
  #endif  
#endif  

#ifndef EEPROM_START
  #define EEPROM_START     0
#else
  #if (EEPROM_START + CONFIG_DATA_SIZE > EEPROM_SIZE)
    #error EPROM_START + CONFIG_DATA_SIZE > EEPROM_SIZE. Please adjust.
  #endif
#endif  

    // Return false if init new EEPROM or SPIFFS. No more need trying to connect. Go directly to config mode 
    bool getConfigData()
    {      
      EEPROM.begin(EEPROM_SIZE);
      EEPROM.get(EEPROM_START, BlynkESP32_WM_config);
      
      displayConfigData();

      if (strncmp(BlynkESP32_WM_config.header, BOARD_TYPE, strlen(BOARD_TYPE)) != 0) 
      {
          memset(&BlynkESP32_WM_config, 0, sizeof(BlynkESP32_WM_config));
                                   
          BLYNK_LOG2(BLYNK_F("Init new EEPROM, size = "), EEPROM_SIZE /*EEPROM.length()*/);          
          // doesn't have any configuration
          strcpy(BlynkESP32_WM_config.header,           BOARD_TYPE);
          strcpy(BlynkESP32_WM_config.wifi_ssid,        NO_CONFIG);
          strcpy(BlynkESP32_WM_config.wifi_pw,          NO_CONFIG);
          strcpy(BlynkESP32_WM_config.blynk_server,     NO_CONFIG);
          BlynkESP32_WM_config.blynk_port = BLYNK_SERVER_HARDWARE_PORT;
          strcpy(BlynkESP32_WM_config.blynk_token,      NO_CONFIG);
          strcpy(BlynkESP32_WM_config.blynk_bt_tk,      NO_CONFIG);
          strcpy(BlynkESP32_WM_config.blynk_ble_tk,     NO_CONFIG);
          strcpy(BlynkESP32_WM_config.board_name,       NO_CONFIG);
          
          EEPROM.put(EEPROM_START, BlynkESP32_WM_config);
          EEPROM.commit();
          
          return false;
      }
      else if ( !strncmp(BlynkESP32_WM_config.wifi_ssid,        NO_CONFIG, strlen(NO_CONFIG))   ||
                !strncmp(BlynkESP32_WM_config.wifi_pw,          NO_CONFIG, strlen(NO_CONFIG) )  ||
                !strncmp(BlynkESP32_WM_config.blynk_server,     NO_CONFIG, strlen(NO_CONFIG) )  ||
                !strncmp(BlynkESP32_WM_config.blynk_token,      NO_CONFIG, strlen(NO_CONFIG) )  ||
                !strncmp(BlynkESP32_WM_config.blynk_bt_tk,      NO_CONFIG, strlen(NO_CONFIG) )  ||
                !strncmp(BlynkESP32_WM_config.blynk_ble_tk,     NO_CONFIG, strlen(NO_CONFIG) ) ) 
      {
        // If SSID, PW, Server,Token ="nothing", stay in config mode forever until having config Data.
        return false;
      }  
      else
      {
        displayConfigData();           
      }
      return true;
    }      
    
    void saveConfigData()
    {      
      BLYNK_LOG2(BLYNK_F("Save EEPROM, size = "), EEPROM_SIZE /*EEPROM.length()*/);   
      EEPROM.put(EEPROM_START, BlynkESP32_WM_config);
      EEPROM.commit();
    }
    
#endif    

    boolean connectToWifi(int timeout)
    {
      int sleep_time = 250;

      WiFi.mode(WIFI_STA);
      setHostname();

			// New from Blynk_WM v1.0.5
      if (static_IP != IPAddress(0, 0, 0, 0))
      {
        BLYNK_LOG1(BLYNK_F("Use static IP"));
        WiFi.config(static_IP, static_GW, static_SN, static_DNS1, static_DNS2);         
      }
      
	    BLYNK_LOG1(BLYNK_F("con2WF: start"));
	
      if (BlynkESP32_WM_config.wifi_pw && strlen(BlynkESP32_WM_config.wifi_pw))
      {
          WiFi.begin(BlynkESP32_WM_config.wifi_ssid, BlynkESP32_WM_config.wifi_pw);
      } 
      else 
      {
          WiFi.begin(BlynkESP32_WM_config.wifi_ssid);
      }

      while (WiFi.status() != WL_CONNECTED && 0 < timeout) 
      {
          delay(sleep_time);
          timeout -= sleep_time;
      }

	    if (WiFi.status() == WL_CONNECTED)
	    {
		    BLYNK_LOG1(BLYNK_F("con2WF: con OK"));
        BLYNK_LOG6(BLYNK_F("IP = "), WiFi.localIP().toString(), BLYNK_F(", GW = "), WiFi.gatewayIP().toString(), 
                   BLYNK_F(", SN = "), WiFi.subnetMask().toString());
        BLYNK_LOG4(BLYNK_F("DNS1 = "), WiFi.dnsIP(0).toString(), BLYNK_F(", DNS2 = "), WiFi.dnsIP(1).toString());   
		  }
	    else
	    {
		    BLYNK_LOG1(BLYNK_F("con2WF: con failed"));
		  }
	
      return WiFi.status() == WL_CONNECTED;    
    }

    
    void handleRequest()
    {
      if (server)
      {
        String key = server->arg("key");
        String value = server->arg("value");
        
        static int number_items_Updated = 0;

        if (key == "" && value == "") 
        {
            String result = root_html_template;
            
            BLYNK_LOG1(BLYNK_F("hR: replace result"));
            
            // Reset configTimeout to stay here until finished.
            configTimeout = 0;

            result.replace("[[wf_id]]",     BlynkESP32_WM_config.wifi_ssid);
            result.replace("[[wf_pw]]",     BlynkESP32_WM_config.wifi_pw);
            result.replace("[[b_svr]]",     BlynkESP32_WM_config.blynk_server);
            result.replace("[[b_pt]]",      String(BlynkESP32_WM_config.blynk_port));
            result.replace("[[b_tok]]",     BlynkESP32_WM_config.blynk_token);
            result.replace("[[b_bt_tk]]",   BlynkESP32_WM_config.blynk_bt_tk);
            result.replace("[[b_ble_tk]]",  BlynkESP32_WM_config.blynk_ble_tk);
            result.replace("[[bd_nm]]",     BlynkESP32_WM_config.board_name);

            server->send(200, "text/html", result);

            return;
        }
       
        if (number_items_Updated == 0)
        {
          memset(&BlynkESP32_WM_config, 0, sizeof(BlynkESP32_WM_config));
          strcpy(BlynkESP32_WM_config.header, BOARD_TYPE);
        }
        
        if (key == "wf_id")
        {
            number_items_Updated++;
            if (strlen(value.c_str()) < sizeof(BlynkESP32_WM_config.wifi_ssid) -1)
              strcpy(BlynkESP32_WM_config.wifi_ssid, value.c_str());
            else
              strncpy(BlynkESP32_WM_config.wifi_ssid, value.c_str(), sizeof(BlynkESP32_WM_config.wifi_ssid) -1);    
        }
        else if (key == "wf_pw") 
        {
            number_items_Updated++;
            if (strlen(value.c_str()) < sizeof(BlynkESP32_WM_config.wifi_pw) -1)
              strcpy(BlynkESP32_WM_config.wifi_pw, value.c_str());
            else
              strncpy(BlynkESP32_WM_config.wifi_pw, value.c_str(), sizeof(BlynkESP32_WM_config.wifi_pw) -1);    
        }

        else if (key == "b_svr") 
        {
            number_items_Updated++;
            if (strlen(value.c_str()) < sizeof(BlynkESP32_WM_config.blynk_server) -1)
              strcpy(BlynkESP32_WM_config.blynk_server, value.c_str());
            else
              strncpy(BlynkESP32_WM_config.blynk_server, value.c_str(), sizeof(BlynkESP32_WM_config.blynk_server) -1);      
        }
        else if (key == "b_pt") 
        {
            number_items_Updated++;
            BlynkESP32_WM_config.blynk_port = value.toInt();
        }
        else if (key == "b_tok") 
        {
            number_items_Updated++;
            if (strlen(value.c_str()) < sizeof(BlynkESP32_WM_config.blynk_token) -1)
              strcpy(BlynkESP32_WM_config.blynk_token, value.c_str());
            else
              strncpy(BlynkESP32_WM_config.blynk_token, value.c_str(), sizeof(BlynkESP32_WM_config.blynk_token) -1);    
        }
        else if (key == "b_bt_tk") 
        {
            number_items_Updated++;
            if (strlen(value.c_str()) < sizeof(BlynkESP32_WM_config.blynk_bt_tk) -1)
              strcpy(BlynkESP32_WM_config.blynk_bt_tk, value.c_str());
            else
              strncpy(BlynkESP32_WM_config.blynk_bt_tk, value.c_str(), sizeof(BlynkESP32_WM_config.blynk_bt_tk) -1);    
        }    
        else if (key == "b_ble_tk") 
        {
            number_items_Updated++;
            if (strlen(value.c_str()) < sizeof(BlynkESP32_WM_config.blynk_ble_tk) -1)
              strcpy(BlynkESP32_WM_config.blynk_ble_tk, value.c_str());
            else
              strncpy(BlynkESP32_WM_config.blynk_ble_tk, value.c_str(), sizeof(BlynkESP32_WM_config.blynk_ble_tk) -1);    
        }        
        else if (key == "bd_nm") 
        {
            number_items_Updated++;
            if (strlen(value.c_str()) < sizeof(BlynkESP32_WM_config.board_name) -1)
              strcpy(BlynkESP32_WM_config.board_name, value.c_str());
            else
              strncpy(BlynkESP32_WM_config.board_name, value.c_str(), sizeof(BlynkESP32_WM_config.board_name) -1);  
        }
        
        server->send(200, "text/html", "OK");
        
        if (number_items_Updated == NUM_CONFIGURABLE_ITEMS)
        {
          #if USE_SPIFFS     
            BLYNK_LOG2(BLYNK_F("hR: Update SPIFFS "), CONFIG_FILENAME);
          #else
            BLYNK_LOG1(BLYNK_F("hR: Update EEPROM"));
          #endif

          saveConfigData();

          BLYNK_LOG1(BLYNK_F("hR: Reset"));
          
          // Delay then reset the ESP8266 after save data
          delay(1000);
          ESP.restart();
        }
      }    // if (server)
    }
        
    void startConfigurationMode()
    {
      #define CONFIG_TIMEOUT			60000L
      
      // turn the LED_BUILTIN ON to tell us we are in configuration mode.
      digitalWrite(LED_BUILTIN, HIGH);  
      
      if ( (portal_ssid == "") || portal_pass == "" )
      {      
        String chipID = String(ESP_getChipId(), HEX);
        chipID.toUpperCase();
        
	      portal_ssid = "ESP_" + chipID;

	      portal_pass = "MyESP_" + chipID;
	    }
	    
	    BLYNK_LOG6(BLYNK_F("startConfigMode with SSID = "), portal_ssid, BLYNK_F(", PW = "), portal_pass,
	               BLYNK_F(" and IP = "), portal_apIP.toString());          
      
      WiFi.mode(WIFI_AP);
      WiFi.softAP(portal_ssid.c_str(), portal_pass.c_str());
      
      delay(100); // ref: https://github.com/espressif/arduino-esp32/issues/985#issuecomment-359157428
      WiFi.softAPConfig(portal_apIP, portal_apIP, IPAddress(255, 255, 255, 0));
      
      if (!server)
	      server = new WebServer;
	      
      //See https://stackoverflow.com/questions/39803135/c-unresolved-overloaded-function-type?rq=1
      if (server)
      {
        server->on("/", [this](){ handleRequest(); });
        server->begin();
      }
      
      // If there is no saved config Data, stay in config mode forever until having config Data.
      if (hadConfigData)      
        configTimeout = millis() + CONFIG_TIMEOUT;
      else
        configTimeout = 0;      
       
      configuration_mode = true;    
    }    
};

static WiFiClient _blynkWifiClient;
static BlynkArduinoClient _blynkTransport(_blynkWifiClient);

// KH
BlynkWifi Blynk_WF(_blynkTransport);

#if defined(Blynk)
#undef Blynk
#define Blynk Blynk_WF
#endif
//

#include <BlynkWidgets.h>

#endif
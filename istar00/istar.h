#ifndef ISTAR_H
#define ISTAR_H
/*
   Main sketch, global variable declarations
   @title ISTAR project sketch
   @version 0.13.3
   @author Christian Schwinne
 */

// version code in format yymmddb (b = daily build)
#define VERSION 2208222

//uncomment this if you have a "my_config.h" file you'd like to use
//#define ISTAR_USE_MY_CONFIG

// ESP8266-01 (blue) got too little storage space to work with ISTAR. 0.10.2 is the last release supporting this unit.

// ESP8266-01 (black) has 1MB flash and can thus fit the whole program, although OTA update is not possible. Use 1M(128K SPIFFS).
// 2-step OTA may still be possible: https://github.com/Aircoookie/WLED/issues/2040#issuecomment-981111096
// Uncomment some of the following lines to disable features:
// Alternatively, with platformio pass your chosen flags to your custom build target in platformio_override.ini

// You are required to disable over-the-air updates:
//#define ISTAR_DISABLE_OTA         // saves 14kb

// You can choose some of these features to disable:
//#define ISTAR_DISABLE_ALEXA       // saves 11kb
//#define ISTAR_DISABLE_BLYNK       // saves 6kb
//#define ISTAR_DISABLE_HUESYNC     // saves 4kb
//#define ISTAR_DISABLE_INFRARED    // there is no pin left for this on ESP8266-01, saves 12kb
#ifndef ISTAR_DISABLE_MQTT
  #define ISTAR_ENABLE_MQTT         // saves 12kb
#endif
#define ISTAR_ENABLE_ADALIGHT     // saves 500b only (uses GPIO3 (RX) for serial)
//#define ISTAR_ENABLE_DMX          // uses 3.5kb (use LEDPIN other than 2)
//#define ISTAR_ENABLE_JSONLIVE     // peek LED output via /json/live (WS binary peek is always enabled)
#ifndef ISTAR_DISABLE_LOXONE
  #define ISTAR_ENABLE_LOXONE       // uses 1.2kb
#endif
#ifndef ISTAR_DISABLE_WEBSOCKETS
  #define ISTAR_ENABLE_WEBSOCKETS
#endif

#define ISTAR_ENABLE_FS_EDITOR      // enable /edit page for editing FS content. Will also be disabled with OTA lock

// to toggle usb serial debug (un)comment the following line
//#define ISTAR_DEBUG

// filesystem specific debugging
//#define ISTAR_DEBUG_FS

#ifndef ISTAR_WATCHDOG_TIMEOUT
  // 3 seconds should be enough to detect a lockup
  // define ISTAR_WATCHDOG_TIMEOUT=0 to disable watchdog, default
  #define ISTAR_WATCHDOG_TIMEOUT 0
#endif

//optionally disable brownout detector on ESP32.
//This is generally a terrible idea, but improves boot success on boards with a 3.3v regulator + cap setup that can't provide 400mA peaks
//#define ISTAR_DISABLE_BROWNOUT_DET

// Library inclusions.
#include <Arduino.h>
#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266mDNS.h>
  #include <ESPAsyncTCP.h>
  #include <LittleFS.h>
  extern "C"
  {
  #include <user_interface.h>
  }
#else // ESP32
  #include <WiFi.h>
  #include <ETH.h>
  #include "esp_wifi.h"
  #include <ESPmDNS.h>
  #include <AsyncTCP.h>
  #if LOROL_LITTLEFS
    #ifndef CONFIG_LITTLEFS_FOR_IDF_3_2
      #define CONFIG_LITTLEFS_FOR_IDF_3_2
    #endif
    #include <LITTLEFS.h>
  #else
    #include <LittleFS.h>
  #endif
  #include "esp_task_wdt.h"
#endif

#include "src/dependencies/network/Network.h"

#ifdef ISTAR_USE_MY_CONFIG
  #include "my_config.h"
#endif

#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#ifndef ISTAR_DISABLE_OTA
  #define NO_OTA_PORT
  #include <ArduinoOTA.h>
#endif
#include <SPIFFSEditor.h>
#include "src/dependencies/time/TimeLib.h"
#include "src/dependencies/timezone/Timezone.h"
#include "src/dependencies/toki/Toki.h"

#ifndef ISTAR_DISABLE_ALEXA
  #define ESPALEXA_ASYNC
  #define ESPALEXA_NO_SUBPAGE
  #define ESPALEXA_MAXDEVICES 1
  // #define ESPALEXA_DEBUG
  #include "src/dependencies/espalexa/Espalexa.h"
#endif
#ifndef ISTAR_DISABLE_BLYNK
  #include "src/dependencies/blynk/BlynkSimpleEsp.h"
#endif

#ifdef ISTAR_ENABLE_DMX
 #ifdef ESP8266
  #include "src/dependencies/dmx/ESPDMX.h"
 #else //ESP32
  #include "src/dependencies/dmx/SparkFunDMX.h"
 #endif  
#endif

#include "src/dependencies/e131/ESPAsyncE131.h"
#include "src/dependencies/async-mqtt-client/AsyncMqttClient.h"

#define ARDUINOJSON_DECODE_UNICODE 0
#include "src/dependencies/json/AsyncJson-v6.h"
#include "src/dependencies/json/ArduinoJson-v6.h"

// ESP32-WROVER features SPI RAM (aka PSRAM) which can be allocated using ps_malloc()
// we can create custom PSRAMDynamicJsonDocument to use such feature (replacing DynamicJsonDocument)
// The following is a construct to enable code to compile without it.
// There is a code thet will still not use PSRAM though:
//    AsyncJsonResponse is a derived class that implements DynamicJsonDocument (AsyncJson-v6.h)
#if defined(ARDUINO_ARCH_ESP32) && defined(ISTAR_USE_PSRAM)
struct PSRAM_Allocator {
  void* allocate(size_t size) {
    if (psramFound()) return ps_malloc(size); // use PSRAM if it exists
    else              return malloc(size);    // fallback
  }
  void deallocate(void* pointer) {
    free(pointer);
  }
};
using PSRAMDynamicJsonDocument = BasicJsonDocument<PSRAM_Allocator>;
#else
#define PSRAMDynamicJsonDocument DynamicJsonDocument
#endif

#include "fcn_declare.h"
#include "html_ui.h"
#include "html_settings.h"
#include "html_other.h"
#include "FX.h"
#include "ir_codes.h"
#include "const.h"
#include "NodeStruct.h"
#include "pin_manager.h"
#include "bus_manager.h"

#ifndef CLIENT_SSID
  #define CLIENT_SSID DEFAULT_CLIENT_SSID
#endif

#ifndef CLIENT_PASS
  #define CLIENT_PASS ""
#endif

#ifndef SPIFFS_EDITOR_AIRCOOOKIE
  #error You are not using the Aircoookie fork of the ESPAsyncWebserver library.\
  Using upstream puts your WiFi password at risk of being served by the filesystem.\
  Comment out this error message to build regardless.
#endif

#ifndef ISTAR_DISABLE_INFRARED
  #include <IRremoteESP8266.h>
  #include <IRrecv.h>
  #include <IRutils.h>
#endif

//Filesystem to use for preset and config files. SPIFFS or LittleFS on ESP8266, SPIFFS only on ESP32 (now using LITTLEFS port by lorol)
#ifdef ESP8266
  #define ISTAR_FS LittleFS
#else
  #if LOROL_LITTLEFS
    #define ISTAR_FS LITTLEFS
  #else
    #define ISTAR_FS LittleFS
  #endif
#endif

// GLOBAL VARIABLES
// both declared and defined in header (solution from http://www.keil.com/support/docs/1868.htm)
//
//e.g. byte test = 2 becomes ISTAR_GLOBAL byte test _INIT(2);
//     int arr[]{0,1,2} becomes ISTAR_GLOBAL int arr[] _INIT_N(({0,1,2}));

#ifndef ISTAR_DEFINE_GLOBAL_VARS
# define ISTAR_GLOBAL extern
# define _INIT(x)
# define _INIT_N(x)
#else
# define ISTAR_GLOBAL
# define _INIT(x) = x

//needed to ignore commas in array definitions
#define UNPACK( ... ) __VA_ARGS__
# define _INIT_N(x) UNPACK x
#endif

#define STRINGIFY(X) #X
#define TOSTRING(X) STRINGIFY(X)

#ifndef ISTAR_VERSION
  #define ISTAR_VERSION "dev"
#endif

// Global Variable definitions
ISTAR_GLOBAL char versionString[] _INIT(TOSTRING(ISTAR_VERSION));
#define ISTAR_CODENAME "Toki"

// AP and OTA default passwords (for maximum security change them!)
ISTAR_GLOBAL char apPass[65]  _INIT(DEFAULT_AP_PASS);
ISTAR_GLOBAL char otaPass[33] _INIT(DEFAULT_OTA_PASS);

// Hardware and pin config
#ifndef BTNPIN
ISTAR_GLOBAL int8_t btnPin[ISTAR_MAX_BUTTONS] _INIT({0});
#else
ISTAR_GLOBAL int8_t btnPin[ISTAR_MAX_BUTTONS] _INIT({BTNPIN});
#endif
#ifndef RLYPIN
ISTAR_GLOBAL int8_t rlyPin _INIT(-1);
#else
ISTAR_GLOBAL int8_t rlyPin _INIT(RLYPIN);
#endif
//Relay mode (1 = active high, 0 = active low, flipped in cfg.json)
#ifndef RLYMDE
ISTAR_GLOBAL bool rlyMde _INIT(true);
#else
ISTAR_GLOBAL bool rlyMde _INIT(RLYMDE);
#endif
#ifndef IRPIN
ISTAR_GLOBAL int8_t irPin _INIT(-1);
#else
ISTAR_GLOBAL int8_t irPin _INIT(IRPIN);
#endif

//ISTAR_GLOBAL byte presetToApply _INIT(0);

ISTAR_GLOBAL char ntpServerName[33] _INIT("0.istar.pool.ntp.org");   // NTP server to use

// WiFi CONFIG (all these can be changed via web UI, no need to set them here)
ISTAR_GLOBAL char clientSSID[33] _INIT(CLIENT_SSID);
ISTAR_GLOBAL char clientPass[65] _INIT(CLIENT_PASS);
ISTAR_GLOBAL char cmDNS[33] _INIT("x");                             // mDNS address (placeholder, is replaced by istarXXXXXX.local)
ISTAR_GLOBAL char apSSID[33] _INIT("");                             // AP off by default (unless setup)
ISTAR_GLOBAL byte apChannel _INIT(1);                               // 2.4GHz WiFi AP channel (1-13)
ISTAR_GLOBAL byte apHide    _INIT(0);                               // hidden AP SSID
ISTAR_GLOBAL byte apBehavior _INIT(AP_BEHAVIOR_BOOT_NO_CONN);       // access point opens when no connection after boot by default
ISTAR_GLOBAL IPAddress staticIP      _INIT_N(((  0,   0,  0,  0))); // static IP of ESP
ISTAR_GLOBAL IPAddress staticGateway _INIT_N(((  0,   0,  0,  0))); // gateway (router) IP
ISTAR_GLOBAL IPAddress staticSubnet  _INIT_N(((255, 255, 255, 0))); // most common subnet in home networks
#ifdef ARDUINO_ARCH_ESP32
ISTAR_GLOBAL bool noWifiSleep _INIT(true);                          // disabling modem sleep modes will increase heat output and power usage, but may help with connection issues
#else
ISTAR_GLOBAL bool noWifiSleep _INIT(false);
#endif

#ifdef ISTAR_USE_ETHERNET
  #ifdef ISTAR_ETH_DEFAULT                                          // default ethernet board type if specified
    ISTAR_GLOBAL int ethernetType _INIT(ISTAR_ETH_DEFAULT);          // ethernet board type
  #else
    ISTAR_GLOBAL int ethernetType _INIT(ISTAR_ETH_NONE);             // use none for ethernet board type if default not defined
  #endif
#endif

// LED CONFIG
ISTAR_GLOBAL bool turnOnAtBoot _INIT(true);                // turn on LEDs at power-up
ISTAR_GLOBAL byte bootPreset   _INIT(0);                   // save preset to load after power-up

//if true, a segment per bus will be created on boot and LED settings save
//if false, only one segment spanning the total LEDs is created,
//but not on LED settings save if there is more than one segment currently
ISTAR_GLOBAL bool autoSegments _INIT(false);
ISTAR_GLOBAL bool correctWB _INIT(false); //CCT color correction of RGB color
ISTAR_GLOBAL bool cctFromRgb _INIT(false); //CCT is calculated from RGB instead of using seg.cct

ISTAR_GLOBAL byte col[]    _INIT_N(({ 255, 160, 0, 0 }));  // current RGB(W) primary color. col[] should be updated if you want to change the color.
ISTAR_GLOBAL byte colSec[] _INIT_N(({ 0, 0, 0, 0 }));      // current RGB(W) secondary color
ISTAR_GLOBAL byte briS     _INIT(128);                     // default brightness

ISTAR_GLOBAL byte nightlightTargetBri _INIT(0);      // brightness after nightlight is over
ISTAR_GLOBAL byte nightlightDelayMins _INIT(60);
ISTAR_GLOBAL byte nightlightMode      _INIT(NL_MODE_FADE); // See const.h for available modes. Was nightlightFade
ISTAR_GLOBAL bool fadeTransition      _INIT(true);   // enable crossfading color transition
ISTAR_GLOBAL uint16_t transitionDelay _INIT(750);    // default crossfade duration in ms

ISTAR_GLOBAL byte briMultiplier _INIT(100);          // % of brightness to set (to limit power, if you set it to 50 and set bri to 255, actual brightness will be 127)

// User Interface CONFIG
#ifndef SERVERNAME
ISTAR_GLOBAL char serverDescription[33] _INIT("ISTAR");  // Name of module - use default
#else
ISTAR_GLOBAL char serverDescription[33] _INIT(SERVERNAME);  // use predefined name
#endif
ISTAR_GLOBAL bool syncToggleReceive     _INIT(false);   // UIs which only have a single button for sync should toggle send+receive if this is true, only send otherwise

// Sync CONFIG
ISTAR_GLOBAL NodesMap Nodes;
ISTAR_GLOBAL bool nodeListEnabled _INIT(true);
ISTAR_GLOBAL bool nodeBroadcastEnabled _INIT(true);

ISTAR_GLOBAL byte buttonType[ISTAR_MAX_BUTTONS]  _INIT({BTN_TYPE_PUSH});
#if defined(IRTYPE) && defined(IRPIN)
ISTAR_GLOBAL byte irEnabled      _INIT(IRTYPE); // Infrared receiver
#else
ISTAR_GLOBAL byte irEnabled      _INIT(0);     // Infrared receiver disabled
#endif
ISTAR_GLOBAL bool irApplyToAllSelected _INIT(true); //apply IR to all selected segments

ISTAR_GLOBAL uint16_t udpPort    _INIT(21324); // ISTAR notifier default port
ISTAR_GLOBAL uint16_t udpPort2   _INIT(65506); // ISTAR notifier supplemental port
ISTAR_GLOBAL uint16_t udpRgbPort _INIT(19446); // Hyperion port

ISTAR_GLOBAL uint8_t syncGroups    _INIT(0x01);                    // sync groups this instance syncs (bit mapped)
ISTAR_GLOBAL uint8_t receiveGroups _INIT(0x01);                    // sync receive groups this instance belongs to (bit mapped)
ISTAR_GLOBAL bool receiveNotificationBrightness _INIT(true);       // apply brightness from incoming notifications
ISTAR_GLOBAL bool receiveNotificationColor      _INIT(true);       // apply color
ISTAR_GLOBAL bool receiveNotificationEffects    _INIT(true);       // apply effects setup
ISTAR_GLOBAL bool receiveSegmentOptions         _INIT(false);      // apply segment options
ISTAR_GLOBAL bool receiveSegmentBounds          _INIT(false);      // apply segment bounds (start, stop, offset)
ISTAR_GLOBAL bool notifyDirect _INIT(false);                       // send notification if change via UI or HTTP API
ISTAR_GLOBAL bool notifyButton _INIT(false);                       // send if updated by button or infrared remote
ISTAR_GLOBAL bool notifyAlexa  _INIT(false);                       // send notification if updated via Alexa
ISTAR_GLOBAL bool notifyMacro  _INIT(false);                       // send notification for macro
ISTAR_GLOBAL bool notifyHue    _INIT(true);                        // send notification if Hue light changes
ISTAR_GLOBAL bool notifyTwice  _INIT(false);                       // notifications use UDP: enable if devices don't sync reliably

ISTAR_GLOBAL bool alexaEnabled _INIT(false);                       // enable device discovery by Amazon Echo
ISTAR_GLOBAL char alexaInvocationName[33] _INIT("Light");          // speech control name of device. Choose something voice-to-text can understand

#ifndef ISTAR_DISABLE_BLYNK
ISTAR_GLOBAL char blynkApiKey[36] _INIT("");                       // Auth token for Blynk server. If empty, no connection will be made
ISTAR_GLOBAL char blynkHost[33] _INIT("blynk-cloud.com");          // Default Blynk host
ISTAR_GLOBAL uint16_t blynkPort _INIT(80);                         // Default Blynk port
#endif

ISTAR_GLOBAL uint16_t realtimeTimeoutMs _INIT(2500);               // ms timeout of realtime mode before returning to normal mode
ISTAR_GLOBAL int arlsOffset _INIT(0);                              // realtime LED offset
ISTAR_GLOBAL bool receiveDirect _INIT(true);                       // receive UDP realtime
ISTAR_GLOBAL bool arlsDisableGammaCorrection _INIT(true);          // activate if gamma correction is handled by the source
ISTAR_GLOBAL bool arlsForceMaxBri _INIT(false);                    // enable to force max brightness if source has very dark colors that would be black

#ifdef ISTAR_ENABLE_DMX
 #ifdef ESP8266
  ISTAR_GLOBAL DMXESPSerial dmx;
 #else //ESP32
  ISTAR_GLOBAL SparkFunDMX dmx; 
 #endif 
ISTAR_GLOBAL uint16_t e131ProxyUniverse _INIT(0);                  // output this E1.31 (sACN) / ArtNet universe via MAX485 (0 = disabled)
#endif
ISTAR_GLOBAL uint16_t e131Universe _INIT(1);                       // settings for E1.31 (sACN) protocol (only DMX_MODE_MULTIPLE_* can span over consequtive universes)
ISTAR_GLOBAL uint16_t e131Port _INIT(5568);                        // DMX in port. E1.31 default is 5568, Art-Net is 6454
ISTAR_GLOBAL byte DMXMode _INIT(DMX_MODE_MULTIPLE_RGB);            // DMX mode (s.a.)
ISTAR_GLOBAL uint16_t DMXAddress _INIT(1);                         // DMX start address of fixture, a.k.a. first Channel [for E1.31 (sACN) protocol]
ISTAR_GLOBAL byte DMXOldDimmer _INIT(0);                           // only update brightness on change
ISTAR_GLOBAL byte e131LastSequenceNumber[E131_MAX_UNIVERSE_COUNT]; // to detect packet loss
ISTAR_GLOBAL bool e131Multicast _INIT(false);                      // multicast or unicast
ISTAR_GLOBAL bool e131SkipOutOfSequence _INIT(false);              // freeze instead of flickering

ISTAR_GLOBAL bool mqttEnabled _INIT(false);
ISTAR_GLOBAL char mqttDeviceTopic[33] _INIT("");            // main MQTT topic (individual per device, default is istar/mac)
ISTAR_GLOBAL char mqttGroupTopic[33] _INIT("istar/all");     // second MQTT topic (for example to group devices)
ISTAR_GLOBAL char mqttServer[33] _INIT("");                 // both domains and IPs should work (no SSL)
ISTAR_GLOBAL char mqttUser[41] _INIT("");                   // optional: username for MQTT auth
ISTAR_GLOBAL char mqttPass[65] _INIT("");                   // optional: password for MQTT auth
ISTAR_GLOBAL char mqttClientID[41] _INIT("");               // override the client ID
ISTAR_GLOBAL uint16_t mqttPort _INIT(1883);

#ifndef ISTAR_DISABLE_HUESYNC
ISTAR_GLOBAL bool huePollingEnabled _INIT(false);           // poll hue bridge for light state
ISTAR_GLOBAL uint16_t huePollIntervalMs _INIT(2500);        // low values (< 1sec) may cause lag but offer quicker response
ISTAR_GLOBAL char hueApiKey[47] _INIT("api");               // key token will be obtained from bridge
ISTAR_GLOBAL byte huePollLightId _INIT(1);                  // ID of hue lamp to sync to. Find the ID in the hue app ("about" section)
ISTAR_GLOBAL IPAddress hueIP _INIT_N(((0, 0, 0, 0))); // IP address of the bridge
ISTAR_GLOBAL bool hueApplyOnOff _INIT(true);
ISTAR_GLOBAL bool hueApplyBri _INIT(true);
ISTAR_GLOBAL bool hueApplyColor _INIT(true);
#endif

ISTAR_GLOBAL uint16_t serialBaud _INIT(1152); // serial baud rate, multiply by 100

// Time CONFIG
ISTAR_GLOBAL bool ntpEnabled _INIT(false);    // get internet time. Only required if you use clock overlays or time-activated macros
ISTAR_GLOBAL bool useAMPM _INIT(false);       // 12h/24h clock format
ISTAR_GLOBAL byte currentTimezone _INIT(0);   // Timezone ID. Refer to timezones array in istar10_ntp.ino
ISTAR_GLOBAL int utcOffsetSecs _INIT(0);      // Seconds to offset from UTC before timzone calculation

ISTAR_GLOBAL byte overlayCurrent _INIT(0);    // 0: no overlay 1: analog clock 2: was single-digit clock 3: was cronixie
ISTAR_GLOBAL byte overlayMin _INIT(0), overlayMax _INIT(DEFAULT_LED_COUNT - 1);   // boundaries of overlay mode

ISTAR_GLOBAL byte analogClock12pixel _INIT(0);               // The pixel in your strip where "midnight" would be
ISTAR_GLOBAL bool analogClockSecondsTrail _INIT(false);      // Display seconds as trail of LEDs instead of a single pixel
ISTAR_GLOBAL bool analogClock5MinuteMarks _INIT(false);      // Light pixels at every 5-minute position

ISTAR_GLOBAL bool countdownMode _INIT(false);                         // Clock will count down towards date
ISTAR_GLOBAL byte countdownYear _INIT(20), countdownMonth _INIT(1);   // Countdown target date, year is last two digits
ISTAR_GLOBAL byte countdownDay  _INIT(1) , countdownHour  _INIT(0);
ISTAR_GLOBAL byte countdownMin  _INIT(0) , countdownSec   _INIT(0);

ISTAR_GLOBAL byte macroNl   _INIT(0);        // after nightlight delay over
ISTAR_GLOBAL byte macroCountdown _INIT(0);
ISTAR_GLOBAL byte macroAlexaOn _INIT(0), macroAlexaOff _INIT(0);
ISTAR_GLOBAL byte macroButton[ISTAR_MAX_BUTTONS]        _INIT({0});
ISTAR_GLOBAL byte macroLongPress[ISTAR_MAX_BUTTONS]     _INIT({0});
ISTAR_GLOBAL byte macroDoublePress[ISTAR_MAX_BUTTONS]   _INIT({0});

// Security CONFIG
ISTAR_GLOBAL bool otaLock     _INIT(false);  // prevents OTA firmware updates without password. ALWAYS enable if system exposed to any public networks
ISTAR_GLOBAL bool wifiLock    _INIT(false);  // prevents access to WiFi settings when OTA lock is enabled
ISTAR_GLOBAL bool aOtaEnabled _INIT(true);   // ArduinoOTA allows easy updates directly from the IDE. Careful, it does not auto-disable when OTA lock is on

ISTAR_GLOBAL uint16_t userVar0 _INIT(0), userVar1 _INIT(0); //available for use in usermod

#ifdef ISTAR_ENABLE_DMX
  // dmx CONFIG
  ISTAR_GLOBAL byte DMXChannels _INIT(7);        // number of channels per fixture
  ISTAR_GLOBAL byte DMXFixtureMap[15] _INIT_N(({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }));
  // assigns the different channels to different functions. See istar21_dmx.ino for more information.
  ISTAR_GLOBAL uint16_t DMXGap _INIT(10);          // gap between the fixtures. makes addressing easier because you don't have to memorize odd numbers when climbing up onto a rig.
  ISTAR_GLOBAL uint16_t DMXStart _INIT(10);        // start address of the first fixture
  ISTAR_GLOBAL uint16_t DMXStartLED _INIT(0);      // LED from which DMX fixtures start
#endif

// internal global variable declarations
// wifi
ISTAR_GLOBAL bool apActive _INIT(false);
ISTAR_GLOBAL bool forceReconnect _INIT(false);
ISTAR_GLOBAL uint32_t lastReconnectAttempt _INIT(0);
ISTAR_GLOBAL bool interfacesInited _INIT(false);
ISTAR_GLOBAL bool wasConnected _INIT(false);

// color
ISTAR_GLOBAL byte lastRandomIndex _INIT(0);        // used to save last random color so the new one is not the same

// transitions
ISTAR_GLOBAL bool transitionActive _INIT(false);
ISTAR_GLOBAL uint16_t transitionDelayDefault _INIT(transitionDelay);
ISTAR_GLOBAL uint16_t transitionDelayTemp _INIT(transitionDelay);
ISTAR_GLOBAL unsigned long transitionStartTime;
ISTAR_GLOBAL float tperLast _INIT(0);        // crossfade transition progress, 0.0f - 1.0f
ISTAR_GLOBAL bool jsonTransitionOnce _INIT(false);

// nightlight
ISTAR_GLOBAL bool nightlightActive _INIT(false);
ISTAR_GLOBAL bool nightlightActiveOld _INIT(false);
ISTAR_GLOBAL uint32_t nightlightDelayMs _INIT(10);
ISTAR_GLOBAL byte nightlightDelayMinsDefault _INIT(nightlightDelayMins);
ISTAR_GLOBAL unsigned long nightlightStartTime;
ISTAR_GLOBAL byte briNlT _INIT(0);                     // current nightlight brightness
ISTAR_GLOBAL byte colNlT[] _INIT_N(({ 0, 0, 0, 0 }));        // current nightlight color

// brightness
ISTAR_GLOBAL unsigned long lastOnTime _INIT(0);
ISTAR_GLOBAL bool offMode _INIT(!turnOnAtBoot);
ISTAR_GLOBAL byte bri _INIT(briS);
ISTAR_GLOBAL byte briOld _INIT(0);
ISTAR_GLOBAL byte briT _INIT(0);
ISTAR_GLOBAL byte briIT _INIT(0);
ISTAR_GLOBAL byte briLast _INIT(128);          // brightness before turned off. Used for toggle function
ISTAR_GLOBAL byte whiteLast _INIT(128);        // white channel before turned off. Used for toggle function

// button
ISTAR_GLOBAL bool buttonPublishMqtt                            _INIT(false);
ISTAR_GLOBAL bool buttonPressedBefore[ISTAR_MAX_BUTTONS]        _INIT({false});
ISTAR_GLOBAL bool buttonLongPressed[ISTAR_MAX_BUTTONS]          _INIT({false});
ISTAR_GLOBAL unsigned long buttonPressedTime[ISTAR_MAX_BUTTONS] _INIT({0});
ISTAR_GLOBAL unsigned long buttonWaitTime[ISTAR_MAX_BUTTONS]    _INIT({0});
ISTAR_GLOBAL byte touchThreshold                               _INIT(TOUCH_THRESHOLD);

// notifications
ISTAR_GLOBAL bool notifyDirectDefault _INIT(notifyDirect);
ISTAR_GLOBAL bool receiveNotifications _INIT(true);
ISTAR_GLOBAL unsigned long notificationSentTime _INIT(0);
ISTAR_GLOBAL byte notificationSentCallMode _INIT(CALL_MODE_INIT);
ISTAR_GLOBAL bool notificationTwoRequired _INIT(false);

// effects
ISTAR_GLOBAL byte effectCurrent _INIT(0);
ISTAR_GLOBAL byte effectSpeed _INIT(128);
ISTAR_GLOBAL byte effectIntensity _INIT(128);
ISTAR_GLOBAL byte effectPalette _INIT(0);
ISTAR_GLOBAL bool stateChanged _INIT(false);

// network
ISTAR_GLOBAL bool udpConnected _INIT(false), udp2Connected _INIT(false), udpRgbConnected _INIT(false);

// ui style
ISTAR_GLOBAL bool showWelcomePage _INIT(false);

// hue
ISTAR_GLOBAL byte hueError _INIT(HUE_ERROR_INACTIVE);
// ISTAR_GLOBAL uint16_t hueFailCount _INIT(0);
ISTAR_GLOBAL float hueXLast _INIT(0), hueYLast _INIT(0);
ISTAR_GLOBAL uint16_t hueHueLast _INIT(0), hueCtLast _INIT(0);
ISTAR_GLOBAL byte hueSatLast _INIT(0), hueBriLast _INIT(0);
ISTAR_GLOBAL unsigned long hueLastRequestSent _INIT(0);
ISTAR_GLOBAL bool hueAuthRequired _INIT(false);
ISTAR_GLOBAL bool hueReceived _INIT(false);
ISTAR_GLOBAL bool hueStoreAllowed _INIT(false), hueNewKey _INIT(false);

// countdown
ISTAR_GLOBAL unsigned long countdownTime _INIT(1514764800L);
ISTAR_GLOBAL bool countdownOverTriggered _INIT(true);

//timer
ISTAR_GLOBAL byte lastTimerMinute  _INIT(0);
ISTAR_GLOBAL byte timerHours[]     _INIT_N(({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }));
ISTAR_GLOBAL int8_t timerMinutes[] _INIT_N(({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }));
ISTAR_GLOBAL byte timerMacro[]     _INIT_N(({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }));
//weekdays to activate on, bit pattern of arr elem: 0b11111111: sun,sat,fri,thu,wed,tue,mon,validity
ISTAR_GLOBAL byte timerWeekday[]   _INIT_N(({ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }));
//upper 4 bits start, lower 4 bits end month (default 28: start month 1 and end month 12)
ISTAR_GLOBAL byte timerMonth[]     _INIT_N(({28,28,28,28,28,28,28,28}));
ISTAR_GLOBAL byte timerDay[]       _INIT_N(({1,1,1,1,1,1,1,1}));
ISTAR_GLOBAL byte timerDayEnd[]		_INIT_N(({31,31,31,31,31,31,31,31}));

// blynk
ISTAR_GLOBAL bool blynkEnabled _INIT(false);

//improv
ISTAR_GLOBAL byte improvActive _INIT(0); //0: no improv packet received, 1: improv active, 2: provisioning
ISTAR_GLOBAL byte improvError _INIT(0);

//playlists
ISTAR_GLOBAL int16_t currentPlaylist _INIT(-1);
//still used for "PL=~" HTTP API command
ISTAR_GLOBAL byte presetCycCurr _INIT(0);
ISTAR_GLOBAL byte presetCycMin _INIT(1);
ISTAR_GLOBAL byte presetCycMax _INIT(5); 

// realtime
ISTAR_GLOBAL byte realtimeMode _INIT(REALTIME_MODE_INACTIVE);
ISTAR_GLOBAL byte realtimeOverride _INIT(REALTIME_OVERRIDE_NONE);
ISTAR_GLOBAL IPAddress realtimeIP _INIT_N(((0, 0, 0, 0)));
ISTAR_GLOBAL unsigned long realtimeTimeout _INIT(0);
ISTAR_GLOBAL uint8_t tpmPacketCount _INIT(0);
ISTAR_GLOBAL uint16_t tpmPayloadFrameSize _INIT(0);
ISTAR_GLOBAL bool useMainSegmentOnly _INIT(false);

// mqtt
ISTAR_GLOBAL unsigned long lastMqttReconnectAttempt _INIT(0);
ISTAR_GLOBAL unsigned long lastInterfaceUpdate _INIT(0);
ISTAR_GLOBAL byte interfaceUpdateCallMode _INIT(CALL_MODE_INIT);
ISTAR_GLOBAL char mqttStatusTopic[40] _INIT("");        // this must be global because of async handlers

// alexa udp
ISTAR_GLOBAL String escapedMac;
#ifndef ISTAR_DISABLE_ALEXA
  ISTAR_GLOBAL Espalexa espalexa;
  ISTAR_GLOBAL EspalexaDevice* espalexaDevice;
#endif

// dns server
ISTAR_GLOBAL DNSServer dnsServer;

// network time
ISTAR_GLOBAL bool ntpConnected _INIT(false);
ISTAR_GLOBAL time_t localTime _INIT(0);
ISTAR_GLOBAL unsigned long ntpLastSyncTime _INIT(999000000L);
ISTAR_GLOBAL unsigned long ntpPacketSentTime _INIT(999000000L);
ISTAR_GLOBAL IPAddress ntpServerIP;
ISTAR_GLOBAL uint16_t ntpLocalPort _INIT(2390);
ISTAR_GLOBAL uint16_t rolloverMillis _INIT(0);
ISTAR_GLOBAL float longitude _INIT(0.0);
ISTAR_GLOBAL float latitude _INIT(0.0);
ISTAR_GLOBAL time_t sunrise _INIT(0);
ISTAR_GLOBAL time_t sunset _INIT(0);
ISTAR_GLOBAL Toki toki _INIT(Toki());

// Temp buffer
ISTAR_GLOBAL char* obuf;
ISTAR_GLOBAL uint16_t olen _INIT(0);

// General filesystem
ISTAR_GLOBAL size_t fsBytesUsed _INIT(0);
ISTAR_GLOBAL size_t fsBytesTotal _INIT(0);
ISTAR_GLOBAL unsigned long presetsModifiedTime _INIT(0L);
ISTAR_GLOBAL JsonDocument* fileDoc;
ISTAR_GLOBAL bool doCloseFile _INIT(false);

// presets
ISTAR_GLOBAL byte currentPreset _INIT(0);

ISTAR_GLOBAL byte errorFlag _INIT(0);

ISTAR_GLOBAL String messageHead, messageSub;
ISTAR_GLOBAL byte optionType;

ISTAR_GLOBAL bool doReboot _INIT(false);        // flag to initiate reboot from async handlers
ISTAR_GLOBAL bool doPublishMqtt _INIT(false);

// server library objects
ISTAR_GLOBAL AsyncWebServer server _INIT_N(((80)));
#ifdef ISTAR_ENABLE_WEBSOCKETS
ISTAR_GLOBAL AsyncWebSocket ws _INIT_N((("/ws")));
#endif
ISTAR_GLOBAL AsyncClient* hueClient _INIT(NULL);
ISTAR_GLOBAL AsyncMqttClient* mqtt _INIT(NULL);

// udp interface objects
ISTAR_GLOBAL WiFiUDP notifierUdp, rgbUdp, notifier2Udp;
ISTAR_GLOBAL WiFiUDP ntpUdp;
ISTAR_GLOBAL ESPAsyncE131 e131 _INIT_N(((handleE131Packet)));
ISTAR_GLOBAL ESPAsyncE131 ddp  _INIT_N(((handleE131Packet)));
ISTAR_GLOBAL bool e131NewData _INIT(false);

// led fx library object
ISTAR_GLOBAL BusManager busses _INIT(BusManager());
ISTAR_GLOBAL WS2812FX strip _INIT(WS2812FX());
ISTAR_GLOBAL BusConfig* busConfigs[ISTAR_MAX_BUSSES] _INIT({nullptr}); //temporary, to remember values from network callback until after
ISTAR_GLOBAL bool doInitBusses _INIT(false);
ISTAR_GLOBAL int8_t loadLedmap _INIT(-1);

// Usermod manager
ISTAR_GLOBAL UsermodManager usermods _INIT(UsermodManager());

#ifndef ISTAR_USE_DYNAMIC_JSON
// global ArduinoJson buffer
ISTAR_GLOBAL StaticJsonDocument<JSON_BUFFER_SIZE> doc;
#endif
ISTAR_GLOBAL volatile uint8_t jsonBufferLock _INIT(0);

// enable additional debug output
#ifdef ISTAR_DEBUG
  #ifndef ESP8266
  #include <rom/rtc.h>
  #endif
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(x...) Serial.printf(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(x...)
#endif

#ifdef ISTAR_DEBUG_FS
  #define DEBUGFS_PRINT(x) Serial.print(x)
  #define DEBUGFS_PRINTLN(x) Serial.println(x)
  #define DEBUGFS_PRINTF(x...) Serial.printf(x)
#else
  #define DEBUGFS_PRINT(x)
  #define DEBUGFS_PRINTLN(x)
  #define DEBUGFS_PRINTF(x...)
#endif

// debug macro variable definitions
#ifdef ISTAR_DEBUG
  ISTAR_GLOBAL unsigned long debugTime _INIT(0);
  ISTAR_GLOBAL int lastWifiState _INIT(3);
  ISTAR_GLOBAL unsigned long wifiStateChangedTime _INIT(0);
  ISTAR_GLOBAL unsigned long loops _INIT(0);
#endif

#ifdef ARDUINO_ARCH_ESP32
  #define ISTAR_CONNECTED (WiFi.status() == WL_CONNECTED || ETH.localIP()[0] != 0)
#else
  #define ISTAR_CONNECTED (WiFi.status() == WL_CONNECTED)
#endif
#define ISTAR_WIFI_CONFIGURED (strlen(clientSSID) >= 1 && strcmp(clientSSID, DEFAULT_CLIENT_SSID) != 0)
#define ISTAR_MQTT_CONNECTED (mqtt != nullptr && mqtt->connected())

//color mangling macros
#define RGBW32(r,g,b,w) (uint32_t((byte(w) << 24) | (byte(r) << 16) | (byte(g) << 8) | (byte(b))))
#define R(c) (byte((c) >> 16))
#define G(c) (byte((c) >> 8))
#define B(c) (byte(c))
#define W(c) (byte((c) >> 24))

// append new c string to temp buffer efficiently
bool oappend(const char* txt);
// append new number to temp buffer efficiently
bool oappendi(int i);

class ISTAR {
public:
  ISTAR();
  static ISTAR& instance()
  {
    static ISTAR instance;
    return instance;
  }

  // boot starts here
  void setup();

  void loop();
  void reset();

  void beginStrip();
  void handleConnection();
  bool initEthernet(); // result is informational
  void initAP(bool resetAP = false);
  void initConnection();
  void initInterfaces();
  void handleStatusLED();
  void enableWatchdog();
  void disableWatchdog();
};
#endif        // ISTAR_H
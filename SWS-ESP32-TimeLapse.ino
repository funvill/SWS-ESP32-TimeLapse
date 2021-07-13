// SWS-ESP32-TimeLapse 
// Written by Steven Smethurst (https://blog.abluestar.com)
// 
// More information: https://github.com/funvill/SWS-ESP32-TimeLapse 
// 
// 


#include "esp_camera.h"
#include <WiFi.h>

#include "file.h"

#include "FS.h"
#include "SD_MMC.h"
#include <ArduinoJson.h>

// #include "Arduino.h"
#include "esp_camera.h"
// WARNING!!! Make sure that you have either selected ESP32 Wrover Module, or another board which has PSRAM enabled
// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// Version 
const uint16_t APPLICATION_VERSION_MAJOR = 0 ; 
const uint16_t APPLICATION_VERSION_MINOR = 2 ; 
const uint16_t APPLICATION_VERSION_PATH  = 0 ; 

    const char *ssid = "CanConfirm";
    const char *password = "DogEagleFish2021";


// Settings defaults 
const uint32_t SETTING_DEFAULT_PHOTO_TIMER = 10 ; // In Seconds 

#define SYSTEM_MODE_TIME_LAPES 0
#define SYSTEM_MODE_WIFI 1

// Camera module default settings 
const uint8_t SETTINGS_DEFAULT_BRIGHTNESS = 0 ; 
const uint8_t SETTINGS_DEFAULT_CONTRAST = 0 ; 
const uint8_t SETTINGS_DEFAULT_SATURATION = 0 ; 
const uint8_t SETTINGS_DEFAULT_SPECIAL_EFFECT = 0 ; 
const uint8_t SETTINGS_DEFAULT_WHITEBAL = 1 ; 
const uint8_t SETTINGS_DEFAULT_AWB_GAIN = 1 ; 
const uint8_t SETTINGS_DEFAULT_WB_MODE = 0 ; 
const uint8_t SETTINGS_DEFAULT_EXPOSURE_CTRL = 1 ; 
const uint8_t SETTINGS_DEFAULT_AEC2 = 0 ; 
const uint8_t SETTINGS_DEFAULT_AE_LEVEL = 0 ; 
const uint8_t SETTINGS_DEFAULT_AEC_VALUE = 300 ; 
const uint8_t SETTINGS_DEFAULT_GAIN_CTRL = 1 ; 
const uint8_t SETTINGS_DEFAULT_AGC_GAIN = 0 ; 
const uint8_t SETTINGS_DEFAULT_BPC = 0 ; 
const uint8_t SETTINGS_DEFAULT_WPC = 1 ; 
const uint8_t SETTINGS_DEFAULT_RAW_GMA = 1 ; 
const uint8_t SETTINGS_DEFAULT_LENC = 1 ; 
const uint8_t SETTINGS_DEFAULT_HMIRROR = 0 ; 
const uint8_t SETTINGS_DEFAULT_VFLIP = 0 ; 
const uint8_t SETTINGS_DEFAULT_DCW = 1 ; 
const uint8_t SETTINGS_DEFAULT_COLORBAR = 0 ;

// Consts 
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
const char *PATH_CONFIGURATION_FILE = "/config.json";  // <- SD library uses 8.3 filenames


// Running variables 
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int lapseIndex = 0;


struct ApplicationConfig {
  uint16_t system_mode ; // 0 = timelaps, 1 = wifi 
  uint32_t photo_timer ; 

  // Camera settings 
  uint8_t brightness;
  uint8_t contrast;
  uint8_t saturation;
  uint8_t special_effect;
  uint8_t whitebal;
  uint8_t awb_gain;
  uint8_t wb_mode;
  uint8_t exposure_ctrl;
  uint8_t aec2;
  uint8_t ae_level;
  uint8_t aec_value;
  uint8_t gain_ctrl;
  uint8_t agc_gain;
  uint8_t bpc;
  uint8_t wpc;
  uint8_t raw_gma;
  uint8_t lenc;
  uint8_t hmirror;
  uint8_t vflip;
  uint8_t dcw;
  uint8_t colorbar;
};


ApplicationConfig  global_config;                         // <- global configuration object


// Functions 
void GoToSleep(); 
void LoadConfiguration(const char *filename, ApplicationConfig &config) ;
void SaveConfiguration(const char *filename, ApplicationConfig &config) ;

void SystemWiFiConfiguration();
void SystemTimeLapse();
void startCameraServer();

void DefaultConfiguration(ApplicationConfig &config) {
  config.photo_timer = SETTING_DEFAULT_PHOTO_TIMER ; 
  config.system_mode = SYSTEM_MODE_TIME_LAPES; 
  

  // Camera module settings 
  config.brightness =  SETTINGS_DEFAULT_BRIGHTNESS ; 
  config.contrast =  SETTINGS_DEFAULT_CONTRAST ; 
  config.saturation =  SETTINGS_DEFAULT_SATURATION ; 
  config.special_effect =  SETTINGS_DEFAULT_SPECIAL_EFFECT ; 
  config.whitebal =  SETTINGS_DEFAULT_WHITEBAL ; 
  config.awb_gain =  SETTINGS_DEFAULT_AWB_GAIN ; 
  config.wb_mode =  SETTINGS_DEFAULT_WB_MODE ; 
  config.exposure_ctrl =  SETTINGS_DEFAULT_EXPOSURE_CTRL ; 
  config.aec2 =  SETTINGS_DEFAULT_AEC2 ; 
  config.ae_level =  SETTINGS_DEFAULT_AE_LEVEL ; 
  config.aec_value =  SETTINGS_DEFAULT_AEC_VALUE ; 
  config.gain_ctrl =  SETTINGS_DEFAULT_GAIN_CTRL ; 
  config.agc_gain =  SETTINGS_DEFAULT_AGC_GAIN ; 
  config.bpc =  SETTINGS_DEFAULT_BPC ; 
  config.wpc =  SETTINGS_DEFAULT_WPC ; 
  config.raw_gma =  SETTINGS_DEFAULT_RAW_GMA ; 
  config.lenc =  SETTINGS_DEFAULT_LENC ; 
  config.hmirror =  SETTINGS_DEFAULT_HMIRROR ; 
  config.vflip =  SETTINGS_DEFAULT_VFLIP ; 
  config.dcw =  SETTINGS_DEFAULT_DCW ; 
  config.colorbar =  SETTINGS_DEFAULT_COLORBAR ; 
}


void setup() {

  // Start the serial port. 
  Serial.begin(115200);
	Serial.setDebugOutput(true);  
	Serial.println();

  // Print version 
  Serial.print("FYI: ESP32-CAM SWS Timelapse v");  
  Serial.print(APPLICATION_VERSION_MAJOR);
  Serial.print(".");
  Serial.print(APPLICATION_VERSION_MINOR);
  Serial.print(".");
  Serial.println(APPLICATION_VERSION_PATH);

  // Starting 
  Serial.println("FYI: Starting");
  delay( 1000 ); 

  Serial.println("FYI: initFileSystem");
  initFileSystem();

  // Load the settings 
  if (!fileExists(PATH_CONFIGURATION_FILE)) {
    Serial.println("FYI: No settings file found. Creating default");  
    Serial.println(PATH_CONFIGURATION_FILE);
    DefaultConfiguration(global_config);
    SaveConfiguration(PATH_CONFIGURATION_FILE, global_config);
    ESP.restart(); 
    
  } else {
    LoadConfiguration(PATH_CONFIGURATION_FILE, global_config);
  }
  // ToDo: Create a settings file 



// initCamera
// ------------------------------
  Serial.println("FYI: initCamera");
  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if (psramFound())
  {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else
  {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // https://randomnerdtutorials.com/esp32-cam-ov2640-camera-settings/
  // The frame size can be set to one of these options:
  // FRAMESIZE_UXGA (1600 x 1200)
  // FRAMESIZE_QVGA (320 x 240)
  // FRAMESIZE_CIF (352 x 288)
  // FRAMESIZE_VGA (640 x 480)
  // FRAMESIZE_SVGA (800 x 600)
  // FRAMESIZE_XGA (1024 x 768)
  // FRAMESIZE_SXGA (1280 x 1024)

  // The image quality (jpeg_quality) can be a number between 0 and 63. 
  // A lower number means a higher quality. However, very low numbers for 
  // image quality, specially at higher resolution can make the ESP32-CAM to 
  // crash or it may not be able to take the photos properly.
  

  // Debug override. 
  config.frame_size = FRAMESIZE_UXGA;
  config.jpeg_quality = 10;
  config.fb_count = 2;


#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    GoToSleep(); return; 
  }

  sensor_t *s = esp_camera_sensor_get();
  //initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID)
  {
    s->set_vflip(s, 1);     //flip it back
    s->set_brightness(s, 1);  //up the blightness just a bit
    s->set_saturation(s, -2); //lower the saturation
  }
  //drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_UXGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif


  // -----------------------------------------

  Serial.println("Setting camera settings.");

  sensor_t * s2 = esp_camera_sensor_get();
  s2->set_brightness(s2, global_config.brightness);     // -2 to 2
  s2->set_contrast(s2, global_config.contrast);       // -2 to 2
  s2->set_saturation(s2, global_config.saturation);     // -2 to 2
  s2->set_special_effect(s2, global_config.special_effect); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
  s2->set_whitebal(s2, global_config.whitebal);       // 0 = disable , 1 = enable
  s2->set_awb_gain(s2, global_config.awb_gain);       // 0 = disable , 1 = enable
  s2->set_wb_mode(s2, global_config.wb_mode);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  s2->set_exposure_ctrl(s2, global_config.exposure_ctrl);  // 0 = disable , 1 = enable
  s2->set_aec2(s2, global_config.aec2);           // 0 = disable , 1 = enable
  s2->set_ae_level(s2, global_config.ae_level);       // -2 to 2
  s2->set_aec_value(s2, global_config.aec_value);    // 0 to 1200
  s2->set_gain_ctrl(s2, global_config.gain_ctrl);      // 0 = disable , 1 = enable
  s2->set_agc_gain(s2, global_config.agc_gain);       // 0 to 30
  s2->set_bpc(s2, global_config.bpc);            // 0 = disable , 1 = enable
  s2->set_wpc(s2, global_config.wpc);            // 0 = disable , 1 = enable
  s2->set_raw_gma(s2, global_config.raw_gma);        // 0 = disable , 1 = enable
  s2->set_lenc(s2, global_config.lenc);           // 0 = disable , 1 = enable
  s2->set_hmirror(s2, global_config.hmirror);        // 0 = disable , 1 = enable
  s2->set_vflip(s2, global_config.vflip);          // 0 = disable , 1 = enable
  s2->set_dcw(s2, global_config.dcw);            // 0 = disable , 1 = enable
  s2->set_colorbar(s2, global_config.colorbar);       // 0 = disable , 1 = enable
  
  s2->set_gainceiling(s2, (gainceiling_t)0);  // 0 to 6

  



  
  // -----------------------------------------


  // print settings 
  Serial.println("");
  Serial.println("Settings:");
  Serial.print("  Photo Timer (Seconds): "); Serial.println( SETTING_DEFAULT_PHOTO_TIMER );
  
  Serial.println("");
  Serial.println("Camera Settings:");
	// sensor_t *s2 = esp_camera_sensor_get();
	Serial.print( "  framesize: ") ; Serial.println( s2->status.framesize);
	Serial.print( "  quality: ") ; Serial.println( s2->status.quality);
	Serial.print( "  brightness: ") ; Serial.println( s2->status.brightness);
	Serial.print( "  contrast: ") ; Serial.println( s2->status.contrast);
	Serial.print( "  saturation: ") ; Serial.println( s2->status.saturation);
	Serial.print( "  sharpness: ") ; Serial.println( s2->status.sharpness);
	Serial.print( "  special_effect: ") ; Serial.println( s2->status.special_effect);
	Serial.print( "  wb_mode: ") ; Serial.println( s2->status.wb_mode);
	Serial.print( "  awb: ") ; Serial.println( s2->status.awb);
	Serial.print( "  awb_gain: ") ; Serial.println( s2->status.awb_gain);
	Serial.print( "  aec: ") ; Serial.println( s2->status.aec);
	Serial.print( "  aec2: ") ; Serial.println( s2->status.aec2);
	Serial.print( "  ae_level: ") ; Serial.println( s2->status.ae_level);
	Serial.print( "  aec_value: ") ; Serial.println( s2->status.aec_value);
	Serial.print( "  agc: ") ; Serial.println( s2->status.agc);
	Serial.print( "  agc_gain: ") ; Serial.println( s2->status.agc_gain);
	Serial.print( "  gainceiling: ") ; Serial.println( s2->status.gainceiling);
	Serial.print( "  bpc: ") ; Serial.println( s2->status.bpc);
	Serial.print( "  wpc: ") ; Serial.println( s2->status.wpc);
	Serial.print( "  raw_gma: ") ; Serial.println( s2->status.raw_gma);
	Serial.print( "  lenc: ") ; Serial.println( s2->status.lenc);
	Serial.print( "  vflip: ") ; Serial.println( s2->status.vflip);
	Serial.print( "  hmirror: ") ; Serial.println( s2->status.hmirror);
	Serial.print( "  dcw: ") ; Serial.println( s2->status.dcw);
	Serial.print( "  colorbar: ") ; Serial.println( s2->status.colorbar);


  if( global_config.system_mode == SYSTEM_MODE_TIME_LAPES ) {
    SystemTimeLapse(); 
  }
  if( global_config.system_mode == SYSTEM_MODE_WIFI ) {
    SystemWiFiConfiguration();




    
  } else {
    Serial.print( "Error: unknonw mode. Mode=") ; Serial.println( global_config.system_mode);
    Serial.println( "reset settings to defaults");
    DefaultConfiguration(global_config);
    SaveConfiguration(PATH_CONFIGURATION_FILE, global_config);
    ESP.restart(); 
  }
}




void SystemWiFiConfiguration() {
  /*
    
  
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    startCameraServer();
    Serial.print("Camera Ready! Use 'http://");
    Serial.print(WiFi.localIP());
    Serial.println("' to connect");


*/
  
}

void SystemTimeLapse() {


  //Increment boot number and print it every reboot
  Serial.println("Boot number: " + String(bootCount));

  // Look though the root folders to seee what lapseIndex we are on 
  char path[32];  
  if( bootCount == 0 ) {
    Serial.println("Calulating lapseIndex...");
    for(lapseIndex = 0 ; lapseIndex < 100; lapseIndex++) {
        sprintf(path, "/lapse%03d", lapseIndex);
        if (!fileExists(path)) {
            createDir(path);
            break ; 
        }
    }
  }
  Serial.println("lapseIndex: " + String(lapseIndex));




  // Take the photo 
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Camera capture failed");
    GoToSleep();
    return ;
  }

  sprintf(path, "/lapse%03d/pic%05d.jpg", lapseIndex, bootCount);  
  if(!writeFile(path, (const unsigned char *)fb->buf, fb->len))
  {
      Serial.println("Error: Could not write to SD Card" );
      GoToSleep();
  }
  
  
  esp_camera_fb_return(fb);


  // All done, go to sleep 
  GoToSleep();
}



void GoToSleep() {
  bootCount++; 

  // Calulate the remaining time between shots 
  int32_t sleeptime = (SETTING_DEFAULT_PHOTO_TIMER * 1000) - millis() ;
  if( sleeptime <= 0 ) {
    sleeptime = 3 * 1000 ; 
  }

  Serial.println("\n\n");
  Serial.println("Going to sleep for " + String(sleeptime / 1000 ) + " Seconds");
  Serial.println("\n\n");
  delay( 10 ); 
  esp_sleep_enable_timer_wakeup((sleeptime / 1000 ) * uS_TO_S_FACTOR);
  esp_deep_sleep_start(); //Restart cam
}




// Loads the configuration from a file
void LoadConfiguration(const char *filename, ApplicationConfig &config) {
  // Open file for reading
  File file = SD_MMC.open(filename);

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use https://arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<512> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println(F("Failed to read file, using default configuration"));

  config.photo_timer       = doc["photo_timer"]     | SETTING_DEFAULT_PHOTO_TIMER ; 
  config.system_mode       = doc["system_mode"]     | SYSTEM_MODE_TIME_LAPES ; 

  // Copy values from the JsonDocument to the Config
  config.brightness       = doc["brightness"]     | SETTINGS_DEFAULT_BRIGHTNESS ; 
  config.contrast         = doc["contrast"]       | SETTINGS_DEFAULT_CONTRAST ; 
  config.saturation       = doc["saturation"]     | SETTINGS_DEFAULT_SATURATION ; 
  config.special_effect   = doc["special_effect"] | SETTINGS_DEFAULT_SPECIAL_EFFECT ; 
  config.whitebal         = doc["whitebal"]       | SETTINGS_DEFAULT_WHITEBAL ; 
  config.awb_gain         = doc["awb_gain"]       | SETTINGS_DEFAULT_AWB_GAIN ; 
  config.wb_mode          = doc["wb_mode"]        | SETTINGS_DEFAULT_WB_MODE ; 
  config.exposure_ctrl    = doc["exposure_ctrl"]  | SETTINGS_DEFAULT_EXPOSURE_CTRL ; 
  config.aec2             = doc["aec2"]           | SETTINGS_DEFAULT_AEC2 ; 
  config.ae_level         = doc["ae_level"]       | SETTINGS_DEFAULT_AE_LEVEL ; 
  config.aec_value        = doc["aec_value"]      | SETTINGS_DEFAULT_AEC_VALUE ; 
  config.gain_ctrl        = doc["gain_ctrl"]      | SETTINGS_DEFAULT_GAIN_CTRL ; 
  config.agc_gain         = doc["agc_gain"]       | SETTINGS_DEFAULT_AGC_GAIN ; 
  config.bpc              = doc["bpc"]            | SETTINGS_DEFAULT_BPC ; 
  config.wpc              = doc["wpc"]            | SETTINGS_DEFAULT_WPC ; 
  config.raw_gma          = doc["raw_gma"]        | SETTINGS_DEFAULT_RAW_GMA ; 
  config.lenc             = doc["lenc"]           | SETTINGS_DEFAULT_LENC ; 
  config.hmirror          = doc["hmirror"]        | SETTINGS_DEFAULT_HMIRROR ; 
  config.vflip            = doc["vflip"]          | SETTINGS_DEFAULT_VFLIP ; 
  config.dcw              = doc["dcw"]            | SETTINGS_DEFAULT_DCW ; 
  config.colorbar         = doc["colorbar"]       | SETTINGS_DEFAULT_COLORBAR ; 


  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();
  Serial.println("Settings file loaded.");
}

void SaveConfiguration(const char *filename, ApplicationConfig &config) {
  // Delete existing file, otherwise the configuration is appended to the file
  SD_MMC.remove(filename);

  // Open file for writing
  File file = SD_MMC.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println(F("Failed to create file"));
    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use https://arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<256> doc;

  // Set the values in the document
  doc["photo_timer"] = config.photo_timer;
  doc["system_mode"] = config.system_mode;

  doc["brightness"]     = config.brightness ;
  doc["contrast"]       = config.contrast ;
  doc["saturation"]     = config.saturation ;
  doc["special_effect"] = config.special_effect ;
  doc["whitebal"]       = config.whitebal ;
  doc["awb_gain"]       = config.awb_gain ;
  doc["wb_mode"]        = config.wb_mode ;
  doc["exposure_ctrl"]  = config.exposure_ctrl ;
  doc["aec2"]           = config.aec2 ;
  doc["ae_level"]       = config.ae_level ;
  doc["aec_value"]      = config.aec_value ;
  doc["gain_ctrl"]      = config.gain_ctrl ;
  doc["agc_gain"]       = config.agc_gain ;
  doc["bpc"]            = config.bpc ;
  doc["wpc"]            = config.wpc ;
  doc["raw_gma"]        = config.raw_gma ;
  doc["lenc"]           = config.lenc ;
  doc["hmirror"]        = config.hmirror ;
  doc["vflip"]          = config.vflip ;
  doc["dcw"]            = config.dcw ;
  doc["colorbar"]       = config.colorbar ;


  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write to file"));
  }

  // Close the file
  file.close();
  Serial.println("Settings file saved");
}




void loop() {
  // Should never be called. 
  delay(1000) ;
}

// SWS-ESP32-TimeLapse 
// Written by Steven Smethurst (https://blog.abluestar.com)
// 
// More information: https://github.com/funvill/SWS-ESP32-TimeLapse 
// 
// 

#include <WiFi.h>
#include "file.h"

#include "Arduino.h"
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
const uint16_t APPLICATION_VERSION_MINOR = 1 ; 
const uint16_t APPLICATION_VERSION_PATH  = 0 ; 


// Settings defaults 
const uint32_t SETTING_DEFAULT_PHOTO_TIMER = 10 ; // In Seconds 


// Consts 
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

// Running variables 
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int lapseIndex = 0;

void GoToSleep(); 



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

/*
  // Load the settings 
  if (!fileExists("settings.txt")) {
    Serial.println("FYI: No settings file found. Creating default");  
    Serial.println("/settings.txt");  
  }
  // ToDo: Create a settings file 
*/


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

  Serial.println("Setting camera seetings.");

  sensor_t * s2 = esp_camera_sensor_get();
  s2->set_brightness(s2, 0);     // -2 to 2
  s2->set_contrast(s2, 0);       // -2 to 2
  s2->set_saturation(s2, 0);     // -2 to 2
  s2->set_special_effect(s2, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
  s2->set_whitebal(s2, 1);       // 0 = disable , 1 = enable
  s2->set_awb_gain(s2, 1);       // 0 = disable , 1 = enable
  s2->set_wb_mode(s2, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  s2->set_exposure_ctrl(s2, 1);  // 0 = disable , 1 = enable
  s2->set_aec2(s2, 0);           // 0 = disable , 1 = enable
  s2->set_ae_level(s2, 0);       // -2 to 2
  s2->set_aec_value(s2, 300);    // 0 to 1200
  s2->set_gain_ctrl(s2, 1);      // 0 = disable , 1 = enable
  s2->set_agc_gain(s2, 0);       // 0 to 30
  s2->set_gainceiling(s2, (gainceiling_t)0);  // 0 to 6
  s2->set_bpc(s2, 0);            // 0 = disable , 1 = enable
  s2->set_wpc(s2, 1);            // 0 = disable , 1 = enable
  s2->set_raw_gma(s2, 1);        // 0 = disable , 1 = enable
  s2->set_lenc(s2, 1);           // 0 = disable , 1 = enable
  s2->set_hmirror(s2, 0);        // 0 = disable , 1 = enable
  s2->set_vflip(s2, 0);          // 0 = disable , 1 = enable
  s2->set_dcw(s2, 1);            // 0 = disable , 1 = enable
  s2->set_colorbar(s2, 0);       // 0 = disable , 1 = enable
  



  
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



void loop() {
  // Should never be called. 
  delay(1000) ;
}

# SWS-ESP32-TimeLapse

Quick and dirty ESP32-cam timelapse camera.

- Starts up and prints version
- Loads settings file if one exists, if not creates one in root folder, then restarts
- Sets up camera based on settings file
- Reads directories in root and creates new folder for new session.
- Takes a photo
- Saves photo into /{lapesIndex}/{BootIndex}
- Goes into deep sleep mode for X secounds based on settings.

ToDo

- If configuration pin is high, then go into AP mode to allow users to configure and preview the image.
- If configurated for wifi, connect to the wifi and send the photo home (MQTT, FTP, etc...), if can't connect, save to SD card and try again.

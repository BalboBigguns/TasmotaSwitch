# Tasmota-Switch
Code to flash tasmota switches to work with hassio

How to connect it:
https://youtu.be/chyVjtYb0EA

1. Open PlatformIO projct in VS Code.
2. Change MQTT PUB/SUB channels.
3. Enter following commands:
    - platformio run
    - platformio run -t upload
4. Configure component on the Hassio server:
   - In configuration.yaml:
     - copy existing switch config
     - change name
     - change PUB/SUB channels
     - change device unique ID (it is visible in router's statistics in device name field)
   - In ui-lovelace.yaml:
     - create component and put it in the right place

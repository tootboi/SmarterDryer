# SmarterDryer
### Smartify an old dummy dryer

This project uses an ESP32 to detect the vibration of a running dryer and notify the user when the dryer stops, via an email.

Components used:
- Generic ESP32 with G22 (SCL) and G21 (SDA) pins for I2C communication
- MPU6050
- SSD1306 OLED display
- 1 led
- 1 switch
- 1 220R resistor for the led

## Circuit diagram
![smarterDryerDiagram](https://raw.githubusercontent.com/tootboi/SmarterDryer/main/src/img/smarterDryerDiagram.png)

## How it works
The code samples the x, y and z acceleration every clock cycle using the MPU6050. It calculates the delta (the change in acceleration) by comparing the previous and current acceleration data. If the delta of any axis (x, y or z) exceeds the delta threshold, then vibration is detected, which signals that the dryer is currently running.

When both previous and current delta reached booleans have been false for 10 continuous seconds, the code detects that the dryer has stopped. It then sends an email to the user and enters deep sleep mode.

The ESP32 can be awakened by closing the switch connected to G13, which triggers an external wake up from deep sleep.

### Notes
- it probably makes more sense to use a vibration sensor module rather than the MPU6050 module. I just used what I had in hand.
- the ssd1306 oled display is used for easier debugging and monitoring. I used sockets so that the display can be easily attached and detached as needed.
- future improvement I want to look into is using the MQTT protocol rather than emailing the user for notification.

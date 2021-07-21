#include <Adafruit_MPU6050.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <ESP_Mail_Client.h>

Adafruit_MPU6050 mpu;
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);

// for vibration detection
int prevAccel[3]; // {x, y, z}
int deltaThreshold = 40;
bool deltaStatus = false;
bool prevDeltaStatus = false;
// for deep sleep
#define wakeBtn 13  // wakeBtn connected to GPIO 13
int timeoutStart;
int timeout = 1000 * 10;   // 1000 == 1 second (default set to 10 seconds)
const byte led = 32;  // led connected to GPIO 32
// variables for wifi
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
// variables for email
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define AUTHOR_EMAIL "AUTHOR_EMAIL@GMAIL.COM"
#define AUTHOR_PASSWORD "AUTHOR_EMAIL_PASSWORD"
#define RECIPIENT_EMAIL "RECIPIENT_EMAIL@DOMAIN.COM"
SMTPSession smtp;
SMTP_Message message;

void setup() {
  Serial.begin(9600);

  // setup for mpu6050 and ssd1306 oled display
  if (!mpu.begin()) {
    Serial.println("Sensor init failed");
    while (1)
      yield();
  }
  Serial.println("Found a MPU6050 sensor");
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  display.display();
  delay(500); // Pause for 2 seconds
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setRotation(0);

  // setup deep sleep & wake up
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 1);
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  pinMode(wakeBtn, PULLDOWN);
  // connect to wifi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(200);
      Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  // setup for email
  message.sender.name = "Smarter Dryer";  // change as needed
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "Dryer done";  // change as needed
  message.addRecipient("Someone", RECIPIENT_EMAIL);
  String textMsg = " ";
  message.text.content = textMsg.c_str();
  ESP_Mail_Session session;
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";
  if (!smtp.connect(&session)) {
    return;
  }
}

void loop() {
  digitalWrite(led, HIGH);  // turn led on to show esp32 running

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  display.clearDisplay();
  display.setCursor(0, 0);

  display.println("Accelerometer - m/s^2");
  display.print(a.acceleration.x*100, 0);
  display.print(", ");
  display.print(a.acceleration.y*100, 0);
  display.print(", ");
  display.print(a.acceleration.z*100, 0);
  display.println("");

  // check if accel in any axis has exceeded the delta required
  for(int i = 0; i< sizeof(prevAccel)/sizeof(prevAccel[0]); i++) {
    int delta;
    switch(i) {
      case 0:
        delta = prevAccel[i] - a.acceleration.x*100;  // calculate delta
        prevAccel[i] = a.acceleration.x*100;  // update prevAccel
        break;
      case 1:
        delta = prevAccel[i] - a.acceleration.y*100;  // calculate delta
        prevAccel[i] = a.acceleration.y*100;  // update prevAccel
        break;
      case 2:
        delta = prevAccel[i] - a.acceleration.z*100;  // calculate delta
        prevAccel[i] = a.acceleration.z*100;  // update prevAccel
        break;
      
      default:
        break;
    }
    // check for delta
    if(delta >= deltaThreshold) {
      deltaStatus = true;
    }
    display.print(i);
    display.print(": ");
    display.print(abs(delta));
    display.print(" | ");
    display.println(prevAccel[i]);
  }

  display.println(deltaStatus? "true":"false");
  // check for falling edge
  if(prevDeltaStatus == true && deltaStatus == false) {
    timeoutStart = millis();
  } 
  // check if delta was false for two cycles (i.e. no vibrations)
  else if(prevDeltaStatus==false && deltaStatus==false) {
    if(millis() - timeoutStart > timeout) { // check if timeout has been reached
      // clear display
      display.clearDisplay();
      display.display();
      // send email
      if (!MailClient.sendMail(&smtp, &message)) {
        Serial.println("Error sending Email, " + smtp.errorReason());
      }
      // enter deep sleep
      esp_deep_sleep_start();
    }
  }

  prevDeltaStatus = deltaStatus;  // update prevDeltaStatus
  deltaStatus = false;  // reset deltaStatus

  display.display();

  delay(100);
}
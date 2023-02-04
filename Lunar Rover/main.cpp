#include <Arduino.h>
#define USE_WIFI_NINA false
#define USE_WIFI101 true
#include <WiFiWebServer.h>
#include "arduinoFFT.h"
arduinoFFT FFT = arduinoFFT(); /* Create FFT object */

const uint16_t samples = 64;           // This value MUST ALWAYS be a power of 2
const double samplingFrequency = 1400; // Hz, must be less than 10000 due to ADC

unsigned int sampling_period_us;
unsigned long microseconds;

double vReal_radio[samples];
double vReal_ir[samples];
double vImag[samples];

#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03

int maximum = 0;

const char ssid[] = "TP-Link_3416";
const char pass[] = "04809524";

const int groupNumber = 221; // Set your group number to make the IP address constant - only do this on the EEERover network

const char webpage[] =
    "<html><head><style>\
.btn {background-color: inherit;padding: 14px 28px;font-size: 16px;}\
.btn:hover {background: #eee;}\
</style></head>\
<body>\
<button class=\"btn\" onclick=\"ledOn()\">LED On</button>\
<button class=\"btn\" onclick=\"ledOff()\">LED Off</button>\
<br>xhttp readystate: <span id=\"state\">OFF</span>\
<br><button class=\"btn\" onclick=\"motorSL()\">Steer Left</button>\
<button class=\"btn\" onclick=\"motorSR()\">Steer Right</button>\
<br><button class=\"btn\" onclick=\"telemID()\">Run AutoID Routine</button>\
</body>\
<script>\
var xhttp = new XMLHttpRequest();\
xhttp.onreadystatechange = function() {if (this.readyState == 4 && this.status == 200) {document.getElementById(\"state\").innerHTML = this.responseText;}};\
function ledOn() {xhttp.open(\"GET\", \"/on\"); xhttp.send();}\
function ledOff() {xhttp.open(\"GET\", \"/off\"); xhttp.send();}\
function motorFWPos() {xhttp.open(\"GET\", \"/motorfwpos\"); xhttp.send();}\
function motorFWNeg() {xhttp.open(\"GET\", \"/motorfwneg\"); xhttp.send();}\
function motorRVPos() {xhttp.open(\"GET\", \"/motorrvpos\"); xhttp.send();}\
function motorRVNeg() {xhttp.open(\"GET\", \"/motorrvneg\"); xhttp.send();}\
function motorSR() {xhttp.open(\"GET\", \"/motorsr\"); xhttp.send();}\
function motorRV() {xhttp.open(\"GET\", \"/motorrv\"); xhttp.send();}\
function telemID() {xhttp.open(\"GET\", \"/telemid\"); xhttp.send();}\
</script></html>";

WiFiWebServer server(80);

void handleRoot(){
  server.send(200, F("text/html"), webpage);
}

void ledON(){
  digitalWrite(LED_BUILTIN, 1);
  Serial.print("ledon");
  server.send(200, F("text/plain"), F("ON"));
}

void ledOFF(){
  digitalWrite(LED_BUILTIN, 0);
  server.send(200, F("text/plain"), F("OFF"));
}

// control PIN MAPPING:
// LEFT DIRECTION PIN 0 or 1 - 6
// LEFT ENABLE PIN 0 or 1 - 7
// RIGHT DIRECTION PIN 0 or 1 - 8
// RIGHT ENABLE PIN 0 or 1 - 9
int left_dir = 3;
int left_en = 4; 
int right_dir = 8;
int right_en = 1;
void motorFW_pos_edge(){
  // SET BOTH LEFT AND RIGHT ENABLE PINS TO 1
  // SET BOTH LEFT AND RIGHT DIR PINS TO 1
  digitalWrite(left_dir, HIGH);
  digitalWrite(left_en, HIGH);
  digitalWrite(right_dir, LOW);
  digitalWrite(right_en, HIGH);
  server.send(200, F("text/plain"), F("ON"));
}

void motorFW_neg_edge(){
  digitalWrite(left_en, LOW);
  digitalWrite(right_en, LOW);
  server.send(200, F("text/plain"), F("ON"));
}

void motorRV_pos_edge(){
  digitalWrite(left_dir, LOW);
  digitalWrite(left_en, HIGH);
  digitalWrite(right_dir, HIGH);
  digitalWrite(right_en, HIGH);
  server.send(200, F("text/plain"), F("ON"));
}

void motorRV_neg_edge()
{
  digitalWrite(left_en, LOW);
  digitalWrite(right_en, LOW);
  server.send(200, F("text/plain"), F("ON"));
}

void motorTL_pos_edge(){
  digitalWrite(left_dir, HIGH);
  digitalWrite(left_en, HIGH);
  digitalWrite(right_dir, HIGH);
  digitalWrite(right_en, HIGH);
  server.send(200, F("text/plain"), F("ON"));
}

void motorTL_neg_edge()
{
  digitalWrite(left_en, LOW);
  digitalWrite(right_en, LOW);
  server.send(200, F("text/plain"), F("ON"));
}

void motorTR_pos_edge()
{
  digitalWrite(left_dir, LOW);
  digitalWrite(left_en, HIGH);
  digitalWrite(right_dir, LOW);
  digitalWrite(right_en, HIGH);
  server.send(200, F("text/plain"), F("ON"));
}

void motorTR_neg_edge()
{
  digitalWrite(left_en, LOW);
  digitalWrite(right_en, LOW);
  server.send(200, F("text/plain"), F("ON"));
}
void simtelemID(){
  server.send(201, F("text/plain"), F("0Hz, 0Hz, 0Hz, NSD"));
}
void telemID(){
  maximum = 0;

  digitalWrite(2, LOW);
  delay(1);
  double time1 = millis();
  while (millis() < time1 + 6.6)
  {
    int value = analogRead(A4);
    if (value > maximum)
    {
      maximum = value;
    }
  }
  int pre_max = maximum;
  delay(500);

  digitalWrite(2, HIGH);
  maximum = 0;
  delay(1);
  double time2 = millis();
  while (millis() < time2 + 6.6)
  {
    int value = analogRead(A4);
    if (value > maximum)
    {
      maximum = value;
    }
  }
  int post_max = maximum;

  int which = 0;
  if ((post_max - pre_max) > 15)
  {
    Serial.println("61kHz detected!"); // mosfet on detects 61kHz
    which = 61;
  }
  else if ((pre_max - post_max) > 15)
  {
    Serial.println("89kHz detected!"); // mosfet off detects 89kHz
    which = 89;
  }
  else
  {
    Serial.println("Neither carrier!"); // default value
    which = 0;
  }
  delay(500);
  /*SAMPLING*/

  if (which == 89)
  {
    digitalWrite(2, LOW); // turn off the MOSFET
  }

  if (which == 61)
  {
    digitalWrite(2, HIGH);
  }

  // radio signal detection
  microseconds = micros(); // Returns the number of microseconds since the Arduino board began running the current program
  for (int i = 0; i < samples; i++)
  {
    vReal_radio[i] = analogRead(A5);
    vImag[i] = 0;
    while (micros() - microseconds < sampling_period_us)
    {
      // empty loop
    }
    microseconds += sampling_period_us;
  }

  FFT.Windowing(vReal_radio, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD); /* Weigh data */
  FFT.Compute(vReal_radio, vImag, samples, FFT_FORWARD);                 /* Compute FFT */
  FFT.ComplexToMagnitude(vReal_radio, vImag, samples);                   /* Compute magnitudes */

  double x1 = FFT.MajorPeak(vReal_radio, samples, samplingFrequency);

  // radio
  if (which == 61 && x1 > 140 && x1 < 160)
  {
    Serial.println(x1, 6); // Print out what frequency is the most dominant.
    Serial.println("Gaborium");
    server.send(200, F("text/plain"), F("Gaborium"));
  }
  else if (which == 61 && x1 > 229 && x1 < 250)
  {
    Serial.println(x1, 6); // Print out what frequency is the most dominant.
    Serial.println("Lathwaite");
    server.send(200, F("text/plain"), F("Lathwaite"));
  }
  else if (which == 89 && x1 > 140 && x1 < 160)
  {
    Serial.println(x1, 6); // Print out what frequency is the most dominant.
    Serial.println("Adamantine");
    server.send(200, F("text/plain"), F("Adamantite"));
  }
  else if (which == 89 && x1 > 229 && x1 < 250)
  {
    Serial.println(x1, 6); // Print out what frequency is the most dominant.
    Serial.println("Xirang");
    server.send(200, F("text/plain"), F("Xirang"));
  }
  // infrared or noise
  else
  {
    for (int i = 0; i < 15; i++) // sampling and FFT 15 times --> print 15 frequency output
    {
      microseconds = micros(); // Returns the number of microseconds since the Arduino board began running the current program
      for (int i = 0; i < samples; i++)
      {
        vReal_ir[i] = analogRead(A3);
        vImag[i] = 0;
        while (micros() - microseconds < sampling_period_us)
        {
          // empty loop
        }
        microseconds += sampling_period_us;
      }
      FFT.Windowing(vReal_ir, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD); /* Weigh data */
      FFT.Compute(vReal_ir, vImag, samples, FFT_FORWARD);                 /* Compute FFT */
      FFT.ComplexToMagnitude(vReal_ir, vImag, samples);                   /* Compute magnitudes */

      double x2 = FFT.MajorPeak(vReal_ir, samples, samplingFrequency);

      if (x2 >= 355 && x2 <= 365) // 353Hz
      {
        Serial.println(x2, 6); // Print out what frequency is the most dominant.
        Serial.println("Thiotimoline");
        server.send(200, F("text/plain"), F("Thiotimoline"));
      }
      else if (x2 >= 576 && x2 <= 586) // 571Hz
      {
        Serial.println(x2, 6); // Print out what frequency is the most dominant.
        Serial.println("Netherite");
        server.send(200, F("text/plain"), F("Netherite"));
      }
      else
      {
        Serial.println(x2, 6); // Print out what frequency is the most dominant.
        Serial.println("No signal detected!");
        server.send(200, F("text/plain"), F("Nothing"));
      }
    }
  }
}


void setup(){
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);

  pinMode(A5, INPUT);
  pinMode(A4, INPUT);
  pinMode(A3, INPUT);

  sampling_period_us = round(1000000 * (1.0 / samplingFrequency));
  //Serial.begin(115200);

  //Wait 10s for the serial connection before proceeding
  //This ensures you can see messages from startup() on the monitor
  //Remove this for faster startup when the USB host isn't attached
  while (!Serial && millis() < 10000);

  //Serial.println(F("\nStarting Web Server"));

  //Check WiFi shield is present
  if (WiFi.status() == WL_NO_SHIELD){
    //Serial.println(F("WiFi shield not present"));
    while (true);
  }

  //Configure the static IP address if group number is set
  if (groupNumber){
    WiFi.config(IPAddress(192, 168, 0, groupNumber + 1));
  }
  // attempt to connect to WiFi network
  //Serial.print(F("Connecting to WPA SSID: "));
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED){
    delay(500);
    Serial.print('.');
  }

  //Register the callbacks to respond to HTTP requests
  server.on(F("/"), handleRoot);
  server.on(F("/on"), ledON);
  server.on(F("/off"), ledOFF);
  server.on(F("/motorfwpos"), motorFW_pos_edge);
  server.on(F("/motorfwneg"), motorFW_neg_edge);

  server.on(F("/motorrvpos"), motorRV_pos_edge);
  server.on(F("/motorrvneg"), motorRV_neg_edge);

  server.on(F("/motortlpos"), motorTL_pos_edge);
  server.on(F("/motortlneg"), motorTL_neg_edge);

  server.on(F("/motortrpos"), motorTR_pos_edge);
  server.on(F("/motortrneg"), motorTR_neg_edge);
  
  server.on(F("/telemid"), telemID);


  server.begin();

  Serial.print(F("HTTP server started @ "));
  Serial.println(static_cast<IPAddress>(WiFi.localIP()));

}

//Call the server polling function in the main loop
void loop(){
  server.handleClient();
}
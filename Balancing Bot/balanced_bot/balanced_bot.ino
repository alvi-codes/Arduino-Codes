#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "johan";
const char* password = "password";

const char* serverURL = "http://192.168.43.199:5000/process";

////////////////////////////   INITIALIZATIONS   //////////////////////////// 
float test_array[4][4] = {0};

void connect_WiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

String send_and_receive(float Node_Data[4][4]) {
  HTTPClient http;

  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");

  const int rows = 4;
  const int cols = 4;
  StaticJsonDocument<1024> jsonDocument;
  JsonArray jsonArray = jsonDocument.to<JsonArray>();

  for (int i = 0; i < rows; i++) {
    JsonArray rowArray = jsonArray.createNestedArray();
    for (int j = 0; j < cols; j++) {
      rowArray.add(Node_Data[i][j]);
    }
  }

  String payload;
  serializeJson(jsonArray, payload);

  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    String response = http.getString();
    return response;
  }
  else{
    return "Sending to server is unsuccessful :(\n";
  }

  http.end();
}

int pinBit1 = 25;   // Pin for bit 1 (least significant bit)
int pinBit2 = 26;   // Pin for bit 2
int pinBit3 = 27;   // Pin for bit 3 (most significant bit) 27, 26, 25

  
void no_step(){
  digitalWrite(pinBit1, LOW);
  digitalWrite(pinBit2, LOW);
  digitalWrite(pinBit3, LOW);
}

void f_step(){
  digitalWrite(pinBit1, HIGH);
  digitalWrite(pinBit2, LOW);
  digitalWrite(pinBit3, LOW);
}

void l_step(){
  digitalWrite(pinBit1, HIGH);
  digitalWrite(pinBit2, HIGH);
  digitalWrite(pinBit3, LOW);
}

void r_step(){
  digitalWrite(pinBit1, LOW);
  digitalWrite(pinBit2, LOW);
  digitalWrite(pinBit3, HIGH);
}


void setup() {
  Serial.begin(9600);
  delay(100);
  connect_WiFi();
  pinMode(pinBit1, OUTPUT);
  pinMode(pinBit2, OUTPUT);
  pinMode(pinBit3, OUTPUT);
}

void loop() {

  String rec_data = send_and_receive(test_array);
  Serial.print(rec_data);
  int signal = rec_data.toInt(); 

  switch(signal){
    case 1:
      f_step();
      delay(50);
      no_step();
    case 2:
      r_step();
      delay(50);
      no_step();
    case 3:
      l_step();
      delay(50);
      no_step();
    default:
      no_step();
  }
  delay(1000);

}
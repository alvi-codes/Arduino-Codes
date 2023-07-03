#include <Wire.h>
#include <Arduino.h>
#include <AccelStepper.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const int stepsPerRevolution = 100;

double F = 0;

int dir1 = 14;
int dir2 = 26;
int step1 = 12;
int step2 = 27;

int countRight = 0;
int countForward = 0;


const char* ssid = "johan";
const char* password = "password";

const char* serverURL = "http://192.168.43.199:5000/process";

#define SAMPLES 4
#define DATA_ELEMENTS 7 // R_area, B_area, Y_area, offset, R_turn, F_turn, L_turn
#define ABSOLUTE_ERROR 10

float Image[4] = {};
float Cam_Data[DATA_ELEMENTS] = {};
float Curr_Data[DATA_ELEMENTS] = {};
float Node_Data[4][4];
static float Cam_Data_Buff[SAMPLES][DATA_ELEMENTS] = {};
int image_number = 0;
bool register_new_image = false;
String response = "";

#define RX_PIN 16
#define TX_PIN 17
#define DATA_SIZE 4 //bytes

TaskHandle_t core0Task;
portMUX_TYPE core0Mutex = portMUX_INITIALIZER_UNLOCKED;

// Memory addresses to write to
volatile uint32_t* memAddress1 = reinterpret_cast<uint32_t*>(0x3FFFC000);
volatile uint32_t* memAddress2 = reinterpret_cast<uint32_t*>(0x3FFFC004);
volatile uint32_t* memAddress3 = reinterpret_cast<uint32_t*>(0x3FFFC008);
volatile uint32_t* memAddress4 = reinterpret_cast<uint32_t*>(0x3FFFC00C);
volatile uint32_t* memAddress5 = reinterpret_cast<uint32_t*>(0x3FFFC010);
volatile uint32_t* memAddress6 = reinterpret_cast<uint32_t*>(0x3FFFC014);
volatile uint32_t* memAddress7 = reinterpret_cast<uint32_t*>(0x3FFFC018);


////////////////////////////   FUNCTIONS   ////////////////////////////

void connect_WiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}



void send_and_receive(int32_t Cam_Data[7], int dir) {
  int32_t Data_To_Send[8] = {0};
  
  for (int i = 0; i<7; i++){
    Data_To_Send[i] = Cam_Data[i];
  }
  Data_To_Send[7] = dir;

  HTTPClient http;

  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<512> jsonDocument;
  JsonArray jsonArray = jsonDocument.to<JsonArray>();

  for (int i = 0; i < 8; i++) {
    jsonArray.add(Data_To_Send[i]);
  }

  String payload;
  serializeJson(jsonArray, payload);

  int httpResponseCode = http.POST(payload);

  http.end();
}

void move_bot(int roverState){

     if(roverState == 1){ //Forward
        Serial.print("F");
        digitalWrite(dir1, HIGH);
        digitalWrite(dir2, LOW);
          for(int i=0; i<20; i++){
            digitalWrite(step1, HIGH);
            digitalWrite(step2, HIGH);
            delay(5);
            digitalWrite(step1, LOW);
            digitalWrite(step2, LOW);
            delay(5);

          }
      }

      else if (roverState == 4){ //Backward
        Serial.print("B");
        digitalWrite(dir1, LOW);
        digitalWrite(dir2, HIGH);
        for(int i=0; i<20; i++){
            digitalWrite(step1, HIGH);
            digitalWrite(step2, HIGH);
            delay(8);
            digitalWrite(step1, LOW);
            digitalWrite(step2, LOW);
            delay(8);
          }
      }

      else if (roverState == 22){ //Right_Half
        Serial.print("R_Half");
        digitalWrite(dir1, HIGH);
        digitalWrite(dir2, HIGH);
        for(int i=0; i<25; i++){
            digitalWrite(step1, HIGH);
            digitalWrite(step2, HIGH);
            delay(8);
            digitalWrite(step1, LOW);
            digitalWrite(step2, LOW);
            delay(8);
          }
      }

      else if (roverState == 2){ //Right
        Serial.print("R");
        digitalWrite(dir1, HIGH);
        digitalWrite(dir2, HIGH);
        for(int i=0; i<10; i++){
            digitalWrite(step1, HIGH);
            digitalWrite(step2, HIGH);
            delay(8);
            digitalWrite(step1, LOW);
            digitalWrite(step2, LOW);
            delay(8);
          }
      }

      else if (roverState == 33){ //Left_Half
        Serial.print("L_Half");
        digitalWrite(dir1, LOW);
        digitalWrite(dir2, LOW);
        for(int i=0; i<50; i++){
            digitalWrite(step1, HIGH);
            digitalWrite(step2, HIGH);
            delay(10);
            digitalWrite(step1, LOW);
            digitalWrite(step2, LOW);
            delay(10);
          }
      }


      else if (roverState == 3){ //Left
        Serial.print("L");
        digitalWrite(dir1, LOW);
        digitalWrite(dir2, LOW);
        for(int i=0; i<100; i++){
            digitalWrite(step1, HIGH);
            digitalWrite(step2, HIGH);
            delay(10);
            digitalWrite(step1, LOW);
            digitalWrite(step2, LOW);
            delay(10);
          }
      }

}

uint32_t get_data() {
  byte buffer[DATA_SIZE];

  // Read the incoming bytes into the buffer
  for (int i = 0; i < DATA_SIZE; i++) {
    buffer[i] = Serial2.read();
  }

  std::swap(buffer[0], buffer[3]);
  std::swap(buffer[1], buffer[2]);

  uint32_t value = (static_cast<uint32_t>(buffer[0]) << 24) |
                   (static_cast<uint32_t>(buffer[1]) << 16) |
                   (static_cast<uint32_t>(buffer[2]) << 8) |
                   (static_cast<uint32_t>(buffer[3]));

  return value;
}

void read_data() {
  portENTER_CRITICAL(&core0Mutex);
  
  uint32_t readData1 = *memAddress1;
  uint32_t readData2 = *memAddress2;
  uint32_t readData3 = *memAddress3;
  uint32_t readData4 = *memAddress4;
  uint32_t readData5 = *memAddress5;
  uint32_t readData6 = *memAddress6;
  uint32_t readData7 = *memAddress7;

  portEXIT_CRITICAL(&core0Mutex);

  if (readData1 == 304964){
    readData1 = 0;
  }
  if (readData2 == 304964){
    readData2 = 0;
  }
  if (readData3 == 304964){
    readData3 = 0;
  }
  int32_t signedData4 = static_cast<int32_t>(readData4);

  Serial.println("Full Data obtained: ");
  Serial.print(readData1);
  Serial.print("  ");
  Serial.print(readData2);
  Serial.print("  ");
  Serial.print(readData3);
  Serial.print("  ");
  Serial.print(signedData4);
  Serial.print("  ");
  Serial.print(readData5);
  Serial.print("  ");
  Serial.print(readData6);
  Serial.print("  ");
  Serial.print(readData7);
  Serial.println(" "); 
}

void get_Cam_Data(int32_t Cam_Data[DATA_ELEMENTS]){
  //returns Cam_Data(R_area, B_area, Y_area, offset, R_turn, F_turn, L_turn)
  portENTER_CRITICAL(&core0Mutex);
  
  uint32_t Red = *memAddress1;
  uint32_t Yellow = *memAddress2;
  uint32_t Blue = *memAddress3;
  uint32_t Offset_raw = *memAddress4;
  uint32_t Right = *memAddress5;
  uint32_t Left = *memAddress6;
  uint32_t Forward = *memAddress7;

  portEXIT_CRITICAL(&core0Mutex);

  if (Red == 304964){
    Red = 0;
  }
  if (Yellow == 304964){
    Yellow = 0;
  }
  if (Blue == 304964){
    Blue = 0;
  }
  int32_t Offset = static_cast<int32_t>(Offset_raw);


  Cam_Data[0] = Red;
  Cam_Data[1] = Yellow;
  Cam_Data[2] = Blue;
  Cam_Data[3] = Offset;
  Cam_Data[4] = Right;
  Cam_Data[5] = Left;
  Cam_Data[6] = Forward;


}

void Core0Task(void* parameter) {
  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  while (true) {
    if (Serial2.available() >= DATA_SIZE) {
      uint32_t value = get_data();
      uint32_t data;
      if (value == 0x00524242) {
        data = get_data();
        portENTER_CRITICAL(&core0Mutex);
        memcpy((void*)memAddress1, &data, sizeof(data));
        data = get_data();
        memcpy((void*)memAddress2, &data, sizeof(data));
        data = get_data();
        memcpy((void*)memAddress3, &data, sizeof(data));
        data = get_data();
        memcpy((void*)memAddress4, &data, sizeof(data));
        data = get_data();
        memcpy((void*)memAddress5, &data, sizeof(data));
        data = get_data();
        memcpy((void*)memAddress6, &data, sizeof(data));
        data = get_data();
        memcpy((void*)memAddress7, &data, sizeof(data));
        portEXIT_CRITICAL(&core0Mutex);
      }
    }
    delay(1);  // Add a small delay to allow other tasks to run
  }
}

void setup() {
  Serial.begin(9600);
  delay(100);
  connect_WiFi();

  // Create Core 0 task
  xTaskCreatePinnedToCore(
      Core0Task,    // Task function
      "Core0Task",  // Task name (optional)
      10000,        // Stack size (bytes)
      NULL,         // Task parameters (optional)
      1,            // Task priority
      &core0Task,   // Task handle
      0             // Core to run the task on (Core 0)
  );

  pinMode(dir1, OUTPUT);
  pinMode(dir2, OUTPUT);
  pinMode(step1, OUTPUT);
  pinMode(step2, OUTPUT);
}


void loop() {

    int32_t Cam_Data[7] = {0};
    get_Cam_Data(Cam_Data);

  while(Cam_Data[6] == 0){
    move_bot(1);
    countForward++;
    if (countForward == 5){
      send_and_receive(Cam_Data, 1);
      countForward = 0;
    }
    get_Cam_Data(Cam_Data);
  }
  if(Cam_Data[6] == 1){
    move_bot(4);
    move_bot(4);
  }
  while(Cam_Data[6] == 1){
    move_bot(2);
    move_bot(2);
    move_bot(2);
    move_bot(2);
    move_bot(2);
    get_Cam_Data(Cam_Data);
    }
    

  countRight++;

  if(countRight == 5) {
    Serial.println(" TURN ");
    send_and_receive(Cam_Data, 2);
    countRight = 0;
  }
  
}
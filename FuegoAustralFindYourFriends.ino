#include <ESP8266WiFi.h>
#include <espnow.h>
#define COMMON_ANODE
// #define DEBUG
#define ESP01

// const int personValues[] = { 255, 0,    0,    30 }; // 1
// const int personValues[] = { 255, 255,  0,    30 }; // 2
// const int personValues[] = { 255, 255,  255,  30 }; // 3

const int personValues[] = { 255, 100,    0,    30 }; // 4
// const int personValues[] = { 255, 0,    255,  30 }; // 5
// const int personValues[] = { 0  , 255,  0,    30 }; // 6

// const int personValues[] = { 0  , 255,  255,  30 }; // 7 Day
// const int personValues[] = { 0  , 0,    255,  30 }; // 8
// const int personValues[] = { 50 , 255,  255,  30 }; // 9

// const int personValues[] = { 255, 210,  20 ,  30 }; // 10


#ifdef ESP01
  int ledpins[] = { 1, 0, 3 };
  int anodePin = 2;
#else
  int ledpins[] = { 13, 10, 15 };
  int anodePin = 2;
#endif

uint8_t broadcastAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// Define variables to store incoming readings
int incomingRed;
int incomingBlue;
int incomingGreen;
int incomingSpeed;
int common = 0;
int anode = 0;
int maxAnode = 230;
int minAnode = 50;

// Define variables for control loops
int receiverCounter = 0;
int lastReceiverCounter = 0;
bool off = false;
bool fadeEnabled = false;
bool fadeDisableStatusUpdated = false;
int checksum = 0;
int previousChecksum = 0;

// Update readings every 1 second
const long interval = 1000;

unsigned long previousMillis = 0;

// Structure to send data
// Must match the receiver structure
typedef struct struct_message {
  int red;
  int green;
  int blue;
  int speed;
} struct_message;

// Create a struct_message called PersonReadings to hold readings
struct_message PersonReadings;

// Create a struct_message to hold incoming readings
struct_message incomingReadings;

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  // Serial.print("Last Packet Send Status: ");
  // if (sendStatus == 0) {
  //   Serial.println("Delivery success");
  // } else {
  //   Serial.println("Delivery fail");
  // }
}

// Callback when data is received
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  // Serial.print("Bytes received: ");
  // Serial.println(len);
  incomingRed = incomingReadings.red;
  incomingGreen = incomingReadings.green;
  incomingBlue = incomingReadings.blue;
  incomingSpeed = incomingReadings.speed;

  receiverCounter += 1;
}

int naiveChecksum(int r, int g, int b, int speed) {
  return r * 8 + g * 4 + b * 2 + speed;
}

int values[] = { 0, 0, 0 };
int maxPwmValue[] = { 0, 0, 0 };
int direction[] = { 1, 1, 1 };
int fadeAmount = 30;  // 100 - 1000

void setup() {
  #ifdef COMMON_ANODE
    common = 255;
  #endif

  pinMode(anodePin, OUTPUT);
  analogWrite(anodePin, 200);

  for (int i = 0; i < 3; i++) {
    pinMode(ledpins[i], OUTPUT);
    analogWrite(ledpins[i], common - personValues[i]);
  }
  delay(1000);

  for (int i = 0; i < 3; i++) {
    analogWrite(ledpins[i], common - 0);
  }
  analogWrite(anodePin, 0);

  #ifdef DEBUG
    Serial.begin(115200);
  #endif

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    PersonReadings.red = personValues[0];
    PersonReadings.blue = personValues[1];
    PersonReadings.green = personValues[2];
    PersonReadings.speed = personValues[3];

    // Send message via ESP-NOW
    esp_now_send(broadcastAddress, (uint8_t *)&PersonReadings, sizeof(PersonReadings));

    checksum = naiveChecksum(incomingRed, incomingGreen, incomingBlue, incomingSpeed);

    Serial.print("R: ");
    Serial.print(incomingRed);
    Serial.print(" G: ");
    Serial.print(incomingGreen);
    Serial.print(" B: ");
    Serial.print(incomingBlue);
    Serial.print(" speed: ");
    Serial.print(incomingSpeed);
    Serial.print(" counter: ");
    Serial.print(receiverCounter);
    Serial.print(" last: ");
    Serial.print(lastReceiverCounter);
    Serial.print(" checksum: ");
    Serial.println(checksum);

    if (checksum != 0 && receiverCounter > lastReceiverCounter) {
      lastReceiverCounter = receiverCounter;
      fadeEnabled = true;
      if (checksum != previousChecksum) {
        previousChecksum = checksum;
        Serial.print("New checksum detected: ");
        Serial.println(checksum);
        maxPwmValue[0] = incomingRed;
        maxPwmValue[1] = incomingGreen;
        maxPwmValue[2] = incomingBlue;
      }
    } else {
      fadeEnabled = false;
      fadeDisableStatusUpdated = false;
    }
  }

  if (fadeEnabled) {
    analogWrite(ledpins[0], common - maxPwmValue[0]);
    analogWrite(ledpins[1], common - maxPwmValue[1]);
    analogWrite(ledpins[2], common - maxPwmValue[2]);

    for (int i = 0; i < 3; i++) {
      anode = anode + (fadeAmount * direction[i]);
      if (anode <= minAnode || anode >= maxAnode) {
        direction[i] = -direction[i];
      }
      if (anode >= maxAnode) {
        anode = maxAnode;
      }
      if (anode <= minAnode) {
        anode = minAnode;
      }
      analogWrite(anodePin, anode);
      delay(incomingSpeed);
    //   values[i] = values[i] + (fadeAmount * direction[i]);
    //   if (values[i] <= 0 || values[i] >= maxPwmValue[i]) {
    //     direction[i] = -direction[i];
    //   }
    //   if (values[i] >= maxPwmValue[i]) {
    //     values[i] = maxPwmValue[i];
    //   }
      
    //   analogWrite(ledpins[i], common - values[i]);
    //   Serial.print(ledpins[i]);
    // Serial.println(values[i]);
    }
    
    
  } else {
    if (!fadeDisableStatusUpdated) {
      fadeDisableStatusUpdated = true;
      Serial.println("off");
      for (int i = 0; i < 3; i++) {
        analogWrite(ledpins[i], common - 0);
      }
    }
  }
}
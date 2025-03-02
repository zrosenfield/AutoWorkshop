#include <painlessMesh.h>
#include "ButtonLight.h"
#include <ArduinoJson.h>
#include "DustGate.h"
#include <functional>
#include <U8g2lib.h>
#include <EEPROM.h>  // Include the EEPROM library

//WIFI NEEDED FOR MAC ADDRESS
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

//Local Storage
#define EEPROM_SIZE 32  // Define the size of EEPROM storage for each tool name

#ifdef LED_BUILTIN
#define LED LED_BUILTIN
#else
#define LED 2
#endif

#define MESH_SSID "workshopnetwork"
#define MESH_PASSWORD "pass@word1!"
#define MESH_PORT 5555

#define SERVO_PIN D4
#define SERVO_DELAY 1000
#define BUTTON_LIGHTPIN D1
#define BUTTON_PIN D2

#define NAME_INDEX 0
#define OPENPOS_INDEX 1
#define CLOSEDPOS_INDEX 2
#define EEPROM_VALUES 3

ButtonLight buttonLight(BUTTON_LIGHTPIN, BUTTON_PIN);  //Light, Button
DustGate gate(SERVO_PIN);

String toolname = "NO NAME!";

Scheduler userScheduler;  // to control your personal task

//Globals
bool isActive = false;
bool flashLights = false;
unsigned long flashStartTime;

painlessMesh mesh;
U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, /* clock=*/D5, /* data=*/D6, /* reset=*/U8X8_PIN_NONE);

//Alert HUB to this machines existance
void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New device connection.  Broadcast to anything that might be a hub:  ");

  //ALERT
  String jsonMsg = createJsonMsg("sharePresence", toolname);
  Serial.println(jsonMsg);

  mesh.sendBroadcast(jsonMsg);
}

//***********EEPROM************//

uint16_t convertStringToUint16(const String& str) {
  // Use the toInt() method of the String class to convert the string to an integer
  return str.toInt();
}

String getStoredValue(int index) {
  // Calculate the EEPROM address based on the index and the size of each value
  int address = index * EEPROM_SIZE;

  // Read the stored value from EEPROM
  String storedValue;
  for (int i = 0; i < EEPROM_SIZE; i++) {
    char character = EEPROM.read(address + i);
    if (character == '\0') {
      break;  // Stop reading at the null terminator
    }
    storedValue += character;
  }

  return storedValue;
}


void storeValue(int index, const String& value) {
  Serial.println("Storing " + value + " in index " + index);
  // Calculate the EEPROM address based on the index and the size of each value
  int address = index * EEPROM_SIZE;

  // Write the value to EEPROM
  for (int i = 0; i < value.length(); i++) {
    EEPROM.write(address + i, value[i]);
  }

  // Null-terminate the string
  EEPROM.write(address + value.length(), '\0');

  // Commit the changes to EEPROM
  EEPROM.commit();

  // Read the data back and verify it
  String readValue = getStoredValue(index);
  if (readValue != value) {
    Serial.println("Error: Data verification failed!");
  }
}

String generateRandomToolName() {
  // Generate a random tool name (e.g., Tool1, Tool2, etc.)
  return "Tool" + String(random(1, 1000));
}

//*****************************//

//Get instructions from a different device
void receivedCallback(uint32_t from, String& msg) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());

  DynamicJsonDocument jsonDoc(1024);
  DeserializationError error = deserializeJson(jsonDoc, msg);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  JsonObject jsonObj = jsonDoc.as<JsonObject>();


  // Extract the instruction type from the message
  const char* type = jsonObj["type"];
  if (!type) {
    Serial.println(F("Invalid message format: missing 'type' field"));
    return;
  }

  // Handle the message based on the instruction type
  if (strcmp(type, "buttonPress") == 0) {
    // Extract the name and id fields from the message
    const char* value = jsonObj["msg"];
    const char* id = jsonObj["id"];
    if (strcmp(value, toolname.c_str()) == 0) {
      //TURN ME ON
      Serial.println("I'VE BEEN ENABLED!");
      turnThisToolOn();
    } else {
      //TURN THIS TOOL OFF
      isActive = false;
      gate.Close();
      delay(SERVO_DELAY);
      Serial.println("Local Gate Closed");
      buttonLight.turnOffLight();

      flashLights = true;
      flashStartTime = millis();

      Serial.printf("Button %s pressed on device with id %d\n", value, id);
    }
  } else if (strcmp(type, "renameTool") == 0) {
    // Handle roll call logic here
    const char* value = jsonObj["msg"];
    storeValue(NAME_INDEX, value);
    toolname = value;
    Serial.print("Renamed to ");
    Serial.println(value);

  } else if (strcmp(type, "setOpenValue") == 0) {
    //Set Open Value
    const char* value = jsonObj["msg"];
    storeValue(OPENPOS_INDEX, String(convertStringToUint16(value)));
    gate.SetOpenPos(convertStringToUint16(value));
    if (isActive) gate.Open();
  } else if (strcmp(type, "setClosedValue") == 0) {
    //Set Closed Value
    const char* value = jsonObj["msg"];
    storeValue(CLOSEDPOS_INDEX, String(convertStringToUint16(value)));
    gate.SetClosedPos(convertStringToUint16(value));
    if (!isActive) gate.Close();
  } else if (strcmp(type, "rollCall") == 0) {
    // Handle roll call logic here
    newConnectionCallback(mesh.getNodeId());
  } else {
    Serial.print(F("Unknown message type: "));
    Serial.println(type);
  }
}

//****************//
///SETUP FUNCTION
//****************//

void setup() {
  Serial.begin(115200);
  Serial.println();

  //for debug:
  pinMode(LED, OUTPUT);

  //EEPROM
  EEPROM.begin(EEPROM_SIZE * EEPROM_VALUES);

  //storeValue(NAME_INDEX, "CNC");

  String storedToolName = getStoredValue(NAME_INDEX);
  if (storedToolName.length() == 0 || storedToolName.length() >= 32) {
    // If no tool name is stored, generate a random one
    String randomToolName = generateRandomToolName();

    // Store the generated tool name in EEPROM
    storeValue(NAME_INDEX, randomToolName);
    storedToolName = randomToolName;

    // Use the generated tool name
    Serial.println("Generated Tool Name: " + randomToolName);
  } else {
    // Use the stored tool name
    Serial.println("Stored Tool Name: " + storedToolName);
  }

  //Read Open/Closed Values
  String openVal = getStoredValue(OPENPOS_INDEX);
  String closedVal = getStoredValue(CLOSEDPOS_INDEX);

  Serial.println("OPENVAL_mem: " + openVal);
  Serial.println("CLOSEDVAL_mem: " + closedVal);

  if (openVal.length() > 0 && convertStringToUint16(openVal) > 0) {
    gate.SetOpenPos(convertStringToUint16(openVal));
    Serial.println("OPEN POS FROM MEMORY: " + openVal);
  }
  if (closedVal.length() > 0 && convertStringToUint16(closedVal) > 0) {
    gate.SetClosedPos(convertStringToUint16(closedVal));
    Serial.println("CLOSED POS FROM MEMORY: " + openVal);
  }

  toolname = storedToolName;
  Serial.println("TOOL NAME IS: " + toolname);

  //Turn all things off by default
  buttonLight.turnOffLight();
  gate.Close();
  isActive = false;

//mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION );  // set before init() so that you can see error messages
  mesh.onReceive(receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
}

//Lights Flash when transition happens
void flashAllLights() {
  unsigned long now = millis();
  if (now - flashStartTime < 3000) {
    buttonLight.setLightState(now % 1000 < 500);
    Serial.print(".");
  } else {
    flashLights = false;
  }
}

//Generate Outbound Messages
String createJsonMsg(String action, String msg) {
  StaticJsonDocument<100> doc;
  doc["type"] = action;
  doc["id"] = WiFi.macAddress();
  doc["msg"] = msg;

  String jsonMsg;
  serializeJson(doc, jsonMsg);

  return jsonMsg;
}


//BUTTON WIRING: Red->LED Pin (13).  Black->Ground.  BLUE->5Vin.  Green->ButtonInput (12) (LOW is press)
void loop() {
  mesh.update();
  digitalWrite(LED, !isActive);


  buttonLight.powerLight();  // Replace with your custom pin configuration
  if (flashLights) {
    flashAllLights();
  } else {
    if (buttonLight.getButtonState()) {
      //Open Gate -- this is the active tool
      turnThisToolOn();
    }
    if (isActive) {
      buttonLight.turnOnLight();
    } else {
      buttonLight.turnOffLight();
    }
  }
}

void turnThisToolOn() {
  isActive = true;
  gate.Open();
  delay(SERVO_DELAY);

  Serial.print(toolname);
  Serial.println(" Turned On!");

  //Tell the rest of the tools!
  String jsonMsg = createJsonMsg("buttonPress", toolname);
  Serial.println(jsonMsg);
  mesh.sendBroadcast(jsonMsg);
}

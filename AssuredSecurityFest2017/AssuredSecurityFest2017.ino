// INCLUDES
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <stdio.h>

#define DHTPIN D4     // what pin the DHT is connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)

// GLOBAL VARIABLES
int pin = D0;
boolean ssidCorrect = false;
boolean wifiConnectionDone = false;
boolean connError = false;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
DHT dht(DHTPIN, DHTTYPE);

// CONFIGURATION (put in file?)
String ssidPattern = "2170"; // "2170" ********** CHANGE
String ssidPasswordPattern = "J.M.Grochow"; // Mandatory min 8 characters on iphone "J.M.Grochow" ********** CHANGE
int modeOfOperation = 2; // 0 = SENSOR1, 1 = C&C, 2 = SENSOR2    ********** CHANGE
const char* hostMQTT = "mqtt.jidhage.com";
const int portMQTT = 1883;
const char* mqttClientName = "Sensor1"; // CommandControl, Sensor1, Sensor2 *********** CHANGE

/*  config contents:  wifissid (list or pattern) & pass   mode (sensor, display)  sensortype (how to poll sensor)     leak info during setup    */

ADC_MODE(ADC_VCC); // Enables ESP.Vcc()

// START OF HELP FUNCTIONS
void printnumBulls(String ssid) {
  int numBulls = 0;
  /* Serial.print(" DEBUG ssid length:");
    Serial.print(ssid.length());
    Serial.print("DEBUG "); */
  for (int i = 0; i < ssid.length(); i++) {
    char a = ssid.charAt(i);
    char b = ssidPattern.charAt(i);
    /* Serial.print(" DEBUG ssid length:");
      Serial.print(a);
      Serial.print(b);
      Serial.print("DEBUG "); */
    if ( a == b) {
      numBulls++;
    }
  }
  Serial.print(numBulls);
  Serial.print("/");
}

void printnumCows(String ssid) {
  int numCows = 0;
  for (int i = 0; i < ssid.length(); i++) {
    for (int j = 0; j < ssidPattern.length(); j++) {
      if (i != j) {
        if (ssid.charAt(i) == ssidPattern.charAt(j)) {
          numCows++;
        }
      }
    }
    yield();
  }
  Serial.print(numCows);
}

void printMasterMindHint(String ssid) {
  Serial.print("\tBullsAndCows: ");
  //  Serial.print(ssid.length());
  if (ssidPattern.equals(ssid)) {
    ssidCorrect = true;
    Serial.print("Moo");
    return;
  } else {
    if (ssid.length() < ssidPattern.length()) {
      Serial.print("Too short");
    } else if (ssid.length() > ssidPattern.length()) {
      Serial.print("Too long");
    } else {
      printnumBulls(ssid);
      printnumCows(ssid);
    }
  }
}

void printEncryptionType(int thisType) {
  // read the encryption type and print out the name:
  switch (thisType) {
    case ENC_TYPE_WEP:
      Serial.print("WEP");
      break;
    case ENC_TYPE_TKIP:
      Serial.print("WPA");
      break;
    case ENC_TYPE_CCMP:
      Serial.print("WPA2");
      break;
    case ENC_TYPE_NONE:
      Serial.print("None");
      break;
    case ENC_TYPE_AUTO:
      Serial.print("Auto");
      break;
  }
}

void listNetworks() {
  char tempString[21] = "                    ";

  // scan for nearby networks:
  Serial.println("** Scanning for valid networks **");
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1) {
    Serial.println("No wifi's available");
    while (true);
  }

  // print the list of networks seen:
  Serial.print("Number of networks found:");
  Serial.println(numSsid);

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    String formString = "";
    String newString = "";
    Serial.print(thisNet);
    Serial.print(") ");
    formString.concat(WiFi.SSID(thisNet));
    formString.concat(tempString);
    formString = formString.substring(0, 16);
    Serial.print(formString);
    Serial.print("\tSignal: ");
    Serial.print(WiFi.RSSI(thisNet));
    Serial.print(" dBm");
    printMasterMindHint(String(WiFi.SSID(thisNet)));
    Serial.print("\tEncryption: ");
    printEncryptionType(WiFi.encryptionType(thisNet));
    Serial.println();
  }
}

void printDiagnostics() {
  Serial.println("** Starting up - running diagnostics **");
  Serial.print("Voltage: ");
  Serial.println(ESP.getVcc());
  Serial.print("Latest reset reason: ");
  Serial.println(ESP.getResetReason());
  Serial.print("Free heap size: ");
  Serial.println(ESP.getFreeHeap());
  Serial.print("ESP8266 chip id: ");
  Serial.println(ESP.getChipId());
  Serial.print("Core version: ");
  Serial.println(ESP.getCoreVersion());
  Serial.print("SDK version: ");
  Serial.println(ESP.getSdkVersion());
  Serial.print("CPU frequency in MHz: ");
  Serial.println(ESP.getCpuFreqMHz());
  Serial.print("Sketch size in bytes: ");
  Serial.println(ESP.getSketchSize());
  Serial.print("Free sketch space: ");
  Serial.println(ESP.getFreeSketchSpace());
  Serial.print("Sketch MD5: ");
  Serial.println(ESP.getSketchMD5());
  Serial.print("Flash chip id: ");
  Serial.println(ESP.getFlashChipId());
  Serial.print("Flash chip size (sdk view): ");
  Serial.println(ESP.getFlashChipSize());
  Serial.print("Flash chip size (chip id view): ");
  Serial.println(ESP.getFlashChipRealSize());
  Serial.print("Flash chip speed in Hz: ");
  Serial.println(ESP.getFlashChipSpeed());
  Serial.print("CPU cycles since start: ");
  Serial.println(ESP.getCycleCount());
  Serial.println();
}


void tryConnect() {
  char ssidP[255];
  ssidPattern.toCharArray(ssidP, 255);
  char ssidPP[255];
  ssidPasswordPattern.toCharArray(ssidPP, 255);
  int i = 0;
  int j = 0;

  WiFi.begin(ssidP, ssidPP);
  Serial.print("Connecting...");
  //Serial.print(WiFi.status());

  // WL_DISCONNECTED = 6 seams to be default and stays here for a while
  while (WiFi.status() == WL_DISCONNECTED)     {
    delay(500);
    Serial.print(".");
    // Serial.print(i);
    i++;
    if (i > 100) {
      connError = true; // something is terribly wrong, start over
      break;
    }
  }
  // W_IDLE_STATUS = ? has been seen on rare occasions
  while (WiFi.status() == WL_IDLE_STATUS)     {
    delay(500);
    Serial.print(",");
    j++;
    if (j > 100) {
      connError = true; // something is terribly wrong, start over
      break;
    }
  }
  Serial.println();
  //Serial.print(WiFi.status());

  // If status moved from default WL_DISCONNECT - where did it go?
  if (WiFi.status() == WL_CONNECT_FAILED) {
    Serial.print("Wrong Password, try again: ");
    printPasswordErrorMsg();
  } else if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Password correct!");
    wifiConnectionDone = true;
  } else if (WiFi.status() == WL_NO_SSID_AVAIL) {
    Serial.println("Network lost, restarting");
    connError = true;
    wifiConnectionDone = true;
  }
  // Serial.print(WiFi.status());
  WiFi.mode(WIFI_STA);
}

void printPasswordErrorMsg() {
  static int counter = 0;
  switch (counter) {
    case 0: Serial.println("Minimum 8 characters");
      break;
    case 1: Serial.println("Minimum is often not good enough, right?");
      break;
    case 2: Serial.println("No spaces allowed");
      break;
    case 3: Serial.println("MIT");
      break;
    case 4: Serial.println("Punctuation is allowed");
      break;
    case 5: Serial.println("Passwords are like names, sort of");
      break;
    case 6: Serial.println("Real-time graphic display of time-sharing system operating characteristics");
      break;
    case 7: Serial.println("I'm thinking of an author");
      break;
    case 8: Serial.println("Not 'Frank King'");
      break;
    case 9: Serial.println("moo?");
  }
  if (counter > 9)  {
    counter = 0;
  }
  else {
    counter++;
  }
}

void callbackCC(char* topic, byte* payload, unsigned int length) {
  //Serial.print("Temperature at ");
  Serial.print(topic);
  Serial.print(" is:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void callbackSE(char* topic, byte* payload, unsigned int length) {
  boolean lightsOn = false;
  boolean validCmd = false;

  char cmdOFF[] = "LIGHT OFF";
  for (int i = 0; i < 9; i++) { //is it an OFF command?
    if (cmdOFF[i] != (char)payload[i]) {
      Serial.print("Not cmd OFF");
      validCmd = false;
      break; // do not change state
    } else {
      validCmd = true;
      lightsOn = false;
    }
  }

  char cmdON[] = "LIGHT ON";
  if (!validCmd) {   // if not an OFF command, could it be an ON command?
    for (int i = 0; i < 8; i++) {
      if (cmdON[i] != (char)payload[i]) {
        validCmd = false;
        Serial.print("Not cmd ON");
        break; // do not change state
      } else {
        validCmd = true;
        lightsOn = true;
      }
    }
  }

  if (validCmd && !lightsOn) {
    Serial.println("CMD IS OFF");
    digitalWrite(pin, HIGH);    // turn the LED off by making the voltage HIGH
  } else if (validCmd && lightsOn) {
    Serial.println("CMD IS ON");
    digitalWrite(pin, LOW);    // turn the LED on by making the voltage LOW
  } else {
    Serial.println("Not a valid command");
  }
}

void callbackSE2(char* topic, byte* payload, unsigned int length) {
  boolean lightsOn = false;
  boolean validCmd = false;

  char cmdOFF[] = "TElHSFQgT0ZG";
  for (int i = 0; i < 12; i++) { //is it an OFF command?
    if (cmdOFF[i] != (char)payload[i]) {
      Serial.print("Not cmd OFF");
      validCmd = false;
      break; // do not change state
    } else {
      validCmd = true;
      lightsOn = false;
    }
  }

  char cmdON[] = "TElHSFQgT04=";
  if (!validCmd) {   // if not an OFF command, could it be an ON command?
    for (int i = 0; i < 12; i++) {
      if (cmdON[i] != (char)payload[i]) {
        validCmd = false;
        Serial.print("Not cmd ON");
        break; // do not change state
      } else {
        validCmd = true;
        lightsOn = true;
      }
    }
  }

  if (validCmd && !lightsOn) {
    Serial.println("CMD IS OFF");
    digitalWrite(pin, HIGH);    // turn the LED off by making the voltage HIGH
  } else if (validCmd && lightsOn) {
    Serial.println("CMD IS ON");
    digitalWrite(pin, LOW);    // turn the LED on by making the voltage LOW
  } else {
    Serial.println("Not a valid command");
  }

}

void connectToHost() {

  Serial.print("connecting to ");
  Serial.print(hostMQTT);
  Serial.print(":");
  Serial.println(portMQTT);

  mqttClient.setServer(hostMQTT, portMQTT);
  if (!mqttClient.connect(mqttClientName)) {
    Serial.println("Connection failed");
    return;
  }

  switch (modeOfOperation) {
    case 0: // Serial.println("Mode of operation is SENSOR1");
      mqttClient.setCallback(callbackSE);
      mqttClient.subscribe("sensor1/lightcommand");
      break;
    case 1: // Serial.println("Mode of operation is COMMAND AND CONTROL");
      mqttClient.setCallback(callbackCC);
      mqttClient.subscribe("sensor1/temperature");
      mqttClient.subscribe("sensor2/temperature");
      mqttClient.subscribe("sensor1/humidity");
      mqttClient.subscribe("sensor2/humidity");
      break;
    case 2: // Serial.println("Mode of operation is SENSOR2");
      mqttClient.setCallback(callbackSE2);
      mqttClient.subscribe("sensor2/lightcommand");
      break;
    default: Serial.println("No mode of operation defined - waiting");
  }
  Serial.println("Connected to broker");
}


void setLightOff() {
  mqttClient.publish("sensor1/lightcommand", "LIGHT OFF");
  mqttClient.publish("sensor2/lightcommand", "TElHSFQgT0ZG"); //LIGHT OFF, LIGHT ON = "TElHSFQgT04="
}

void pushValues() {
  char temp[12];
  char hum[12];
  // get DHT values
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  Serial.println(temperature);
  Serial.println(humidity);
  dtostrf(temperature, 6, 2, temp);
  dtostrf(humidity, 6, 2, hum);
  // publish to server
  mqttClient.publish("sensor1/temperature", temp); // temp
  mqttClient.publish("sensor1/humidity", hum); // humidity
}

void pushValues2() {
  char temp[12];
  char hum[12];
  // get DHT values
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  Serial.println(temperature);
  Serial.println(humidity);
  dtostrf(temperature, 6, 2, temp);
  dtostrf(humidity, 6, 2, hum);
  // publish to server
  mqttClient.publish("sensor2/temperature", temp); // temp
  mqttClient.publish("sensor2/humidity", hum); // humidity
}

void getCommand() {
  mqttClient.loop();
}

void pollValueAndPrint() {
  mqttClient.loop();
}

void doCommandWork() {
  //Make sure the lights are off
  setLightOff();
  //Look for values and print them
  pollValueAndPrint();
}

void doSensorWork() {
  //Push sensor values
  pushValues();
  //Check if there are any commands for me (light on/off?) // LIGHT ON = "TElHSFQgT04="
  getCommand();
}

void doSensorWork2() {
  //Push sensor values
  pushValues2();
  //Check if there are any commands for me (light on/off?) // LIGHT ON = "TElHSFQgT04="
  getCommand();
}

void connectToValidNetwork() {
  // CONNECT TO VALID NETWORK

  do {
    connError = false;
    wifiConnectionDone = false;
    ssidCorrect = false;


    // FIND THE CORRECT SSID
    while (!ssidCorrect) {
      listNetworks();
      Serial.println();
      if (!ssidCorrect) {
        Serial.println("No matching SSIDs found, retrying in 30s");
        Serial.println();
      }
      delay(30000);
    }
    Serial.println("Valid network found, trying to connect");
    Serial.println();
    WiFi.disconnect();

    // FIND THE CORRECT PASSWORD
    while (!wifiConnectionDone) {
      tryConnect();
    }
  } while (connError);
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Setup done, changing to running mode");
  connectToHost();
}



// SETUP & LOOP

void setup() {
  // enable serial monitor
  Serial.begin(115200);
  Serial.println("Debug is enabled");
  // initialize GPIO 2 as an output.
  pinMode(pin, OUTPUT);
  // blink once to show we are in debug
  digitalWrite(pin, LOW);    // turn the LED on by making the voltage LOW
  delay(1000);
  digitalWrite(pin, HIGH);    // turn the LED off by making the voltage HIGH
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  // print a lot of info to make it look cool
  printDiagnostics();
  delay(3000);
  connectToValidNetwork();
}


void loop() {

  if (WiFi.status() == WL_CONNECTED) {
    // Serial.println("Doing work");
    // digitalWrite(pin, LOW);   // turn the LED on
    if (!mqttClient.connected()) {
      connectToHost();
    }
    // add modeofoperation as switch in connect to host - selects callback
    switch (modeOfOperation) {
      case 0: // Serial.println("Mode of operation is SENSOR");
        doSensorWork();
        break;
      case 1: // Serial.println("Mode of operation is COMMAND AND CONTROL");
        doCommandWork();
        break;
      case 2: // Serial.println("Mode of operation is COMMAND AND CONTROL");
        doSensorWork2();
        break;
      default:
        Serial.println("No mode of operation defined - waiting");
    }
  }
  else {
    // digitalWrite(pin, HIGH);    // turn the LED off by making the voltage LOW
    printDiagnostics();
    delay(3000);
    connectToValidNetwork();
  }
  mqttClient.loop();
  delay(1000);
  mqttClient.loop();
  delay(1000);
  mqttClient.loop();
  delay(1000);
  mqttClient.loop();
  delay(1000);
  mqttClient.loop();
  delay(1000);
  mqttClient.loop();
  delay(1000);
  mqttClient.loop();
  delay(1000);
  mqttClient.loop();
  delay(1000);
  mqttClient.loop();
  delay(1000);
  mqttClient.loop();
  delay(1000);
}

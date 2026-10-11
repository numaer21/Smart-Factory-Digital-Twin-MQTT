#include <WiFi.h>
#include <PubSubClient.h>

// =====================================================
// WIFI SETTINGS
// =====================================================

const char* ssid = "Numaer iPhone";
const char* password = "123456789";


// =====================================================
// MQTT BROKER SETTINGS
// =====================================================

const char* mqtt_server = "172.20.10.2";
const int mqtt_port = 1883;


// =====================================================
// MQTT TOPICS
// =====================================================

const char* telemetry_topic =
  "factory/machine01/telemetry";

const char* status_topic =
  "factory/machine01/status";

const char* command_topic =
  "factory/machine01/command";

const char* state_topic =
  "factory/machine01/state";


// =====================================================
// WIFI + MQTT CLIENT
// =====================================================

WiFiClient espClient;
PubSubClient client(espClient);


// =====================================================
// MACHINE STATE
// =====================================================

bool machineRunning = true;
unsigned long lastMessage = 0;


// =====================================================
// FAULT MODES
// =====================================================

enum FaultMode {
  NO_FAULT,
  OVERHEAT_FAULT,
  VIBRATION_FAULT,
  OVERLOAD_FAULT
};

FaultMode currentFault = NO_FAULT;


// =====================================================
// CONNECT TO WIFI
// =====================================================

void connectWiFi() {

  Serial.println();
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Wi-Fi connected!");

  Serial.print("ESP32 IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Wi-Fi RSSI: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}


// =====================================================
// RECEIVE MQTT COMMANDS
// =====================================================

void mqttCallback(
  char* topic,
  byte* payload,
  unsigned int length
) {

  String command = "";

  for (unsigned int i = 0; i < length; i++) {
    command += (char)payload[i];
  }

  command.trim();

  Serial.println();
  Serial.print("Command received: ");
  Serial.println(command);


  // START
  if (command == "START") {

    machineRunning = true;

    client.publish(
      state_topic,
      "RUNNING",
      true
    );

    Serial.println("Machine STARTED");
  }


  // STOP
  else if (command == "STOP") {

    machineRunning = false;

    client.publish(
      state_topic,
      "STOPPED",
      true
    );

    Serial.println("Machine STOPPED");
  }


  // OVERHEAT
  else if (command == "FAULT_OVERHEAT") {

    currentFault = OVERHEAT_FAULT;

    Serial.println(
      "OVERHEAT FAULT ACTIVATED"
    );
  }


  // HIGH VIBRATION
  else if (command == "FAULT_VIBRATION") {

    currentFault = VIBRATION_FAULT;

    Serial.println(
      "HIGH VIBRATION FAULT ACTIVATED"
    );
  }


  // OVERLOAD
  else if (command == "FAULT_OVERLOAD") {

    currentFault = OVERLOAD_FAULT;

    Serial.println(
      "OVERLOAD FAULT ACTIVATED"
    );
  }


  // RESET FAULT
  else if (command == "RESET_FAULT") {

    currentFault = NO_FAULT;

    Serial.println("FAULT RESET");
  }


  // UNKNOWN COMMAND
  else {

    Serial.println("Unknown command");
  }
}


// =====================================================
// CONNECT TO MQTT
// =====================================================

void connectMQTT() {

  while (!client.connected()) {

    Serial.print(
      "Connecting to MQTT broker..."
    );

    bool connected = client.connect(
      "ESP32S3-Machine01",
      NULL,
      NULL,

      // Last Will topic
      status_topic,

      // QoS
      1,

      // Retain
      true,

      // Last Will payload
      "OFFLINE"
    );


    if (connected) {

      Serial.println("CONNECTED!");


      // Publish ONLINE
      client.publish(
        status_topic,
        "ONLINE",
        true
      );


      // Subscribe to commands
      client.subscribe(
        command_topic,
        1
      );

      Serial.println(
        "Subscribed to command topic"
      );


      // Publish current state
      if (machineRunning) {

        client.publish(
          state_topic,
          "RUNNING",
          true
        );

      } else {

        client.publish(
          state_topic,
          "STOPPED",
          true
        );
      }

    } else {

      Serial.print(
        "MQTT connection failed. State = "
      );

      Serial.println(
        client.state()
      );

      Serial.println(
        "Trying again in 3 seconds..."
      );

      delay(3000);
    }
  }
}


// =====================================================
// SEND TELEMETRY
// =====================================================

void sendTelemetry() {

  float temperature;
  int rpm;
  float vibration;
  int load;


  // =================================================
  // MACHINE RUNNING
  // =================================================

  if (machineRunning) {

    // Normal ranges stay below warning thresholds

    temperature =
      random(350, 541) / 10.0;

    // 35.0 - 54.0 C


    rpm =
      random(1200, 2401);

    // 1200 - 2400 RPM


    vibration =
      random(10, 35) / 10.0;

    // 1.0 - 3.4 mm/s


    load =
      random(30, 75);

    // 30 - 74 %
  }


  // =================================================
  // MACHINE STOPPED
  // =================================================

  else {

    temperature =
      random(300, 401) / 10.0;

    // 30.0 - 40.0 C

    rpm = 0;
    vibration = 0.0;
    load = 0;
  }


  // =================================================
  // APPLY FAULT
  // =================================================

  if (machineRunning) {

    if (
      currentFault == OVERHEAT_FAULT
    ) {

      temperature = 85.0;
    }


    else if (
      currentFault == VIBRATION_FAULT
    ) {

      vibration = 6.5;
    }


    else if (
      currentFault == OVERLOAD_FAULT
    ) {

      load = 95;
    }
  }


  // =================================================
  // REAL ESP32 DATA
  // =================================================

  int rssi = WiFi.RSSI();

  unsigned long uptime =
    millis() / 1000;


  // =================================================
  // CREATE JSON MESSAGE
  // =================================================

  char message[256];

  snprintf(
    message,
    sizeof(message),

    "{"
    "\"temperature\":%.1f,"
    "\"rpm\":%d,"
    "\"vibration\":%.1f,"
    "\"load\":%d,"
    "\"rssi\":%d,"
    "\"uptime\":%lu"
    "}",

    temperature,
    rpm,
    vibration,
    load,
    rssi,
    uptime
  );


  // =================================================
  // PUBLISH TELEMETRY
  // =================================================

  client.publish(
    telemetry_topic,
    message
  );


  Serial.println(
    "MQTT telemetry sent:"
  );

  Serial.println(message);
}


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(1500);

  Serial.println();

  Serial.println(
    "================================"
  );

  Serial.println(
    " SMART FACTORY DIGITAL TWIN"
  );

  Serial.println(
    " MACHINE 01 - ESP32-S3"
  );

  Serial.println(
    "================================"
  );


  // Connect Wi-Fi
  connectWiFi();


  // Configure MQTT
  client.setServer(
    mqtt_server,
    mqtt_port
  );


  client.setCallback(
    mqttCallback
  );


  // Connect MQTT
  connectMQTT();


  randomSeed(
    micros()
  );
}


// =====================================================
// LOOP
// =====================================================

void loop() {


  // Reconnect Wi-Fi if needed
  if (
    WiFi.status() != WL_CONNECTED
  ) {

    connectWiFi();
  }


  // Reconnect MQTT if needed
  if (
    !client.connected()
  ) {

    connectMQTT();
  }


  // Receive MQTT commands
  client.loop();


  // Send telemetry every 2 seconds
  unsigned long now = millis();

  if (
    now - lastMessage >= 2000
  ) {

    lastMessage = now;

    sendTelemetry();
  }
}
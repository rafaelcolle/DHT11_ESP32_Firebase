// Include required libraries
#if defined(ESP32)
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#endif

#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <DHT.h>  // Include the DHT sensor library

// Define DHT sensor parameters
#define DHTPIN 21
#define DHTTYPE DHT11

// Define WiFi credentials
#define WIFI_SSID "REDE"
#define WIFI_PASSWORD "SENHA"

// Define Firebase API Key, Project ID, and user credentials
#define API_KEY "CHAVEAPI"
#define FIREBASE_PROJECT_ID "NOMEDOPROJETO"
#define USER_EMAIL "emailcadastradonofirebase"
#define USER_PASSWORD "senha"

// Define Firebase Data object, Firebase authentication, and configuration
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Initialize the DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// NTP server settings
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -10800;  // Adjust based on timezone (GMT-3 for Brazil)
const int   daylightOffset_sec = 3600;

void setup() {
  Serial.begin(115200); //inicializa o Monitor Serial
  dht.begin(); //inicializa o sensor de umidade e temperatura
  
  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  // Initialize Firebase config and authentication
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Initialize NTP for date and time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
} //fim do SETUP

String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "";
  }
  char timeStringBuff[50];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(timeStringBuff);
}

void loop() {
  // Generate a unique document ID based on timestamp
  String documentPath = "esp32dados/DHT11_" + String(millis()); //esp32dados é o nome da coleção

  // Create a FirebaseJson object for storing data
  FirebaseJson content;

  // Read temperature and humidity from the DHT sensor
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  // Get current date and time as a formatted string
  String timestamp = getFormattedTime();

  // Print values for debugging
  Serial.print("Temperature: ");
  Serial.println(temperature);
  Serial.print("Humidity: ");
  Serial.println(humidity);
  Serial.print("Timestamp: ");
  Serial.println(timestamp);

//envio para o Firebase com a criação do documento
  if (!isnan(temperature) && !isnan(humidity) && !timestamp.isEmpty()) {
    // Set fields in the FirebaseJson object
    content.set("fields/temperatura/doubleValue", temperature);
    content.set("fields/umidade/doubleValue", humidity);
    content.set("fields/timestamp/stringValue", timestamp);

    Serial.print("Creating new document with DHT data... ");

    // Use createDocument to add a new document each time with unique ID
    if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())) {
      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    } else {
      Serial.println(fbdo.errorReason());
    }
  } else {
    Serial.println("Failed to read DHT data or obtain time.");
  }

  delay(10000);  // Adjust the delay as needed for the reading frequency
}

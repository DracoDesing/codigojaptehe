
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <Wire.h>

OneWireESP ourWire(D8);
int phval = A0;
float calibration_value = 21.34;
unsigned long int avgval;
int temp;
DallasTemperature sensors(&ourWire);
int trigPin = D1;
int echoPin = D2;
float tiempo;
float distancia;
int turbidityPin = A1;

void setup() {
  delay(1000);
  Serial.begin(115200);
  sensors.begin();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(trigPin, LOW);

  WiFi.begin("Draco", "Kevin123");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a red WiFi...");
  }
  Serial.println("Conexión WiFi establecida.");
}

void loop() {
  // Lectura de pH
  avgval = analogRead(phval);
  float volt = (float)avgval * 5.0 / 1024 / 6;
  float ph_act = -5.70 * volt + calibration_value;
  Serial.print("pH = ");
  Serial.println(ph_act);
  delay(2000);

  //Lectura de Temperatura
  sensors.requestTemperatures();
  float water_temp = sensors.getTempCByIndex(0);
  Serial.print("Temperatura del agua = ");
  Serial.print(water_temp);
  Serial.println(" C");
  delay(2000);

  //Lectura de nivel
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  tiempo = pulseIn(echoPin, HIGH);
  distancia = tiempo / 59;
  Serial.print("Distancia = ");
  Serial.print(distancia);
  Serial.println(" cm");
  delay(500);

  // Lectura de turbidez
  int turbidity_val = analogRead(turbidityPin);
  Serial.print("Turbidez = ");
  Serial.println(turbidity_val);
  delay(2000);

  // guardar datos en firebase
  // aquí se debe añadir el código para enviar los datos a Firebase
  // utilizando la conexión WiFi establecida anteriormente
  #include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include <addons/TokenHelper.h>

/* 1. Define the WiFi credentials */
#define WIFI_SSID "Draco"
#define WIFI_PASSWORD "Kevin123"

/* 2. Define the API Key */
#define API_KEY "AIzaSyDzT5WRPf1cl7PQ12RO8xRFDVQJNWgDyjA"

/* 3. Define the project ID */
#define FIREBASE_PROJECT_ID "japtehe-855b2"

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "memski24@gmail.com"
#define USER_PASSWORD "224566"


// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long dataMillis = 0;
int count = 0;

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
WiFiMulti multi;
#endif

// The Firestore payload upload callback function
void fcsUploadCallback(CFS_UploadStatusInfo info)
{
    if (info.status == fb_esp_cfs_upload_status_init)
    {
        Serial.printf("\nUploading data (%d)...\n", info.size);
    }
    else if (info.status == fb_esp_cfs_upload_status_upload)
    {
        Serial.printf("Uploaded %d%s\n", (int)info.progress, "%");
    }
    else if (info.status == fb_esp_cfs_upload_status_complete)
    {
        Serial.println("Upload completed ");
    }
    else if (info.status == fb_esp_cfs_upload_status_process_response)
    {
        Serial.print("Processing the response... ");
    }
    else if (info.status == fb_esp_cfs_upload_status_error)
    {
        Serial.printf("Upload failed, %s\n", info.errorMsg.c_str());
    }
}

void setup()
{

    Serial.begin(115200);

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    multi.addAP(WIFI_SSID, WIFI_PASSWORD);
    multi.run();
#else
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif

    Serial.print("Connecting to Wi-Fi");
    unsigned long ms = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
        if (millis() - ms > 10000)
            break;
#endif
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    /* Assign the api key (required) */
    config.api_key = API_KEY;

    /* Assign the user sign in credentials */
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;

    // The WiFi credentials are required for Pico W
    // due to it does not have reconnect feature.
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    config.wifi.clearAP();
    config.wifi.addAP(WIFI_SSID, WIFI_PASSWORD);
#endif

    /* Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

#if defined(ESP8266)
    // In ESP8266 required for BearSSL rx/tx buffer for large data handle, increase Rx size as needed.
    fbdo.setBSSLBufferSize(2048 /* Rx buffer size in bytes from 512 - 16384 */, 2048 /* Tx buffer size in bytes from 512 - 16384 */);
#endif

    // Limit the size of response payload to be collected in FirebaseData
    fbdo.setResponseSize(2048);

    Firebase.begin(&config, &auth);

    Firebase.reconnectWiFi(true);

    // For sending payload callback
    // config.cfs.upload_callback = fcsUploadCallback;
}

void loop()
{

    // Firebase.ready() should be called repeatedly to handle authentication tasks.

    if (Firebase.ready() && (millis() - dataMillis > 5000 || dataMillis == 0))
    {
        dataMillis = millis();

        // For the usage of FirebaseJson, see examples/FirebaseJson/BasicUsage/Create.ino
        FirebaseJson content;

        // Note: If new document created under non-existent ancestor documents, that document will not appear in queries and snapshot 
        // https://cloud.google.com/firestore/docs/using-console#non-existent_ancestor_documents.

         // We will create the document in the parent path "a0/b?
        // a0 is the collection id, b? is the document id in collection a0.

        String documentPath = "a0/b" + String(count);

        // If the document path contains space e.g. "a b c/d e f"
        // It should encode the space as %20 then the path will be "a%20b%20c/d%20e%20f"

        // double
        content.set("fields/temperatura/doubleValue", water_temp);

        // boolean
        content.set("fields/myBool/booleanValue", true);

        // integer
        content.set("fields/myInteger/integerValue", String(random(500, 1000)));

        // null
        content.set("fields/myNull/nullValue"); // no value set

        String doc_path = "projects/";
        doc_path += FIREBASE_PROJECT_ID;
        doc_path += "/databases/(default)/documents/c1j/codigoar"; // coll_id and doc_id are your collection id and document id

        // reference
        content.set("fields/myRef/referenceValue", doc_path.c_str());

        // timestamp
        content.set("fields/myTimestamp/timestampValue", "2014-10-02T15:01:23Z"); // RFC3339 UTC "Zulu" format

        // bytes
        content.set("fields/myBytes/bytesValue", "aGVsbG8="); // base64 encoded

        // array
        content.set("fields/myArray/arrayValue/values/[0]/stringValue", "test");
        content.set("fields/myArray/arrayValue/values/[1]/integerValue", "20");
        content.set("fields/myArray/arrayValue/values/[2]/booleanValue", true);

        // map
        content.set("fields/myMap/mapValue/fields/name/stringValue", "wrench");
        content.set("fields/myMap/mapValue/fields/mass/stringValue", "1.3kg");
        content.set("fields/myMap/mapValue/fields/count/integerValue", "3");

        // lat long
        content.set("fields/myLatLng/geoPointValue/latitude", 1.486284);
        content.set("fields/myLatLng/geoPointValue/longitude", 23.678198);

        count++;

        Serial.print("Create a document... ");

        if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw()))
            Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
        else
            Serial.println(fbdo.errorReason());
    }
}

}

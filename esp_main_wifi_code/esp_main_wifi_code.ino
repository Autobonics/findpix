//Firebase=================================================
//WiFi

//Firebase
#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
// Provide the token generation process info.
#include <addons/TokenHelper.h>
// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>
/* 1. Define the WiFi credentials */
#define WIFI_SSID "bonic"
#define WIFI_PASSWORD "bonic@123"
// For the following credentials, see examples/Authentications/SignInAsUser/EmailPassword/EmailPassword.ino
/* 2. Define the API Key */
#define API_KEY "AIzaSyDfgzl6ucMinUyZCJaj_fNYN-4A9CT5Zco"
/* 3. Define the RTDB URL */
#define DATABASE_URL "https://findpix-5c4a0-default-rtdb.asia-southeast1.firebasedatabase.app/" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app
/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "device@gmail.com"
#define USER_PASSWORD "12345678"
// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;
// Variable to save USER UID
String uid;
//Databse
String path;

//=============================================
#include <HardwareSerial.h>
#include <TinyGPS++.h>

#include <Adafruit_MPU6050.h>
#include <Wire.h>
Adafruit_MPU6050 mpu;

HardwareSerial neo(1); // Use UART 1 on ESP32
String lati = "";
String longi = "";
float speed_kmph = 0.0; 
TinyGPSPlus gps;

#define pushButton 8
bool isSos = false;

void setup() {
  Serial.begin(115200);
  neo.begin(9600, SERIAL_8N1, 3, 2); // Use pins 16 (RX) and 17 (TX) for UART communication with GPS module

  pinMode(pushButton, INPUT);
 
  Serial.println();
  delay(1000);
  Serial.println("Setupig MPU-6050 sensor");

  while (!Serial) delay(10);

  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip,Rotary module is okay, but program cannot be started");
    while (1) delay(10);
  }

  Serial.println("MPU6050 Found!");

  // Configure sensor settings
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);

  Serial.println("");
  delay(100);


  //WIFI
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  unsigned long ms = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();


  //FIREBASE
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  // Limit the size of response payload to be collected in FirebaseData
  fbdo.setResponseSize(2048);

  Firebase.begin(&config, &auth);

  // Comment or pass false value when WiFi reconnection will control by your code or third party library
  Firebase.reconnectWiFi(true);

  Firebase.setDoubleDigits(5);

  config.timeout.serverResponse = 10 * 1000;

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  path = "devices/" + uid + "/reading";
}


void updateData(float temp, bool isSos, float acl_x, float acl_y, float acl_z, float gyro_x, float gyro_y, float gyro_z){
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 4000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    FirebaseJson json;
    json.set("lat", lati);
    json.set("long", longi);
    json.set("speed", speed_kmph);
    json.set("temp", temp);
    json.set("sos", isSos);
    json.set("acl_x", acl_x);
    json.set("acl_y", acl_y);
    json.set("acl_z", acl_z);
    json.set("gyro_x", gyro_x);
    json.set("gyro_y", gyro_y);
    json.set("gyro_z", gyro_z);
    json.set(F("ts/.sv"), F("timestamp"));
    // Serial.printf("Set json... %s\n", Firebase.RTDB.set(&fbdo, path.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Set data with timestamp... %s\n", Firebase.setJSON(fbdo, path.c_str(), json) ? fbdo.to<FirebaseJson>().raw() : fbdo.errorReason().c_str());
    Serial.println(""); 
  }
}

void loop() {

  bool buttonState = digitalRead(pushButton);
  if(buttonState){
    isSos = !isSos;
    delay(100);
  }
  
  //Values from MPU-6050
sensors_event_t accel, gyro, temp;
mpu.getEvent(&accel, &gyro, &temp);


  // Print accelerometer data
// Print accelerometer data
Serial.print("Acceleration (X,Y,Z): ");
Serial.print(accel.acceleration.x);
Serial.print(", ");
Serial.print(accel.acceleration.y);
Serial.print(", ");
Serial.print(accel.acceleration.z);
Serial.println(" m/s^2");

// Print gyroscope data
Serial.print("Rotation (X,Y,Z): ");
Serial.print(gyro.gyro.x);
Serial.print(", ");
Serial.print(gyro.gyro.y);
Serial.print(", ");
Serial.print(gyro.gyro.z);
Serial.println(" rad/s");

// Print temperature data
Serial.print("Temperature: ");
Serial.print(temp.temperature);
Serial.println(" degC");

  Serial.println("");

  smartDelay(1000);

  Serial.println("GPS data received");
  Serial.println("Latitude: " + lati);
  Serial.println("Longitude: " + longi);
  Serial.print("Speed: ");
  Serial.print(speed_kmph);
  Serial.println(" km/h");

  delay(1000); // Wait for one second before the next iteration

  updateData(temp.temperature, isSos, accel.acceleration.x, accel.acceleration.y, accel.acceleration.z, gyro.gyro.x, gyro.gyro.y, gyro.gyro.z);
}

static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (neo.available()) {
      char c = neo.read();
      gps.encode(c);
    }
  } while (millis() - start < ms);

  // Update latitude, longitude, and speed
  lati = String(gps.location.lat(), 8);
  longi = String(gps.location.lng(), 8);
  speed_kmph = gps.speed.kmph();
}

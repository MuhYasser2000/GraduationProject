//4/8/2023
//simple code firebase with esp32

#define BLYNK_TEMPLATE_ID "TMPL6ItN9_gy6"
#define BLYNK_TEMPLATE_NAME "Fuel Master"
#define BLYNK_AUTH_TOKEN "4rJ99IbxDVsp4GQ5nMHmUluWnCKGmMRN"

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "Shehda"
#define WIFI_PASSWORD "123muhammed#"

// Insert Firebase project API Key
#define API_KEY "AIzaSyAVc16BfLZRcAraTjWND_RjxecQuzxc29Q"
// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://graduationproject-rtdb-default-rtdb.firebaseio.com/"

#define Tank_Inlet 23 //pushButton for tank inlet
#define Tank_Outlet 22 //pin for pushButton for outlet tool زر لفرد التعبئة
#define Outlet_Pump 13 //pin for outlet pump (outlet tool)
#define LevelSensor 34 //pin for level sensor
#define TempretureSensor 35 //pin for tempreture sensor
#define PressureSensor 32 //pin for pressure sensor
#define Compressor 19 //pin for compressor 
#define Buzzer 18 //pin for buzzer
#define Backup_Pump 21 //pin for backup pump

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

//Define Firebase Data object
FirebaseData fbdo; 
FirebaseAuth auth;
FirebaseConfig config;

int ldrData=0;
float voltage=0.0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;
  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

/* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  pinMode(23, INPUT); // pin for tank inlet
  pinMode(22, INPUT); //pin for زر فرد التعبئة
  pinMode(13, OUTPUT); // محاكاة مضخة فرد التعبئة
  pinMode(34, INPUT); //pin for level sensor read
  pinMode(35, INPUT); //pin for tempreture sensor
  pinMode(32, INPUT); //pin for tempreture sensor
  pinMode(19, OUTPUT); //pin for compressor
  pinMode(18, OUTPUT); //pin for buzzer
  pinMode(21, OUTPUT); //pin for emergency valve
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    // ---------- Store sensor data to RTDB
    int InputRead = digitalRead(Tank_Inlet); //هان بنعمل قراءة لمفتاح دخل التنك
    if(InputRead==0){
      int outlet=digitalRead(Tank_Outlet); //بنعمل قراءة لمفتاح فرد التعبئة
      if(outlet==1){
        Serial.println("The filling device is working");
        digitalWrite(Outlet_Pump, HIGH);
      }else{
        digitalWrite(Outlet_Pump, LOW);
      }

      //reading and display level sensor
      int LevelRead=analogRead(LevelSensor);
      float LevelVoltage= LevelRead * (3.3 / 4095.0);
      float Liters= LevelVoltage*3.93939394;
      Serial.println("the current ullage of tank:");
      Serial.println(Liters);
      Serial.print("liters ");
      if(Firebase.RTDB.setFloat(&fbdo,"sensor/Level",Liters)){
        Serial.println();
        Serial.print(Liters);
        Serial.print("-- successfully saved to:"+fbdo.dataPath());
        Serial.print("("+fbdo.dataType()+")");
      }else{
        Serial.print("Failed :"+fbdo.errorReason());
      }

      //جزئية قياس الحرارة وتحويلها لسيلزيوس
      int TempretureRead=analogRead(35);
      float TempVoltage= TempretureRead * (3.3 / 4095.0);
      float FuelTemp= (TempVoltage-0.5)*100;
      Serial.println("fuel tempreture is:");
      Serial.println(FuelTemp);
      Serial.println("siliceous");
      if(Firebase.RTDB.setFloat(&fbdo,"sensor/Tempreture",FuelTemp)){
        Serial.println();
        Serial.print(FuelTemp);
        Serial.print("-- successfully saved to:"+fbdo.dataPath());
        Serial.print("("+fbdo.dataType()+")");
      }else{
        Serial.print("Failed :"+fbdo.errorReason());
      }


      /*float slope = ...; // Get the slope value from the datasheet
      float intercept = ...; // Get the intercept value from the datasheet

      float pressure = slope * voltage + intercept;
      */
      int PressureRead=analogRead(32);
      int pressureSlope = 85; //from map sensor dataSheet
      int pressureIntercept=50;  //from map sensor DataSheet
      float PreVoltage= PressureRead* (3.3 / 4095.0);
      float pressure = pressureSlope * PreVoltage + pressureIntercept; //equation to convert voltage to bar
      Serial.println("Tank Pressure :");
      Serial.println(pressure);
      Serial.println("Bar");
      if(pressure<=250){
        digitalWrite(Buzzer, HIGH); //to enable buzzer(alarm)
        digitalWrite(Compressor,OUTPUT); //to enable alarm(buzzer)
        digitalWrite(Backup_Pump,OUTPUT); //to enable secound valve
      }
      else{
        digitalWrite(Buzzer,LOW); 
        digitalWrite(Compressor,LOW); 
      }

    }
    /*
    ldrData= analogRead(LDR_PIN);
    //voltage=(float)analogReadMilliVoltas(LDR_PIN)/1000;
    if(Firebase.RTDB.setInt(&fbdo,"sensor/ldr_Data",ldrData)){
      Serial.println();
      Serial.print(ldrData);
      Serial.print("-- successfully saved to:"+fbdo.dataPath());
      Serial.print("("+fbdo.dataType()+")");
    }else{
      Serial.print("Failed :"+fbdo.errorReason());

    }*/
  }

}
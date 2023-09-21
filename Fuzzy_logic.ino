#include <Fuzzy.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>;
#include <ThingSpeak.h>;
#include "CTBot.h"
CTBot myBot;

//String apiKey = "isi_auth_token_Anda";
const char *ssid =  "Kantor";    
const char *pass =  "tanyasajidyeah";
const char* server = "api.thingspeak.com";
String token = ""; //token telegram

WiFiClient client;
unsigned long myChannelNumber = 1587676;
const char * myWriteAPIKey = "";

#define DHTPIN D1         
DHT dht(DHTPIN, DHT22);

//uploadThingpspeak
bool uploadThingspeak = true;
bool uploadSuhu = true;
bool uploadKelembapan = false;
bool uploadKecepatan = false;
bool uploadJarak = false;

//Threshold Air
int relay1 = 13; //D7
float threshold = 2.80;
String pompa;

void cekThreshold(float x, float threshold){
  if (x > threshold){
    digitalWrite(relay1, HIGH);
    pompa = "Pompa Air Menyala";
    Serial.print(pompa);
    Serial.print("\n");
//    myBot.sendMessage(msg.sender.id, "Jarak Ketinggian Air : " + String(x) + "\n"
//                                      + "Pompa Menyala");
    delay(5000);
    digitalWrite(relay1, LOW);
  }else {
    digitalWrite(relay1, LOW);
    pompa = "Pompa Air Mati";
    Serial.print(pompa);
    Serial.print("\n");
    delay(5000);
  }
}



//SensorUltraSonic
const int trigPin = 12; //D6
const int echoPin = 14; //D5

//define sound velocity in cm/uS
#define SOUND_VELOCITY 0.034
#define CM_TO_INCH 0.393701

long duration;
float distanceCm;

// PWM
const int pwmPin = 2; //D4

Fuzzy *fuzzy = new Fuzzy();

//FuzzyInput Suhu
FuzzySet *dingin    = new FuzzySet(0, 32, 35, 37);
FuzzySet *sedang    = new FuzzySet(35, 37, 40, 42);
FuzzySet *panas    = new FuzzySet(40, 42, 47, 100);

//FuzzyInput Kelembapan
FuzzySet *kering    = new FuzzySet(0, 0, 16, 33);
FuzzySet *normal    = new FuzzySet(16, 33, 49, 66);
FuzzySet *lembap    = new FuzzySet(49, 66, 100, 100);

//FuzzyOutput Kipas
FuzzySet *lambat    = new FuzzySet(0, 0, 42, 85);
FuzzySet *biasa    = new FuzzySet(42, 85, 127, 170);
FuzzySet *cepat    = new FuzzySet(127, 170, 255, 255);

void setup()
{
  Serial.begin(115200);
  dht.begin();
  //randomSeed(analogRead(0));
  
  Serial.println("Connecting ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  ThingSpeak.begin(client);
  myBot.setTelegramToken(token);

  // check if all things are ok
  if (myBot.testConnection())
    Serial.println("\ntestConnection OK");
  else
    Serial.println("\ntestConnection Not OK");

    
  //Relay
  pinMode(relay1, OUTPUT);
  digitalWrite(relay1, LOW);

  //SensorUltraSonic
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input


  //Fuzzy input suhu
  FuzzyInput *suhu = new FuzzyInput(1);
  suhu->addFuzzySet(dingin);
  suhu->addFuzzySet(sedang);
  suhu->addFuzzySet(panas);
  fuzzy->addFuzzyInput(suhu);

  //Fuzzy input kelembapan
  FuzzyInput *kelembapan = new FuzzyInput(2);
  kelembapan->addFuzzySet(kering);
  kelembapan->addFuzzySet(normal);
  kelembapan->addFuzzySet(lembap);
  fuzzy->addFuzzyInput(kelembapan);

  //Fuzzy output Kipas
  FuzzyOutput *kipas = new FuzzyOutput(1);
  kipas->addFuzzySet(lambat);
  kipas->addFuzzySet(biasa);
  kipas->addFuzzySet(cepat);
  fuzzy->addFuzzyOutput(kipas);

  //Fuzzy Rule
 
  FuzzyRuleAntecedent *dingin_kering = new FuzzyRuleAntecedent();
  dingin_kering->joinWithAND(dingin, kering);
  FuzzyRuleConsequent *kipas_lambat1 = new FuzzyRuleConsequent();
  kipas_lambat1->addOutput(lambat);
  FuzzyRule *fuzzyrule1 = new FuzzyRule(1, dingin_kering, kipas_lambat1);
  fuzzy->addFuzzyRule(fuzzyrule1);
  
  FuzzyRuleAntecedent *dingin_normal = new FuzzyRuleAntecedent();
  dingin_normal->joinWithAND(dingin, normal);
  FuzzyRuleConsequent *kipas_lambat2 = new FuzzyRuleConsequent();
  kipas_lambat2->addOutput(lambat);
  FuzzyRule *fuzzyrule2 = new FuzzyRule(2, dingin_normal, kipas_lambat2);
  fuzzy->addFuzzyRule(fuzzyrule2);
  
  FuzzyRuleAntecedent *dingin_lembap = new FuzzyRuleAntecedent();
  dingin_lembap->joinWithAND(dingin, lembap);
  FuzzyRuleConsequent *kipas_lambat3 = new FuzzyRuleConsequent();
  kipas_lambat3->addOutput(lambat);
  FuzzyRule *fuzzyrule3 = new FuzzyRule(3, dingin_normal, kipas_lambat3);
  fuzzy->addFuzzyRule(fuzzyrule3);
  
  FuzzyRuleAntecedent *sedang_kering = new FuzzyRuleAntecedent();
  sedang_kering->joinWithAND(sedang, kering);
  FuzzyRuleConsequent *kipas_lambat4 = new FuzzyRuleConsequent();
  kipas_lambat4->addOutput(lambat);
  FuzzyRule *fuzzyrule4 = new FuzzyRule(4, sedang_kering, kipas_lambat4);
  fuzzy->addFuzzyRule(fuzzyrule4);
    
  FuzzyRuleAntecedent *sedang_normal = new FuzzyRuleAntecedent();
  sedang_normal->joinWithAND(sedang, normal);
  FuzzyRuleConsequent *kipas_lambat5 = new FuzzyRuleConsequent();
  kipas_lambat5->addOutput(lambat);
  FuzzyRule *fuzzyrule5 = new FuzzyRule(5, sedang_normal, kipas_lambat5);
  fuzzy->addFuzzyRule(fuzzyrule5);

  FuzzyRuleAntecedent *sedang_lembap = new FuzzyRuleAntecedent();
  sedang_lembap->joinWithAND(sedang, lembap);
  FuzzyRuleConsequent *kipas_lambat6 = new FuzzyRuleConsequent();
  kipas_lambat6->addOutput(lambat);
  FuzzyRule *fuzzyrule6 = new FuzzyRule(6, sedang_lembap, kipas_lambat6);
  fuzzy->addFuzzyRule(fuzzyrule6);
    
  FuzzyRuleAntecedent *panas_kering = new FuzzyRuleAntecedent();
  panas_kering->joinWithAND(panas, kering);
  FuzzyRuleConsequent *kipas_cepat1 = new FuzzyRuleConsequent();
  kipas_cepat1->addOutput(cepat);
  FuzzyRule *fuzzyrule7 = new FuzzyRule(7, panas_kering, kipas_cepat1);
  fuzzy->addFuzzyRule(fuzzyrule7);
      
  FuzzyRuleAntecedent *panas_normal = new FuzzyRuleAntecedent();
  panas_normal->joinWithAND(panas, normal);
  FuzzyRuleConsequent *kipas_cepat2 = new FuzzyRuleConsequent();
  kipas_cepat2->addOutput(cepat);
  FuzzyRule *fuzzyrule8 = new FuzzyRule(8, panas_normal, kipas_cepat2);
  fuzzy->addFuzzyRule(fuzzyrule8);
      
  FuzzyRuleAntecedent *panas_lembap = new FuzzyRuleAntecedent();
  panas_lembap->joinWithAND(panas, lembap);
  FuzzyRuleConsequent *kipas_cepat3 = new FuzzyRuleConsequent();
  kipas_cepat3->addOutput(cepat);
  FuzzyRule *fuzzyrule9 = new FuzzyRule(9, panas_lembap, kipas_cepat3);
  fuzzy->addFuzzyRule(fuzzyrule9);

}

void loop()
{
  // check if all things are ok
  if (myBot.testConnection())
    Serial.println("\ntestConnection OK\n");
  else
    Serial.println("\ntestConnection Not OK\n");
    
  float kelembapan = dht.readHumidity();
  float suhu = dht.readTemperature();
      if (isnan(kelembapan) || isnan(suhu)) 
   {
       Serial.println("Sensor Tidak Dapat Mendeteksi");
       delay(10000);
        return;
   }
  Serial.print(F("Suhu: "));
  Serial.print(suhu);
  Serial.print(F("°C "));
  Serial.print("\n");
  Serial.print(F("Kelembapan: "));
  Serial.print(kelembapan);
  Serial.print("\n");
  fuzzy->setInput(1, suhu);
  fuzzy->setInput(2, kelembapan);
  fuzzy->fuzzify();
  float output = fuzzy->defuzzify(1);
  analogWrite(pwmPin, output);

  // OUTPUT HASIL
  
  Serial.println("Result: ");
  Serial.print("Speed: ");
  Serial.println(output);

  //* KETINGGIAN AIR *//
  
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance
  distanceCm = duration * SOUND_VELOCITY/2;
  
  // Prints the distance on the Serial Monitor
  Serial.print("Jarak Ketinggian Air (cm): ");
  Serial.println(distanceCm);
  cekThreshold(distanceCm,threshold);
  Serial.print("\n");


  String Message = "Suhu : " + String(suhu, 2) + "°C \n" +
                   "Kelembapan: " + String(kelembapan, 2) + "%\n" +
                   "Speed : " + String(output, 2) + "\n" +
                   "Jarak Ketinggian Air (cm): " + String(distanceCm, 2) + "\n" + pompa;
                   

  //set telegram
  // a variable to store telegram message data
  TBMessage msg;

  // if there is an incoming message...
  if (myBot.getNewMessage(msg)) {
    if (msg.text.equalsIgnoreCase("/status")) {                   //Perintah dari telegram ke perangkat
      myBot.sendMessage(msg.sender.id, Message);
    }
    else {                                                    // otherwise...
      // generate the message for the sender
      String reply;
      reply = (String)"Welcome " + msg.sender.username + (String)". coba /status." + "\n Pesan akan dibalas maksimal 15 detik";
      myBot.sendMessage(msg.sender.id, reply);             // and send it
    }
  }

  delay(15000);
  
  while(uploadThingspeak){
    if(uploadSuhu){
      ThingSpeak.writeField(myChannelNumber, 1,suhu, myWriteAPIKey);
      uploadSuhu = false;
      uploadKelembapan = true;
      Serial.println("upload suhu");
      return;
    }else if(uploadKelembapan){
      ThingSpeak.writeField(myChannelNumber, 2,kelembapan, myWriteAPIKey);
      uploadKelembapan = false;
      uploadKecepatan = true;
      Serial.println("upload kelembapan");
      return;
    }else if(uploadKecepatan){
      ThingSpeak.writeField(myChannelNumber, 3,output, myWriteAPIKey);
      uploadKecepatan = false;
      uploadJarak = true;
      Serial.println("upload kecepatan");
      return;
    }else if(uploadJarak){
      ThingSpeak.writeField(myChannelNumber, 4,distanceCm, myWriteAPIKey);
      uploadKecepatan = false;
      uploadSuhu = true;
      Serial.println("upload jarak");
      return;
    }
  }
}

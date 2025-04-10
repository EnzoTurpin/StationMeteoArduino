#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// Configuration WiFi - À MODIFIER
const char* ssid = "VOTRE_SSID";           // Nom de votre réseau WiFi
const char* password = "VOTRE_PASSWORD";    // Mot de passe WiFi

// Configuration du serveur - À MODIFIER si nécessaire
const char* serverUrl = "http://192.168.1.XX:3001/api/weather/update";  // Remplacez XX par l'IP de votre PC

// Configuration des pins
#define DHTPIN 4        // DHT11 connecté au GPIO4
#define DHTTYPE DHT11   // Type de capteur (DHT11)

// Initialisation des objets
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Adresse I2C 0x27, 16 colonnes et 2 lignes

// Variables pour le délai entre les mesures
unsigned long lastTime = 0;
const long interval = 5000;  // Intervalle de 5 secondes entre les mesures

void setup() {
  // Démarrage de la communication série
  Serial.begin(115200);
  Serial.println("Démarrage de la station météo");
  
  // Initialisation du LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Station Meteo");
  lcd.setCursor(0, 1);
  lcd.print("Initialisation...");
  
  // Initialisation du DHT11
  dht.begin();
  
  // Connexion WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connexion au WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(0, 1);
    lcd.print("Connexion WiFi...");
  }
  
  Serial.println("");
  Serial.println("WiFi connecté!");
  Serial.print("Adresse IP: ");
  Serial.println(WiFi.localIP());
  
  lcd.clear();
  lcd.print("WiFi Connecte!");
  delay(2000);
}

void loop() {
  unsigned long currentTime = millis();
  
  // Vérification si c'est le moment de faire une nouvelle mesure
  if (currentTime - lastTime >= interval) {
    lastTime = currentTime;
    
    // Lecture des données du capteur
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    
    // Vérification si la lecture a réussi
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Erreur de lecture du capteur DHT11!");
      lcd.clear();
      lcd.print("Erreur capteur!");
      return;
    }
    
    // Affichage sur le LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperature, 1);
    lcd.print("C");
    
    lcd.setCursor(0, 1);
    lcd.print("Hum: ");
    lcd.print(humidity, 1);
    lcd.print("%");
    
    // Affichage sur le moniteur série
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print("°C, Humidité: ");
    Serial.print(humidity);
    Serial.println("%");
    
    // Envoi des données au serveur si le WiFi est connecté
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverUrl);
      http.addHeader("Content-Type", "application/json");
      
      // Création du JSON
      StaticJsonDocument<200> doc;
      doc["temperature"] = temperature;
      doc["humidity"] = humidity;
      
      String jsonString;
      serializeJson(doc, jsonString);
      
      // Envoi de la requête POST
      int httpResponseCode = http.POST(jsonString);
      
      if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
      } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      
      http.end();
    }
  }
} 
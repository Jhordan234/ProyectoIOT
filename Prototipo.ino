#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT22
#define CO2_PIN 0

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  
  Wire.begin(21, 20);
  dht.begin();
  
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Iniciando...");
  delay(2000);
  lcd.clear();
}

void loop() {
  // Leer DHT22
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Leer CO2
  int valorCO2 = analogRead(CO2_PIN);
  float voltaje = valorCO2 * (3.3 / 4095.0);

  // Serial
  if (isnan(h) || isnan(t)) {
    Serial.println("Error DHT22");
  } else {
    Serial.print("Temp: "); Serial.print(t); Serial.println(" C");
    Serial.print("Hum: ");  Serial.print(h); Serial.println(" %");
  }
  Serial.print("CO2 ADC: "); Serial.print(valorCO2);
  Serial.print("  Voltaje: "); Serial.print(voltaje); Serial.println(" V");
  Serial.println("---");

  // LCD - pantalla 1: Temperatura y Humedad
  if (isnan(h) || isnan(t)) {
    lcd.setCursor(0, 0);
    lcd.print("Error sensor    ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(t, 1);
    lcd.print("C H:");
    lcd.print(h, 0);
    lcd.print("%  ");
    
    // LCD - pantalla 2: CO2
    lcd.setCursor(0, 1);
    lcd.print("CO2:");
    lcd.print(valorCO2);
    lcd.print(" ");
    lcd.print(voltaje, 2);
    lcd.print("V  ");
  }

  delay(2000);
}
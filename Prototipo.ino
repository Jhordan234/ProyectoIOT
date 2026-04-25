#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

// ==================== CONFIG ====================

#define DHTPIN 2
#define DHTTYPE DHT22
#define CO2_PIN 0

const char* ssid     = "nombre de tu red";
const char* password = "contraseña de tu red";

String firebaseURL = "URL de tu RealTime en FireBase";

const char* ntpServer   = "pool.ntp.org";
const long  gmtOffset_s = -18000; // UTC-5 Perú
const int   dstOffset_s = 0;

// ==================== TIEMPOS ====================
//
//  [--- 5 min ACTIVO (LCD + sensores) ---][-- 1 min SLEEP --]
//  |__________________________________________||_____________|
//              ciclo total = 6 min
//
//  Dentro del activo:
//    - LCD se refresca cada 2s (delay al final del loop)
//    - Revisión de cambio cada 30s
//    - Guardado forzado cada 2 min (garantiza puntos en gráfica)
//    - Al terminar los 5 min: vacía cola → duerme 1 min

const unsigned long TIEMPO_ACTIVO_MS  = 300000ULL; // 5 min en ms
const uint64_t      TIEMPO_SLEEP_US   = 60ULL * 1000000ULL; // 1 min en µs

const long INTERVALO_REVISION = 30000;  // revisión de umbral cada 30s
const long INTERVALO_FORZADO  = 120000; // guardado forzado cada 2 min

// ==================== FILTRO ====================

const float ALPHA = 0.3;

// ==================== UMBRALES ====================

const float UMBRAL_T   = 0.8;
const float UMBRAL_H   = 2.0;
const float UMBRAL_CO2 = 50.0;

// ==================== MEMORIA RTC ====================
// Solo primitivos — String NO sobrevive deep sleep

RTC_DATA_ATTR float t_filtrado   = 0;
RTC_DATA_ATTR float h_filtrado   = 0;
RTC_DATA_ATTR float co2_filtrado = 0;

RTC_DATA_ATTR float t_anterior   = -999;
RTC_DATA_ATTR float h_anterior   = -999;
RTC_DATA_ATTR float co2_anterior = -999;

// ==================== COLA OFFLINE ====================

struct LecturaRTC {
  float temperatura;
  float humedad;
  float co2;
  float voltaje;
  long  epochTime; // Unix timestamp — no String
  bool  esForzado;
};

RTC_DATA_ATTR LecturaRTC cola[10];
RTC_DATA_ATTR int colaSize = 0;

// ==================== OBJETOS ====================

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ==================== TIMERS INTERNOS ====================

unsigned long inicioActivo        = 0;
unsigned long tiempoUltimaRevision = 0;
unsigned long tiempoUltimoForzado  = 0;

// ==================== FUNCIONES ====================

long obtenerEpoch() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return millis() / 1000;
  return (long)mktime(&timeinfo);
}

String epochAISO(long epoch) {
  time_t t = (time_t)epoch;
  struct tm* ti = localtime(&t);
  char buf[24];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", ti);
  return String(buf);
}

String construirJSON(LecturaRTC& lec) {
  String json = "{";
  json += "\"temperatura\":"  + String(lec.temperatura, 2) + ",";
  json += "\"humedad\":"      + String(lec.humedad, 2)     + ",";
  json += "\"co2\":"          + String(lec.co2, 2)         + ",";
  json += "\"voltaje\":"      + String(lec.voltaje, 4)     + ",";
  json += "\"motivo\":\""    + String(lec.esForzado ? "forzado" : "cambio") + "\",";
  json += "\"timestamp\":\"" + epochAISO(lec.epochTime)    + "\"";
  json += "}";
  return json;
}

bool enviarAFirebase(LecturaRTC& lec) {
  if (WiFi.status() != WL_CONNECTED) return false;
  HTTPClient http;
  http.begin(firebaseURL);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);
  int codigo = http.POST(construirJSON(lec));
  http.end();
  Serial.printf("  POST [%s]: %d\n", lec.esForzado ? "forzado" : "cambio", codigo);
  return (codigo == 200 || codigo == 204);
}

void enviarBatch() {
  if (colaSize == 0) { Serial.println("Cola vacia"); return; }
  if (WiFi.status() != WL_CONNECTED) { Serial.println("Sin WiFi - cola intacta"); return; }

  Serial.printf("Enviando lote: %d registro(s)\n", colaSize);
  int ok = 0;
  for (int i = 0; i < colaSize; i++) {
    if (enviarAFirebase(cola[i])) ok++;
  }
  Serial.printf("Lote OK: %d/%d\n", ok, colaSize);
  colaSize = 0;
}

void agregarACola(float t, float h, float co2, float v, bool forzado) {
  if (colaSize >= 10) {
    Serial.println("Cola llena - enviando antes de agregar");
    enviarBatch();
  }
  cola[colaSize++] = { t, h, co2, v, obtenerEpoch(), forzado };
  Serial.printf("En cola: %d/10\n", colaSize);
}

void actualizarLCD(float t, float h, float co2, float v) {
  // Fila 0: Temperatura y Humedad completos
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(t, 1);
  lcd.print("C H:");
  lcd.print(h, 1);
  lcd.print("%  ");

  // Fila 1: CO2 y Voltaje completos
  lcd.setCursor(0, 1);
  lcd.print("CO2:");
  lcd.print((int)co2);
  lcd.print(" ");
  lcd.print(v, 2);
  lcd.print("V  ");
}

void dormirAhora() {
  // Vaciar cola antes de dormir — nunca perder datos
  if (colaSize > 0) {
    Serial.printf("Vaciando cola antes de dormir (%d datos)\n", colaSize);
    enviarBatch();
  }

  // Apagar pantalla para ahorrar energía durante el sleep
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Durmiendo 1min");
  delay(800);
  lcd.noBacklight();

  Serial.println("Entrando en Deep Sleep (1 min)");
  Serial.flush();

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  esp_sleep_enable_timer_wakeup(TIEMPO_SLEEP_US);
  esp_deep_sleep_start();
}

// ==================== SETUP ====================

void setup() {
  Serial.begin(115200);
  delay(500);

  inicioActivo = millis();

  // WiFi con timeout de 10s
  WiFi.begin(ssid, password);
  Serial.print("Conectando WiFi");
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 20) {
    delay(500);
    Serial.print(".");
    intentos++;
  }
  Serial.println(WiFi.status() == WL_CONNECTED
    ? "\nWiFi OK ✅" : "\nWiFi FALLO ❌");

  // NTP solo si hay WiFi
  if (WiFi.status() == WL_CONNECTED) {
    configTime(gmtOffset_s, dstOffset_s, ntpServer);
    struct tm ti;
    Serial.println(getLocalTime(&ti) ? "NTP OK ✅" : "NTP fallo ⚠️");
  }

  // Sensores
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  Wire.begin(21, 20);
  dht.begin();

  // LCD
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Iniciando...");
  delay(1500);
  lcd.clear();
}

// ==================== LOOP ====================

void loop() {

  unsigned long ahora = millis();

  // ========== ¿Tiempo activo terminó? → dormir ==========
  if (ahora - inicioActivo >= TIEMPO_ACTIVO_MS) {
    dormirAhora(); // no regresa — esp_deep_sleep_start()
    return;
  }

  // ========== LECTURA DE SENSORES ==========
  float h       = dht.readHumidity();
  float t       = dht.readTemperature();
  int   rawCO2  = analogRead(CO2_PIN);
  float voltaje = rawCO2 * (3.3 / 4095.0);

  if (isnan(h) || isnan(t)) {
    Serial.println("Error DHT22 - reintentando...");
    delay(2000);
    return;
  }

  // ========== FILTRO EMA (persiste en RTC) ==========
  t_filtrado   = (t_filtrado   * (1 - ALPHA)) + (t      * ALPHA);
  h_filtrado   = (h_filtrado   * (1 - ALPHA)) + (h      * ALPHA);
  co2_filtrado = (co2_filtrado * (1 - ALPHA)) + (rawCO2 * ALPHA);

  // ========== LCD EN TIEMPO REAL ==========
  actualizarLCD(t_filtrado, h_filtrado, co2_filtrado, voltaje);

  // ========== SERIAL ==========
  Serial.printf("T:%.2fC  H:%.2f%%  CO2:%.1f  V:%.4f\n",
                t_filtrado, h_filtrado, co2_filtrado, voltaje);

  // ========== GUARDADO FORZADO CADA 2 MIN ==========
  // Garantiza puntos en la gráfica aunque el ambiente esté estable
  if (ahora - tiempoUltimoForzado >= INTERVALO_FORZADO) {
    tiempoUltimoForzado  = ahora;
    tiempoUltimaRevision = ahora; // evita doble guardado en mismo ciclo
    Serial.println("Guardado forzado (2 min)");
    agregarACola(t_filtrado, h_filtrado, co2_filtrado, voltaje, true);
    t_anterior = t_filtrado;
    h_anterior = h_filtrado;
    co2_anterior = co2_filtrado;

  // ========== REVISIÓN DE CAMBIO CADA 30s ==========
  } else if (ahora - tiempoUltimaRevision >= INTERVALO_REVISION) {
    tiempoUltimaRevision = ahora;

    bool hayCambio =
      fabs(t_filtrado   - t_anterior)   > UMBRAL_T   ||
      fabs(h_filtrado   - h_anterior)   > UMBRAL_H   ||
      fabs(co2_filtrado - co2_anterior) > UMBRAL_CO2;

    if (hayCambio) {
      Serial.println("Cambio notable → cola");
      agregarACola(t_filtrado, h_filtrado, co2_filtrado, voltaje, false);
      t_anterior   = t_filtrado;
      h_anterior   = h_filtrado;
      co2_anterior = co2_filtrado;
    } else {
      Serial.println("Sin cambio notable");
    }
  }

  // ========== ENVIAR LOTE SI HAY 5+ DATOS ==========
  if (colaSize >= 5) {
    Serial.println("Lote de 5 listo → enviando");
    enviarBatch();
  }

  // ========== REFRESCO LCD: 2 SEGUNDOS ==========
  delay(2000);
}
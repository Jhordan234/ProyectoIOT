# 🌡️📡 Sistema IoT de Monitoreo Ambiental con ESP32

**Optimizado con Deep Sleep, Batching y Detección por Umbral**

---

## 📌 Descripción

Este proyecto implementa un sistema IoT basado en **ESP32** para el monitoreo de variables ambientales:

* 🌡️ Temperatura
* 💧 Humedad
* 🫁 CO₂ (sensor analógico)

Los datos se procesan localmente y se envían a **Firebase Realtime Database** de forma optimizada, reduciendo consumo energético y tráfico de red mediante:

* ✔ Detección por umbral
* ✔ Envío por lotes (batching)
* ✔ Ciclos híbridos (activo + Deep Sleep)

---

## ⚙️ Arquitectura del Sistema

```
[ Sensores ] → [ ESP32 ] → [ Procesamiento local ]
                                ↓
                         [ Cola (Batching) ]
                                ↓
                      [ Firebase Realtime DB ]
                                ↓
                        [ App / Dashboard ]
```

---

## 🔄 Ciclo de Funcionamiento

```
[ 5 minutos ACTIVO ]
  ├─ Lectura cada 2s
  ├─ Evaluación cada 30s
  ├─ Envío forzado cada 2 min
  └─ Acumulación en cola

[ 1 minuto SLEEP ]
  └─ Deep Sleep (ahorro energético)

→ Ciclo total: 6 minutos
```

---

## 🚀 Características Principales

### 🟢 1. Detección por Umbral

Evita envíos innecesarios:

* Temperatura: ±0.8 °C
* Humedad: ±2 %
* CO₂: ±50 (ADC)

---

### 📦 2. Batching (Envío por Lotes)

* Los datos se almacenan en una cola de hasta 10 registros
* Se envían cuando:

  * Se acumulan ≥ 5 lecturas
  * Antes de entrar en Deep Sleep

👉 Reduce significativamente el número de peticiones HTTP

---

### 🔋 3. Ahorro de Energía (Deep Sleep)

* El ESP32 entra en reposo durante 1 minuto
* Se apagan WiFi y LCD
* Uso de memoria RTC para persistencia

---

### 🔁 4. Modo Híbrido

Combina:

* Monitoreo en tiempo real (LCD cada 2s)
* Optimización energética por ciclos

---

### 🧠 5. Filtro de Suavizado (EMA)

Reduce ruido en sensores:

```
valor_filtrado = anterior * (1 - α) + nuevo * α
```

Con:

* α = 0.3

---

## ☁️ Integración con Firebase

Los datos se almacenan usando `push` automático:

```
sensores/
  esp32_1/
    lecturas/
      -ID_AUTO/
        temperatura
        humedad
        co2
        voltaje
        timestamp
        motivo
```

### 📝 Ejemplo de registro

```json
{
  "temperatura": 27.39,
  "humedad": 47.6,
  "co2": 1093.55,
  "voltaje": 0.8977,
  "motivo": "cambio",
  "timestamp": "2026-04-17T11:59:27"
}
```

---

## 🧩 Tecnologías Utilizadas

* ESP32
* Sensor DHT22
* Sensor CO₂ analógico
* LCD I2C 16x2
* WiFi (HTTPClient)
* NTP (sincronización de tiempo)
* Firebase Realtime Database

---

## 🧠 Gestión de Memoria (RTC)

Se utiliza memoria RTC para conservar datos entre ciclos de Deep Sleep:

* Últimos valores filtrados
* Valores anteriores (comparación de umbral)
* Cola de datos (batching)

⚠️ Nota:
`String` **no es persistente en RTC**, se usan tipos primitivos.

---

## 🔌 Requisitos de Hardware

* ESP32
* Sensor DHT22
* Sensor CO₂ analógico
* Pantalla LCD I2C (0x27)
* Resistencias y cableado básico

---

## 🛠️ Configuración

### 1. Credenciales WiFi

```cpp
const char* ssid = "TU_RED";
const char* password = "TU_PASSWORD";
```

### 2. Firebase

```cpp
String firebaseURL = "Aqui va tu ruta url de tu RealTime";
```

---

## 📊 Optimización Implementada

| Técnica                 | Implementada |
| ----------------------- | ------------ |
| Umbral de cambio        | ✅            |
| Reducción de frecuencia | ✅            |
| Batching                | ✅            |
| Deep Sleep              | ✅            |
| Persistencia RTC        | ✅            |

---

## ⚠️ Consideraciones

* El sensor de CO₂ es analógico → valores relativos
* Se requiere calibración para ppm reales
* Deep Sleep reinicia el microcontrolador en cada ciclo

---

## 📈 Ventajas del Sistema

* 🔻 Reducción de tráfico a Firebase
* 🔋 Bajo consumo energético
* 📊 Datos más limpios y relevantes
* 🔄 Escalable para múltiples nodos

---

## 🧑‍💻 Autor

Proyecto desarrollado como implementación de técnicas de optimización en sistemas IoT.

---

## 🏁 Conclusión

Este sistema demuestra un enfoque eficiente para IoT moderno, combinando:

* Procesamiento en el edge
* Optimización de comunicaciones
* Gestión energética

👉 Ideal para aplicaciones reales de monitoreo ambiental.

---

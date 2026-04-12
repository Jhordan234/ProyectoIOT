# 🌿 Sistema IoT de Monitoreo Ambiental (CO₂, Temperatura y Humedad)

## 📌 Descripción

Este proyecto consiste en un sistema de monitoreo ambiental basado en un microcontrolador ESP32 que permite medir:

* 🌡️ Temperatura
* 💧 Humedad
* 🌫️ Nivel de CO₂ (analógico)

Los datos se muestran en tiempo real mediante:

* 📟 Pantalla LCD I2C
* 💻 Monitor serial

---

## 🧰 Componentes Utilizados

* 🔹 ESP32 (Super Mini)
* 🔹 Sensor de CO₂ (salida analógica)
* 🔹 Sensor **DHT22 Temperature and Humidity Sensor**
* 🔹 Pantalla LCD I2C 16x2
* 🔹 Resistencias (para divisor de voltaje en CO₂)
* 🔹 Protoboard y cables

---

## 🔌 Conexiones

### 📍 Sensor DHT22

| Pin  | Conexión |
| ---- | -------- |
| VCC  | 3.3V     |
| GND  | GND      |
| DATA | GPIO 2   |

---

### 📍 Sensor CO₂

| Pin  | Conexión                             |
| ---- | ------------------------------------ |
| +    | 5V                                   |
| GND  | GND                                  |
| AOUT | GPIO 0 (mediante divisor de voltaje) |

⚠️ **Importante:**
Se utiliza un divisor de voltaje (10kΩ + 10kΩ) para proteger el ESP32.

---

### 📍 Pantalla LCD I2C

| Pin | Conexión |
| --- | -------- |
| VCC | 5V       |
| GND | GND      |
| SDA | GPIO 21  |
| SCL | GPIO 20  |

---

## ⚙️ Funcionamiento

El sistema realiza las siguientes tareas:

1. Inicializa sensores y pantalla LCD
2. Lee temperatura y humedad del DHT22
3. Lee señal analógica del sensor de CO₂
4. Convierte el valor ADC a voltaje
5. Muestra datos en:

   * Monitor serial
   * Pantalla LCD

---

## 🧪 Lógica del sistema

* Si el DHT22 falla → muestra error
* Si funciona → muestra temperatura y humedad
* Siempre muestra valor de CO₂ (ADC + voltaje)

---

## 🖥️ Ejemplo de salida (Serial)

```
Temp: 25.3 C
Hum: 60 %
CO2 ADC: 2150  Voltaje: 1.73 V
---
```

---

## 📟 Visualización en LCD

**Fila 1:**

```
T:25.3C H:60%
```

**Fila 2:**

```
CO2:2150 1.73V
```

---

## 📚 Librerías necesarias

Instalar en Arduino IDE:

* `Wire.h`
* `LiquidCrystal_I2C.h`
* `DHT.h`

---

## ⚠️ Consideraciones importantes

* 🔌 El ESP32 trabaja a 3.3V → proteger entradas analógicas
* 🌬️ Los sensores deben estar en un ambiente ventilado
* ⚡ Evitar superficies metálicas en contacto directo

---

## 🚀 Posibles mejoras

* 📡 Enviar datos a una plataforma web
* 📊 Visualización en dashboard IoT
* 🚨 Sistema de alertas por niveles de CO₂
* ☁️ Integración con IoT (Firebase, MQTT, etc.)

---

## 👨‍💻 Autor

Proyecto desarrollado como sistema de monitoreo ambiental IoT.

---

## 📌 Estado del proyecto

✅ Funcional
🔧 En mejora continua

---
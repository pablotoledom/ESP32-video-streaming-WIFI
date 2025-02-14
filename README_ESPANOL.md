# 🎥 ESP32 Video Streaming via WIFI a Pantalla ST7789 (Modo Access Point y WebSocket)

Este proyecto permite transmitir video desde un PC o dispositivo móvil a una pantalla **ST7789** utilizando un **ESP32** configurado como Access Point. El ESP32 aloja un servidor web con soporte para WebSocket, lo que permite que un cliente (por ejemplo, un navegador web) capture y envíe fotogramas comprimidos en **JPEG** a demanda. Los fotogramas se decodifican en el ESP32 y se muestran en la pantalla ST7789, con optimizaciones que reducen el flickering y mejoran la sincronización.

[![IMAGE Esp32 showing DooM video](https://raw.githubusercontent.com/pablotoledom/ESP32-video-streaming-WIFI/refs/heads/ESP32-video-streaming-WIFI/image.jpg)](https://youtu.be/23evMsoWspA)

---

## 🚀 Características
- ✅ Transmisión de video en tiempo real desde un PC o dispositivo móvil a la pantalla ST7789.
- ✅ El ESP32 se configura como Access Point, eliminando la necesidad de conectarse a una red WiFi existente.
- ✅ Comunicación vía WebSocket para solicitar fotogramas bajo demanda, asegurando una sincronización controlada.
- ✅ Conversión y compresión de frames a **JPEG** (calidad del 25%) para optimizar la transmisión.
- ✅ Decodificación de JPEG en el ESP32 y visualización en la pantalla ST7789.
- ✅ Optimización del refresco mediante doble buffering y actualización atómica para reducir el flickering.

---

## 📌 Requisitos

### 👉 **Hardware**
- 🎣 **Pantalla ST7789** (135x240 píxeles, conexión SPI)
- 🎒 **ESP32** (preferiblemente con PSRAM para mayor rendimiento)
- 🖥️ **PC o dispositivo móvil** con navegador web moderno (para enviar video)
- 🔌 **Cable USB** para programar y comunicar con el ESP32

### 👉 **Software y Librerías**

#### 📚 **En el ESP32 (Arduino)**
Instala las siguientes librerías en el **Arduino IDE**, si no las encuentras en la librería de Arduino puedes copiarlas directamente a tu computadora desde el directorio arduino/libraries de este proyecto y copiarlas dentro de la carpeta libraries que crea el sofware Arduino en tu PC:
- [`TFT_eSPI`](https://github.com/Bodmer/TFT_eSPI) → Manejo de la pantalla ST7789.
- [`JPEGDecoder`](https://github.com/Bodmer/JPEGDecoder) → Decodificación de imágenes JPEG.
- [`ESPAsyncWebServer`](https://github.com/me-no-dev/ESPAsyncWebServer) y [`AsyncTCP`](https://github.com/me-no-dev/AsyncTCP) → Servidor web y comunicación vía WebSocket.

📌 **Configuraciones necesarias en `TFT_eSPI`**:  
Edita `User_Setup.h` y asegúrate de definir:
```cpp
#define ST7789_DRIVER
#define TFT_WIDTH  135
#define TFT_HEIGHT 240
#define TFT_MISO -1   // No se utiliza para ST7789
#define TFT_MOSI 23   // GPIO 23
#define TFT_SCLK 18   // GPIO 18
#define TFT_CS   5    // GPIO 5
#define TFT_DC   2    // GPIO 2
#define TFT_RST  4    // GPIO 4
#define TFT_BL   32   // Para señal de enable (3.3V o un GPIO)
#define SPI_FREQUENCY  40000000  // Opcionalmente, 80 MHz si lo soporta el display
#define USE_DMA  // Para mejorar el rendimiento
```

#### 🌐 **Cliente Web (HTML/JavaScript)**
El ESP32 aloja una página web que permite:
- Seleccionar un video.
- Capturar fotogramas, rotarlos 90° (sentido horario) y redimensionarlos a 135×240.
- Comprimir los fotogramas a **JPEG** y enviarlos mediante WebSocket a demanda (al recibir la solicitud `"requestFrame"` del ESP32).

---

## 🛠️ Instalación y Uso

### 1️⃣ **Configurar y Cargar el Código en el ESP32**
1. **Abrir Arduino IDE** y cargar el código en `display.ino`.
2. **Conectar la pantalla ST7789 al ESP32** mediante SPI:
   | **ESP32** | **ST7789** |
   |-----------|------------|
   | **3.3V**  | **VCC**    |
   | **GND**   | **GND**    |
   | **18**    | **SCK**    |
   | **19**    | **MOSI**   |
   | **5**     | **DC**     |
   | **23**    | **CS**     |
   | **4**     | **RST**    |
3. El ESP32 se configurará como Access Point utilizando el SSID y contraseña definidos en el código (por ejemplo, `"ESP32-WIFI-video"` / `"TRC12345678"`).
4. Carga el código en el ESP32.

### 2️⃣ **Conectar y Usar la Interfaz Web**
1. En tu PC o dispositivo móvil, conéctate a la red WiFi creada por el ESP32.
2. Abre un navegador web y accede a la dirección IP del Access Point, normalmente `http://192.168.4.1`.
3. Selecciona un video mediante el input de la página.
4. Haz clic en **"Iniciar Captura"** para comenzar la reproducción.  
   - El cliente captura y envía el primer fotograma.
   - Una vez que el ESP32 procesa el frame recibido, envía el mensaje `"requestFrame"` a través del WebSocket.
   - Al recibir dicho mensaje, el cliente captura y envía el siguiente fotograma, creando un ciclo sincronizado.
5. La imagen se decodifica y se muestra en la pantalla ST7789.

---

## 🛠️ Solución de Problemas

### ❌ **Colores distorsionados**
✅ Solución: Asegúrate de que en el ESP32 esté activado `tft.setSwapBytes(true);`.

### ❌ **Imagen rotada incorrectamente**
✅ Solución: La imagen se rota 90° en sentido horario en el cliente web; revisa la función de rotación en el JavaScript si es necesario.

### ❌ **Fotogramas incompletos o flickering**
✅ Solución:
- Se ha implementado doble buffering y verificación del marcador de fin (0xFF, 0xD9) del JPEG para procesar solo frames completos.
- Se actualiza la pantalla de forma atómica para evitar parpadeos.

---

## 📝 Licencia

Este proyecto está bajo la licencia **MIT**. Puedes usarlo y modificarlo libremente.

Desarrollado por Pablo Toledo


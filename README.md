# 📽️ ESP32 Video Streaming via WIFI to ST7789 Screen (Access Point Mode and WebSocket)

This project allows video streaming from a PC or mobile device to an **ST7789** screen using an **ESP32** configured as an Access Point. The ESP32 hosts a web server with WebSocket support, enabling a client (e.g., a web browser) to capture and send **JPEG**-compressed frames on demand. The frames are decoded on the ESP32 and displayed on the ST7789 screen, with optimizations that reduce flickering and improve synchronization.

[![IMAGE Esp32 showing DooM video](https://raw.githubusercontent.com/pablotoledom/esp32-video-streaming/refs/heads/main/image.jpg)](https://www.youtube.com/watch?v=Cykcpi9xnGo)

---

## 🚀 Features
- ✅ Real-time video streaming from a PC or mobile device to the ST7789 screen.
- ✅ The ESP32 is set up as an Access Point, eliminating the need to connect to an existing WiFi network.
- ✅ WebSocket communication to request frames on demand, ensuring controlled synchronization.
- ✅ Frame conversion and compression to **JPEG** (25% quality) to optimize transmission.
- ✅ JPEG decoding on the ESP32 and display rendering on the ST7789 screen.
- ✅ Refresh rate optimization using double buffering and atomic updates to reduce flickering.

---

## 📌 Requirements

### 🔹 **Hardware**
- 📟 **ST7789 Screen** (135x240 pixels, SPI connection)
- 🎛️ **ESP32** (preferably with PSRAM for better performance)
- 🖥️ **PC or mobile device** with a modern web browser (for video transmission)
- 🔌 **USB Cable** for programming and communication with the ESP32

### 🔹 **Software and Libraries**

#### 📂 **On the ESP32 (Arduino)**
Install the following libraries in **Arduino IDE**, If you can't find them in the Arduino library, you can copy them directly to your computer from the arduino/libraries directory of this project and copy them into the libraries folder that the Arduino software creates on your PC:
- [`TFT_eSPI`](https://github.com/Bodmer/TFT_eSPI) → ST7789 screen handling.
- [`JPEGDecoder`](https://github.com/Bodmer/JPEGDecoder) → JPEG image decoding.
- [`ESPAsyncWebServer`](https://github.com/me-no-dev/ESPAsyncWebServer) and [`AsyncTCP`](https://github.com/me-no-dev/AsyncTCP) → Web server and WebSocket communication.

📌 **Necessary configurations in `TFT_eSPI`**:  
Edit `User_Setup.h` and ensure the following settings:
```cpp
#define ST7789_DRIVER
#define TFT_WIDTH  135
#define TFT_HEIGHT 240
#define TFT_MISO -1   // Not used for ST7789
#define TFT_MOSI 23   // GPIO 23
#define TFT_SCLK 18   // GPIO 18
#define TFT_CS   5    // GPIO 5
#define TFT_DC   2    // GPIO 2
#define TFT_RST  4    // GPIO 4
#define TFT_BL   32   // Enable signal (3.3V or a GPIO)
#define SPI_FREQUENCY  40000000  // Optionally, 80 MHz if supported by the display
#define USE_DMA  // To improve performance
```

#### 🌐 **Web Client (HTML/JavaScript)**
The ESP32 hosts a web page that allows:
- Selecting a video.
- Capturing frames, rotating them 90° (clockwise), and resizing them to 135×240.
- Compressing frames to **JPEG** and sending them via WebSocket on demand (upon receiving the `"requestFrame"` request from the ESP32).

---

## 🔧 Installation and Usage

### 1️⃣ **Set Up and Upload Code to the ESP32**
1. **Open Arduino IDE** and upload the `display.ino` code.
2. **Connect the ST7789 screen to the ESP32** via SPI:
   | **ESP32** | **ST7789** |
   |-----------|------------|
   | **3.3V**  | **VCC**    |
   | **GND**   | **GND**    |
   | **18**    | **SCK**    |
   | **19**    | **MOSI**   |
   | **5**     | **DC**     |
   | **23**    | **CS**     |
   | **4**     | **RST**    |
3. The ESP32 will be configured as an Access Point using the SSID and password defined in the code (e.g., `"ArdillasFamily"` / `"MilE#1403.$"`).
4. Upload the code to the ESP32.

### 2️⃣ **Connect and Use the Web Interface**
1. On your PC or mobile device, connect to the WiFi network created by the ESP32.
2. Open a web browser and go to the Access Point IP address, usually `http://192.168.4.1`.
3. Select a video via the input field on the page.
4. Click **"Start Capture"** to begin playback.  
   - The client captures and sends the first frame.
   - Once the ESP32 processes the received frame, it sends the `"requestFrame"` message via WebSocket.
   - Upon receiving the message, the client captures and sends the next frame, creating a synchronized cycle.
5. The image is decoded and displayed on the ST7789 screen.

---

## 🛠️ Troubleshooting

### ❌ **Distorted Colors**
✔️ Solution: Ensure that `tft.setSwapBytes(true);` is enabled on the ESP32.

### ❌ **Incorrectly Rotated Image**
✔️ Solution: The image is rotated 90° clockwise on the web client; check the rotation function in JavaScript if needed.

### ❌ **Incomplete Frames or Flickering**
✔️ Solution:
- Double buffering and end-marker verification (0xFF, 0xD9) have been implemented to ensure only complete frames are processed.
- The screen updates atomically to prevent flickering.

---

## 📜 License

This project is licensed under **MIT**. You are free to use and modify it.

Developed by Pablo Toledo


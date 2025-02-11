#include <WiFi.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <TFT_eSPI.h>           // Library to handle the display
#include <JPEGDecoder.h>        // Library to decode JPEG

// ----------------- Access Point Configuration -----------------
const char* ssid     = "ESP32-WIFI-video"; // AP Name
const char* password = "TRC12345678";      // AP Password

// ----------------- Create the HTTP and WebSocket Server -----------------
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ----------------- Display Configuration -----------------
TFT_eSPI tft = TFT_eSPI();  // Configuration is defined in User_Setup.h of TFT_eSPI

// ----------------- HTML/JS Page (Frontend) -----------------
// Served from flash memory (PROGMEM)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Frame Capture via WebSocket</title>
  <style>
    /* Hide the canvas; it can be shown for debugging */
    #canvasFrame { display: none; }
  </style>
</head>
<body>
  <h1>Select Video and Capture Frames</h1>
  <!-- Input to select the video -->
  <input type="file" id="videoInput" accept="video/*">
  <br><br>
  <!-- Video element; the video is expected to have 240×135 resolution -->
  <video id="videoPlayer" width="240" height="135" controls></video>
  <br><br>
  <!-- Canvas to capture and rotate the frame (dimensions: 135×240 to reflect 90° rotation) -->
  <canvas id="canvasFrame" width="135" height="240"></canvas>
  <br>
  <button id="startCapture">Start Capture</button>
  <button id="stopCapture">Stop Capture</button>


  <br>
  <div style="margin-top: 100px;">
    <h2 style="font-weight: 500;">Developed by <b>Pablo Toledo</b></h2>
    <div>
      <ul>
        <li><a href="https://github.com/pablotoledom/">My github</a></li>
        <li><a href="https://theretrocenter.com">The Retro Center website</a></li>
      </ul>
    </div>
  
    <div style="font-size: 10pt; font-family: Monospace; white-space: pre;">
    ░▒▓████████▓▒░▒▓███████▓▒░ ░▒▓██████▓▒░  
       ░▒▓█▓▒░   ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░  
       ░▒▓█▓▒░   ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░         
       ░▒▓█▓▒░   ░▒▓███████▓▒░░▒▓█▓▒░         
       ░▒▓█▓▒░   ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░         
       ░▒▓█▓▒░   ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░  
       ░▒▓█▓▒░   ░▒▓█▓▒░░▒▓█▓▒░░▒▓██████▓▒░  
    </div>
  </div>

  <script>
    const videoInput = document.getElementById('videoInput');
    const videoPlayer = document.getElementById('videoPlayer');
    const canvasFrame = document.getElementById('canvasFrame');
    const startCapture = document.getElementById('startCapture');
    const stopCapture = document.getElementById('stopCapture');
    
    // WebSocket connection on the /ws route of the same host
    const ws = new WebSocket('ws://' + location.hostname + '/ws');
    ws.binaryType = 'blob';
    ws.onopen = () => { console.log("WebSocket connected"); };
    ws.onclose = () => { console.log("WebSocket disconnected"); };
    ws.onerror = (e) => { console.error("WebSocket error:", e); };
    
    // Instead of sending at fixed intervals, we wait for the ESP32's request message
    ws.onmessage = (event) => {
      if (event.data === "requestFrame") {
        captureFrameAndSend();
      } else {
        console.log("Server message:", event.data);
      }
    };
    
    videoInput.addEventListener('change', (e) => {
      const file = e.target.files[0];
      if (file) {
        const url = URL.createObjectURL(file);
        videoPlayer.src = url;
      }
    });
    
    // Buttons to control playback (video starts and stops)
    startCapture.addEventListener('click', () => {
      videoPlayer.play();
      // Send the first frame to start the process
      captureFrameAndSend();
    });
    
    stopCapture.addEventListener('click', () => {
      videoPlayer.pause();
    });
    
    function captureFrameAndSend() {
      const ctx = canvasFrame.getContext('2d');
      ctx.clearRect(0, 0, canvasFrame.width, canvasFrame.height);
      ctx.save();
      // Rotate 90° clockwise:
      ctx.translate(canvasFrame.width, 0);
      ctx.rotate(Math.PI / 2);
      // Draw the video frame (originally 240x135) on the rotated canvas
      ctx.drawImage(videoPlayer, 0, 0, 240, 135);
      ctx.restore();
      
      // Convert canvas to Data URL in JPEG format with 25% quality
      const dataURL = canvasFrame.toDataURL('image/jpeg', 0.25);
      const blob = dataURItoBlob(dataURL);
      
      if (ws.readyState === WebSocket.OPEN) {
        ws.send(blob);
        console.log("Frame sent via WebSocket");
      } else {
        console.log("WebSocket is not open");
      }
    }
    
    function dataURItoBlob(dataURI) {
      const byteString = atob(dataURI.split(',')[1]);
      const mimeString = dataURI.split(',')[0].split(':')[1].split(';')[0];
      const ab = new ArrayBuffer(byteString.length);
      const ia = new Uint8Array(ab);
      for (let i = 0; i < byteString.length; i++) {
        ia[i] = byteString.charCodeAt(i);
      }
      return new Blob([ab], { type: mimeString });
    }
  </script>
</body>
</html>
)rawliteral";

// ----------------- Static Buffer to Reassemble the JPEG -----------------
#define MAX_IMAGE_SIZE 30000  // Adjust this size as needed
uint8_t imageBufferStatic[MAX_IMAGE_SIZE];
uint8_t imageBufferProcesamiento[MAX_IMAGE_SIZE];
size_t imageBufferLength = 0;
volatile bool imageReady = false;
volatile bool processing = false;

// ----------------- WebSocket Callback -----------------
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
               void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WebSocket client #%u connected\n", client->id());
  }
  else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
  }
  else if (type == WS_EVT_DATA) {
    AwsFrameInfo * info = (AwsFrameInfo*) arg;
    if (info->opcode == WS_BINARY) {
      if (processing) return;  // Avoid overwriting while processing an image
      if (info->index == 0) {
        imageBufferLength = 0; // Reset bufferr
      }
      if (imageBufferLength + len <= MAX_IMAGE_SIZE) {
        memcpy(imageBufferStatic + imageBufferLength, data, len);
        imageBufferLength += len;
      } else {
        Serial.println("Error: Image buffer exceeded");
        imageBufferLength = 0;
      }
      
      if (info->final) {
        // Verify that the JPEG is complete by checking the end marker (0xFF, 0xD9)
        if (imageBufferLength >= 2 &&
            imageBufferStatic[imageBufferLength-2] == 0xFF &&
            imageBufferStatic[imageBufferLength-1] == 0xD9) {
          memcpy(imageBufferProcesamiento, imageBufferStatic, imageBufferLength);
          imageReady = true;
        } else {
          Serial.println("Incomplete JPEG (end marker not found)");
        }
      }
    }
  }
}

// ----------------- Function to Process the Image in loop() -----------------
void processImage() {
  if (imageReady && !processing) {
    processing = true;
    int decodeResult = JpegDec.decodeArray(imageBufferProcesamiento, imageBufferLength);
    if (decodeResult == 1) {
      Serial.println("Decodificación exitosa, mostrando imagen");
      // If the image covers the entire screen, clearing is not necessary before rendering.
      // tft.fillScreen(TFT_BLACK);
      uint16_t mcu_w = JpegDec.MCUWidth;
      uint16_t mcu_h = JpegDec.MCUHeight;
      uint16_t *pImg;
      while (JpegDec.read()) {
        int mcu_x = JpegDec.MCUx * mcu_w;
        int mcu_y = JpegDec.MCUy * mcu_h;
        uint16_t block_w = mcu_w;
        uint16_t block_h = mcu_h;
        if (mcu_x + block_w > JpegDec.width) {
          block_w = JpegDec.width - mcu_x;
        }
        if (mcu_y + block_h > JpegDec.height) {
          block_h = JpegDec.height - mcu_y;
        }
        pImg = JpegDec.pImage;
        tft.pushImage(mcu_x, mcu_y, block_w, block_h, pImg);
      }
    } else {
      Serial.printf("Error decoding image, code: %d\n", decodeResult);
    }
    imageReady = false;
    processing = false;
    
    // Request the client to send the next frame
    ws.textAll("requestFrame");
  }
}

void setup() {
  Serial.begin(115200);
  
  // Start the ESP32 as an Access Point
  Serial.println("Starting Access Point...");
  if (WiFi.softAP(ssid, password)) {
    Serial.println("Access Point started successfully.");
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Error starting Access Point.");
  }
  
  // Initialize SPIFFS (optional)
  if (!SPIFFS.begin(true)) {
    Serial.println("Error montando SPIFFS");
    return;
  }
  
  // Initialize the display
  tft.init();
  tft.setRotation(0);      // 90° rotation based on your hardware
  tft.setSwapBytes(true);  // Byte swap for color format
  tft.fillScreen(TFT_BLACK);
  
  // Configure the WebSocket and assign the callback
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  
  // Serve the HTML page at path "/"
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  
  // Start HTTP server
  server.begin();
  Serial.println("Servidor HTTP y WebSocket iniciados");
}

void loop() {
  // Process received image if ready
  processImage();
}

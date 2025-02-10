#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <JPEGDecoder.h>
#include <TFT_eSPI.h>  // Librería TFT_eSPI

// ================== Configuración de WiFi ==================
const char* ssid = "ArdillasFamily";
const char* password = "MilE#1403.$";

// ================== Creación del objeto TFT ==================
TFT_eSPI tft = TFT_eSPI();  // La configuración (pines, resolución, etc.) se define en User_Setup.h

// ================== Configuración del Servidor Web ==================
WebServer server(80);

// HTML para subir la imagen JPEG
const char* uploadHTML = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Captura de Fotogramas de Video</title>
  <style>
    /* Puedes ocultar el canvas o mostrarlo para depuración */
    #canvasFrame {
      display: none;
    }
  </style>
</head>
<body>
  <h1>Seleccionar Video y Capturar Fotogramas</h1>
  <!-- Input para seleccionar video -->
  <input type="file" id="videoInput" accept="video/*">
  <br><br>
  <!-- Elemento de video: se espera que el video tenga resolución 240x135 -->
  <video id="videoPlayer" width="240" height="135" controls></video>
  <br><br>
  <!-- Canvas para capturar y rotar el fotograma.
       Se define con dimensiones 135x240 ya que se rota 90° -->
  <canvas id="canvasFrame" width="135" height="240"></canvas>
  <br>
  <button id="startCapture">Iniciar Captura</button>
  <button id="stopCapture">Detener Captura</button>

  <script>
    // Referencias a los elementos
    const videoInput = document.getElementById('videoInput');
    const videoPlayer = document.getElementById('videoPlayer');
    const canvasFrame = document.getElementById('canvasFrame');
    const startCapture = document.getElementById('startCapture');
    const stopCapture = document.getElementById('stopCapture');
    let captureInterval;

    // Cuando se selecciona un video, se carga en el elemento video
    videoInput.addEventListener('change', function(e) {
      const file = e.target.files[0];
      if (file) {
        const url = URL.createObjectURL(file);
        videoPlayer.src = url;
      }
    });

    // Inicia la captura de fotogramas cada 1 segundo
    startCapture.addEventListener('click', function() {
      videoPlayer.play();
      captureInterval = setInterval(captureFrameAndSend, 4000); // Cada 1 segundo
    });

    // Detiene la captura
    stopCapture.addEventListener('click', function() {
      clearInterval(captureInterval);
    });

    // Función que captura un fotograma, lo rota 90° y lo envía al servidor
    function captureFrameAndSend() {
      const ctx = canvasFrame.getContext('2d');
      // Limpiar el canvas
      ctx.clearRect(0, 0, canvasFrame.width, canvasFrame.height);
      
      // Guardar el estado actual
      ctx.save();
      // Para rotar 90° (horario) se hace:
      // 1. Se traslada el contexto al ancho del canvas (nueva base en la esquina superior derecha)
      // 2. Se rota 90° (Math.PI/2 radianes)
      ctx.translate(canvasFrame.width, 0);
      ctx.rotate(Math.PI / 2);
      
      // Dibujar el fotograma del video en el canvas
      // El video tiene 240x135 y, al rotarlo, se adapta al canvas (135x240)
      ctx.drawImage(videoPlayer, 0, 0, 240, 135);
      
      // Restaurar el estado original del contexto
      ctx.restore();
      
      // Convertir el contenido del canvas a un data URL en formato JPEG
      const dataURL = canvasFrame.toDataURL('image/jpeg');
      
      // Convertir el data URL a Blob para poder enviarlo en un FormData
      const blob = dataURItoBlob(dataURL);
      
      // Preparar FormData con el fotograma
      const formData = new FormData();
      formData.append("uploadfile", blob, "frame.jpg");
      
      // Enviar el fotograma al servidor (endpoint /uploadFrame)
      fetch('/upload', {
        method: 'POST',
        body: formData
      })
      .then(response => {
        console.log('Fotograma enviado correctamente.');
      })
      .catch(error => {
        console.error('Error al enviar el fotograma:', error);
      });
    }

    // Función para convertir un dataURL a Blob
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

// ================== Función para dibujar el JPEG usando JPEGDecoder y TFT_eSPI ==================
void drawJPEG(const char* filename, int xpos, int ypos) {
  Serial.print("Decodificando ");
  Serial.println(filename);

  // Abrir el archivo JPEG desde SPIFFS
  File jpegFile = SPIFFS.open(filename, "r");
  if (!jpegFile) {
    Serial.println("Error al abrir el archivo JPEG");
    return;
  }

  size_t fileSize = jpegFile.size();
  Serial.print("Tamaño del archivo: ");
  Serial.println(fileSize);

  // Asignar memoria para el buffer
  uint8_t *jpegBuffer = (uint8_t*) malloc(fileSize);
  if (!jpegBuffer) {
    Serial.println("No se pudo asignar memoria para la imagen");
    jpegFile.close();
    return;
  }

  // Leer el contenido del archivo en el buffer
  jpegFile.read(jpegBuffer, fileSize);
  jpegFile.close();

  // Imprimir los primeros 16 bytes del archivo para verificar la firma JPEG
  Serial.print("Primeros 16 bytes del archivo: ");
  for (uint8_t i = 0; i < 16 && i < fileSize; i++) {
    Serial.printf("%02X ", jpegBuffer[i]);
  }
  Serial.println();

  // Decodificar la imagen a partir del buffer (se pasan 2 parámetros)
  int decodeResult = JpegDec.decodeArray(jpegBuffer, fileSize);
  if (decodeResult != 1) {
    Serial.print("Error al decodificar la imagen. Código de error: ");
    Serial.println(decodeResult);
    Serial.println("Verifica que la imagen sea JPEG Baseline y use un subsampling soportado.");
    free(jpegBuffer);
    return;
  }

  free(jpegBuffer);

  // Imprimir dimensiones de la imagen decodificada
  Serial.print("Dimensiones de la imagen decodificada: ");
  Serial.print(JpegDec.width);
  Serial.print(" x ");
  Serial.println(JpegDec.height);

  // La imagen se decodifica en bloques (MCU)
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint16_t *pImg;

  Serial.println("Iniciando dibujo de bloques MCU...");
  // Recorremos todos los bloques (MCUs) y los dibujamos en la pantalla
  while (JpegDec.read()) {
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // Ajustar dimensiones si el bloque se extiende fuera de la imagen decodificada
    uint16_t block_w = mcu_w;
    uint16_t block_h = mcu_h;
    if (mcu_x + block_w > xpos + JpegDec.width) {
      block_w = (xpos + JpegDec.width) - mcu_x;
    }
    if (mcu_y + block_h > ypos + JpegDec.height) {
      block_h = (ypos + JpegDec.height) - mcu_y;
    }

    pImg = JpegDec.pImage;

    // Log para cada bloque MCU
    Serial.printf("Dibujando MCU en (%d, %d) de tamaño %dx%d\n", mcu_x, mcu_y, block_w, block_h);

    // Dibujar el bloque usando pushImage de TFT_eSPI
    tft.pushImage(mcu_x, mcu_y, block_w, block_h, pImg);
  }

  Serial.println("Imagen mostrada correctamente.");
}

// Función para limpiar la pantalla y mostrar la imagen
void displayJPG(const char* path) {
  // Para visualizar la diferencia, llenamos la pantalla de un color de fondo (azul)
  tft.fillScreen(TFT_BLUE);
  delay(100);  // Breve retraso para notar el cambio
  drawJPEG(path, 0, 0);
}

// ================== Handlers del Servidor Web ==================
void handleRoot() {
  server.send(200, "text/html", uploadHTML);
}

void handleUpload() {
  HTTPUpload &upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Inicio de carga: %s\n", upload.filename.c_str());
    String path = "/" + upload.filename;
    if (SPIFFS.exists(path)) {
      SPIFFS.remove(path);
    }
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    String path = "/" + upload.filename;
    File file = SPIFFS.open(path, FILE_APPEND);
    if (file) {
      file.write(upload.buf, upload.currentSize);
      file.close();
    }
    else {
      Serial.println("Error abriendo archivo para escritura");
    }
  }
  else if (upload.status == UPLOAD_FILE_END) {
    Serial.printf("Carga finalizada: %s, Tamaño: %u bytes\n", upload.filename.c_str(), upload.totalSize);
    server.send(200, "text/html", "<html><body><h1>Imagen subida correctamente</h1><a href='/'>Volver</a></body></html>");
    String path = "/" + upload.filename;
    displayJPG(path.c_str());
  }
}

// ================== Función setup() ==================
void setup() {
  Serial.begin(115200);

  if (!SPIFFS.begin(true)) {
    Serial.println("Error al montar SPIFFS");
    return;
  }

  // Inicializar la pantalla TFT_eSPI; la configuración (pines, resolución, etc.) se define en User_Setup.h
  tft.init();

  // Configurar la rotación para girar la imagen 90° (esto intercambia las dimensiones de la pantalla)
  tft.setRotation(0);
  
  // Habilitar el intercambio de bytes, si es necesario para el formato de color
  tft.setSwapBytes(true);
  
  // Llenar la pantalla de negro
  tft.fillScreen(TFT_BLACK);

  // Conectar a WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado. IP: ");
  Serial.println(WiFi.localIP());

  // Configurar rutas del servidor web
  server.on("/", HTTP_GET, handleRoot);
  server.on("/upload", HTTP_POST, [](){
    server.send(200);
  }, handleUpload);

  server.begin();
  Serial.println("Servidor HTTP iniciado");
}

// ================== Función loop() ==================
void loop() {
  server.handleClient();
}

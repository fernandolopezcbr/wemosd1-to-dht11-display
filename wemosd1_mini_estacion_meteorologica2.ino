#include <ESP8266WiFi.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define DHTPIN D4 // Pin donde está conectado el DHT11
#define DHTTYPE DHT11 // Tipo de sensor

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 20, 4); // Dirección I2C del LCD (verifica si es 0x27)

const char* ssid = "";
const char* password = "";
WiFiServer server(80);

// NTP setup
WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", -21600, 60000); // Offset de -21600 segundos para UTC-6 (Guatemala)

// Arreglo para almacenar los datos
const int maxDatos = 100;
float temperaturas[maxDatos];
float humedades[maxDatos];
String fechas[maxDatos]; // Array para almacenar las fechas
int contador = 0;

void setup() {
  Serial.begin(115200);
  dht.begin();
  lcd.init();
  lcd.backlight();

  // Conectar a Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a WiFi...");
  }
  
  server.begin();
  Serial.println("Servidor iniciado.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Iniciar el cliente NTP
  timeClient.begin();
}

void loop() {
  WiFiClient client = server.available();
  
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // Comprobar si la lectura es válida
    if (isnan(h) || isnan(t)) {
      h = 0;
      t = 0;
    }

    // Almacenar datos si hay espacio
    if (contador < maxDatos) {
      temperaturas[contador] = t;
      humedades[contador] = h;
      timeClient.update(); // Actualizar tiempo
      unsigned long epochTime = timeClient.getEpochTime();
      fechas[contador] = formatTime(epochTime); // Obtener la fecha y hora en formato local
      contador++;
    }

    // Mostrar datos en el LCD
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Temp: " + String(t) + " C");
    lcd.setCursor(2, 2);
    lcd.print("Humed: " + String(h) + " %");

    // Manejar la solicitud de datos JSON
    if (request.indexOf("/datos") != -1) {
      String jsonResponse = "{\"temp\": " + String(t) + ", \"hum\": " + String(h) + "}";
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.println(jsonResponse);
      return; // Terminar la función para evitar enviar el HTML
    }

    // Manejar la solicitud para exportar CSV
    if (request.indexOf("/exportar") != -1) {
      String csvData = "Fecha,Temperatura,Humedad\n";
      for (int i = 0; i < contador; i++) {
        csvData += fechas[i] + "," + String(temperaturas[i]) + "," + String(humedades[i]) + "\n";
      }
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/csv");
      client.println("Content-Disposition: attachment; filename=\"datos.csv\"");
      client.println("Connection: close");
      client.println();
      client.println(csvData);
      return; // Terminar la función
    }

    // Generar respuesta HTML
    String response = "<!DOCTYPE HTML>\n<html>\n<head>\n<title>Monitor DHT11</title>\n";
    response += "<meta name='viewport' content='width=device-width, initial-scale=1'>\n"; // Para móviles
    response += "<style>body { font-family: Arial; text-align: center; margin-top: 50px; }";
    response += "h1 { color: #333; } .data { font-size: 24px; margin: 20px; }";
    response += "img { width: 50px; }";
    response += "table { margin: 20px auto; border-collapse: collapse; }";
    response += "td, th { border: 1px solid #333; padding: 8px; }</style>\n";

    response += "<script>\n";
    response += "function actualizarDatos() {\n";
    response += "  var xhttp = new XMLHttpRequest();\n";
    response += "  xhttp.onreadystatechange = function() {\n";
    response += "    if (this.readyState == 4 && this.status == 200) {\n";
    response += "      var data = JSON.parse(this.responseText);\n";
    response += "      document.getElementById('temperatura').innerHTML = 'Temperatura: ' + data.temp + ' °C';\n";
    response += "      document.getElementById('humedad').innerHTML = 'Humedad: ' + data.hum + ' %';\n";
    response += "    }\n";
    response += "  };\n";
    response += "  xhttp.open('GET', '/datos', true);\n";
    response += "  xhttp.send();\n";
    response += "}\n";
    response += "setInterval(actualizarDatos, 5000);\n"; // Actualizar cada 5 segundos
    response += "</script>\n";
    response += "</head>\n<body>\n<h1>Datos del DHT11</h1>\n";
    response += "<img src='https://cdn-icons-png.flaticon.com/512/3653/3653294.png' alt='Termómetro'>";
    response += "<div class='data' id='temperatura'>Temperatura: " + String(t) + " °C</div>\n";
    response += "<img src='https://cdn-icons-png.flaticon.com/512/4148/4148460.png' alt='Humedad'>";
    response += "<div class='data' id='humedad'>Humedad: " + String(h) + " %</div>\n";
    
    // Tabla de datos
    response += "<details><summary>Ver Tabla de datos</summary> <table><tr><th>Fecha y Hora</th><th>Temperatura (°C)</th><th>Humedad (%)</th></tr>";
    for (int i = 0; i < contador; i++) {
      response += "<tr><td>" + fechas[i] + "</td><td>" + String(temperaturas[i]) + "</td><td>" + String(humedades[i]) + "</td></tr>";
    }
    response += "</table></details>";

    response += "<button onclick='window.location.reload();'>Actualizar</button>\n";
    response += "<a href='/exportar'><button>Exportar a Excel</button></a>\n"; // Botón de exportación
    response += "</body>\n</html>";

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println(response);
    
    delay(1);
    client.stop();
  }
  delay(1000);
}

// Función para formatear el tiempo
String formatTime(unsigned long epochTime) {
  // Convertir el tiempo a una cadena legible
  time_t rawTime = epochTime;
  struct tm *timeInfo;
  timeInfo = localtime(&rawTime);
  
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeInfo);
  return String(buffer);
}

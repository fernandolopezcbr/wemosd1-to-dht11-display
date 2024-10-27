#include <ESP8266WiFi.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN D4 // Pin donde está conectado el DHT11
#define DHTTYPE DHT11 // Tipo de sensor

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 20, 4); // Dirección I2C del LCD (verifica si es 0x27)

const char* ssid = "CLARO_Z5vwXA";
const char* password = "FDDFF60637";
WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  dht.begin();
  lcd.init();
  lcd.begin(20,4);
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

    // Mostrar datos en el LCD
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Temp: " + String(t) + " C");
    lcd.setCursor(2, 2);
    lcd.print("Humed: " + String(h) + " %");


    // Generar respuesta HTML
    String response = "<!DOCTYPE HTML>\n<html>\n<head>\n<title>Monitor DHT11</title>\n";
    response += "<meta http-equiv='refresh' content='10'>\n"; // Refresca cada 10 segundos
    response += "<style>body { font-family: Arial; text-align: center; margin-top: 50px; }";
    response += "h1 { color: #333; } .data { font-size: 24px; margin: 20px; }";
    response += "img { width: 50px; }</style>\n";
    response += "</head>\n<body>\n<h1>Datos del DHT11</h1>\n";
    response += "<img src='https://image.shutterstock.com/image-vector/thermometer-icon-vector-260nw-1661950554.jpg' alt='Termómetro'>";
    response += "<div class='data'>Temperatura: " + String(t) + " °C</div>\n";
    response += "<img src='https://image.shutterstock.com/image-vector/humidity-icon-vector-260nw-1661950570.jpg' alt='Humedad'>";
    response += "<div class='data'>Humedad: " + String(h) + " %</div>\n";
    response += "<button onclick='window.location.reload();'>Actualizar</button>\n";
    response += "</body>\n</html>";

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println(response);
    
    delay(1);
    client.stop();
  }
}

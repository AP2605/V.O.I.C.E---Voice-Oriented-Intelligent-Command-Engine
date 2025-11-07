#include <WiFi.h>
#include <WebSocketsServer.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* ssid = "realme 9 Pro+";
const char* password = "skt@hotspot";

WebSocketsServer webSocket = WebSocketsServer(81);

const int LIGHT0_PIN = 12;
const int FAN_PIN    = 14;
const int RELAY0_PIN = 27;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String chatbotServer = "http://10.39.52.40:3000/chat"; 

void setup() {
  Serial.begin(115200);

  pinMode(LIGHT0_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(RELAY0_PIN, OUTPUT);

  digitalWrite(LIGHT0_PIN, LOW);
  digitalWrite(FAN_PIN, LOW);
  digitalWrite(RELAY0_PIN, LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  showMessage("Starting...");
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  showMessage("Connecting WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  showMessage("WiFi OK\nIP: " + WiFi.localIP().toString());

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started");
}

void loop() {
  webSocket.loop();
}

void showMessage(const String &msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  int y = 20;

  display.setCursor(0, y);
  if (msg.length() > 15) {
    int splitIndex = msg.lastIndexOf(' ', msg.length() / 2);
    if (splitIndex == -1) splitIndex = msg.length() / 2;

    String line1 = msg.substring(0, splitIndex);
    String line2 = msg.substring(splitIndex + 1);

    display.setCursor(0, 20);
    display.println(line1);
    display.setCursor(0, 35);
    display.println(line2);
  } else {
    display.setCursor(0, 25);
    display.println(msg);
  }
  display.display();
}

void executeCode(const String &code) {
  String msg = "";
  String cmd = code;
  cmd.trim();
  cmd.toLowerCase();

  if (cmd == "41") { // Light and Fan ON
    digitalWrite(LIGHT0_PIN, HIGH);
    digitalWrite(FAN_PIN, HIGH);
    msg = "Light and Fan ON";
  }
  else if (cmd == "40") { // Light and Fan OFF
    digitalWrite(LIGHT0_PIN, LOW);
    digitalWrite(FAN_PIN, LOW);
    msg = "Light and Fan OFF";
  }
  else if (cmd == "50") { // All ON
    digitalWrite(LIGHT0_PIN, HIGH);
    digitalWrite(RELAY0_PIN, HIGH);
    digitalWrite(FAN_PIN, HIGH);
    msg = "ALL ON";
  }
  else if (cmd == "51") { // All OFF
    digitalWrite(LIGHT0_PIN, LOW);
    digitalWrite(RELAY0_PIN, LOW);
    digitalWrite(FAN_PIN, LOW);
    msg = "ALL OFF";
  }
  else if (cmd == "42") {
    digitalWrite(LIGHT0_PIN, HIGH);
    digitalWrite(FAN_PIN, LOW);
    msg = "Light ON Fan OFF";
  }
  else if (cmd == "43") {
    digitalWrite(LIGHT0_PIN, LOW);
    digitalWrite(FAN_PIN, HIGH);
    msg = "Light OFF Fan ON";
  }
  else if (cmd == "01") {
    digitalWrite(LIGHT0_PIN, HIGH);
    msg = "Light ON";
  }
  else if (cmd == "00") {
    digitalWrite(LIGHT0_PIN, LOW);
    msg = "Light OFF";
  }
  else if (cmd == "21") {
    digitalWrite(FAN_PIN, HIGH);
    msg = "Fan ON";
  }
  else if (cmd == "20") {
    digitalWrite(FAN_PIN, LOW);
    msg = "Fan OFF";
  }
  else if (cmd == "31") {
    digitalWrite(RELAY0_PIN, HIGH);
    msg = "Relay ON";
  }
  else if (cmd == "30") {
    digitalWrite(RELAY0_PIN, LOW);
    msg = "Relay OFF";
  }
  else {
    msg = "Unknown cmd";
    showMessage("Querying bot...");
    String reply = queryChatbot(code);

    int dotIndex = reply.indexOf('.');
    if (dotIndex != -1) {
      reply = reply.substring(0, dotIndex + 1);
    }

    showMessage(reply);
    Serial.println("Chatbot reply (trimmed): " + reply);
    return;
  }

  Serial.println(msg);
  showMessage(msg);
}

String queryChatbot(const String &query) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return "No WiFi!";
  }

  HTTPClient http;
  http.begin(chatbotServer);
  http.addHeader("Content-Type", "application/json");

  String body = "{\"prompt\":\"" + query + "\"}";
  int httpResponseCode = http.POST(body);

  String response = "";
  if (httpResponseCode == 200) {
    response = http.getString();

    int start = response.indexOf("\"reply\":\"");
    int end = response.indexOf("\"", start + 9);
    if (start != -1 && end != -1)
      response = response.substring(start + 9, end);
  } else {
    response = "Chatbot err: " + String(httpResponseCode);
  }

  http.end();
  return response;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  if (type == WStype_TEXT) {
    String data = String((char*)payload).substring(0, length);
    data.trim();
    Serial.println("Received: " + data);

    if (data.startsWith("CHAT:")) {
      String message = data.substring(5);
      Serial.println("Chat message: " + message);

      int dotIndex = message.indexOf('.');
      if (dotIndex != -1) {
        message = message.substring(0, dotIndex + 1);
      }

      showMessage(message);
      webSocket.sendTXT(num, "Displayed Chat Message");
      return;
    }
    
    int start = 0;
    while (true) {
      int idx = data.indexOf(',', start);
      String cmd;
      if (idx == -1) {
        cmd = data.substring(start);
        executeCode(cmd);
        break;
      } else {
        cmd = data.substring(start, idx);
        executeCode(cmd);
        start = idx + 1;
        delay(100);
      }
    }
    webSocket.sendTXT(num, "Executed: " + data);
  }
}
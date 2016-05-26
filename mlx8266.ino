/* ESP8266 wifi non-contact thermomter
   creates an AP called thermometer,
   serves a web-app to display temperature info
   provides captive portal


    #TODO - clean this POS up
    #TODO - P2P mode with power saving    

*/

#include <Adafruit_MLX90614.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <FS.h>


/* DHTServer - ESP8266 Webserver with a DHT sensor as an input

  Based on ESP8266Webserver, DHTexample, and BlinkWithoutDelay (thank you)

  Version 1.0 5/3/2014 Version 1.0 Mike Barela for Adafruit Industries
*/

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
const char WiFiAPPSK[] = "thermometer";
MDNSResponder mdns;
const byte DNS_PORT = 53;
DNSServer dnsServer;
ESP8266WebServer server(80);

float temp_f; // Values read from sensor
float temp_fa;
float temp_c;
float temp_ca;
String webString = ""; // String to display
int level;//battery level
// Generally, you should use "unsigned long" for variables that hold time
unsigned long previousTempMillis = 0; // will store last temp was read
unsigned long previousBattMillis = 0; // will store last batt voltage was read
const long interval = 1000; // interval at which to read sensor
const long battInterval = 10000; //interval at which to read battery voltage
unsigned long currentMillis;
bool handleFileRead(String path) {
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void getMainPage() {
  handleFileRead("/therm.html");
}

void handleFileList() {
if(!server.hasArg("dir")) {server.send(500, "text/plain", "BAD ARGS"); return;}
String path = server.arg("dir");
Serial.println("handleFileList: " + path);
Dir dir = SPIFFS.openDir(path);
path = String();
String output = "[";
while(dir.next()){
File entry = dir.openFile("r");
if (output != "[") output += ',';
bool isDir = false;
output += "{\"type\":\"";
output += (isDir)?"dir":"file";
output += "\",\"name\":\"";
output += String(entry.name()).substring(1);
output += "\"}";
entry.close();
}
output += "]";
server.send(200, "text/plain", output);
}

String getContentType(String filename) {
  if (server.hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".ogg")) return "audio/ogg";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

void setup(void) {
  // You can open the Arduino IDE Serial Monitor window to see what the code is doing
  //Serial.begin(115200); // Serial connection from ESP-01 via 3.3v console cable
  pinMode(LED_BUILTIN, OUTPUT);  
  digitalWrite(LED_BUILTIN, HIGH);
  mlx.begin(); // initialize temperature sensor
  
  // Connect to WiFi network
  setupWiFi();
  // Set up FS
  SPIFFS.begin();
  Serial.print("\n\r \n\rWorking to connect");

  Serial.println("");
  Serial.println("MLX Weather Reading Server");

  Serial.println(WiFi.softAPIP());
  
  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
 /* if (!mdns.begin("thermometer", WiFi.softAPIP())) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }*/

  server.on("/", getMainPage);
  server.on("/temp.json", []() { // if you add this subdirectory to your webserver call, you get text below :)
    
    gettemperature(); // read sensor
    webString = "{ \"tAmbF\": " + String((int) temp_fa) + "," + "\"tObjF\": " + String((int) temp_f) + "," + "\"tObjC\": " + String((int) temp_c) + "," + "\"tAmbC\": " + String((int) temp_ca) + " }"; // Arduino has a hard time with float to string
    server.send(200, "application/json", webString); // send to someones browser when asked
  });
  server.on("/batt.json", []() { // if you add this subdirectory to your webserver call, you get text below :)
    
    getBatteryLevel(); // read sensor
    webString = "{ \"batt\": " + String((int) level) + " }"; // Arduino has a hard time with float to string
    server.send(200, "application/json", webString); // send to someones browser when asked
  });
  server.on("/list", HTTP_GET, handleFileList);
  server.onNotFound([]() {
    if (!handleFileRead(server.uri()))
      handleFileRead("/therm.html");
  });
  server.begin();
  Serial.println("HTTP server started");
}
void loop(void) {

  dnsServer.processNextRequest();
  server.handleClient();
}

void gettemperature() {
  // Wait at least 1 second seconds between measurements.
  // if the difference between the current time and last time you read
  // the sensor is bigger than the interval you set, read the sensor
  // Works better than delay for things happening elsewhere also
  currentMillis = millis();
  if (currentMillis - previousTempMillis >= interval) {
    digitalWrite(LED_BUILTIN, LOW);
    // save the last time you read the sensor
    previousTempMillis = currentMillis;

    // Reading temperature for humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 1 seconds 'old' (it's a very slow sensor)
    
    temp_c = mlx.readObjectTempC(); 
    temp_ca = mlx.readAmbientTempC(); 
    temp_f = temp_c * 9/5 + 32; 
    temp_fa = temp_ca * 9/5 + 32 ; 
    // Check if any reads failed and exit early (to try again).
    if (isnan(temp_f)) {
      Serial.println("Failed to read from MLX sensor!");
    }
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

int getBatteryLevel() {
  currentMillis = millis();
  if (currentMillis - previousBattMillis >= battInterval) {
    level = analogRead(A0);
   
    // convert battery level to percent
    level = map(level, 580, 774, 0, 100);
    previousBattMillis = currentMillis;
  }
  return level;
}
void setupWiFi() {
  WiFi.mode(WIFI_AP);

  char AP_NameChar[11 + 1] = "thermometer";

  WiFi.softAP(AP_NameChar, NULL);
  //192.168.244.1 , 192.168.244.1 , 255.255.255.0
  WiFi.softAPConfig(0x01F4A8C0, 0x01F4A8C0, 0x00FFFFFF);
}

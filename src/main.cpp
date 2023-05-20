
#include "Arduino.h"
#include "DHTesp.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>

bool alarmActivated = false;
const int soundPin = 4;
const int ledPin = 2;
const int sensorPin = 14;
const float alarmHumidity = 55.0;
float temp = 0;
float humidity = 0;
DHTesp dht;
WiFiServer server(80);

void setup()
{
  // initialize LED digital pin as an output.
  pinMode(soundPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(sensorPin, INPUT);
  Serial.begin(9600);
  digitalWrite(ledPin, HIGH);
  dht.setup(sensorPin, DHTesp::DHT11);

  WiFiManager wifiManager;
  wifiManager.autoConnect("HumidityAlarm");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  delay(100);

  if (MDNS.begin("alarm"))
  { // Start the mDNS responder for esp8266.local
    Serial.println("mDNS responder started");
  }
  else
  {
    Serial.println("Error setting up MDNS responder!");
  }

  // or use this for auto generated name ESP + ChipID
  server.begin();

  digitalWrite(ledPin, HIGH);
}

void alarm()
{
  if (alarmActivated)
  {
    // toggle alarm on and off
    if (millis() / 1000 % 2 == 0)
    {
      digitalWrite(4, HIGH);
      delay(3);
      digitalWrite(soundPin, LOW);
      delay(3);
    }
    else
    {
      digitalWrite(soundPin, LOW);
    }

    if ((millis() / 250) % 2 == 0)
    {
      digitalWrite(ledPin, HIGH);
    }
    else
    {
      digitalWrite(ledPin, LOW);
    }
  }
  else
  {
    digitalWrite(soundPin, LOW);
    digitalWrite(ledPin, LOW);
  }
}

unsigned long last_sample = 0;
void sample_humidity()
{
  if (2100 < (millis() - last_sample))
  {
    humidity = dht.getHumidity();
    temp = dht.getTemperature();
    // Serial.print(dht.getStatusString());
    Serial.print("humidity: ");
    Serial.println(humidity, 1);
    Serial.print("temp: ");
    Serial.println(temp, 1);
    last_sample = millis();
  }
}

// Variable to store the HTTP request
char header[2048];
// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void generatePage(WiFiClient client)
{
  if (strstr(header, "GET /temp") != nullptr)
  {
    client.println(temp);
  }
  else if (strstr(header, "GET /humidity") != nullptr)
  {
    client.println(humidity);
  }
  else
  {
    bool autoRefresh = strstr(header, "GET /auto") != nullptr;
    // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
    // and a content-type so the client knows what's coming, then a blank line:
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println("Connection: close");
    client.println();

    // Display the HTML web page
    client.println("<!DOCTYPE html><html>");
    client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
    if (autoRefresh)
    {
      client.println("<meta http-equiv=\"refresh\" content=\"2\">");
    }
    client.println("<link rel=\"icon\" href=\"data:,\">");
    // CSS to style the on/off buttons
    // Feel free to change the background-color and font-size attributes to fit your preferences
    client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
    client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;</style></head>");
    if (autoRefresh){
      client.println("<p><a href=\"home\"><button class=\"button\">disable auto refresh</button></a></p>");
    }else{
      client.println("<p><a href=\"auto\"><button class=\"button\">enable auto refresh</button></a></p>");
    }

    // Display current state, and ON/OFF buttons for GPIO 5
    client.print("<h1>humidity: ");
    client.print(humidity);
    client.println("%</h1>");
    client.print("<h1>temp: ");
    client.print(temp);
    client.println(" degrees C</h1>");

    client.println("</body></html>");
  }
  // The HTTP response ends with another blank line
  client.println();
}

void handleNetwork()
{
  WiFiClient client = server.available(); // Listen for incoming clients

  if (client)
  {                                // If a new client connects,
    Serial.println("New Client."); // print a message out in the serial port
    char currentLine[1024] = "";    // make a C string to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    size_t headerLen = 0;

    while (client.connected() && currentTime - previousTime <= timeoutTime)
    { // loop while the client's connected
      currentTime = millis();
      if (client.available())
      {                         // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        Serial.write(c);        // print it out the serial monitor

        if (headerLen < sizeof(header) - 2)
        {
          header[headerLen++] = c;
          header[headerLen] = '\0';
        }else{
          Serial.println("header buffer exceeded");
          client.write("header buffer limit exceeded");
          break;
        }

        if (c == '\n')
        { // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (strlen(currentLine) == 0)
          {
            generatePage(client);
            // Break out of the while loop
            break;
          }
          else
          { // if you got a newline, then clear currentLine
            currentLine[0] = '\0';
          }
        }
        else if (c != '\r')
        {                   // if you got anything else but a carriage return character,
          if (strlen(currentLine) < sizeof(currentLine) - 2)
          {
            strncat(currentLine, &c, 1); // add it to the end of the currentLine
          }
        }
      }
    }
    // Clear the header variable
    header[0] = '\0';
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

void loop()
{
  MDNS.update();
  handleNetwork();
  alarmActivated = humidity > alarmHumidity;
  sample_humidity();
  alarm();
}

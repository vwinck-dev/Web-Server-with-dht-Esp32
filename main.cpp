#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>

const char *ssid = "SSID_WIFI";
const char *password = "WIFI_PASSWORD";

// CHANGE THE PIN TO THE ONE YOU'LL USE
#define DHTPIN 25 // Pin connected to the DHT sensor

// YOU CAN CHANGE TO USE DHT 11
#define DHTTYPE DHT22 // Type of DHT sensor being used (DHT22)
DHT dht(DHTPIN, DHTTYPE); // Initialize the DHT sensor

// THE PIN WHERE THE LED WILL BE
#define LEDPIN 2 // Pin connected to the LED

// THE PORT WHERE THE SERVER WILL BE
AsyncWebServer server(80); // Initialize the web server on port 80

// Global variables to store temperature and humidity readings
float global_temp = 0; // Stores the latest temperature reading
float global_hum = 0;  // Stores the latest humidity reading

// Variables for LED control without delay
unsigned long previousMillis = 0; // Tracks the last time the LED state was toggled
bool ledState = LOW;              // Current state of the LED (LOW = off, HIGH = on)
int delayTime = 1000;             // Delay time for LED blinking, adjusted based on temperature

// Variables for DHT sensor reading control without delay
unsigned long previousDHTMillis = 0; // Tracks the last time the DHT sensor was read
const unsigned long dhtInterval = 2000; // Interval between DHT sensor readings (2 seconds)

void setup()
{
    Serial.begin(115200); // Start serial communication at 115200 baud rate
    dht.begin();          // Initialize the DHT sensor
    pinMode(LEDPIN, OUTPUT); // Set the LED pin as an output

    // Connect to Wi-Fi
    WiFi.begin(ssid, password); // Start Wi-Fi connection with provided credentials
    unsigned long startAttemptTime = millis(); // Record the start time of the connection attempt
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
        delay(500); // Wait 500ms between connection attempts
        Serial.println("Trying to connect to Wi-Fi...");
    }

    // Check if Wi-Fi connection was successful
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("Wi-Fi connected! IP: ");
        Serial.println(WiFi.localIP()); // Print the device's IP address
    } else {
        Serial.println("Failed to connect to Wi-Fi, continuing without connection...");
    }

    delay(1000); // Short delay before starting the server

    // Define the root endpoint ("/") to serve the HTML page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        // Send an HTML page with JavaScript to fetch and display temperature and humidity data
        request->send_P(200, "text/html", R"rawliteral(
            <!DOCTYPE html>
            <html lang="en">
            <head>
                <meta charset="UTF-8">
                <meta name="viewport" content="width=device-width, initial-scale=1.0">
                <title>DHT22 Monitoring</title>
                <script>
                    function updateData() {
                        fetch('/data') // Fetch data from the "/data" endpoint
                            .then(response => response.json()) // Parse the response as JSON
                            .then(data => {
                                // Update the HTML elements with the fetched data
                                document.getElementById("temp").innerText = data.temp.toFixed(2);
                                document.getElementById("hum").innerText = data.hum.toFixed(2);
                            })
                            .catch(err => {
                                console.error('Error fetching data:', err); // Log errors
                                setTimeout(updateData, 5000); // Retry fetching data after 5 seconds
                            });
                    }
                    setInterval(updateData, 2000); // Update data every 2 seconds
                    window.onload = updateData; // Fetch data when the page loads
                </script>
            </head>
            <body>
                <h1>Temperature and Humidity Monitoring</h1>
                <p>Temperature: <span id="temp">--</span> °C</p>
                <p>Humidity: <span id="hum">--</span> %</p>
            </body>
            </html>
        )rawliteral");
    });

    // Define the "/data" endpoint to serve temperature and humidity data as JSON
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        // Create a JSON string with the latest temperature and humidity readings
        String json = "{\"temp\":" + String(global_temp) + ",\"hum\":" + String(global_hum) + "}";
        request->send(200, "application/json", json); // Send the JSON response
    });

    server.begin(); // Start the web server
}

void loop()
{
    unsigned long currentMillis = millis(); // Get the current time in milliseconds

    // DHT reading every 2 seconds
    if (currentMillis - previousDHTMillis >= dhtInterval) {
        previousDHTMillis = currentMillis; // Update the last DHT reading time

        float temp = dht.readTemperature(); // Read temperature from the DHT sensor
        float hum = dht.readHumidity();    // Read humidity from the DHT sensor

        // Check if the readings are valid (not NaN)
        if (!isnan(temp) && !isnan(hum)) {
            global_temp = temp; // Update the global temperature variable
            global_hum = hum;   // Update the global humidity variable
        } else {
            Serial.println("Failed to read from DHT!"); // Log an error message
        }

        // Print the temperature and humidity readings to the serial monitor
        Serial.print("Temperature: ");
        Serial.print(global_temp);
        Serial.print(" °C  |  Humidity: ");
        Serial.print(global_hum);
        Serial.println(" %");

        // Adjust the LED blinking delay time based on the temperature
        if (global_temp > 0) {
            delayTime = map((int)global_temp, 15, 40, 1000, 100); // Map temperature to delay time
            delayTime = constrain(delayTime, 100, 1000); // Constrain the delay time within limits
        }
    }

    // LED control based on temperature
    if (currentMillis - previousMillis >= (delayTime / 2)) {
        previousMillis = currentMillis; // Update the last LED toggle time
        ledState = !ledState; // Toggle the LED state
        digitalWrite(LEDPIN, ledState ? HIGH : LOW); // Set the LED pin to the new state
    }
}

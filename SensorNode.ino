//#define BLYNK_PRINT //Serial    // Comment this out to disable prints and save space
//#define BLYNK_DEBUG
// Node 1 = Outside
// Node 2 = Bathroom

#include <ESP8266WiFi.h>
#include "Seeed_BME280.h"
#include <Wire.h>
#include <EEPROM.h>
#include <InfluxDb.h>

BME280 bme280;
// EEPROM address
int addr = 0;

//WiFiClient client;
#define INFLUXDB_HOST "192.168.0.50"
#define INFLUXDB_USER "xxx"
#define INFLUXDB_PASS "xxx"

// Instance of the InfluxDB class
Influxdb influx(INFLUXDB_HOST);

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "xxx";
char pass[] = "xxx";

//unsigned int raw = 0;
//float volt = 0.0;

void setup()
{
    //Serial.begin(115200);
    // Connect D0 to RST to wake up
    pinMode(D0, WAKEUP_PULLUP);     delay(250);
    //Serial.println();
    EEPROM.begin(4); delay(250);
    //Serial.println("STARTING");
    //Serial.println();

    //Serial.print("Connecting");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        //Serial.print(".");
    }
    //Serial.println();

    //Serial.print("Connected, IP address: ");
    //Serial.println(WiFi.localIP());
    pinMode(A0, INPUT);
    if (!bme280.init()) {
        //Serial.println("Device error!");
    }
    else
    {
    	 //Serial.println("Device connected");
    }

    // If we were in emergency power mode before the last reset, enter it again now
    if (EEPROM.read(addr) == 99)
    {
        EmergencyPowerMode();
    }
    else
    {
        EEPROM.write(addr, 0);
        EEPROM.commit();
    }
    influx.setDbAuth("Weather", INFLUXDB_USER, INFLUXDB_PASS);
}

void postData(float temperature, float humidity, float pressure, float voltage)
{
    InfluxData row("temperature2");
    row.addTag("device", "Bathroom");
    row.addValue("value", temperature);
    influx.write(row);

    InfluxData row2("humidity2");
    row2.addTag("device", "Bathroom");
    row2.addValue("value", humidity);
    influx.write(row2);

    InfluxData row3("pressure2");
    row3.addTag("device", "Bathroom");
    row3.addValue("value", pressure);
    influx.write(row3);

    InfluxData row4("voltage2");
    row4.addTag("device", "Bathroom");
    row4.addValue("value", voltage);
    influx.write(row4);
}

void EmergencyPowerMode()
{
    // if the last mode before reset was CHARGING then continue
    if (EEPROM.read(addr) == 99)
    {
        delay(1000);
    }
    else
    {
        // if this is the first time we have entered EPM then write thsis to eeprom
        EEPROM.write(addr, 99);
        EEPROM.commit();
    }

    // Voltage divider R1 = 220k+100k+220k =540k and R2=100k
    float calib_factor = 5.21; // change this value to calibrate the battery voltage
    unsigned long raw = analogRead(A0);
    float volt = raw * calib_factor / 1024;

    if (volt <= 3.8	)
    {
        EEPROM.write(addr, 99);
        EEPROM.commit();
        //Serial.print("Emergency Power Mode. Going into Deep Sleep for 1 hour");
        ESP.deepSleep(60U * 60 * 1000000); //
    }
    else
    {

        // write normal mode to eeprom
        EEPROM.write(addr, 0);
        EEPROM.commit();
        //Serial.print("Normal mode");
    }
}

void loop()
{

    //get and print temperatures
    float temp = bme280.getTemperature();
    //Serial.print("Temp: ");
    //Serial.print(temp);
    //Serial.println("C");//The unit for  Celsius because original arduino don't support speical symbols


    //get and print atmospheric pressure data
    float pressure = bme280.getPressure(); // pressure in Pa
    float p = pressure / 100.0 ; // pressure in hPa
    //Serial.print("Pressure: ");
    //Serial.print(p);
    //Serial.println("hPa");

    //get and print humidity data
    float humidity = bme280.getHumidity();
    //Serial.println("Humidity: " + String(humidity) + "%");
    //Serial.println();

    // Voltage divider R1 = 220k+100k+220k =540k and R2=100k
    float calib_factor = 5.21; // change this value to calibrate the battery voltage
    unsigned long raw = analogRead(A0);
    float volt = raw * calib_factor / 1024;
    //Serial.print("Voltage: ");
    //Serial.print(volt);
    //Serial.println("v");

    // Write the data to InfluxDB
    postData(temp, humidity, pressure, volt);

    // If battery critical enter emergency power mode
    if (volt <= 3.7)
    {
        EmergencyPowerMode();
    }

    // deepSleep time is defined in microseconds. Multiply seconds by 1e6
    // MINUTES x SECONDS x MICROSECONDS
     //Serial.print("Sleeping for 20 mins");
    ESP.deepSleep(20 * 60 * 1000000); //
}

/*
  Pinbelegung:
  Display:      ESP32 Lolin32:
  BUSY          4
  RST           16
  DC            17
  CS            5   (SS)
  CLK           18  (SCK)
  DIN           23  (MOSI)
  GND           GND
  3.3V          3V

  BME280:       ESP32 Lolin32:
  VCC           3V
  GND           GND
  SCL           22
  SDA           21
*/

/************************( Importieren der genutzten Bibliotheken )************************/

// Bibliothek zum Umgang mit Zeiten
#include <time.h>

// Bibliothek zur Nutzung von WiFi
#include <WiFi.h>

// Bibliothek zur Nutzung von http
#include <HTTPClient.h>

// Bibliothek für ePaper-Displays
#include <GxEPD.h>

// Genutztes Display 2.9 inch Schwarz / Weiss E-Ink Display
#include <GxGDEH029A1/GxGDEH029A1.cpp>

#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>

// Bibliothek, um JSON-Strings nutzen zu können
#include <ArduinoJson.h>

// Icons
#include <../lib/icons/clear_sky.h>
#include <../lib/icons/few_clouds.h>
#include <../lib/icons/humidity.h>
#include <../lib/icons/mist.h>
#include <../lib/icons/rain.h>
#include <../lib/icons/scattered_broken_clouds.h>
#include <../lib/icons/shower_rain.h>
#include <../lib/icons/snow.h>
#include <../lib/icons/sunrise.h>
#include <../lib/icons/sunset.h>
#include <../lib/icons/thunderstorm.h>
#include <../lib/icons/wind.h>

/**************************(Definieren der genutzten Variablen)****************************/

// WiFi Daten
const char *ssid = "***SSID***";
const char *password = "***WIFI_PASSWORD***";

// Referenzen deklarieren
const char *current_url = "http://api.weatherbit.io/v2.0/current?city_id=***CITY_ID***&key=***API_KEY***&lang=DE";
const char *oneday3hourly_url = "https://api.weatherbit.io/v2.0/forecast/3hourly?city_id=***CITY_ID***&key=***API_KEY***&lang=DE&days=1";
const char *forecast_url = "https://api.weatherbit.io/v2.0/forecast/daily?city_id=***CITY_ID***&key=***API_KEY***&lang=DE";

// Konstanten fuer horizontale Rasterhoehe des Forecasts deklarieren
const int yForecastSymbol = 63;

// genutzte Ports definieren
GxIO_Class io(SPI, SS, 17, 16); //SPI,SS,DC,RST
GxEPD_Class display(io, 16, 4); //io,RST,BUSY

/*****************************************( Setup )****************************************/
void setup()
{
    Serial.begin(9600);

    WiFi.begin(ssid, password);

    display.init();         // e-Ink Display initialisieren
    display.setRotation(1); // Display um 90° drehen
    display.fillRect(0, 0, GxEPD_HEIGHT, GxEPD_WIDTH, GxEPD_WHITE);
    display.drawLine(0, 59, 296, 59, GxEPD_BLACK); // Mittlere horizontale Linie zeichnen
    display.update();                              // Display aktualisieren

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
}

String calculateTime(long zeit, bool param)
{
    char s[30];
    size_t i;
    struct tm tim;
    tim = *(gmtime(&zeit));
    if (param == true)
    {
        i = strftime(s, 30, "%H:%M\n", &tim);
    }
    else
    {
        i = strftime(s, 30, "%b %d, %Y; %H:%M\n", &tim);
    }
    return s;
}

double calcXPoint(int xstart, int radius, int angle)
{
    if (angle >= 360)
    {
        angle = angle - 360;
    }
    if (angle < 0)
    {
        angle = angle + 360;
    }
    return (xstart + (radius * sin(angle * PI / 180)));
}

double calcYPoint(int ystart, int radius, int angle)
{
    if (angle >= 360)
    {
        angle = angle - 360;
    }
    if (angle < 0)
    {
        angle = angle + 360;
    }
    return (ystart + (radius * cos(angle * PI / 180) * -1));
}

int drawWind(int angle, int x, int y, int r)
{
    x += r;
    y += r;
    double xPoint1 = calcXPoint(x, r, angle);
    double yPoint1 = calcYPoint(y, r, angle);
    double xPoint2 = calcXPoint(x, r, angle + 180);
    double yPoint2 = calcYPoint(y, r, angle + 180);
    double xPoint3 = calcXPoint(xPoint2, r, angle + 24);
    double yPoint3 = calcYPoint(yPoint2, r, angle + 24);
    double xPoint4 = calcXPoint(xPoint2, r, angle - 24);
    double yPoint4 = calcYPoint(yPoint2, r, angle - 24);
    display.drawLine(xPoint1, yPoint1, xPoint2, yPoint2, GxEPD_BLACK);
    display.fillTriangle(xPoint2, yPoint2, xPoint3, yPoint3, xPoint4, yPoint4, GxEPD_BLACK);
}

void drawWeatherSymbol(int x, int y, int weather_id)
{
    // Wetter-Symbol zeichnen
    if (weather_id == 800)
    {
        display.drawBitmap(x, y, gImage_clear_sky, 48, 48, GxEPD_BLACK);
    }
    if (weather_id >= 801 && weather_id <= 802)
    {
        display.drawBitmap(x, y, gImage_few_clouds, 48, 48, GxEPD_BLACK);
    }
    if (weather_id >= 803 && weather_id <= 804)
    {
        display.drawBitmap(x, y, gImage_scattered_broken_clouds, 48, 48, GxEPD_BLACK);
    }
    if (weather_id >= 700 && weather_id <= 751)
    {
        display.drawBitmap(x, y, gImage_mist, 48, 48, GxEPD_BLACK);
    }
    if ((weather_id >= 600 && weather_id <= 623) || weather_id == 511)
    {
        display.drawBitmap(x, y, gImage_snow, 48, 48, GxEPD_BLACK);
    }
    if (weather_id >= 500 && weather_id <= 511)
    {
        display.drawBitmap(x, y, gImage_rain, 48, 48, GxEPD_BLACK);
    }
    if ((weather_id >= 520 && weather_id <= 522) || (weather_id >= 300 && weather_id <= 302))
    {
        display.drawBitmap(x, y, gImage_shower_rain, 48, 48, GxEPD_BLACK);
    }
    if (weather_id >= 200 && weather_id <= 233)
    {
        display.drawBitmap(x, y, gImage_thunderstorm, 48, 48, GxEPD_BLACK);
    }
}

void drawCurrent(String url)
{
    HTTPClient http;
    http.begin(url);
    http.GET();
    String payload = http.getString();
    http.end();

    const size_t bufferSize = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(33) + 570;
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject &current_data = jsonBuffer.parseObject(payload);

    long dt = current_data["data"][0]["ts"];
    float temp = current_data["data"][0]["temp"];
    int humidity = current_data["data"][0]["rh"];
    int weather_code_current = current_data["data"][0]["weather"]["code"];
    String sunrise = current_data["data"][0]["sunrise"];
    String sunset = current_data["data"][0]["sunset"];
    float wind_speed = current_data["data"][0]["wind_spd"];
    int wind_deg = current_data["data"][0]["wind_dir"];

    // Zeitstempel zeichnen
    display.setCursor(120, 0);
    display.print(calculateTime(dt, false));

    // Aktuelles Wettersymbol zeichnen
    drawWeatherSymbol(122, 9, weather_code_current);

    // Cursor setzen, Temperatur zeichnen
    display.setCursor(190, 17);
    if (temp >= 0 && temp < 10)
    {
        display.print(" ");
    }
    if (temp > 10 || (temp < 0 && temp > (-10)))
    {
        display.print("");
    }
    display.print(temp, 1);
    // Grad-Zeichen zeichnen
    display.drawCircle(215, 18, 1, GxEPD_BLACK);

    // Sonnenaufgang: Cursor setzen, Zeit zeichnen
    display.setCursor(25, 8);
    display.print(sunrise);

    // Sonnenuntergang: Cursor setzen, Zeit zeichnen
    display.setCursor(85, 8);
    display.print(sunset);

    // Luftfeuchtigkeit: Cursor setzen, Luftfeuchtigkeit zeichnen
    display.setCursor(25, 38);
    display.print(humidity);

    // Wind: Cursor setzen, Windrichtung & Geschwindigkeit zeichnen
    drawWind(wind_deg, 60, 30, 12);
    display.setCursor(86, 33);
    display.print(wind_speed, 1);
}

void drawOneday3hourly(String url)
{
    HTTPClient http;
    http.begin(url);
    http.GET();
    String payload = http.getString();
    http.end();

    const size_t bufferSize = JSON_ARRAY_SIZE(8) + 8 * JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(7) + 4 * JSON_OBJECT_SIZE(22) + 4 * JSON_OBJECT_SIZE(24) + 2800;
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject &outlook_data = jsonBuffer.parseObject(payload);

    long dt = outlook_data["data"][0]["ts"];
    float temp = outlook_data["data"][0]["temp"];
    int weather_code_outlook = outlook_data["data"][0]["weather"]["code"];

    // Wettersymbol fuer die naechste Stunde zeichnen
    drawWeatherSymbol(244, 9, weather_code_outlook);

    // Zeitstempel zeichnen
    display.setCursor(250, 0);
    display.print(calculateTime(dt, true));

    // Cursor setzen, Temperatur zeichnen
    display.setCursor(190, 41);
    if (temp >= 0 && temp < 10)
    {
        display.print(" ");
    }
    display.print(temp, 1);
    // Grad-Zeichen zeichnen
    display.drawCircle(215, 43, 1, GxEPD_BLACK);
}

void drawForecast(String url)
{
    HTTPClient http;
    http.begin(url);
    http.GET();
    String payload = http.getString();
    http.end();

    const size_t bufferSize = JSON_ARRAY_SIZE(16) + 16 * JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(7) + 16 * JSON_OBJECT_SIZE(24) + 6500;
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject &forecast_data = jsonBuffer.parseObject(payload);

    int xForecastSymbol = 0;
    for (int i = 1; i <= 5; i++)
    {
        int weather_code_forecast = forecast_data["data"][i]["weather"]["code"];
        int temp = forecast_data["data"][i]["temp"];
        String datetime = forecast_data["data"][i]["datetime"];
        datetime.remove(0, 5);

        drawWeatherSymbol(xForecastSymbol, yForecastSymbol, weather_code_forecast);

        display.setCursor(xForecastSymbol + 15, yForecastSymbol + 49);
        if (temp >= 0 && temp < 10)
        {
            display.print(" ");
        }
        if (temp > 10 || (temp < 0 && temp > (-10)))
        {
            display.print("");
        }
        display.print(temp);

        // Grad-Zeichen zeichnen
        display.drawCircle(xForecastSymbol + 28, yForecastSymbol + 49, 1, GxEPD_BLACK);

        // Datum (MM-DD) zeichnen
        display.setCursor(xForecastSymbol + 10, yForecastSymbol + 58);
        display.print(datetime);
        xForecastSymbol += 61;
    }
}

/*************************************(Hauptprogramm)**************************************/
void loop()
{
    // Bereich von current & outlook mit leerem rechteck füllen
    display.fillRect(0, 0, GxEPD_HEIGHT, 58, GxEPD_WHITE);

    WiFiClient client;

    display.setRotation(1);            // Display um 90° drehen
    display.setTextColor(GxEPD_BLACK); // Schriftfarbe Schwarz
    display.setTextSize(1);            // Schriftgroesse 1

    // aktuelles Wetter zeichnen
    drawCurrent(current_url);

    // Sonnenaufgang: Symbol zeichnen
    display.drawBitmap(0, 0, gImage_sunrise, 24, 24, GxEPD_BLACK);
    // Sonnenuntergang: Symbol zeichnen
    display.drawBitmap(61, 0, gImage_sunset, 24, 24, GxEPD_BLACK);
    // Luftfeuchtigkeit: Symbol zeichnen
    display.drawBitmap(0, 30, gImage_humidity, 24, 24, GxEPD_BLACK);
    // Wind: Geschwindigkeitseinheit unter Geschwindigkeit zeichnen
    display.setCursor(86, 43);
    display.print("m/s");

    // Ausblick für naechste Stunde zeichnen
    drawOneday3hourly(oneday3hourly_url);

    // Linien zur Trennung von current und outlook
    display.drawLine(236, 0, 236, 31, GxEPD_BLACK);
    display.drawLine(177, 32, 236, 32, GxEPD_BLACK);
    display.drawLine(177, 33, 177, 58, GxEPD_BLACK);

    // Display updaten
    display.updateWindow(0, 0, GxEPD_HEIGHT, 58, true);

    // gesamtes Display mit leerem rechteck füllen
    display.fillRect(0, 60, GxEPD_HEIGHT, GxEPD_WIDTH, true);

    // Strukturen fuer die naechsten 5 Tage erstellen und befuellen, fuer jeden Tag Symbol zeichnen, Temperatur und Datum schreiben
    drawForecast(forecast_url);

    // vertikales Raster zeichnen
    display.drawLine(59, 60, 59, GxEPD_WIDTH, GxEPD_BLACK);
    display.drawLine(118, 60, 118, GxEPD_WIDTH, GxEPD_BLACK);
    display.drawLine(177, 60, 177, GxEPD_WIDTH, GxEPD_BLACK);
    display.drawLine(236, 60, 236, GxEPD_WIDTH, GxEPD_BLACK);

    // Display updaten
    display.updateWindow(0, 60, GxEPD_HEIGHT, GxEPD_WIDTH, true);

    delay(1800000);
}

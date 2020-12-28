// Sketch for ESP32 to fetch the Weather Forecast from OpenWeather
// an example from the library here:
// https://github.com/Bodmer/OpenWeather

// Sign up for a key and read API configuration info here:
// https://openweathermap.org/

// You can change the number of hours and days for the forecast in the
// "User_Setup.h" file inside the OpenWeather library folder.
// By default this is 6 hours (can be up to 48) and 5 days
// (can be up to 8 days = today plus 7 days)

// Choose library to load
#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <WiFiClientSecure.h>
#else // ESP32
  #include <WiFi.h>
#endif

#include <JSON_Decoder.h>

#include <OpenWeather.h>

// Just using this library for unix time conversion
#include <Time.h>

// =====================================================
// ========= User configured stuff starts here =========
// Further configuration settings can be found in the
// OpenWeather library "User_Setup.h" file

// Change to suit your WiFi router
#define WIFI_SSID     "Your_SSID"
#define WIFI_PASSWORD "Your_password"

// OpenWeather API Details, replace x's with your API key
#define api_key       "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" // Obtain this from your OpenWeather account

// Set both your longitude and latitude to at least 4 decimal places
#define latitude      "27.9881" // 90.0000 to -90.0000 negative for Southern hemisphere
#define longitude     "86.9250" // 180.000 to -180.000 negative for West

#define units         "metric"  // or "imperial"
#define language      "en"   // See notes tab

// =========  User configured stuff ends here  =========
// =====================================================

OW_Weather ow; // Weather forecast library instance
uint32_t timezone_offset = 0;

void setup() { 
  Serial.begin(250000); // Fast to stop it holding up the stream
  // 115200 or less is very unstable

  Serial.printf("\nConnecting to %s", WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
   
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println(" Connected!");
}

void loop() {
  printCurrentWeather("current"); // get current weather once because it updates every 10 minutes
  delay(125 * 1000);
  for (int i = 0; i < 4; i++) // get onecall weather 4 times because it updates every 2 minutes
  {
    printCurrentWeather("onecall");
    delay(125 * 1000);
  }
  // We can make 1000 requests a day. 74880 secs per day / 125 secs = 599 requests per day
}

/***************************************************************************************
**                          Send weather info to serial port
***************************************************************************************/
void printCurrentWeather(String api)
{
  // Create the structures that hold the retrieved weather
  OW_current *current = new OW_current;
  OW_hourly *hourly = new OW_hourly;
  OW_daily  *daily = new OW_daily;

  Serial.print("\nRequesting weather information from OpenWeatherMap (choosed API is "); Serial.print(api); Serial.println(")...");

  ow.getForecast(current, hourly, daily, api_key, latitude, longitude, units, language, api);

  timezone_offset = ow.timezone_offset;

  Serial.println("Weather from OpenWeatherMap was received\n");

  // Position as reported by Open Weather
  Serial.print("Latitude            : "); Serial.println(ow.lat);
  Serial.print("Longitude           : "); Serial.println(ow.lon);
  if (api == "onecall")
  {
    Serial.print("Timezone            : "); Serial.println(ow.timezone);
  }
  Serial.print("Timezone_offset     : "); Serial.println(ow.timezone_offset);
  Serial.print("dt (time when weather updated on the server): "); Serial.print(strTime(current->dt));
  Serial.println();

  if (current)
  {
    Serial.println("############### Current weather ###############\n");
    Serial.print("sunrise          : "); Serial.print(strTime(current->sunrise));
    Serial.print("sunset           : "); Serial.print(strTime(current->sunset));
    Serial.print("temp             : "); Serial.println(current->temp);
    Serial.print("feels_like       : "); Serial.println(current->feels_like);
    Serial.print("pressure         : "); Serial.println(current->pressure);
    Serial.print("humidity         : "); Serial.println(current->humidity);
    Serial.print("clouds           : "); Serial.println(current->clouds);
    Serial.print("visibility       : "); Serial.println(current->visibility);
    Serial.print("wind_speed       : "); Serial.println(current->wind_speed);
    Serial.print("wind_gust        : "); Serial.println(current->wind_gust);
    Serial.print("wind_deg         : "); Serial.println(current->wind_deg);
    Serial.println();
    Serial.print("id               : "); Serial.println(current->id);
    Serial.print("main             : "); Serial.println(current->main);
    Serial.print("description      : "); Serial.println(current->description);
    Serial.print("icon             : "); Serial.println(current->icon);
    Serial.println();
    if (api == "onecall")
    {
      Serial.print("dew_point        : "); Serial.println(current->dew_point);
      Serial.print("uvi              : "); Serial.println(current->uvi);
    }
    if (api == "current")
    {
      Serial.print("city_id          : "); Serial.println(current->city_id);
      Serial.print("city_name        : "); Serial.println(current->city_name);
      Serial.print("country          : "); Serial.println(current->country);
      Serial.print("rain             : "); Serial.println(current->rain);
      Serial.print("snow             : "); Serial.println(current->snow);
      Serial.print("rain_3h          : "); Serial.println(current->rain_3h);
      Serial.print("snow_3h          : "); Serial.println(current->snow_3h);
      Serial.print("temp_min         : "); Serial.println(current->temp_min);
      Serial.print("temp_max         : "); Serial.println(current->temp_max);
      Serial.print("sea_level        : "); Serial.println(current->sea_level);
      Serial.print("grnd_level       : "); Serial.println(current->grnd_level);
    }

    Serial.println();
  }
  if (api == "onecall")
  {
    if (hourly)
    {
      Serial.println("############### Hourly weather  ###############\n");
      for (int i = 0; i < MAX_HOURS; i++)
      {
        Serial.print("Hourly summary  "); if (i < 10) Serial.print(" "); Serial.print(i);
        Serial.println();
        Serial.print("dt (time)        : "); Serial.print(strTime(hourly->dt[i]));
        Serial.print("temp             : "); Serial.println(hourly->temp[i]);
        Serial.print("feels_like       : "); Serial.println(hourly->feels_like[i]);
        Serial.print("pressure         : "); Serial.println(hourly->pressure[i]);
        Serial.print("humidity         : "); Serial.println(hourly->humidity[i]);
        Serial.print("dew_point        : "); Serial.println(hourly->dew_point[i]);
        Serial.print("clouds           : "); Serial.println(hourly->clouds[i]);
        Serial.print("wind_speed       : "); Serial.println(hourly->wind_speed[i]);
        Serial.print("wind_gust        : "); Serial.println(hourly->wind_gust[i]);
        Serial.print("wind_deg         : "); Serial.println(hourly->wind_deg[i]);
        Serial.print("rain             : "); Serial.println(hourly->rain[i]);
        Serial.print("snow             : "); Serial.println(hourly->snow[i]);
        Serial.println();
        Serial.print("id               : "); Serial.println(hourly->id[i]);
        Serial.print("main             : "); Serial.println(hourly->main[i]);
        Serial.print("description      : "); Serial.println(hourly->description[i]);
        Serial.print("icon             : "); Serial.println(hourly->icon[i]);
        Serial.print("pop              : "); Serial.println(hourly->pop[i]);
  
        Serial.println();
      }
    }
  
    if (daily)
    {
      Serial.println("###############  Daily weather  ###############\n");
      for (int i = 0; i < MAX_DAYS; i++)
      {
        Serial.print("Daily summary   "); if (i < 10) Serial.print(" "); Serial.print(i);
        Serial.println();
        Serial.print("dt (time)        : "); Serial.print(strTime(daily->dt[i]));
        Serial.print("sunrise          : "); Serial.print(strTime(daily->sunrise[i]));
        Serial.print("sunset           : "); Serial.print(strTime(daily->sunset[i]));
  
        Serial.print("temp.morn        : "); Serial.println(daily->temp_morn[i]);
        Serial.print("temp.day         : "); Serial.println(daily->temp_day[i]);
        Serial.print("temp.eve         : "); Serial.println(daily->temp_eve[i]);
        Serial.print("temp.night       : "); Serial.println(daily->temp_night[i]);
        Serial.print("temp.min         : "); Serial.println(daily->temp_min[i]);
        Serial.print("temp.max         : "); Serial.println(daily->temp_max[i]);
  
        Serial.print("feels_like.morn  : "); Serial.println(daily->feels_like_morn[i]);
        Serial.print("feels_like.day   : "); Serial.println(daily->feels_like_day[i]);
        Serial.print("feels_like.eve   : "); Serial.println(daily->feels_like_eve[i]);
        Serial.print("feels_like.night : "); Serial.println(daily->feels_like_night[i]);
  
        Serial.print("pressure         : "); Serial.println(daily->pressure[i]);
        Serial.print("humidity         : "); Serial.println(daily->humidity[i]);
        Serial.print("dew_point        : "); Serial.println(daily->dew_point[i]);
        Serial.print("uvi              : "); Serial.println(daily->uvi[i]);
        Serial.print("clouds           : "); Serial.println(daily->clouds[i]);
        Serial.print("visibility       : "); Serial.println(daily->visibility[i]);
        Serial.print("wind_speed       : "); Serial.println(daily->wind_speed[i]);
        Serial.print("wind_gust        : "); Serial.println(daily->wind_gust[i]);
        Serial.print("wind_deg         : "); Serial.println(daily->wind_deg[i]);
        Serial.print("rain             : "); Serial.println(daily->rain[i]);
        Serial.print("snow             : "); Serial.println(daily->snow[i]);
        Serial.println();
        Serial.print("id               : "); Serial.println(daily->id[i]);
        Serial.print("main             : "); Serial.println(daily->main[i]);
        Serial.print("description      : "); Serial.println(daily->description[i]);
        Serial.print("icon             : "); Serial.println(daily->icon[i]);
        Serial.print("pop              : "); Serial.println(daily->pop[i]);
  
        Serial.println();
      }
    }
  }

  // Delete to free up space and prevent fragmentation as strings change in length
  delete current;
  delete hourly;
  delete daily;
}

/***************************************************************************************
**                          Convert unix time to a time string
***************************************************************************************/
String strTime(time_t unixTime)
{
  unixTime += timezone_offset;
  return ctime(&unixTime);
}

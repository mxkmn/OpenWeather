// Client library for the OpenWeather data-point server
// https://openweathermap.org/

// Created by Bodmer 9/4/2020
// This is a beta test version and is subject to change!

// See license.txt in root folder of library

#define SECURE_CONNECTION // Define if you want to use HTTPS instead of HTTP for security (3x slower)

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #ifdef SECURE_CONNECTION
    BearSSL::WiFiClientSecure client;
  #endif
#else // ESP32
  #include <WiFi.h>
#endif

#include <WiFiClientSecure.h>

// The streaming parser to use is not the Arduino IDE library manager default,
// but this one which is slightly different and renamed to avoid conflicts:
// https://github.com/Bodmer/JSON_Decoder

#include <JSON_Listener.h>
#include <JSON_Decoder.h>

#include "OpenWeather.h"


/***************************************************************************************
** Function name:           getForecast
** Description:             Setup the weather forecast request
***************************************************************************************/
// The structures etc are created by the sketch and passed to this function.
// Pass a nullptr for current, hourly or daily pointers to exclude in response.
bool OW_Weather::getForecast(OW_current *current, OW_hourly *hourly, OW_daily *daily,
                             String api_key, String latitude, String longitude,
                             String units, String language, String api_type) {

  data_set = "";
  hourly_index = 0;
  daily_index = 0;

  if (api_type == "onecall") api_id = 1;
  else if (api_type == "current") {
    api_id = 2;
    api_type = "weather"; // Changing the type from human-understandable ("current") to OWT-understandable ("weather")
  }

  // Local copies of structure pointers, the structures are filled during parsing
  this->current = current;
  this->hourly  = hourly;
  this->daily   = daily;

  // Exclude some info by passing fn a NULL pointer to reduce memory needed
  String exclude = "";
  if (!current)  exclude += "current,";
  if (!hourly)   exclude += "hourly,";
  if (!daily)    exclude += "daily";

  String url = "https://api.openweathermap.org/data/2.5/" + api_type + "?lat=" + latitude + "&lon=" + longitude + "&exclude=minutely,alerts," + exclude + "&units=" + units + "&lang=" + language + "&appid=" + api_key;

  // Send GET request and feed the parser
  bool result = parseRequest(url);

  // Null out pointers to prevent crashes
  this->current = nullptr;
  this->hourly  = nullptr;
  this->daily   = nullptr;

  return result;
}

/***************************************************************************************
** Function name:           partialDataSet
** Description:             Set requested data set to partial (true) or full (false)
***************************************************************************************/
void OW_Weather::partialDataSet(bool partialSet) {
  this->partialSet = partialSet;
}

/***************************************************************************************
** Function name:           parseRequest
** Description:             Fetches the JSON message and feeds to the parser
***************************************************************************************/
bool OW_Weather::parseRequest(String url) {

  uint32_t dt = millis();

#ifdef SECURE_CONNECTION
  #ifdef ESP32
    WiFiClientSecure client;
  #endif

  #ifdef ESP8266
    client.setInsecure();
  #endif
#else
  WiFiClient client;
#endif

  JSON_Decoder parser;
  parser.setListener(this);

  const char* host = "api.openweathermap.org";

#ifdef SECURE_CONNECTION
  if (!client.connect(host, 443))
#else
 if (!client.connect(host, 80))
#endif
  {
#ifdef LOG_ERRORS
    Serial.println("Connection failed.");
#endif
    return false;
  }

  uint32_t timeout = millis();
  char c = 0;
  parseOK = false;

#ifdef SHOW_JSON
  int ccount = 0;
#endif

  // Send GET request
#ifdef LOG_UNNECESSARY_INFO
  Serial.println("Sending GET request to api.openweathermap.org...");
#endif
  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");

  // Pull out any header, X-Forecast-API-Calls: reports current daily API call count
#ifdef ESP8266
  while (client.available() || client.connected())
#else
  while (client.connected())
#endif
  {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
#ifdef LOG_UNNECESSARY_INFO
      Serial.println("Header end found");
#endif
      break;
    }

#ifdef SHOW_HEADER
    Serial.println(line);
#endif

    if ((millis() - timeout) > 5000UL)
    {
#ifdef LOG_ERRORS
      Serial.println ("HTTP header timeout");
#endif
      client.stop();
      return false;
    }
  }
#ifdef LOG_UNNECESSARY_INFO
  Serial.println("Parsing JSON");
#endif
  
  // Parse the JSON data, available() includes yields
#ifdef ESP8266
  while (client.available() || client.connected())
  {
    while (client.available())
#else
  while (client.available() > 0 || client.connected())
  {
    while (client.available() > 0)
#endif
    {
      c = client.read();
      parser.parse(c);

#ifdef SHOW_JSON
      if (c == '{' || c == '[' || c == '}' || c == ']') Serial.println();
      Serial.print(c); if (ccount++ > 100 && c == ',') {ccount = 0; Serial.println();}
#endif

      if ((millis() - timeout) > 8000UL)
      {
#ifdef LOG_ERRORS
        Serial.println ("JSON parse client timeout");
#endif
        parser.reset();
        client.stop();
        return false;
      }
#ifdef ESP32
      yield();
#endif
    }
  }

#ifdef LOG_UNNECESSARY_INFO
  Serial.print("Done in "); Serial.print(millis()-dt); Serial.println(" ms");
#endif

  parser.reset();

  client.stop();
  
  // A message has been parsed without error but the data-point correctness is unknown
  return parseOK;
}

/***************************************************************************************
** Function name:           key etc
** Description:             These functions are called while parsing the JSON message
***************************************************************************************/
void OW_Weather::key(const char *key) {

  currentKey = key;

#ifdef SHOW_CALLBACK
  Serial.print("\n>>> Key >>>" + (String)key);
#endif
}

void OW_Weather::startDocument() {

  currentParent = currentKey = currentSet = "";
  objectLevel = 0;
  valuePath = "";
  arrayIndex = 0;
  arrayLevel = 0;
  parseOK = true;

#ifdef SHOW_CALLBACK
  Serial.print("\n>>> Start document >>>");
#endif
}

void OW_Weather::endDocument() {

  currentParent = currentKey = "";
  objectLevel = 0;
  valuePath = "";
  arrayIndex = 0;
  arrayLevel = 0;

#ifdef SHOW_CALLBACK
  Serial.print("\n<<< End document <<<");
#endif
}

void OW_Weather::startObject() {

  if (arrayIndex == 0 && objectLevel == 1) currentParent = currentKey;
  currentSet = currentKey;
  objectLevel++;

#ifdef SHOW_CALLBACK
  Serial.print("\n>>> Start object level:" + (String) objectLevel + " array level:" + (String) arrayLevel + " array index:" + (String) arrayIndex +" >>>");
#endif
}

void OW_Weather::endObject() {

  if (arrayLevel == 0) currentParent = "";
  if (arrayLevel == 1  && objectLevel == 2) arrayIndex++;
  objectLevel--;
  

#ifdef SHOW_CALLBACK
  Serial.print("\n<<< End object <<<");
#endif
}

void OW_Weather::startArray() {

  arrayLevel++;
  valuePath = currentParent + "/" + currentKey; // aka = current Object, e.g. "daily:data"

#ifdef SHOW_CALLBACK
  Serial.print("\n>>> Start array " + valuePath + "/" + (String) arrayLevel + "/" + (String) arrayIndex +" >>>");
#endif
}

void OW_Weather::endArray() {
  if (arrayLevel > 0) arrayLevel--;
  if (arrayLevel == 0) arrayIndex = 0;
  valuePath = "";

#ifdef SHOW_CALLBACK
  Serial.print("\n<<< End array <<<");
#endif
}

void OW_Weather::whitespace(char c) {
}

void OW_Weather::error( const char *message ) {
  Serial.print("\nParse error message: ");
  Serial.print(message);
  parseOK = false;
}

/***************************************************************************************
** Function name:           value (full or partial data set)
** Description:             Stores the parsed data in the structures for sketch access
***************************************************************************************/
void OW_Weather::value(const char *val)
{
  if (!partialSet) fullDataSet(val);
  else partialDataSet(val);
}

/***************************************************************************************
** Function name:           fullDataSet
** Description:             Collects full data set
***************************************************************************************/
void OW_Weather::fullDataSet(const char *val) {

  String value = val;

  if (api_id == 1) // OneCall
  {
    // Start of JSON
    if (currentParent == "") {
      if (currentKey == "lat") lat = value.toFloat();
      else
      if (currentKey == "lon") lon = value.toFloat();
      else
      if (currentKey == "timezone") timezone = value;
      else
      if (currentKey == "timezone_offset") timezone_offset = (uint32_t)value.toInt();
    }

    // Current forecast - no array index - short path
    if (currentParent == "current") {
      data_set = "current";
      if (currentKey == "dt") current->dt = (uint32_t)value.toInt();
      else
      if (currentKey == "sunrise") current->sunrise = (uint32_t)value.toInt();
      else
      if (currentKey == "sunset") current->sunset = (uint32_t)value.toInt();
      else
      if (currentKey == "temp") current->temp = value.toFloat();
      else
      if (currentKey == "feels_like") current->feels_like = value.toFloat();
      else
      if (currentKey == "pressure") current->pressure = value.toFloat();
      else
      if (currentKey == "humidity") current->humidity = value.toInt();
      else
      if (currentKey == "dew_point") current->dew_point = value.toFloat();
      else
      if (currentKey == "uvi") current->uvi = value.toFloat();
      else
      if (currentKey == "clouds") current->clouds = value.toInt();
      else
      if (currentKey == "visibility") current->visibility = value.toInt();
      else
      if (currentKey == "wind_speed") current->wind_speed = value.toFloat();
      else
      if (currentKey == "wind_gust") current->wind_gust = value.toFloat();
      else
      if (currentKey == "wind_deg") current->wind_deg = (uint16_t)value.toInt();
      else /*
      if (currentKey == "rain") current->rain = value.toFloat(); // It won't work because the full way is current.rain.1h, not current.rain
      else
      if (currentKey == "snow") current->snow = value.toFloat(); // It won't work because the full way is current.snow.1h, not current.snow
      else */

      if (currentKey == "id") current->id = value.toInt();
      else
      if (currentKey == "main") current->main = value;
      else
      if (currentKey == "description") current->description = value;
      else
      if (currentKey == "icon") current->icon = value;

      return;
    }

    // Hourly forecast
    if (currentParent == "hourly") {
      data_set = "hourly";
      
      if (arrayIndex >= MAX_HOURS) return;
      
      if (currentKey == "dt") hourly->dt[arrayIndex] = (uint32_t)value.toInt();
      else
      if (currentKey == "temp") hourly->temp[arrayIndex] = value.toFloat();
      else
      if (currentKey == "feels_like") hourly->feels_like[arrayIndex] = value.toFloat();
      else
      if (currentKey == "pressure") hourly->pressure[arrayIndex] = value.toFloat();
      else
      if (currentKey == "humidity") hourly->humidity[arrayIndex] = value.toInt();
      else
      if (currentKey == "dew_point") hourly->dew_point[arrayIndex] = value.toFloat();
      else
      if (currentKey == "clouds") hourly->clouds[arrayIndex] = value.toInt();
      else
      if (currentKey == "wind_speed") hourly->wind_speed[arrayIndex] = value.toFloat();
      else
      if (currentKey == "wind_gust") hourly->wind_gust[arrayIndex] = value.toFloat();
      else
      if (currentKey == "wind_deg") hourly->wind_deg[arrayIndex] = (uint16_t)value.toInt();
      else
      if (currentKey == "rain") hourly->rain[arrayIndex] = value.toFloat();
      else
      if (currentKey == "snow") hourly->snow[arrayIndex] = value.toFloat();
      else

      if (currentKey == "id") hourly->id[arrayIndex] = value.toInt();
      else
      if (currentKey == "main") hourly->main[arrayIndex] = value;
      else
      if (currentKey == "description") hourly->description[arrayIndex] = value;
      else
      if (currentKey == "icon") hourly->icon[arrayIndex] = value;
      else
      if (currentKey == "pop") hourly->pop[arrayIndex] = value.toFloat();

      return;
    }


    // Daily forecast
    if (currentParent == "daily") {
      data_set = "daily";
      
      if (arrayIndex >= MAX_DAYS) return;
      
      if (currentKey == "dt") daily->dt[arrayIndex] = (uint32_t)value.toInt();
      else
      if (currentKey == "sunrise") daily->sunrise[arrayIndex] = (uint32_t)value.toInt();
      else
      if (currentKey == "sunset") daily->sunset[arrayIndex] = (uint32_t)value.toInt();
      else
      if (currentKey == "pressure") daily->pressure[arrayIndex] = value.toFloat();
      else
      if (currentKey == "humidity") daily->humidity[arrayIndex] = value.toInt();
      else
      if (currentKey == "dew_point") daily->dew_point[arrayIndex] = value.toFloat();
      else
      if (currentKey == "clouds") daily->clouds[arrayIndex] = value.toInt();
      else
      if (currentKey == "wind_speed") daily->wind_speed[arrayIndex] = value.toFloat();
      else
      if (currentKey == "wind_gust") daily->wind_gust[arrayIndex] = value.toFloat();
      else
      if (currentKey == "wind_deg") daily->wind_deg[arrayIndex] = (uint16_t)value.toInt();
      else
      if (currentKey == "rain") daily->rain[arrayIndex] = value.toFloat();
      else
      if (currentKey == "snow") daily->snow[arrayIndex] = value.toFloat();
      else

      if (currentKey == "id") daily->id[arrayIndex] = value.toInt();
      else
      if (currentKey == "main") daily->main[arrayIndex] = value;
      else
      if (currentKey == "description") daily->description[arrayIndex] = value;
      else
      if (currentKey == "icon") daily->icon[arrayIndex] = value;
      else
      if (currentKey == "pop") daily->pop[arrayIndex] = value.toFloat();

      if (currentSet == "temp") {
        if (currentKey == "morn") daily->temp_morn[arrayIndex] = value.toFloat();
        else
        if (currentKey == "day") daily->temp_day[arrayIndex] = value.toFloat();
        else
        if (currentKey == "eve") daily->temp_eve[arrayIndex] = value.toFloat();
        else
        if (currentKey == "night") daily->temp_night[arrayIndex] = value.toFloat();
        else
        if (currentKey == "min") daily->temp_min[arrayIndex] = value.toFloat();
        else
        if (currentKey == "max") daily->temp_max[arrayIndex] = value.toFloat();
      }

      if (currentSet == "feels_like") {
        if (currentKey == "morn") daily->feels_like_morn[arrayIndex] = value.toFloat();
        else
        if (currentKey == "day") daily->feels_like_day[arrayIndex] = value.toFloat();
        else
        if (currentKey == "eve") daily->feels_like_eve[arrayIndex] = value.toFloat();
        else
        if (currentKey == "night") daily->feels_like_night[arrayIndex] = value.toFloat();
      }

      return;
    }
  }
  else if (api_id == 2) // Current
  {
    // Start of JSON
    if (currentParent == "") {
      if (currentKey == "visibility") current->visibility = value.toInt();
      else
      if (currentKey == "dt") current->dt = (uint32_t)value.toInt();
      else
      if (currentKey == "timezone") timezone_offset = (uint32_t)value.toInt();
      else
      if (currentKey == "id") current->city_id = (uint32_t)value.toInt();
      else
      if (currentKey == "name") current->city_name = value;
      
      return;
    }
    else if (currentParent == "coord") {
      data_set = "coord";

      if (currentKey == "lat") lat = value.toFloat();
      else
      if (currentKey == "lon") lon = value.toFloat();

      return;
    }
    else if (currentParent == "weather") {
      data_set = "weather";

      if (currentKey == "id") current->id = value.toInt();
      else
      if (currentKey == "main") current->main = value;
      else
      if (currentKey == "description") current->description = value;
      else
      if (currentKey == "icon") current->icon = value;

      return;
    }
    else if (currentParent == "main") {
      data_set = "main";

      if (currentKey == "temp") current->temp = value.toFloat();
      else
      if (currentKey == "feels_like") current->feels_like = value.toFloat();
      else
      if (currentKey == "pressure") current->pressure = value.toFloat();
      else
      if (currentKey == "humidity") current->humidity = value.toInt();
      else
      if (currentKey == "temp_min") current->temp_min = value.toFloat();
      else
      if (currentKey == "temp_max") current->temp_max = value.toFloat();
      else
      if (currentKey == "sea_level") current->sea_level = value.toInt();
      else
      if (currentKey == "grnd_level") current->grnd_level = value.toInt();

      return;
    }
    else if (currentParent == "wind") {
      data_set = "wind";

      if (currentKey == "speed") current->wind_speed = value.toFloat();
      else
      if (currentKey == "deg") current->wind_deg = (uint16_t)value.toInt();
      else
      if (currentKey == "wind_gust") current->wind_gust = value.toFloat();

      return;
    }
    else if (currentParent == "clouds") {
      data_set = "clouds";

      if (currentKey == "clouds") current->clouds = value.toInt();

      return;
    }
    else if (currentParent == "rain") {
      data_set = "rain";

      if (currentKey == "1h") current->rain = value.toFloat();
      else
      if (currentKey == "3h") current->rain_3h = value.toFloat();

      return;
    }
    else if (currentParent == "snow") {
      data_set = "snow";

      if (currentKey == "1h") current->snow = value.toFloat();
      else
      if (currentKey == "3h") current->snow_3h = value.toFloat();

      return;
    }
    else if (currentParent == "sys") {
      data_set = "sys";

      if (currentKey == "sunrise") current->sunrise = (uint32_t)value.toInt();
      else
      if (currentKey == "sunset") current->sunset = (uint32_t)value.toInt();
      else
      if (currentKey == "country") current->country = value;

      return;
    }
  }
}

/***************************************************************************************
** Function name:           partialDataSet
** Description:             Collects partial data set
***************************************************************************************/
void OW_Weather::partialDataSet(const char *val) {

   String value = val;

  // Current forecast - no array index - short path
  if (currentParent == "current") {
    data_set = "current";
    if (currentKey == "dt") current->dt = (uint32_t)value.toInt();
    else
    if (currentKey == "sunrise") current->sunrise = (uint32_t)value.toInt();
    else
    if (currentKey == "sunset") current->sunset = (uint32_t)value.toInt();
    else
    if (currentKey == "temp") current->temp = value.toFloat();
    //else
    //if (currentKey == "feels_like") current->feels_like = value.toFloat();
    else
    if (currentKey == "pressure") current->pressure = value.toFloat();
    else
    if (currentKey == "humidity") current->humidity = value.toInt();
    //else
    //if (currentKey == "dew_point") current->dew_point = value.toFloat();
    //else
    //if (currentKey == "uvi") current->uvi = value.toFloat();
    else
    if (currentKey == "clouds") current->clouds = value.toInt();
    //else
    //if (currentKey == "visibility") current->visibility = value.toInt();
    else
    if (currentKey == "wind_speed") current->wind_speed = value.toFloat();
    //else
    //if (currentKey == "wind_gust") current->wind_gust = value.toFloat();
    else
    if (currentKey == "wind_deg") current->wind_deg = (uint16_t)value.toInt();
    //else
    //if (currentKey == "rain") current->rain = value.toFloat();
    //else
    //if (currentKey == "snow") current->snow = value.toFloat();

    else
    if (currentKey == "id") current->id = value.toInt();
    else
    if (currentKey == "main") current->main = value;
    else
    if (currentKey == "description") current->description = value;
    //else
    //if (currentKey == "icon") current->icon = value;

    return;
  }

/*
  // Hourly forecast
  if (currentParent == "hourly") {
    data_set = "hourly";
    
    if (arrayIndex >= MAX_HOURS) return;
    
    if (currentKey == "dt") hourly->dt[arrayIndex] = (uint32_t)value.toInt();
    else
    if (currentKey == "temp") hourly->temp[arrayIndex] = value.toFloat();
    else
    if (currentKey == "feels_like") hourly->feels_like[arrayIndex] = value.toFloat();
    else
    if (currentKey == "pressure") hourly->pressure[arrayIndex] = value.toFloat();
    else
    if (currentKey == "humidity") hourly->humidity[arrayIndex] = value.toInt();
    else
    if (currentKey == "dew_point") hourly->dew_point[arrayIndex] = value.toFloat();
    else
    if (currentKey == "clouds") hourly->clouds[arrayIndex] = value.toInt();
    else
    if (currentKey == "wind_speed") hourly->wind_speed[arrayIndex] = value.toFloat();
    else
    if (currentKey == "wind_gust") hourly->wind_gust[arrayIndex] = value.toFloat();
    else
    if (currentKey == "wind_deg") hourly->wind_deg[arrayIndex] = (uint16_t)value.toInt();
    else
    if (currentKey == "rain") hourly->rain[arrayIndex] = value.toFloat();
    else
    if (currentKey == "snow") hourly->snow[arrayIndex] = value.toFloat();
    else

    if (currentKey == "id") hourly->id[arrayIndex] = value.toInt();
    else
    if (currentKey == "main") hourly->main[arrayIndex] = value;
    else
    if (currentKey == "description") hourly->description[arrayIndex] = value;
    else
    if (currentKey == "icon") hourly->icon[arrayIndex] = value;

    return;
  }
*/

  // Daily forecast
  if (currentParent == "daily") {
    data_set = "daily";
    
    if (arrayIndex >= MAX_DAYS) return;
    
    if (currentKey == "dt") daily->dt[arrayIndex] = (uint32_t)value.toInt();
    else
    //if (currentKey == "sunrise") daily->sunrise[arrayIndex] = (uint32_t)value.toInt();
    //else
    //if (currentKey == "sunset") daily->sunset[arrayIndex] = (uint32_t)value.toInt();
    //else
    //if (currentKey == "pressure") daily->pressure[arrayIndex] = value.toFloat();
    //else
    //if (currentKey == "humidity") daily->humidity[arrayIndex] = value.toInt();
    //else
    //if (currentKey == "dew_point") daily->dew_point[arrayIndex] = value.toFloat();
    //else
    //if (currentKey == "clouds") daily->clouds[arrayIndex] = value.toInt();
    //else
    //if (currentKey == "wind_speed") daily->wind_speed[arrayIndex] = value.toFloat();
    //else
    //if (currentKey == "wind_gust") daily->wind_gust[arrayIndex] = value.toFloat();
    //else
    //if (currentKey == "wind_deg") daily->wind_deg[arrayIndex] = (uint16_t)value.toInt();
    //else
    //if (currentKey == "rain") daily->rain[arrayIndex] = value.toFloat();
    //else
    //if (currentKey == "snow") daily->snow[arrayIndex] = value.toFloat();
    //else

    if (currentKey == "id") daily->id[arrayIndex] = value.toInt();
    //else
    //if (currentKey == "main") daily->main[arrayIndex] = value;
    //else
    //if (currentKey == "description") daily->description[arrayIndex] = value;
    //else
    //if (currentKey == "icon") daily->icon[arrayIndex] = value;

    if (currentSet == "temp") {
      //if (currentKey == "morn") daily->temp_morn[arrayIndex] = value.toFloat();
      //else
      //if (currentKey == "day") daily->temp_day[arrayIndex] = value.toFloat();
      //else
      //if (currentKey == "eve") daily->temp_eve[arrayIndex] = value.toFloat();
      //else
      //if (currentKey == "night") daily->temp_night[arrayIndex] = value.toFloat();
      //else
      if (currentKey == "min") daily->temp_min[arrayIndex] = value.toFloat();
      else
      if (currentKey == "max") daily->temp_max[arrayIndex] = value.toFloat();
    }

    //if (currentSet == "feels_like") {
      //if (currentKey == "morn") daily->feels_like_morn[arrayIndex] = value.toFloat();
      //else
      //if (currentKey == "day") daily->feels_like_day[arrayIndex] = value.toFloat();
      //else
      //if (currentKey == "eve") daily->feels_like_eve[arrayIndex] = value.toFloat();
      //else
      //if (currentKey == "night") daily->feels_like_night[arrayIndex] = value.toFloat();
    //}

    return;
  }
}

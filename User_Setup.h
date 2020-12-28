// Configuration settings for OpenWeather library


// These parameters set the data point count stored in program memory (not the datapoint
// count sent by the server). So they determine the memory used during collection
// of the data points.

#define MAX_HOURS 6    // Maximum "hourly" forecast period, can be up 1 to 48
                       // Hourly forecast not used by TFT_eSPI_OpenWeather example

#define MAX_DAYS 5     // Maximum "daily" forecast periods can be 1 to 8 (Today + 7 days = 8 maximum)
                       // TFT_eSPI_OpenWeather example requires this to be >= 5 (today + 4 forecast days)

#define LOG_UNNECESSARY_INFO // Define if you want to check serial messages of progress
#define LOG_ERRORS // Define if you want to check serial messages of errors
//#define SHOW_HEADER   // Debug only - for checking response header via serial message
//#define SHOW_JSON     // Debug only - simple serial output formatting of whole JSON message
//#define SHOW_CALLBACK // Debug only to show the decode tree


// ###############################################################################
// DO NOT tinker below, this is configuration checking that helps stop crashes:
// ###############################################################################

// Check and correct bad settings
#if (MAX_HOURS > 48) || (MAX_HOURS < 1)
  #undef  MAX_HOURS
  #define MAX_HOURS 48 // Ignore compiler warning!
#endif

#if (MAX_DAYS > 8) || (MAX_DAYS < 1)
  #undef  MAX_DAYS
  #define MAX_DAYS 8  // Ignore compiler warning!
#endif

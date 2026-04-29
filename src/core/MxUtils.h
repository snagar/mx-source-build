#ifndef MXUTILS_H_
#define MXUTILS_H_
#pragma once

#include <iomanip>
#include <list>
// v25.08.1 used with PyDict
#include <iostream>
#include <unordered_map>
#include <variant>
#include <string>
#include <optional>
#include <random>
// #include <concepts>

#ifdef APL
#include <cstdlib>
#endif

#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"

#ifndef IBM
// #include "base_c_includes.h" // v3.303.14 removed
  #include <algorithm>
  // #include <math.h>
  #include <cmath>
#endif

#include "xx_mission_constants.hpp"
#include "mx_return.hpp"

//#include "XPLMPlugin.h"
//#include "XPGL.h" // The header that defines the XPGL struct


namespace missionx
{

// 1. Declare the function pointer at the global or class scope
//    The X-Plane SDK provides the necessary function pointer type
//    in its headers, but you can define it yourself if needed.
//typedef void (APIENTRY *PFNGLGENERATEMIPMAPPROC) (GLenum target);
//static PFNGLGENERATEMIPMAPPROC glGenerateMipmap = NULL;

using PyValue = std::variant<int, double, std::string, bool>;

class PyDict {
  std::unordered_map<std::string, PyValue> data;

public:
  // Insert like Python dict: dict["key"] = value
  PyValue& operator[](const std::string& key) {
    return data[key];
  }

  // --- Safe type check ---
  template <typename T>
  bool is_type(const std::string& key) const {
    const auto it = data.find(key);
    return (it != data.end()) && std::holds_alternative<T>(it->second);
  }

  // --- Safe get (returns optional) ---
  template <typename T>
  std::optional<T> get_if(const std::string& key) {
    if (const auto it = data.find(key); it != data.end()) {
      if (auto p = std::get_if<T>(&(it->second))) {
        return *p;
      }
    }
    return std::nullopt;
  }

  // --- Get with default ---
  template <typename T>
  T get(const std::string& key, const T& default_val) {
    if (auto val = get_if<T>(key)) {
      return *val;
    }
    return default_val;
  }
};







// A simple and basic equation calculation based on the basic algebra
class calc
{
  // based on https://stackoverflow.com/questions/9329406/evaluating-arithmetic-expressions-from-string-in-c
public:
  bool        isValidExpression{ true };
  std::string expressionToParse_s = "";
  const char* expressionToParse;

  void setExpression(const std::string &inExprestion)
  {
    expressionToParse_s = inExprestion;
    expressionToParse   = expressionToParse_s.c_str();
  }

  calc(const std::string& inExp) { setExpression(inExp);  }
  ~calc(){}

  double calculateExpression() { return expression(); }

private:
  char peek() { return *expressionToParse; }

  char get() { return *expressionToParse++; }


  double number()
  {
    double result = get() - '0';

    while (peek() >= '0' && peek() <= '9')
    {
      result = 10 * result + get() - '0';
    }

    if (peek() == '.') // add decimal number
    {
      get();
      int    result_decimal     = 0;
      //int    numbers_in_decimal = 0; // how many decimal numbers are there // no real need since we immediately calculate the "divide" factor
      double divide_by          = 1.0;

      while (peek() >= '0' && peek() <= '9')
      {
        result_decimal = 10 * result_decimal + get() - '0';
        //numbers_in_decimal++;
        divide_by *= 10;
      }
      // Add the right side number to the left side one
      double dec = result_decimal / divide_by;
      result += dec;
    }


    return result;
  }

  double factor()
  {
    if (peek() >= '0' && peek() <= '9')
      return number();
    else if (peek() == '(')
    {
      get(); // '('
      double result = expression();
      get(); // ')'
      return result;
    }
    else if (peek() == '-')
    {
      get();
      return -factor();
    }
    return 0.0; // error
  }

  double term()
  {
    double result = factor();
    while (peek() == '*' || peek() == '/')
      if (get() == '*')
        result *= factor();
      else
        result /= factor();
    return result;
  }

  double expression()
  {
    double result = term();
    while (peek() == '+' || peek() == '-')
      if (get() == '+')
        result += term();
      else
        result -= term();
    return result;
  }
};


class mxUtils
{
public:
  mxUtils();
//  ~mxUtils(); // https://stackoverflow.com/questions/51863588/warning-definition-of-implicit-copy-constructor-is-deprecated


  // Plugin start
  void plugin_start ();

  // Members
  static std::string stringToUpper(std::string strToConvert);
  static std::string stringToLower(std::string strToConvert);
  static bool        is_alpha(const std::string& str);
  static bool        is_digits(const std::string& str);
  static bool        is_number(const std::string& s);
  static bool        isNumeric(const std::string& str);
  static bool        isScientific(const std::string& str);
  static double      convertScientificToDecimal(const std::string& str);
  static std::string ltrim(std::string str, const std::string chars = std::string("\t\n\v\f\r "));
  static std::string rtrim(std::string str, const std::string chars = std::string("\t\n\v\f\r "));
  static std::string trim(std::string str, const std::string chars = std::string("\t\n\v\f\r "));
  static int         countCharsInString(const std::string& inText, const char& inCharToCount); // v3.303.9.1
  static bool        compare (const std::string &inStr1, const std::string &inStr2, bool inCaseSensitive = true ); // v24.05.1
  static size_t      find_text (const std::string &in_source, const std::string &in_search_text, bool inCaseSensitive = true ); // v25.06.1 find text with ignore case
  static std::string format (const std::string & inText, std::map<int, std::string> & inArgs); // v24.05.2
  static int         calc_minutes_from_seconds(const int &in_seconds_from_midnight); // v25.04.2

  static bool isStringBool(std::string inTestValue, bool& outStringEvalResult_asBool);
  // -------------------------------------------
  // v3.0.255.1
  static std::string remove_char_from_string(const std::string& inVal, const char inCharToRemove); // remove specific char from a given string. Return a new strin without this specific char
  static std::string sanitize_text (const std::string &inText, const std::string &chars_to_replace = " \t\n\r\f", const char c_replace_with = '_');
  // -------------------------------------------
  static std::vector<std::string> split(const std::string& s, char delimiter = ' ', const bool bKeepEmptyTokens = false); // v3.0.219.12 from https://www.fluentcpp.com/2017/04/21/how-to-split-a-string-in-c/
  static std::vector<std::string> split_v2(const std::string& text, const std::string& delimiter, const bool bKeepEmptyTokens = true); // v25.06.1 removed delimiter = " \t\n\r\f"
  //static std::vector<std::string> split_skipEmptyTokens(const std::string& text, const std::string& delimeter = " \t\n\r\f");
  static std::vector<std::string> split_skipEmptyTokens(const std::string& text, const char& delimiter = ' ');


  static std::string translateMxPadLabelPositionToValid(std::string inLabelPosition);

  static std::string                  translateMessageChannelTypeToString(mx_message_channel_type_enum mType);
  static mx_message_channel_type_enum translateMessageTypeToEnum(std::string& inType);

  static enums::mx_rnd_task_type translate_mission_type_to_task_type(const std::string& inType); // v25.10.1
  static missionx::mx_ui_mission_type translate_mission_type_to_med_cargo_or_oilrig_task_type(const std::string& inType); // v25.10.1

  static std::string emptyReplace(std::string inValue, std::string inReplaceWith);

  static float       convert_skewed_bearing_to_degrees(const float inBearing);                          // convert the bearing number to valid degree between 0-359
  static float       convert_skewed_bearing_to_degrees(const std::string inBearing_s);                  // convert the bearing number to valid degree between 0-359
  static std::string convert_skewed_bearing_to_degrees_return_as_string(const std::string inBearing_s); // convert the bearing number to valid degree between 0-359

 // ------------------------------------

  //// Templates //////// Templates //////// Templates ////

 // ------------------------------------

  template<class Container>
  static bool isElementExists(Container& inMap, const typename Container::key_type& key)
  {
    if (inMap.find(key) != inMap.end())
      return true;

    return false;
  }

 // ------------------------------------

  template<class Container>
  static bool getElementValOrAlternative(Container& inMap, const typename Container::key_type& key, typename Container::mapped_type inAlternative)
  {
    if (inMap.find(key) != inMap.end())
      return inMap[key];

    return inAlternative;
  }

 // ------------------------------------
  /// v3.0.255.3 This template search and return the "key" value only if exists. If not then it returns "default value) // should have been defined many version back.
  // http://www.cppblog.com/mzty/archive/2005/12/14/1728.html
  template<class Container>
  static typename Container::mapped_type getValueFromElement(Container& inMap, typename Container::key_type key, typename Container::mapped_type default_value)
  {
    if (inMap.find(key) != inMap.end())
      return inMap[key];

    return default_value;
  }


 // ------------------------------------

  template<class Container, typename T>
  static T getElementValue(Container& inMap, const typename Container::key_type key, T optional_value)
  {

    if (inMap.find(key) != inMap.end())
      return inMap[key]; // return value

    return optional_value;
  }



 // ------------------------------------
  template<typename T>
  static bool isElementExistsInList(std::list<T>& inContainer, T& key) // v3.0.205.2
  {
    auto it = std::find(inContainer.begin(), inContainer.end(), key);
    if (it != inContainer.end())
      return true;

    return false;
  }


 // ------------------------------------
  template<typename T>
  static bool isElementExistsInVec(const std::vector<T>& inContainer, const T& value) // v3.0.205.2
  {
    auto it = std::find(inContainer.begin(), inContainer.end(), value);
    if (it != inContainer.end())
      return true;

    return false;
  }


 // ------------------------------------
  static bool isStringExistsInVec(const std::vector<std::string>& inContainer, const std::string& inValue);
  

 // ------------------------------------
  template<class Container>
  static void purgeQueueContainer(Container& inContainer)
  {
    while (!inContainer.empty())
      inContainer.pop();

  }
 // ------------------------------------

  template<typename T>
  static bool isStringNumber(T& t, const std::string& s, std::ios_base& (*f)(std::ios_base&))
  {
    std::istringstream iss(s);

    return !(iss >> f >> t).fail();
  }
  // isStringNumber

 // ------------------------------------

  // Convert String into Number
  template<typename N>
  static N stringToNumber(std::string s)
  {
    std::istringstream is(s);
    N                  n;
    is >> n;

    return n;
  }
 // ------------------------------------

  // Convert String into Number with precision
  template<typename N>
  static N stringToNumber(std::string s, size_t howManyDigitsToKeepInTheNumber)
  {
    std::istringstream is(s);
    N                  n;
    std::string        num;
    num.clear();

    if (std::is_arithmetic<N>::value)
    {
      for (size_t i = 0; i < howManyDigitsToKeepInTheNumber; i++)
      {
        char c = '\0';
        is.get(c);
        num += c;
      }
      std::istringstream isNum(num);
      isNum >> n;

      return n;
    }

    return static_cast<N>(0);
  }


  // -------------------------------------------
  template<typename N>
  static int getPrecision(N n)
  {
    int count = 0;

    if (std::is_floating_point<N>::value)
    {
#ifdef IBM
      n = std::abs(n);
#else
      n = fabs(n);
#endif
      n = n - int(n);                     // remove left of decimal.
      while (n >= 0.0000001 && count < 7) // limit precision to 8
      {
        n = n * 10;
        count++;
        n = n - int(n); // remove left of decimal
      }
    } // if floating point


    return count;
  }

  // -------------------------------------------

  // Number To String Template - NO formatting
  template<typename N>
  static std::string formatNumber(N inVal)
  {
    std::ostringstream o;
    if (!(o << inVal))
    {
      // std::snprintf ( missionx::LOG_BUFF, LOG_BUFF_SIZE, " The number %f, could not be converted.", (N)inVal );
      // log("The number could not be converted.");
      return EMPTY_STRING;
    }

    return o.str();
  }

  // -------------------------------------------

  // Number to String Template - WITH precision formatting
  template<typename N>
  static std::string formatNumber(N inVal, int precision)
  {
    std::ostringstream o;
    if (!(o << std::setiosflags(std::ios::fixed) << std::setprecision(precision) << inVal))
    {
      std::snprintf(missionx::LOG_BUFF, sizeof(missionx::LOG_BUFF), " The number %f, could not be converted.", (float)inVal);
      XPLMDebugString(missionx::LOG_BUFF);
      return EMPTY_STRING;
    }


    return o.str();
  }

  // -------------------------------------------

  static missionx::mx_btn_colors translateStringToButtonColor(std::string inColor);

  // -------------------------------------------
  static std::string getFreqFormated(const int freq);

  // -------------------------------------------

  // based on https://stackoverflow.com/questions/3418231/replace-part-of-a-string-with-another-string
  static std::string replaceAll(std::string str, const std::string& from, const std::string& to);

  // Function to convert NM to DEG for longitude, adjusted by latitude
  static double nmToDegLon(double nm, double latDegrees);

  //#include <regex>
  //    ...
  //      std::string string("hello $name");
  //    string = std::regex_replace(string, std::regex("\\$name"), "Somename");


  // -------------------------------------------
  template<typename T>
  static void eval_min_max(const T& inVal, T& outMin, T& outMax)
  {
    if (inVal > outMin)
    {
      if (inVal > outMax)
        outMax = inVal;
    }
    else
      outMin = inVal;
  }

  // -------------------------------------------
  template<typename T>
  static T mx_min(const T inVal1, T inVal2)
  {
    if (inVal1 < inVal2)    
      return inVal1;    
    
    return inVal2;
  }
  // -------------------------------------------

  template<typename T>
  static T mx_max(const T inVal1, T inVal2)
  {
    if (inVal1 > inVal2)    
      return inVal1;    
    
    return inVal2;
  }
  // -------------------------------------------

  // v25.10.1 make sure that inVal1 is smaller than inVal2
  template<typename T>
  static void mx_eval_min_max(T &inVal1, T &inVal2)
  {
    static_assert(std::is_arithmetic_v<T>, "T must be a numeric type");

    if (inVal1 > inVal2)
    {
      std::swap(inVal1, inVal2);
    }
  }
  // -------------------------------------------
  // v25.10.1 make sure that inVal1 is smaller than inVal2, use a rule to calculate minVal1 if they are equal.
  template<typename T>
  static void mx_eval_min_always_smaller_than_max(T &inVal1, T &inVal2, const T &in_percent_smaller = 0.5)
  {
    static_assert(std::is_arithmetic_v<T>, "T must be a numeric type");

    const T tmp_val = mx_min(inVal1, inVal2);

    if (inVal1 > inVal2)
    {
      std::swap(inVal1, inVal2);
    }

    // handle equal cases
    if (inVal1 == inVal2 && inVal1 > 0.0)
    {
      inVal1 *= in_percent_smaller;
    }

    if (inVal1 == inVal2 && inVal1 < 0.0)
    {
      inVal2 *= in_percent_smaller;
    }

  }
  // -------------------------------------------
  // This is the most "bulletproof" version for cross-platform (Win/Mac/Linux)
  template <typename T, typename... Args>
  static bool mx_in(const T& target, const Args&... args) {
    return ((target == args) || ...);
  }

  // -------------------------------------------

  // Test if value is "equal or greater" than Min and "equal or lower" than max
  template<typename T>
  static bool mx_between(const T in_value_to_test, const T inLow, T inMax, const missionx::enums::mx_between_types inType)
  {
    switch (inType)
    {
      case missionx::enums::mx_between_types::gt_min_eqles_max:
      {
        if ( in_value_to_test > inLow && in_value_to_test <= inMax)
          return true;
      }
      break;
      case missionx::enums::mx_between_types::gt_min_less_max:
      {
        if ( in_value_to_test > inLow && in_value_to_test < inMax)
          return true;
      }
      break;
      case missionx::enums::mx_between_types::both_can_be_equal:
      {
        if ( in_value_to_test >= inLow && in_value_to_test <= inMax)
          return true;
      }
      break;
      case missionx::enums::mx_between_types::eqgt_min_less_max:
      {
        if ( in_value_to_test >= inLow && in_value_to_test < inMax)
          return true;
      }
      break;
      default:
        break;
    }

    return false;
  }
  // -------------------------------------------

  static void mxCalcPointBasedOnDistanceInMetersAndBearing_2DPlane(double& outLat, double& outLon, const double inLat, const double inLon, const float inHdg_deg, const double inDistance_meters, const double planet_great_circle_in_meters = (double)missionx::EARTH_RADIUS_M);

  // -------------------------------------------

  double static mxCalcBearingBetween2Points(const double gFromLat, const double gFromLong, const double gTargetLat, const double gTargetLong);
  // -------------------------------------------
  bool static mxIsPointInPolyArea(const std::vector<missionx::mxVec2d> inPolyArea, const missionx::mxVec2d inPoint);
  // -------------------------------------------
  
  // Calculate distance between points, default returning units "nautical miles", planet great circle is received in meters
  double static mxCalcDistanceBetween2Points(const double gFromLat, const double gFromLong, const double gTargetLat, const double gTargetLong,
                                              const missionx::mx_units_of_measure inReturnInUnits = missionx::mx_units_of_measure::nm, const double planet_great_circle_in_meters = missionx::EARTH_RADIUS_M); // calculate distance between 2 points in Nauticle Miles  
  // -------------------------------------------
  static std::string mx_translateDrefTypeToString(const XPLMDataTypeID &inType);
  // -------------------------------------------
  // -------------------------------------------
  static missionx::mxRGB hexToRgb(std::string hexValue_s, const float inModifier = 1.0);
  static missionx::mxRGB hexToNormalizedRgb(std::string hexValue_s, const float inModifier = 1.0);
  // -------------------------------------------
  static missionx::mx_fetch_info fetch_next_string(const std::string& inText, const size_t inOffset, std::string inDelimiter);
  // -------------------------------------------
  static std::string remove_quotes(const std::string inString, bool bFromBeginning = true, bool bFromEnd = true);
  // -------------------------------------------

  // v3.305.3
 #if defined(__cpp_lib_char8_t)
  // https://stackoverflow.com/questions/56833000/c20-with-u8-char8-t-and-stdstring
  static std::string from_u8string(const std::u8string &s);
 #endif
  static std::string from_u8string(const std::string &s);
  static std::string from_u8string(std::string &&s);
  static std::map<char, size_t> getHowManyDelimitersInString(const std::string& inLine, const std::string delimiters);

  static int coord_in_rect(float x, float y, const float bounds_ltrb[4]);

  static std::string get_mx_osm_region_trans (missionx::enums::mx_osm_region_enum inRegion);

  // -------------------------------------------
  static bool check_file_exists (const std::string& in_path);
  static bool check_folder_exists (const std::string& in_path);

  static std::string eval_text(bool in_has_value, const std::string &in_preferred_outcome, const std::string &in_fallback_outcome);

  static std::mt19937 create_random_engine();

  // Copy a string to array buffer
  static void reset_buffer (char& out_value_array, size_t in_value_array_size, const char &in_reset_char = '\0');
  static size_t copy_string_to_buffer (const std::string &in_source, char& out_value_array, const size_t in_value_array_size);


  // -------------------------------------------

  template <typename C, typename K>
  static bool is_element_in_container(const C& container, const K& key)
  {
    // We use std::begin/end for maximum compatibility
    return std::find(std::begin(container), std::end(container), key) != std::end(container);
  }

  // -------------------------------------------
  // -------------------------------------------
  // -------------------------------------------
};

} // namespace missionx

#endif // !MXUTILS_H_

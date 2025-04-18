#include "MxUtils.h"

#ifdef IBM
  #include <algorithm>
#endif

#ifdef MAC
#include <cmath> // solve std::remindar in XCode
#endif

missionx::mxUtils::mxUtils() {}

//missionx::mxUtils::~mxUtils() {}


/*
THE FOLLOWING FUNCTIONS WERE COPIED FROM "uTILS" CLASS SO PROPERTIES WILL BE INDEPENDENT FOR ANY CLASS
*/


/* ********************************************** */
// std::string str_toupper(std::string s) {
//  std::transform(s.begin(), s.end(), s.begin(),
//    [](unsigned char c) { return std::toupper(c); } // correct
//  );
//  return s;
//}

std::string
missionx::mxUtils::stringToUpper(std::string strToConvert)
{ // change each element of the string to upper case
  // for (size_t i = 0; i<strToConvert.length(); i++)
  //{
  //  strToConvert[i] = toupper(strToConvert[i]);
  //}
  // return strToConvert;//return the converted string
  std::for_each(strToConvert.begin(), strToConvert.end(), [](char& c) { c = toupper(c); });
  return strToConvert;
}

// ----------------------------------------------

std::string
missionx::mxUtils::stringToLower(std::string strToConvert)
{ // change each element of the string to lower case
  //for (size_t i = 0; i < strToConvert.length(); i++)
  //{
  //  strToConvert[i] = tolower(strToConvert[i]);
  //}
  for (auto& c : strToConvert)
    c = tolower(c);

  return strToConvert; // return the converted string
}


// ----------------------------------------------

bool
missionx::mxUtils::is_alpha(const std::string& str)
{
  return std::all_of(str.begin(), str.end(), ::isalpha); // C++11
}

// ----------------------------------------------

bool
missionx::mxUtils::is_digits(const std::string& str)
{
  // return str.find_first_not_of("0123456789.") == std::string::npos;
  return std::all_of(str.begin(), str.end(), ::isdigit); // C++11
}

// ----------------------------------------------

// Is string has only these characters - "0123456789.", and no more than 1 "." !!!
bool
missionx::mxUtils::is_number(const std::string& s)
{
  bool validNumber = true;

  bool minusPlusNotAllowed = false; // as long as we did not found digits, we can use "+/-" signs

  bool foundDecimalDot = false;

  std::string::const_iterator it = s.begin();
  while (it != s.end() && validNumber)
  {
    // number is legal if starts with "+/-" signs
    if (!minusPlusNotAllowed && (((*it) == '+') || ((*it) == '-')))
    {
      minusPlusNotAllowed = true; // only one sign is allowed at the begining
      ++it;
    }
    else if ((*it) == '.' && !foundDecimalDot)
    {
      foundDecimalDot     = true;
      minusPlusNotAllowed = true;
      ++it;
    }
    else if (::isdigit(*it))
    {
      minusPlusNotAllowed = true;
      ++it;
    }
    else
      validNumber = false;
  } // end while

  return !s.empty() && it == s.end();
}

bool missionx::mxUtils::isNumeric(const std::string& str){
  std::istringstream iss(str);
  double number;
  iss >> number;
  return iss.eof() && !iss.fail();
}

bool missionx::mxUtils::isScientific(const std::string& str){
  std::istringstream iss(str);
  double number;
  iss >> std::scientific >> number;
  return iss.eof() && !iss.fail();

}

double missionx::mxUtils::convertScientificToDecimal(const std::string& str){
  double number = 0.0;
  std::istringstream iss(str);
  iss >> std::scientific >> number;
  return number;
}

// ----------------------------------------------

std::string
missionx::mxUtils::ltrim(std::string str, const std::string chars)
{
  // trim leading spaces
  size_t startpos = str.find_first_not_of(chars); //(" \t");
  if (std::string::npos != startpos)
  {
    str = str.substr(startpos);
  }

  return str;
} // ltrim

// ----------------------------------------------
std::string
missionx::mxUtils::rtrim(std::string str, const std::string chars)
{
  str.erase(str.find_last_not_of(chars) + 1);
  return str;
}

// ----------------------------------------------
std::string
missionx::mxUtils::trim(std::string str, const std::string chars)
{
  return ltrim(rtrim(str, chars), chars);
}

// ----------------------------------------------

int
missionx::mxUtils::countCharsInString(const std::string& inText, const char& inCharToCount)
{
  int counter = 0;
  for (const auto& c : inText)
    counter += (c == inCharToCount) ? 1 : 0;

  return counter;
}

// ----------------------------------------------

bool missionx::mxUtils::compare(const std::string &inStr1, const std::string &inStr2, const bool inCaseSensitive)
{
  if (inCaseSensitive)
    return inStr1 == inStr2;

  return mxUtils::stringToLower(inStr1) == mxUtils::stringToLower(inStr2); // case-insensitive check
}

// ----------------------------------------------

std::string missionx::mxUtils::format(const std::string &inText, std::map<int, std::string> &inArgs)
{
  bool flagFoundLeftCurlyBraces = false;
  std::string pendingString;
  std::stringstream ss;
  auto iter_inArgs = inArgs.cbegin();
  int curly_braces_counter=0;

  for (const char c : inText )
  {
    switch (c) {
    case '{':
    {
      if (flagFoundLeftCurlyBraces)
      {
        ss << pendingString;
        pendingString.clear();
      }
      flagFoundLeftCurlyBraces = true;

    } // end "{"
    break;
    case '}':
    {
      if (pendingString.empty() ) // if we found "{}" without a number
      {
        if (iter_inArgs != inArgs.end())
        {
          ss << iter_inArgs->second;
          ++iter_inArgs; // v25.03.3
        }
        else
        {
          ss << "{" << curly_braces_counter << "}"; // v25.03.3
        }
      }
      else if ( std::ranges::all_of (pendingString, ::isdigit) )
      {
        int key = std::stoi (pendingString);
        if (mxUtils::isElementExists (inArgs, key) )
        {
          ss << inArgs[key];
        }
        else {
          ss << "{" <<  pendingString << c;
        }

        if (iter_inArgs != inArgs.end ())
          ++iter_inArgs; // v25.03.3
      }
      else // if {N} is not digit then concatenate as is
      {
        ss << "{" << pendingString << c;
      }

      flagFoundLeftCurlyBraces = false;
      pendingString.clear ();
      ++curly_braces_counter; // v25.03.3

    } // end "}"
    break;
    default:
    {
      if (flagFoundLeftCurlyBraces)
      {
        pendingString += c;
      }
      else
      {
        ss << c;
      }
    }
    break;
    } // switch

  } // end loop

  if ( ! pendingString.empty() )
  {
    ss << pendingString;
  }

  return ss.str();
}

// ----------------------------------------------


bool
missionx::mxUtils::isStringBool(std::string inTestValue, bool& outStringEvalResult_asBool)
{
  const std::string inValue = stringToLower(inTestValue);

  if (inValue.length() == 1)
  {
//    char c = inValue[0];
    // flag_is_bool = (bool)c;

    switch (inValue[0])
    {
      case '1':
      case 'y':
      {
        outStringEvalResult_asBool = true;
        return true;
      }
      break;
      case '0':
      case 'n':
      {
        outStringEvalResult_asBool = false;
        return true;
      }
      break;
      default:
        break;
    }

    outStringEvalResult_asBool = false;
    return false; // we can use this one char value as bool
  }
  else if ((mxconst::get_MX_TRUE().compare(inValue) == 0) || (mxconst::get_MX_YES().compare(inValue) == 0))
  {
    outStringEvalResult_asBool = true; // true for boolean or numbers
    return true;
  }
  else if ((mxconst::get_MX_FALSE().compare(inValue) == 0) || (mxconst::get_MX_NO().compare(inValue) == 0))
  {
    outStringEvalResult_asBool = false; // false for boolean or numbers
    return true;                    // special case where the value is boolean but its value is false, we need to return if the value was found from function
  }
  else if (is_number(inValue))
  {
    outStringEvalResult_asBool = stringToNumber<bool>(inValue);
    return true; // special case // number can be 0 or minus
  }
  else
  {
    // fail to parse attrib string to number. Setting property to false;
    outStringEvalResult_asBool = false;
  }

  return false;
}

// ----------------------------------------------

std::string
missionx::mxUtils::remove_char_from_string(const std::string& inVal, const char inCharToRemove)
{
  // The following function will return a new string without the specific character. This character will be removed from all the string
  std::string retVal_s;
  for (auto& c : inVal)
  {
    if (c == inCharToRemove)
      continue;

    retVal_s.push_back(c);
  }

  return retVal_s;
}

// ----------------------------------------------

std::vector<std::string>
missionx::mxUtils::split(const std::string& s, char delimiter, const bool bKeepEmptyTokens)
{
  std::vector<std::string> tokens;
  std::string              token;
  std::istringstream       tokenStream(s);
  while (std::getline(tokenStream, token, delimiter))
  {
    if ( (bKeepEmptyTokens == false) && token.empty()) // v3.303.12 added bKeepEmptyTokens
      continue;

    tokens.push_back(token);
  }
  return tokens;
}

// ----------------------------------------------

std::vector<std::string>
missionx::mxUtils::split_v2(const std::string& text, const std::string& delimeter, const bool bKeepEmptyTokens)
{
  std::vector<std::string> vecTokens;
  std::string              token;
  for (const auto& c : text)
  {
    //if (delimeter.find(c) != std::string::npos && (false == token.empty())) // if we found delimiter and we have a token
    if (delimeter.find(c) != std::string::npos ) // if we found delimiter and we have a token
    {
      if (token.empty() && !bKeepEmptyTokens) // v3.305.4 should we keep empty tokens ? Original code kept them
        continue;

      vecTokens.push_back(token);
      token.clear();
    }
    else
    {
      token.push_back(c);
    }
  }

  if ( ! token.empty()) // add last part of the 
    vecTokens.push_back(token);

  return vecTokens;
}

// ----------------------------------------------

std::vector<std::string>
//missionx::mxUtils::split_skipEmptyTokens(const std::string& text, const std::string& delimeter)
missionx::mxUtils::split_skipEmptyTokens(const std::string& text, const char& delimeter)
{
  std::vector<std::string> vecTokens;
  std::string              token;
  for (const auto& c : text)
  {
    // if (delimeter.find(c) != std::string::npos && (false == token.empty())) // if we found delimiter and we have a token
    if (delimeter == c ) // if we found delimiter and we have a token
    {
      if (token.empty() ) // skip empty tokens
        continue;

      vecTokens.push_back(token);
      token.clear();
    }
    else
    {
      token.push_back(c);
    }
  }

  if (!token.empty()) // add last part of the
    vecTokens.push_back(token);

  return vecTokens;
}

// ----------------------------------------------

std::string
missionx::mxUtils::translateMxPadLabelPositionToValid(std::string inLabelPosition)
{
  std::string mLabelPosition;
  // v3.0.197 - translate position string to valid one
  mLabelPosition = mxUtils::stringToUpper(inLabelPosition);
  if (mLabelPosition.compare("L") == 0 || mLabelPosition.compare("LEFT") == 0)
    mLabelPosition = "L";
  else if (mLabelPosition.compare("R") == 0 || mLabelPosition.compare("RIGHT") == 0)
    mLabelPosition = "R";
  else if (!mLabelPosition.empty())
    mLabelPosition = "L";


  return mLabelPosition;
}

// ----------------------------------------------


std::string
missionx::mxUtils::translateMessageChannelTypeToString(mx_message_channel_type_enum mType)
{
  switch (mType)
  {
    case mx_message_channel_type_enum::comm:
      return mxconst::get_CHANNEL_TYPE_COMM();
      break;
    case mx_message_channel_type_enum::background:
      return mxconst::get_CHANNEL_TYPE_BACKGROUND();
      break;
    // v3.0.194 deprecate pad
    // case mx_message_channel_type_enum::pad:
    //  return mxconst::CHANNEL_TYPE_PAD;
    //  break;
    default:
      break;
  }

  return "Not valid type";
}

missionx::mx_message_channel_type_enum
missionx::mxUtils::translateMessageTypeToEnum(std::string& inType)
{
  inType = stringToLower(inType);
  if (mxconst::get_CHANNEL_TYPE_COMM().compare(inType) == 0)
    return missionx::mx_message_channel_type_enum::comm;
  else if (mxconst::get_CHANNEL_TYPE_BACKGROUND().compare(inType) == 0)
    return missionx::mx_message_channel_type_enum::background;
  // v3.0.194 deprecate pad
  // else if (CHANNEL_TYPE_PAD.compare(inType) == 0)
  //  return missionx::mx_message_channel_type_enum::pad;


  return missionx::mx_message_channel_type_enum::no_type;
}

// ----------------------------------------------

std::string
missionx::mxUtils::emptyReplace(std::string inValue, std::string inReplaceWith)
{
  if (inValue.empty())
    return inReplaceWith;

  return inValue;
}

// ----------------------------------------------

float
missionx::mxUtils::convert_skewed_bearing_to_degrees(const float inBearing)
{
  if (inBearing >= 0.0f && inBearing <= 360.0f)
    return inBearing;

  // const auto reminder = std::remainder(inBearing, missionx::DEGREESE_IN_CIRCLE); // 360.0f represent 360 degrees in a circle.
  // const float correct_bearing_f = (reminder < 0)? reminder + missionx::DEGREESE_IN_CIRCLE : reminder;

  // const float correct_bearing_f_v2 = fmod(inBearing, 360.0f);

  // return correct_bearing_f;
  return fmod(inBearing, 360.0f);
}

// ----------------------------------------------

float
missionx::mxUtils::convert_skewed_bearing_to_degrees(const std::string inBearing_s)
{
  // const float bearing_f = mxUtils::stringToNumber<float>(inBearing_s, 3);
  // return missionx::mxUtils::convert_skewed_bearing_to_degrees(bearing_f);

  return fmod(mxUtils::stringToNumber<float>(inBearing_s, 3), 360.0f);
}

// ----------------------------------------------

std::string
missionx::mxUtils::convert_skewed_bearing_to_degrees_return_as_string(const std::string inBearing_s)
{
  // auto bearing_f = mxUtils::convert_skewed_bearing_to_degrees(inBearing_s);
  return mxUtils::formatNumber<float>(mxUtils::convert_skewed_bearing_to_degrees(inBearing_s), 3);
}


// ----------------------------------------------


bool
missionx::mxUtils::isStringExistsInVec(const std::vector<std::string>& inContainer, const std::string& inValue)
{
  return (std::find(inContainer.begin(), inContainer.end(), inValue) != inContainer.end());
}

// ----------------------------------------------

missionx::mx_btn_colors
missionx::mxUtils::translateStringToButtonColor(std::string inColor)
{
  // mx_btn_colors color_val;

  if (mxconst::get_YELLOW().compare(inColor) == 0)
    return missionx::mx_btn_colors::yellow;
  else if (mxconst::get_RED().compare(inColor) == 0)
    return missionx::mx_btn_colors::red;
  else if (mxconst::get_GREEN().compare(inColor) == 0)
    return missionx::mx_btn_colors::green;
  else if (mxconst::get_BLUE().compare(inColor) == 0)
    return missionx::mx_btn_colors::blue;
  else if (mxconst::get_BLACK().compare(inColor) == 0)
    return missionx::mx_btn_colors::black;

  return missionx::mx_btn_colors::white;
}

// ----------------------------------------------

std::string
missionx::mxUtils::getFreqFormated(const int freq)
{
  const std::string freq_s = mxUtils::formatNumber<int>(freq);
  switch (freq_s.length())
  {
    case 5:
    case 6:
    {
      return (freq_s.substr(0, 3) + "." + freq_s.substr(3));
    }
    break;
    default:
      break;

  } // end switch

  return freq_s;
}

// ----------------------------------------------

std::string
missionx::mxUtils::replaceAll(std::string str, const std::string& from, const std::string& to)
{
  if (!from.empty())
  {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
  } // end if "from" has value

  return str;
}

// ----------------------------------------------

void
missionx::mxUtils::mxCalcPointBasedOnDistanceAndBearing_2DPlane(double& outLat, double& outLon, const double inLat, const double inLon, const float inHdg_deg, const double inDistance_meters, const double planet_great_circle_in_meters)
{
  double epsilon = 0.000001; // # threshold for floating-point equality

  // Convert info to radians
  double rlat1    = DegToRad * inLat; //
  double rlon1    = DegToRad * inLon;
  float  rbearing = DegToRad * (360 - inHdg_deg);        // Fix heading with X-Plane
  double distance = inDistance_meters / planet_great_circle_in_meters;   //missionx::EARTH_RADIUS_M; // EARTH_AVG_RADIUS_NM; // # normalize linear distance to radian slope_angle

  double rlat = asin(sin(rlat1) * cos(distance) + cos(rlat1) * sin(distance) * cos(rbearing));
  double rlon = 0.0;

  if (cos(rlat) == 0 || fabs(cos(rlat)) < epsilon) //# Endpoint a pole
    rlon = rlon1;
  else
  {
    double dlon = atan2(sin(rbearing) * sin(distance) * cos(rlat1), cos(distance) - sin(rlat1) * sin(rlat));
    rlon        = fmod(rlon1 - dlon + missionx::PI, missionx::PI2) - missionx::PI;
  }

  outLat = rlat * RadToDeg;
  outLon = rlon * RadToDeg;

}

// ----------------------------------------------

double
missionx::mxUtils::mxCalcBearingBetween2Points(const double gFromLat, const double gFromLong, const double gTargetLat, const double gTargetLong)
{
  double pTargetLatRad, pTargetLongRad;

  // convert lat/long to Radiance
  const double pLatRad  = missionx::PI / 180 * gFromLat;
  const double pLongRad = missionx::PI / 180 * gFromLong;

  pTargetLatRad  = missionx::PI / 180 * gTargetLat;
  pTargetLongRad = missionx::PI / 180 * gTargetLong;

  double tc1 = fmod(atan2(sin(pTargetLongRad - pLongRad) * cos(pTargetLatRad), cos(pLatRad) * sin(pTargetLatRad) - sin(pLatRad) * cos(pTargetLatRad) * cos(pTargetLongRad - pLongRad)), missionx::PI2);
  double val = RadToDeg * tc1;


  // from javascript: http://instantglobe.com/CRANES/GeoCoordTool.html (check the source of the page)
  // Number.prototype.toBrng = function() {  // convert radians to degrees (as bearing: 0...360)
  //  return (this.toDeg() + 360) % 360;
  //}
  double bearing = std::fmod(val + 360.0, 360.0); // handle negative bearing correctly

  return bearing;
}

// ----------------------------------------------

bool
missionx::mxUtils::mxIsPointInPolyArea(const std::vector<missionx::mxVec2d> inPolyArea, const missionx::mxVec2d inPoint)
{
  /* The coordinates of the plane coordinations */
  double px, py;
  missionx::mxVec2d p1;
  missionx::mxVec2d p2;

  

  px = inPoint.lat; // plane or point lat position
  py = inPoint.lon; // plane or point lon position

  // Calculate How many times the ray crosses the area segments
  int crossings = 0;

  double x1, x2;

  size_t counter = 0;
  for (auto& itPoint : inPolyArea)
  {
    counter++;
    p1 = (itPoint);
    if (counter < inPolyArea.size())
      p2 = inPolyArea.at(counter);
    else
      p2 = inPolyArea.at(0);

    // This is done to ensure that we get the same result when
    // the line goes from left to right and right to left
    if (p1.lat < p2.lat)
    {
      x1 = p1.lat;
      x2 = p2.lat;
    }
    else
    {
      x1 = p2.lat;
      x2 = p1.lat;
    }

    // First check if the ray is possible to cross the line
    if (px > x1 && px <= x2 && (py < p1.lon || py <= p2.lon))
    {
      static const double eps = 0.000001f;


      // Calculate the equation of the line
      double dx = p2.lat - p1.lat;
      double dy = p2.lon - p1.lon;
      double k;

      if (fabs(dx) < eps)
      {
#ifdef IBM
        k = std::numeric_limits<double>::infinity(); // max();  //INFINITY; // math.h
#else
        k = std::numeric_limits<double>::max();
#endif
      }
      else
      {
        k = dy / dx;
      }

      double m = p1.lon - k * p1.lat;

      // Find if the ray crosses the line
      double y2 = k * px + m;
      if (py <= y2)
      {
        crossings++;
      }
    }

  } // end for loop

  // Evaluate if plane in Area
  return (crossings % 2 == 1);

}

// default mx_units_of_measure = nm
double
missionx::mxUtils::mxCalcDistanceBetween2Points(const double gFromLat, const double gFromLong, const double gTargetLat, const double gTargetLong, const missionx::mx_units_of_measure inReturnInUnits, const double planet_great_circle_in_meters)
{
  //double angle, pLatRad, pLongRad, pTargetLatRad, pTargetLongRad;

  ////// convert lat/long to Radiance
  ////pLatRad  = missionx::PI / 180 * gFromLat;
  ////pLongRad = missionx::PI / 180 * gFromLong;
  ////pTargetLatRad  = missionx::PI / 180 * gTargetLat;
  ////pTargetLongRad = missionx::PI / 180 * gTargetLong;

  //pLatRad  = gFromLat * missionx::DegToRad;
  //pLongRad = gFromLong * missionx::DegToRad;
  //pTargetLatRad  = gTargetLat  * missionx::DegToRad;
  //pTargetLongRad = gTargetLong * missionx::DegToRad;

  const double teta1 = missionx::DegToRad*(gFromLat);
  const double teta2 = missionx::DegToRad*(gFromLong);
  const double teta3 = missionx::DegToRad*(gTargetLat);
  const double teta4 = missionx::DegToRad*(gTargetLong);

  // circumference in miles at equator, if you want km, use km value here
  // double circ = EQUATER_LEN_NM; // v3.0.303.7 deprecated
  //double lon = (pLongRad - pTargetLongRad);

  // simple absolute function
  //if (lon == 0.00) // v3.0.303.7 fix edge case where lon value is 0.0
  //  lon = 0.0000001;
  //else if (lon < 0.0)
  //  lon = -1 * (lon);

  //if (lon > missionx::PI)
  //{
  //  lon = missionx::PI2 - lon;
  //}

  //angle = acos(sin(pTargetLatRad) * sin(pLatRad) + cos(pTargetLatRad) * cos(pLatRad) * cos(lon));

  //const double retValue_d = planet_great_circle_in_meters * angle / (missionx::PI2);
  const double retValue_d = round(planet_great_circle_in_meters * acos(cos(teta1) * cos(teta3) * cos(teta2 - teta4) + (sin(teta1) * sin(teta3))));

  switch (inReturnInUnits)
  {
    case missionx::mx_units_of_measure::nm:
    {
      return retValue_d * missionx::meter2nm;
    }
    break;
    case missionx::mx_units_of_measure::meter:
    {
      return retValue_d;
    }
    break;
    case missionx::mx_units_of_measure::km:
    {
      return retValue_d * missionx::meter2nm * missionx::nm2km;
    }
    break;
    case missionx::mx_units_of_measure::ft:
    {
      return retValue_d * missionx::meter2feet;
    }
    break;
    default:
      break;
  }


  return retValue_d; // distance in meters
}


// ----------------------------------------------


std::string
missionx::mxUtils::mx_translateDrefTypeToString(const XPLMDataTypeID &inType)
{
  switch (inType)
  {      
    case xplmType_Int:
      return "int";
    case xplmType_Float:
      return "float";
    case xplmType_Double:
      return "double";
    case xplmType_FloatArray:
      return "float[";
    case xplmType_IntArray:
      return "int[";
    default:
      break;
  }

return "";
}


// ----------------------------------------------


missionx::mxRGB
missionx::mxUtils::hexToRgb(std::string hexValue_s, const float inModifier)
{
missionx::mxRGB rgbColor;
bool       bLooksValid = false;

if (hexValue_s.length() == 8 && (hexValue_s.find("0x") == 0 || hexValue_s.find("0X") == 0))
    bLooksValid = true;
else if (hexValue_s.length() == 7 && hexValue_s.find('#') == 0)
{
    bLooksValid = true;
    // replace # with 0x
    hexValue_s = hexValue_s.substr(1);
    hexValue_s = "0x" + hexValue_s;
}

if (bLooksValid)
{
    unsigned int hexValue = 0;
    try
    {
      hexValue = std::stoul(hexValue_s, nullptr, 16);
      //  Convert to RGB
      rgbColor.r = ((hexValue >> 16) & 0xFF) / inModifier; // Extract the RR byte
      rgbColor.g = ((hexValue >> 8) & 0xFF) / inModifier;  // Extract the GG byte
      rgbColor.b = ((hexValue)&0xFF) / inModifier;         // Extract the BB byte
    }
    catch (...)
    {
      rgbColor.setRGB(255.0, 255.0, 255.0); // white
    }
}
else
    rgbColor.setRGB(255.0, 255.0, 255.0); // white

return rgbColor;
}

// ----------------------------------------------

missionx::mxRGB
missionx::mxUtils::hexToNormalizedRgb(std::string hexValue_s, const float inModifier)
{
  const missionx::mxRGB rgb = missionx::mxUtils::hexToRgb(hexValue_s, inModifier);
  missionx::mxRGB       norm_rgb;
  norm_rgb.r = rgb.r / 255.0f;
  norm_rgb.g = rgb.g / 255.0f;
  norm_rgb.b = rgb.b / 255.0f;

  return norm_rgb;
}


// ----------------------------------------------


missionx::mx_fetch_info
missionx::mxUtils::fetch_next_string(const std::string& inText, const size_t inOffset, std::string inDelimiter)
{
  missionx::mx_fetch_info result;

  for (const auto& c : inText)
  {
    if (result.lastPos < inOffset)
    {
      result.lastPos++;
      continue;
    }

    result.lastPos++;
    if (inDelimiter.find(c) != std::string::npos && (false == result.token.empty())) // if we found delimiter and we have a token
    {
      return result;
    }
    else
    {
      result.token.push_back(c);
    }
  }

  return result;

}


// ----------------------------------------------


std::string
missionx::mxUtils::remove_quotes(const std::string inString, bool bFromBeginning, bool bFromEnd)
{
  std::string result{ inString };

  if (bFromBeginning)
    result = (!inString.empty() && inString.front() == '"') ? inString.substr(1) : result;

  if (bFromEnd)
    result = (!inString.empty() && inString.back() == '"') ? result.substr(0, result.length() - 1) : result;

  return result;
}

// ----------------------------------------------

#if defined(__cpp_lib_char8_t)
std::string missionx::mxUtils::from_u8string(const std::u8string &s)
{
    return std::string(s.begin(), s.end());
}
#endif

// ----------------------------------------------

std::string missionx::mxUtils::from_u8string(const std::string &s)
{
  return s;
}

// ----------------------------------------------

std::string missionx::mxUtils::from_u8string(std::string &&s)
{
  return std::move(s);
}

std::map<char, size_t>
missionx::mxUtils::getHowManyDelimitersInString(const std::string& inLine, const std::string delimiters)
{
  std::map<char, size_t> mapDelimiters;
  mapDelimiters.clear();

  for (const auto& c : inLine)
  {
    if (delimiters.find(c) != std::string::npos)
      mapDelimiters[c] += 1;
  }

  return mapDelimiters;
}

int
missionx::mxUtils::coord_in_rect(float x, float y, const float bounds_ltrb[4])
{
  return ((x >= bounds_ltrb[0]) && (x < bounds_ltrb[2]) && (y >= bounds_ltrb[3]) && (y < bounds_ltrb[1]));
}





// ----------------------------------------------

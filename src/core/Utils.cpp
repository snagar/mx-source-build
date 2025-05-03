#include <cassert>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

#include "Utils.h"
#include "XPLMScenery.h"
#include "fmod_errors.h"



using namespace missionx;

namespace missionx
{
int         Utils::seqTimerFunc{ 0 };
//std::string Utils::logFilePath;
// XML
IXMLDomParser   missionx::Utils::xml_iDomXSD;
ITCXMLNode      missionx::Utils::xml_xMainXSDNode;
const char*     missionx::Utils::DEG_SIM = "\u00B0";
missionx::mutex missionx::Utils::s_CalcDistMutex; // v3.305.2


// -------------------------------------------
// -------------------------------------------
// -------------------------------------------

TimerFunc::TimerFunc() {
  m_startTimepoint = std::chrono::high_resolution_clock::now();
  seq              = ++Utils::seqTimerFunc;
}

// -------------------------------------------

TimerFunc::TimerFunc (const std::string &inFilename, const std::string &inSourceFuncName, const bool inThread)
{
  init(inFilename, inSourceFuncName, inThread);
}

// -------------------------------------------

TimerFunc::TimerFunc (const std::string &inFilename, const std::string &inSourceFuncName, const std::string& info, const bool inThread)
{
  init(inFilename, inSourceFuncName, inThread);

  char buf_filename[TimerFunc::FILENAME_WIDTH_CHAR];
  char buf_funcName[TimerFunc::FUNCNAME_WIDTH_CHAR];
  char buff_seq[11];

  std::snprintf(buf_filename, sizeof(buf_filename), "%-*.*s", static_cast<int32_t> (TimerFunc::FILENAME_WIDTH_CHAR) - 1, static_cast<int32_t> (TimerFunc::FILENAME_WIDTH_CHAR) - 1, this->sourceFileName.c_str());
  std::snprintf(buf_funcName, sizeof(buf_funcName), "%-*.*s", static_cast<int32_t> (TimerFunc::FUNCNAME_WIDTH_CHAR) - 1, static_cast<int32_t> (TimerFunc::FUNCNAME_WIDTH_CHAR) - 1, this->sourceFuncName.c_str());
  std::snprintf(buff_seq, sizeof(buff_seq) - static_cast<size_t> (1), "%i", this->seq);

  const std::string info_buff = std::string(buf_filename) + "\t:" + buf_funcName + "(" + buff_seq + ")\t: \t\t\t" + info;

  Log::logMsg(info_buff + "(" + buff_seq + ")");
}

// -------------------------------------------

void
TimerFunc::init(const std::string& inFilename, const std::string &inSourceFuncName, const bool inThread)
{
  m_startTimepoint     = std::chrono::high_resolution_clock::now();

  this->isThread       = inThread;
  this->sourceFuncName = inSourceFuncName;


  const std::filesystem::path path(inFilename);
  this->sourceFileName = path.filename().string();

  seq = ++Utils::seqTimerFunc;
}

// -------------------------------------------

TimerFunc::~TimerFunc()
{
  stop();
}

// -------------------------------------------

void
TimerFunc::stop() const
{
  const auto endTimePoint  = std::chrono::high_resolution_clock::now();
  auto       durationMicro = std::chrono::duration_cast<std::chrono::microseconds> (endTimePoint - m_startTimepoint).count ();
  const auto durationMilli = durationMicro * 0.001;
  const auto durationSec   = durationMilli * 0.001;


  // https://en.cppreference.com/w/cpp/io/c/fprintf
  char buf_filename[TimerFunc::FILENAME_WIDTH_CHAR];
  char buf_funcName[TimerFunc::FUNCNAME_WIDTH_CHAR];
  char buf_duration_mcs[23];
  char buff_seq[11];

  #ifndef RELEASE
  auto sizeDebug = sizeof(buf_duration_mcs);
  #endif

  std::snprintf(buf_filename, sizeof(buf_filename), "%-*.*s", static_cast<int32_t> (TimerFunc::FILENAME_WIDTH_CHAR) - 1, static_cast<int32_t> (TimerFunc::FILENAME_WIDTH_CHAR) - 1, this->sourceFileName.c_str());
  std::snprintf(buf_funcName, sizeof(buf_funcName), "%-*.*s", static_cast<int32_t> (TimerFunc::FUNCNAME_WIDTH_CHAR) - 1, static_cast<int32_t> (TimerFunc::FUNCNAME_WIDTH_CHAR) - 1, this->sourceFuncName.c_str());
  std::snprintf(buff_seq, sizeof(buff_seq) - static_cast<size_t> (1), "%i", this->seq);

  #if defined LIN
  std::snprintf(buf_duration_mcs, sizeof(char) * sizeof(buf_duration_mcs) - static_cast<size_t> (1), "%*ld", 10, durationMicro);
  #else
  std::snprintf(buf_duration_mcs, sizeof(buf_duration_mcs) - (size_t)1, "%*lld", 10, durationMicro);
  #endif // !IBM


  const std::string duration_s = std::string(buf_filename) + "\t:" + buf_funcName + "(" + buff_seq + ")\t: " + buf_duration_mcs + "mcs, " + mxUtils::formatNumber<double>(durationMilli, 4) + "ms, " + mxUtils::formatNumber<double>(durationSec, 4) + "sec";
  Log::logMsg(duration_s, isThread);
}

} // missionx namespace

// -------------------------------------------

missionx::Utils::Utils()
{

  missionx::Utils::prepare_static_XSD(); // v3.0.241.1
}

missionx::Utils::~Utils() = default;


// -------------------------------------------

////////////////////// Convert Units of Measure ////////////////

missionx::mx_units_of_measure
missionx::Utils::translateStringToUnits(std::string& inVal)
{
  inVal = Utils::stringToLower(inVal); // to lower case

  if (inVal == "ft")
    return missionx::mx_units_of_measure::ft;
  else if (inVal == "mt")
    return missionx::mx_units_of_measure::meter;
  else if (inVal == "km")
    return missionx::mx_units_of_measure::km;
  else if (inVal == "nm")
    return mx_units_of_measure::nm;

  return missionx::mx_units_of_measure::unit_unsupports;
}

// -------------------------------------------
float
missionx::Utils::convertNmTo(const float& nm, const mx_units_of_measure from)
{
  switch (from)
  {
    case mx_units_of_measure::ft:
    {
      return ((nm * nm2meter) * meter2feet);
    }
    break;
    case mx_units_of_measure::meter:
    {
      return nm * missionx::nm2meter;
    }
    break;
    case mx_units_of_measure::km:
    {
      return nm * missionx::nm2km;
    }
    break;
    default:
      break;
  }

  return nm;
}

// -------------------------------------------
float
missionx::Utils::convertToNm (const float & v, const mx_units_of_measure from)
{
  switch (from)
  {
    case mx_units_of_measure::ft:
    {
      return ((v * feet2meter) * meter2nm);
    }
    case mx_units_of_measure::meter:
    {
      return v * missionx::meter2nm;
    }
    break;
    case mx_units_of_measure::km:
    {
      return v * missionx::km2nm;
    }
    break;
    default:
      break;
  }

  return v;
}

// -------------------------------------------

// FMOD_RESULT
// missionx::Utils::check_fmod_result (const FMOD_RESULT result)
// {
//   if (result != FMOD_OK)
//   {
//     std::string err(FMOD_ErrorString(result));
//     err = "FMOD error! (code: " + Utils::formatNumber<int>(result) + ") " + err + mxconst::get_UNIX_EOL();
//     Log::logMsgErr(err);
//   }
//   // return FmodErrorCheck(result);
//
//   return result;
// }


// -------------------------------------------
// -------------------------------------------

std::string
missionx::Utils::replaceChar1WithChar2_v2(const std::string& inString, const char charToReplace, const std::string &newChar)
{
  std::string result;
  result.clear();

  //  int length = (int)inString.length();

  for (auto& c : inString)
  {
    if (c == charToReplace)
    {
      if (newChar.empty())
        continue;

      result += newChar;
    }
    else
      result.push_back(c);
  }


  return result;
}

// replaceChar1WithChar2_v2

// -------------------------------------------
/**
replaceCharsWithString
Replaces set of characters, one by one, with another string of characters
Uses: remove the LF and UNIX_EOL from strings
*/
std::string
missionx::Utils::replaceCharsWithString(const std::string& outString, const std::string& charsToReplace, const std::string &newChar)
{
  std::string result = outString;
  bool no_more_to_search = false;

  size_t loc = 0;
  for (const char c : charsToReplace)
  {
    no_more_to_search = false;
    for (size_t i = 0; i < result.length() && !no_more_to_search; i++)
    {
      loc = result.find(c, i);
      if (loc != std::string::npos)
        result.replace(loc, 1, newChar);
      else
        no_more_to_search = true; // skip loop if nothing was found

    } // end internal loop
  }   // end external loop

  // outString = result;
  return result;
}

// -------------------------------------------

std::string
missionx::Utils::replaceStringWithOtherString(std::string inStringToModify, const std::string& inStringToReplace, const std::string& inNewString, const bool flag_forAllOccurences, const size_t skip_occurence)
{
  //  bool flag_continueSearch = true;
  size_t occurrence = 0;
  auto   pos       = inStringToModify.find(inStringToReplace);
  while (pos != std::string::npos)
  {
    ++occurrence;
    if (skip_occurence && occurrence == skip_occurence)
    {
      pos = inStringToModify.find(inStringToReplace, pos + static_cast<size_t> (1));
      continue;
    }
    else
    {
      inStringToModify.replace(pos, inStringToReplace.length(), inNewString);
      pos = std::string::npos;
    }

    if (flag_forAllOccurences)
      pos = inStringToModify.find(inStringToReplace);
    else
      break;
  }

  return inStringToModify;
}

// replaceCharsWithString

// -------------------------------------------

double
missionx::Utils::calcDistanceBetween2Points_nm_ts(const double gFromLat, const double gFromLong, const double gTargetLat, const double gTargetLong, const mx_units_of_measure inReturnInUnits)
{
  std::lock_guard<std::mutex> lock(missionx::Utils::s_CalcDistMutex);

  // convert lat/long to Radiance
  const double pLatRad  = missionx::PI / 180 * gFromLat;
  const double pLongRad = missionx::PI / 180 * gFromLong;

  const double pTargetLatRad  = missionx::PI / 180 * gTargetLat;
  const double pTargetLongRad = missionx::PI / 180 * gTargetLong;

  // circumference in miles at the equator, if you want km, use km value here
  double lon = (pLongRad - pTargetLongRad);

  // simple absolute function
  if (lon == 0.00) // v3.0.303.7 fix edge case where lon value is 0.0
    lon = 0.0000001;
  else if (lon < 0.0)
    lon = -1 * (lon);

  if (lon > missionx::PI)
  {
    lon = missionx::PI2 - lon;
  }

  const double angle = acos (sin (pTargetLatRad) * sin (pLatRad) + cos (pTargetLatRad) * cos (pLatRad) * cos (lon));

  const double retValue_d = missionx::EQUATER_LEN_NM * angle / (missionx::PI2);


  if (inReturnInUnits == missionx::mx_units_of_measure::nm) // probably in most cases
    return retValue_d;
  else if (inReturnInUnits == missionx::mx_units_of_measure::meter)
    return retValue_d * missionx::nm2meter;
  else if (inReturnInUnits == missionx::mx_units_of_measure::km)
    return retValue_d * missionx::nm2km;
  else if (inReturnInUnits == missionx::mx_units_of_measure::ft)
    return retValue_d * missionx::nm2meter * missionx::meter2feet;


  return retValue_d; // circ* angle / (missionx::PI2); // default in nm
}

// -------------------------------------------

double
missionx::Utils::calcDistanceBetween2Points_nm(const double gFromLat, const double gFromLong, const double gTargetLat, const double gTargetLong, const mx_units_of_measure inReturnInUnits)
{

  // convert lat/long to Radiance
  const double pLatRad  = missionx::PI / 180 * gFromLat;
  const double pLongRad = missionx::PI / 180 * gFromLong;

  const double pTargetLatRad  = missionx::PI / 180 * gTargetLat;
  const double pTargetLongRad = missionx::PI / 180 * gTargetLong;

  // circumference in miles at the equator, if you want km, use km value here
  double lon  = (pLongRad - pTargetLongRad);

  // simple absolute function
  if (lon == 0.00) // v3.0.303.7 fix edge case where lon value is 0.0
    lon = 0.0000001;
  else if (lon < 0.0)
    lon = -1 * (lon);

  if (lon > missionx::PI)
  {
    lon = missionx::PI2 - lon;
  }

  const double angle = acos (sin (pTargetLatRad) * sin (pLatRad) + cos (pTargetLatRad) * cos (pLatRad) * cos (lon));

  const double retValue_d = missionx::EQUATER_LEN_NM * angle / (missionx::PI2);


  if (inReturnInUnits == missionx::mx_units_of_measure::nm) // probably in most cases
    return retValue_d;
  else if (inReturnInUnits == missionx::mx_units_of_measure::meter)
    return retValue_d * missionx::nm2meter;
  else if (inReturnInUnits == missionx::mx_units_of_measure::km)
    return retValue_d * missionx::nm2km;
  else if (inReturnInUnits == missionx::mx_units_of_measure::ft)
    return retValue_d * missionx::nm2meter * missionx::meter2feet;


  return retValue_d; // circ* angle / (missionx::PI2); // default in nm
}

// -------------------------------------------
double
missionx::Utils::calcBearingBetween2Points(const double gFromLat, const double gFromLong, const double gTargetLat, const double gTargetLong)
{
  return mxCalcBearingBetween2Points(gFromLat, gFromLong, gTargetLat, gTargetLong);
}
// -------------------------------------------

// provide two points and elevation between them. Function will retrieve the slope slope_angle
double
missionx::Utils::calcSlopeBetween2PointsWithGivenElevation(const double gFromLat, const double gFromLong, const double gTargetLat, const double gTargetLong, const double relativeElevInFeet)
{
  // http://www.cplusplus.com/forum/beginner/90710/
  // https://www.geeksforgeeks.org/find-two-sides-right-angle-triangle/
  // http://www.analyzemath.com/Geometry_calculators/right_triangle_calculator.html (calculator)
  /*
    cout << "  |* ";
  cout << "\n  |   * ";
  cout << "\n  | b    *       C";
  cout << "\n  |         * ";
  cout << "\nA |            * ";
  cout << "\n  |               * ";
  cout << "\n  | 90           a   * ";
  cout << "\n  |_____________________* ";
  cout << "\n             B \n\n";

  A^2 + B^2 = C^2 =
  A*A + B*B = C*C

  */

  double       slope_angle_deg = 0.0f;
  const double distanceIn_nm   = calcDistanceBetween2Points_nm (gFromLat, gFromLong, gTargetLat, gTargetLong);
  const double C               = distanceIn_nm * nm2meter * meter2feet; // FEET_TO_NM; // convert to FEET // Represent B leg
  const double A               = relativeElevInFeet;                    // represents Elevation in feet
                                                                        //  double B = sqrt(C * C - A * A); // we need to find B


  const double b     = A / C;
  const double a_rad = asin (b);
  slope_angle_deg    = a_rad * 180.0 / missionx::PI;

  return slope_angle_deg;
}

// -------------------------------------------

float
missionx::Utils::calcElevBetween2Points_withGivenAngle_InFeet(const float distanceBetweenPointsInFeet, const float slopeAngle)
{

  double       radTangA = 0.0f; // in radians
  double       sideA    = 0.0f;
  const double sideB    = distanceBetweenPointsInFeet;

  radTangA = slopeAngle * missionx::PI / 180; // radians
  sideA    = tan(radTangA) * sideB;


  return static_cast<float> (sideA); // Elevation
}


// -------------------------------------------
/**
 * calcPointBasedOnDistanceAndBearing_2DPlane function return a Point based on location.
 * inLat, inLon: plane latitude/longitude
 * inHdg: the function angles are (counterclockwise): 0, 270, 180, 90
 *        x-plane angles (clockwise): 0, 90, 180, 270
 *        In order to compensate for the un-identical measures, we use 360 - Angle in order to be on par with X-Plane angles.
 *        The Headings must be in PSI and not in magnetic PSI.
 *        PSI: sim/flightmodel/position/psi
 */

void
missionx::Utils::calcPointBasedOnDistanceAndBearing_2DPlane(double& outLat, double& outLon, const double inLat, const double inLon, const float inHdg_deg, const double inDistance_nm)
{
  mxUtils::mxCalcPointBasedOnDistanceAndBearing_2DPlane(outLat, outLon, inLat, inLon, inHdg_deg, inDistance_nm * missionx::nm2meter);
}

// -------------------------------------------

const missionx::mxVec2d
Utils::getPointBasedOnDistanceAndBearing_2DPlane(const double inLat, const double inLon, const float inHdg, const double inDistance)
{
  double rlat, rlon;
  mxUtils::mxCalcPointBasedOnDistanceAndBearing_2DPlane(rlat, rlon, inLat, inLon, inHdg, inDistance * missionx::nm2meter);
  return missionx::mxVec2d(rlat, rlon);
}


// -------------------------------------------

std::vector<std::string>
missionx::Utils::buildSentenceFromWords(const std::vector<std::string>& inTokens, const size_t allowedSentenceLength, const size_t maxLinesAllowed /*, size_t *outRealLinesCount*/)
{

  std::vector<std::string>                 vecSentence;
  std::string                              sentence;
  sentence.clear();
  size_t lineLen       = 0;
  size_t lineCounter   = 0;
//  size_t realLineCount = 0; // was meant to check how many lines are created during parsing, but we never implemented the feature.

  for (auto itToken = inTokens.begin (); itToken < inTokens.end() && lineCounter <= maxLinesAllowed; ++itToken)
  // for (auto &itToken : inTokens)
  {
    // check if new Token.length() + lineLen < sentenceLength ? sentance+=Token : start new line
    if (itToken->length() + lineLen <= allowedSentenceLength)
    {
      sentence = (sentence.empty()) ? *itToken : sentence.append(" ").append(*itToken);
      lineLen  = sentence.length();
    }
    else
    {

      // add sentence to vector
      vecSentence.emplace_back(sentence);
      sentence.clear();
      lineCounter++;
//      realLineCount++;
      lineLen = 0;

      sentence = *itToken;
      lineLen  = sentence.length();

    } // end sentence length check
  }   // end loop

  // handle remaining string
  if (!sentence.empty() && sentence.length() <= allowedSentenceLength && lineCounter < maxLinesAllowed)
  {
    vecSentence.emplace_back(sentence);
  }

  return vecSentence;
}
// -------------------------------------------
// -------------------------------------------

// split string by fetching last to characters from a string, and returning the split strings
bool
missionx::Utils::extractUnitsFromString(const std::string& inNumWithUnits, std::string& outNumber, std::string& outUnit)
{
  bool success = true;

  outUnit.clear();
  outNumber.clear();

  const auto lmbda_extract_num_from_units_of_measure = [=](const std::string& inNumWithUnits, std::string& outUnit) {
    if (inNumWithUnits.length() < 3) // probably no units where defined
    {
      return inNumWithUnits;
    }
    else // check units
    {
      if (const std::string units = inNumWithUnits.substr (inNumWithUnits.length () - 2, 2)
        ; Utils::is_alpha(units))
      {
        outUnit = units;
        return inNumWithUnits.substr(0, inNumWithUnits.length() - 2);
      }
      else // if units are not set then assume only numbers
        return inNumWithUnits;

    } // end check length
  };


  if (const auto num = lmbda_extract_num_from_units_of_measure (inNumWithUnits, outUnit)
    ; Utils::is_number(num))
    outNumber = num;
  else
    success = false;

  return success;
}


// -------------------------------------------


std::map<int, std::string>
missionx::Utils::splitStringToMap(const std::string& inString, const std::string& delimateChars)
{
  int                        seq = 0;
  std::map<int, std::string> mapStrings;
  mapStrings.clear();

  for (std::vector<std::string> vecSplitValues = mxUtils::split_v2 (inString, delimateChars)
    ; const auto& val : vecSplitValues)
  {
    Utils::addElementToMap(mapStrings, seq, val);
    ++seq;
  }


  return mapStrings;
}

// -------------------------------------------
std::list<std::string>
missionx::Utils::splitStringToList(const std::string& inString, const std::string& delimateChars)
{
  // https://stackoverflow.com/questions/458476/best-way-to-copy-a-vector-to-a-list-in-stl
  // https://www.techiedelight.com/convert-vector-list-cpp/

  const auto vec = mxUtils::split_v2(inString, delimateChars); // v3.305.1
  return std::list<std::string>(vec.begin(), vec.end());
}


// -------------------------------------------
// Extract base string from a string. Good to extract file name without the extension "[file].exe", for example.
std::string
missionx::Utils::extractBaseFromString(const std::string& inFullFileName, const std::string &delimiter, std::string* outRestOfString)
{

  if (const size_t lengthString = inFullFileName.length ()
    ; delimiter.empty () || lengthString == 1)
    return inFullFileName;

  const char   d   = delimiter.front ();
  const size_t loc = inFullFileName.find_first_of(d);

  if (loc == std::string::npos) // did not find
  {
    if (outRestOfString != nullptr)
      outRestOfString->clear();

    return inFullFileName;
  }

  std::string result = inFullFileName.substr(0, loc);
  if (outRestOfString != nullptr)
    (*outRestOfString) = inFullFileName.substr((loc + 1));


  return result;
}

// -------------------------------------------
// Extract Last string from a string. Good to extract file name without the PATH, for example.
std::string
missionx::Utils::extractLastFromString(std::string inFullFileName, const std::string &delimiter, std::string* outRestOfString)
{
  std::string result;
  result.clear();

  if (const size_t lengthString = inFullFileName.length (); delimiter.empty () || lengthString == 1)
    return inFullFileName;

  const char   d   = delimiter.front ();
  const size_t loc = inFullFileName.find_last_of(d);

  if (loc == std::string::npos) // did not find
  {
    if (outRestOfString != nullptr)
      outRestOfString->clear();

    return inFullFileName;
  }

  result = inFullFileName.substr(loc + 1);
  if (outRestOfString != nullptr)
    (*outRestOfString) = inFullFileName.substr(0, loc);



  return result;
}

// -------------------------------------------

std::string
missionx::Utils::add_word_to_line(std::deque<std::string>& outDeque, const std::string &inCurrentLine_s, const std::string &inNewWord_s, const int inMaxLineLength_i, const bool flag_force_new_line)
{
  std::string newSentence     = inCurrentLine_s;
  std::string newWord         = inNewWord_s;
  const auto  lineLength      = newSentence.length ();
  const auto  max_line_length = static_cast<size_t> (inMaxLineLength_i);

  if (!newWord.empty() || flag_force_new_line)
  {
    // check length is valid

    if (size_t tmp_len = lineLength + newWord.length ()
      ; tmp_len <= max_line_length)
    {
      newSentence += (newSentence.empty()) ? newWord : std::string(" ") + newWord;

      if (flag_force_new_line)
      {
        outDeque.emplace_back(newSentence); // add current good sentence length
        newSentence.clear();
      }

    } // end if sentence is in good length
    else if (tmp_len > max_line_length)
    {
      if (!newSentence.empty())
      {
        outDeque.emplace_back(newSentence); // add current good sentence length
        newSentence.clear();
      }


      tmp_len             = newWord.length(); // check if new word is also too long
      const auto devide_i = (size_t)(tmp_len / max_line_length);
      const auto mod_i    = (size_t)(tmp_len % max_line_length);

      for (size_t i1 = 0; i1 < devide_i; ++i1) // add parts of the work as a sentence
      {
        const size_t from_st = i1 * max_line_length;
        newSentence          = newWord.substr (from_st, max_line_length);
        outDeque.emplace_back (newSentence);
        newSentence.clear ();
      }

      if (mod_i > 0)
      {
        newSentence = newWord.substr(devide_i * max_line_length); // start new sentence.
      }

    } // end if temp_len is too long or at correct length

    newWord.clear();
  } // end add newWord

  return newSentence;
}

// -------------------------------------------

std::vector<std::string>
missionx::Utils::sentenceTokenizerWithBoundaries (const std::string &inString, const std::string& delimiterChar, const size_t width, const std::string &in_special_LF_char)
{
  std::deque<std::string> dequeSentences;

  // if width = 0 then immediate return vecWord. For backward compatibility with old code, if any.
  if (width == 0)
  {
    std::vector<std::string> vecWords = mxUtils::split_v2(delimiterChar); // v3.305.1
    return vecWords; // no need to create sentence
  }

  Utils::sentenceTokenizerWithBoundaries(dequeSentences, inString, delimiterChar, width, in_special_LF_char); // main function to build the sentences, using deque instead vector, mainly for memory management benefits

  std::vector<std::string> vecSentence({ dequeSentences.begin(), dequeSentences.end() }); // converts deque to vector

  return vecSentence;
}

// -------------------------------------------

void
missionx::Utils::sentenceTokenizerWithBoundaries(std::deque<std::string>& outDeque, const std::string& inString, const std::string &delimiterChar, const size_t width, const std::string &in_special_LF_char)
{

  const char delimiter = (delimiterChar.empty()) ? ' ' : delimiterChar.front();

  const std::string& xWord = inString;


  const std::string& inSpecialChar = in_special_LF_char;
  // auto lineLength = (size_t)0;
  const auto max_line_length = width;

  std::string eol_found;

  std::string newSentence;
  std::string newWord;
  size_t      charPos_i    = -1;
  size_t      skip_chars_i = 0;

  bool flag_skip = false;

  charPos_i           = -1;
  auto pos_winEol     = xWord.find (mxconst::get_WIN_EOL());
  auto pos_unixEol    = xWord.find (mxconst::get_UNIX_EOL());
  auto pos_specialEol = xWord.find (inSpecialChar);
  eol_found.clear();


  for (auto chr : xWord)
  {
    ++charPos_i; // current char position

    // check if we need to skip chars, in case of: "\n" or "\r\n"
    if (!flag_skip)
    {
      // handle Win EOL "\r\n"
      if (charPos_i == pos_winEol)
      {
        flag_skip    = true;
        eol_found    = mxconst::get_WIN_EOL();
        skip_chars_i = eol_found.length(); // We just need one more character to skip. calculate how many chars to skip

        pos_winEol  = xWord.find(mxconst::get_WIN_EOL(), charPos_i + skip_chars_i);
        pos_unixEol = xWord.find(mxconst::get_UNIX_EOL(), charPos_i + skip_chars_i);
      }
      else if (charPos_i == pos_unixEol)
      {
        eol_found    = mxconst::get_UNIX_EOL();
        skip_chars_i = eol_found.length(); // - 1; // calculate how many chars to skip
        flag_skip    = true;

        pos_winEol  = xWord.find(mxconst::get_WIN_EOL(), charPos_i + skip_chars_i);
        pos_unixEol = xWord.find(mxconst::get_UNIX_EOL(), charPos_i + skip_chars_i);
      }
      else
        // handle Special EOL ";"
        if (charPos_i == pos_specialEol)
      {
        eol_found    = inSpecialChar;
        skip_chars_i = eol_found.length(); // calculate how many chars to skip
        flag_skip    = true;

        pos_specialEol = xWord.find(inSpecialChar, charPos_i + skip_chars_i);
      }
    }


    // check if we need to skip chars, in case of: "\n" or "\r\n"
    if (flag_skip)
    {
      --skip_chars_i;

      if (skip_chars_i <= 0)
      {
        flag_skip = false; // reset skip flag

        // add newWord to sentence and add sentence to vector. reset newWord and sentence
        newSentence = Utils::add_word_to_line(outDeque, newSentence, newWord, static_cast<int> (max_line_length), true);

        newWord.clear();
      }
    }
    else
    {
      if (chr == delimiter)
      {
        newSentence = add_word_to_line(outDeque, newSentence, newWord, static_cast<int> (max_line_length));
        newWord.clear();
      }
      else
        newWord += chr; // add characters to string
    }


  } // end word chars loop

  if (!newWord.empty())
    newSentence = Utils::add_word_to_line(outDeque, newSentence, newWord, static_cast<int> (max_line_length));


  newWord.clear();

  // Add last word if it has value in it
  if (!newSentence.empty())
    outDeque.emplace_back(newSentence);
}

// -------------------------------------------

std::vector<std::string>
missionx::Utils::tokenizer(const std::string &inString, const char delimate_char, const size_t width)
{
  size_t      sourceLen      = 0;
  size_t      strLastPos     = 0;
  size_t      lastNewLinePos = std::string::npos;
//  size_t      strLeftLength  = sourceLen; // constraint the length of the string feature was never implemented
  size_t      widthToCut     = 0;
  size_t      spacePos       = 0;
  int         debugCounter   = 0;
  std::string SPACE          = " ";

  sourceLen = inString.length();
  std::vector<std::string> vecWord;

  vecWord.clear();

  // *************
  while (strLastPos < sourceLen && debugCounter < 500)
  {
    debugCounter++;
    // find first ";" (UNIX_EOL) in this widget for new line
    lastNewLinePos = inString.find(delimate_char, ((lastNewLinePos == std::string::npos) ? 0 : strLastPos + 1));

    if (lastNewLinePos != std::string::npos && ((lastNewLinePos - strLastPos) <= width)) // if found ' ' before "width"
    {
      widthToCut = lastNewLinePos - strLastPos;
      vecWord.emplace_back(ltrim(inString.substr(strLastPos, widthToCut)));

//      strLeftLength -= widthToCut; // decrease left length
      strLastPos += widthToCut;

      // if next "delimate_char" is greater than "width", add ' '
    }
    else if ((lastNewLinePos != std::string::npos && lastNewLinePos > width) || lastNewLinePos == std::string::npos)
    {
      // make sure lastNewLinePos > -1 ( Avoid Out of range error )
      lastNewLinePos = (lastNewLinePos == std::string::npos) ? 0 : lastNewLinePos;

      widthToCut = width;
      if (inString.at(lastNewLinePos) != ' ')
      {
        // calculate new lastNewLinePos to point to SPACE if possible
        spacePos = inString.find_last_of(' ', (strLastPos + width));

        if (spacePos != std::string::npos && spacePos > strLastPos)
          widthToCut = spacePos - strLastPos; // change the width to cut
      }

      vecWord.emplace_back(ltrim(inString.substr(strLastPos, widthToCut)));

      strLastPos += widthToCut;
//      strLeftLength -= widthToCut;

    } // if lastNLPos > width


  } // end while


  return vecWord;

} // end tokenizer



// -------------------------------------------
// -------------------------------------------
// -------------------------------------------


double
missionx::Utils::lRound (const double x)
{

  double intPart;

  if (modf(x, &intPart) >= .5)
    return (x >= 0) ? ceil(x) : floor(x);

  return (x < 0) ? ceil(x) : floor(x);
}

void
missionx::Utils::convert_qArr_to_Quaternion(const float in_q_arr[4], QUATERNION q)
{
  q.w = in_q_arr[0];
  q.x = in_q_arr[1];
  q.y = in_q_arr[2];
  q.z = in_q_arr[3];
}

// -------------------------------------------

// -------------------------------------------

float
missionx::Utils::calc_heading_base_on_plane_move(double& inCurrentLat, const double & inCurrentLong, double& inTargetLat, const double & inTargetLong, const int & inTargetHeading)
{
  // decide if you need to fix heading
  // if Long was [+] and new Long is [-] than modify heading by "+90" relative to the horizon
  static constexpr int degree_in_circle = 360;

  int heading = inTargetHeading;

  if (inCurrentLong >= 0 && inTargetLong < 0) // +90
  {
    heading += 90;
    if (heading > degree_in_circle)
      heading -= degree_in_circle;
  }
  else if (inCurrentLong < 0 && inTargetLong >= 0)
  {
    heading -= 90;
    if (heading <= 0)
      heading += degree_in_circle;
  }


  return static_cast<float> (heading);
} // calc_heading_base_on_plane_move

// -------------------------------------------

double
missionx::Utils::readNumericAttrib(const ITCXMLNode& node, const std::string &inAttribName, const double defaultValue)
{

  if (node.isEmpty())
    return defaultValue;

  double returnValue = defaultValue; // init with default value

  std::string attribValue;
  attribValue.clear();

  const std::string& attribName = inAttribName; // v3.0.194 always convert attribute "name" to lower case

  if (char exists = node.isAttributeSet (attribName.c_str ()))
  {
    attribValue = node.getAttribute(attribName.c_str());
    attribValue = Utils::trim(attribValue); // always trim numeric values from spaces

    if (!Utils::trim(attribValue).empty())
    {
      // check if value is a numeric
      if (Utils::is_number(attribValue))
      {
        returnValue = Utils::stringToNumber<double>(attribValue);
      }
    }
  }

  return returnValue; // calling function will have to cast to the correct numeric type (int/float...)
}

// -------------------------------------------

double
missionx::Utils::readNumericAttrib(const ITCXMLNode& node, const std::string &attribOptionName1, const std::string &attribOptionName2, const double defaultValue)
{

  if (node.isEmpty())
    return defaultValue;

  double returnValue = defaultValue; // init with default value

  std::string attribNameToUse; // v3.0.221.15rc5
  std::string attribValue;

  attribNameToUse.clear(); // v3.0.221.15rc5
  attribValue.clear ();

  const char exists  = node.isAttributeSet (attribOptionName1.c_str ());
  const char exists2 = node.isAttributeSet(attribOptionName2.c_str());

  if (exists)
    attribNameToUse = attribOptionName1;
  else if (exists2)
    attribNameToUse = attribOptionName2;


  if (attribNameToUse.empty()) // if one of the attributes exists
  {
    returnValue = defaultValue;
  }
  else
  {
    attribValue = (node.getAttribute(attribNameToUse.c_str()));
    if (Utils::trim(attribValue).empty() || !(Utils::is_number(attribValue)))
    {
      returnValue = defaultValue;
    }
    else
    {
      returnValue = Utils::stringToNumber<double>(attribValue, static_cast<int> (attribValue.length ()));
    }
  }

  return returnValue; // calling function will have to cast to the correct numeric type (int/float...)
}

// -------------------------------------------
bool
missionx::Utils::readBoolAttrib(const ITCXMLNode& node, const std::string &attribName, const bool inDefaultValue)
{
  bool flag_found = false;
  if (node.isEmpty())
    return inDefaultValue;

  bool returnValue = inDefaultValue; // init with default value

  std::string attribValue = Utils::xml_get_attribute_value(node.deepCopy(), attribName, flag_found);

  if (flag_found)
  {
    attribValue = Utils::trim(attribValue); // always trim bool values from spaces

    if (!Utils::trim(attribValue).empty())
    {
      // check if value is a boolean
      missionx::mxUtils::isStringBool(attribValue, returnValue); // will check and assign the bool value
    }
  }

  return returnValue;
}

// -------------------------------------------

std::string
missionx::Utils::readAttrib (const ITCXMLNode & node, const std::string &attribName, std::string defaultValue, const bool needStringTrim)
{
  if (node.isEmpty ())
    return defaultValue;

  const IXMLNode xNode = node.deepCopy();
  return Utils::readAttrib(xNode, attribName, "", defaultValue, needStringTrim);
}

// -------------------------------------------

std::string
missionx::Utils::readAttrib (const ITCXMLNode & node, const std::string &attribOptionName1, const std::string &attribOptionName2, std::string defaultValue, const bool needStringTrim)
{
  if (node.isEmpty ())
    return defaultValue;

  const auto n =  node.deepCopy (); //node.deepCopyConstant();
  return Utils::readAttrib(n, attribOptionName1, attribOptionName2, defaultValue, needStringTrim);

}

// -------------------------------------------

std::string
missionx::Utils::readAttrib(const IXMLNode& node, const std::string& attribOptionName1, const std::string& attribOptionName2, std::string defaultValue, const bool needStringTrim)
{

  if (node.isEmpty())
    return defaultValue;

  std::string attribNameToUse; // v3.0.221.15rc5
  std::string attribValue;

  //attribNameToUse.clear(); // v3.0.221.15rc5
  // attribValue.clear();

  const char exists  = node.isAttributeSet (attribOptionName1.c_str ());
  const char exists2 = (attribOptionName2.empty()) ? '\0' : node.isAttributeSet(attribOptionName2.c_str());

  if (exists)
    attribNameToUse = attribOptionName1;
  else if (exists2)
    attribNameToUse = attribOptionName2;


  if (!attribNameToUse.empty()) // if one of the attributes exists
  {
    attribValue = (node.getAttribute(attribNameToUse.c_str()));
    if (Utils::trim(attribValue).empty())
    {
      attribValue = defaultValue;
    }
  }
  else
  {
    attribValue = defaultValue;
  }

  return (needStringTrim) ? Utils::trim(attribValue) : attribValue;
}

// -------------------------------------------

std::string
missionx::Utils::readAttrib(const IXMLNode& node, const std::string &attribOptionName1, const std::string &defaultValue, const bool needStringTrim)
{
  return Utils::readAttrib(node, attribOptionName1, "", defaultValue, needStringTrim);
}


// -------------------------------------------
// -------------------------------------------

bool
missionx::Utils::xml_search_and_set_attribute_in_IXMLNode(IXMLNode& inNode, const std::string& inAttribName, const std::string& attribValue, const std::string &inModifyByElementName)
{

  // Search attribute in current Node
  if (!inNode.isEmpty() && !inAttribName.empty())
  {
    bool flag_same_name_for_element_and_search_name = false;
    if (const std::string currentElementTagName = inNode.getName ()
      ; currentElementTagName == inModifyByElementName)
      flag_same_name_for_element_and_search_name = true;

    // check all attributes in current Node level. If no attribute found then recurse to lower level
    const int attribCounter  = inNode.nAttribute();
    bool      flag_canChange = false;          // will be used after attribute loop.
    for (int i1 = 0; i1 < attribCounter; ++i1) // loop over all attributes of current Node
    {
      const std::string attribName       = inNode.getAttributeName(i1);

      // decide if we can set the attribute we found.
      // if (const bool flag_attribFound = (inAttribName == attribName)
      //   ; flag_attribFound && inModifyByElementName.empty())
      //   flag_canChange = true;
      // else if (flag_attribFound && !inModifyByElementName.empty() && flag_same_name_for_element_and_search_name)
      //   flag_canChange = true;

      flag_canChange = static_cast<bool> ((inAttribName == attribName) * (inModifyByElementName.empty() + (!inModifyByElementName.empty() * flag_same_name_for_element_and_search_name ) ) );

      if (flag_canChange)
      {
        inNode.updateAttribute(attribValue.c_str(), inAttribName.c_str(), i1);
        return true;
      }
    }

    // *********************************************
    // SPECIAL CASE:
    // Did not find attribute but on same element as the search one. In this case we will create the attribute
    if (!flag_canChange && flag_same_name_for_element_and_search_name)
    {
      inNode.addAttribute(inAttribName.c_str(), attribValue.c_str());
      return true;
    }
    //// END Special Case **************************

    // if we reach here, it means we did not find the attribute. Loop over all sub-nodes and recurse the search
    const int nChilds = inNode.nChildNode();
    for (int i1 = 0; i1 < nChilds; ++i1)
    {
      IXMLNode cNode = inNode.getChildNode(i1);
      if (Utils::xml_search_and_set_attribute_in_IXMLNode(cNode, inAttribName, attribValue, inModifyByElementName))
        return true;
    }

  } // check if node is valid


  return false;
}

// -------------------------------------------

bool
missionx::Utils::xml_copy_nodes_from_one_parent_to_another_IXMLNode(IXMLNode& outTargetNode, const IXMLNode & inSourceNode, const std::string& nameOfChildTag, const bool bIsDeepCopy)
{
  if (outTargetNode.isEmpty() + inSourceNode.isEmpty())
    return false;
  else
  {
    const auto nChilds = (nameOfChildTag.empty()) ? inSourceNode.nChildNode() : inSourceNode.nChildNode(nameOfChildTag.c_str());
    for (int i1=0; i1 < nChilds; ++i1)
    {
      auto node_ptr =  (nameOfChildTag.empty())? inSourceNode.getChildNode(i1): inSourceNode.getChildNode(nameOfChildTag.c_str(), i1);
      outTargetNode.addChild((bIsDeepCopy) ? node_ptr.deepCopy() : node_ptr);

    }
  }

  return true;
}

// -------------------------------------------

bool
missionx::Utils::xml_add_node_to_element_IXMLNode(IXMLNode& rootNode, IXMLNode& inNodeToAdd, const std::string& nameOfParentElementToAddTo)
{
  IXMLNode dummy;

  if (nameOfParentElementToAddTo.empty())
  {
    dummy = rootNode.addChild(inNodeToAdd);
    if (dummy.isEmpty())
      return false;

    return true;
  }

  if (const std::string tagName = rootNode.getName (); tagName == nameOfParentElementToAddTo)
  {
    dummy = rootNode.addChild (inNodeToAdd);
    if (dummy.isEmpty ())
      return false;

    return true;
  }

  const int nChilds = rootNode.nChildNode();
  for (int i1 = 0; i1 < nChilds; ++i1)
  {
    if (IXMLNode cNode = rootNode.getChildNode (i1)
      ; xml_add_node_to_element_IXMLNode (cNode, inNodeToAdd, nameOfParentElementToAddTo))
      return true;
  }

  return false;
}

// -------------------------------------------
IXMLNode missionx::Utils::xml_get_node_ptr_randomly_by_attrib_and_value (const IXMLNode & rootNode, const std::string &inTagToSearch, const std::string &inAttribName, const std::string &inAttribValue)
{
  IXMLNode result_ptr = IXMLNode::emptyIXMLNode;
  std::vector <int> vecValidTagsIndex;
  vecValidTagsIndex.clear();

  if (rootNode.isEmpty())
    return IXMLNode::emptyIXMLNode;

  int nChilds = (inTagToSearch.empty()) ? rootNode.nChildNode() : rootNode.nChildNode(inTagToSearch.c_str());

  // Loop over all sub elements and fetch nodes with "inTagToSearch" and attribute "
  // Check that the attribute value is: "inAttribValue"
  for (int loop01 = 0; loop01 < nChilds; ++loop01)
  {
    bool           bAttribFound = false;
    const IXMLNode node = rootNode.getChildNode(inTagToSearch.c_str(), loop01);
    if (std::string attribValue = Utils::xml_get_attribute_value (node, inAttribName, bAttribFound)
      ; attribValue == inAttribValue )
      vecValidTagsIndex.emplace_back( loop01 );
  }

  // If we have valid nodes, randomly pick one of them.
  if ( ! vecValidTagsIndex.empty() )
  {
    const int rndResultIndex = Utils::getRandomIntNumber(0, static_cast<int>(vecValidTagsIndex.size()) - 1);
    result_ptr = (inTagToSearch.empty())? rootNode.getChildNode(rndResultIndex) :  rootNode.getChildNode(inTagToSearch.c_str(), rndResultIndex);
  }

  return result_ptr;
}



  // -------------------------------------------

IXMLNode
missionx::Utils::xml_get_node_randomly_by_name_IXMLNode (const IXMLNode & rootNode, const std::string &inTagToSearch, std::string& outErr, const bool flag_removePicked /*default false*/)
{
  outErr.clear();
  IXMLNode result = IXMLNode::emptyIXMLNode;

  if (const int nChilds = (inTagToSearch.empty ()) ? rootNode.nChildNode () : rootNode.nChildNode (inTagToSearch.c_str ())
    ; nChilds > 0)
  {
    const int rnd_node_in_tree_i = Utils::getRandomIntNumber(0, nChilds - 1);
    result = (inTagToSearch.empty()) ? rootNode.getChildNode(rnd_node_in_tree_i) : rootNode.getChildNode(inTagToSearch.c_str(), rnd_node_in_tree_i); // if we send empty tag name then retrieve any child OR retrieve the Nth "tag name" child.
  }
  else
  {
    outErr = "[utils random pick] Failed to find tag: <" + inTagToSearch + "> to pick from root node: " + rootNode.getName() + ". Check your XML file.";
  }

  if (flag_removePicked)
    return result; // this is a pointer of the node, if we add the node to other element it will detach it from original node.

  return result.deepCopy();
}

// -------------------------------------------

IXMLNode
missionx::Utils::xml_get_node_randomly_by_name_and_distance_IXMLNode (const IXMLNode & rootNode, const std::string &inTagToSearch, const double inLat, const double inLon, std::string& outErr, double inMinDistance, double inMaxDistance, const bool flag_removePicked)
{
  outErr.clear();
  std::vector<IXMLNode> vecFilteredResults;
  IXMLNode              result            = IXMLNode::emptyIXMLNode;
  IXMLNode              closestResult_ptr = IXMLNode::emptyIXMLNode;

  vecFilteredResults.clear();

  // if min/max defined
  if (inMinDistance < 0.0 || inMinDistance == inMaxDistance)
    inMinDistance = 0.0;

  if (inMaxDistance < 0.0)
    inMaxDistance = 0.0;

  if (inMinDistance > inMaxDistance && inMaxDistance > 0.0)
  {
    const double tmp = inMaxDistance;
    inMaxDistance    = inMinDistance;
    inMinDistance    = tmp;
  }


  // if no distance restrictions
  if (inMinDistance == 0.0 && inMaxDistance == 0.0)
  {
    result = Utils::xml_get_node_randomly_by_name_IXMLNode(rootNode, inTagToSearch, outErr, flag_removePicked);
    return result;
  }
  else
  {
    double prevDistance = 0.0;
    /// filter points
    double lat_d = 0.0, lon_d = 0.0;
    int    nChilds = rootNode.nChildNode(inTagToSearch.c_str());
    for (int i1 = 0; i1 < nChilds; ++i1)
    {
      IXMLNode cNode = rootNode.getChildNode(i1);
      if (cNode.isEmpty())
        continue;

      lat_d = Utils::readNumericAttrib(cNode, mxconst::get_ATTRIB_LAT(), 0.0);
      lon_d = Utils::readNumericAttrib (cNode, mxconst::get_ATTRIB_LONG(), 0.0);

      const double distance = Utils::calcDistanceBetween2Points_nm(inLat, inLon, lat_d, lon_d);

      // store closest result even if not in restricted distance
      if (i1 == 0)
      {
        closestResult_ptr = cNode;
        prevDistance      = distance;
      }
      else if (distance < prevDistance)
      {
        prevDistance      = distance;
        closestResult_ptr = cNode;
      }

      if ((distance >= inMinDistance && distance <= inMaxDistance) || (distance >= inMinDistance && inMaxDistance == 0.0))
        vecFilteredResults.emplace_back(cNode);
    }

    #ifndef RELEASE
    Log::logMsgThread("[get node randomly] after filtering distance.");
    #endif

    if (const int vecSize = static_cast<int> (vecFilteredResults.size ())
      ; vecSize == 0)
      result = closestResult_ptr;
    else
    {
      int random_i = Utils::getRandomIntNumber(0, vecSize - 1);

      #ifndef RELEASE
      Log::logMsgThread("\t[get node randomly] Pick index: " + Utils::formatNumber<int>(random_i));
      #endif
      if (random_i >= vecSize)
        random_i = vecSize - 1;

      result = vecFilteredResults.at(random_i);
    }
  }


  if (flag_removePicked)
    return result; // this is a pointer of the node, if we add the node to other element it will detach itself from original node.

  if (result.isEmpty())
    return IXMLNode::emptyIXMLNode;

  return result.deepCopy(); // return a copy of the XML Node
}

// -------------------------------------------

void
missionx::Utils::xml_delete_empty_nodes(IXMLNode& rootNode /*, std::deque<IXMLNode *> & nodesToDelete*/)
{
  /****
  Function searches for <point> elements and check their validity.
  If lat or long attributes are empty, the are flagges as
  ****/
  std::string err;
  err.clear();

  if (rootNode.isEmpty())
    return;

  if (mxconst::get_ELEMENT_POINT() == rootNode.getName())
  {
    const std::string lat = Utils::readAttrib(rootNode, mxconst::get_ATTRIB_LAT(), "");
    const std::string lon = Utils::readAttrib (rootNode, mxconst::get_ATTRIB_LONG(), "");

    if (lat.empty() * lon.empty())
    {
      rootNode.deleteNodeContent(); // DELETE Node
    }

  } // if in point
  else if (mxconst::get_ELEMENT_ITEM() == rootNode.getName())
  {

    if (std::string barcode = Utils::readAttrib (rootNode, mxconst::get_ATTRIB_BARCODE(), "")
      ; barcode.empty())
    {
      rootNode.deleteNodeContent(); // DELETE Node
    }
  }
  else
  {
    // Drill Down
    const int nChilds = rootNode.nChildNode();
    for (int i1 = 0; i1 < nChilds; ++i1)
    {
      IXMLNode cNode = rootNode.getChildNode(i1);
      Utils::xml_delete_empty_nodes(cNode);
    }
  }

} // xml_delete_empty_point_nodes


// -------------------------------------------



std::string
missionx::Utils::xml_get_attribute_value_drill (const IXMLNode & node, const std::string &inAttribName, bool& flag_found, const std::string &inTagName)
{
  flag_found = false;
  /****
  Function searches for specific attribute value, either in specific element or the first it finds.
  ****/

  if (node.isEmpty())
    return missionx::EMPTY_STRING;

  #ifndef RELEASE
  [[maybe_unused]]
  const bool flag_canSearch = inTagName.empty() + !inTagName.empty() * (inTagName == node.getName()); // newer implementation
  #endif // !RELEASE


  if (inTagName.empty() + !inTagName.empty() * (inTagName == node.getName()))
  {
    if (node.isAttributeSet(inAttribName.c_str()))
    {
      flag_found = true;
      return Utils::readAttrib(node, inAttribName, "");
    }
  }
  else
  {
    // Drill Down
    const auto nChilds = node.nChildNode();
    for (int i1 = 0; i1 < nChilds; ++i1)
    {
      IXMLNode    cNode = node.getChildNode(i1);
      std::string val   = Utils::xml_get_attribute_value_drill(cNode, inAttribName, flag_found, inTagName);
      if (flag_found)
        return val;
    }
  }

  return missionx::EMPTY_STRING;
}

// -------------------------------------------

bool
missionx::Utils::xml_delete_all_node_attribute(IXMLNode& inNode)
{
  if (inNode.isEmpty ())
    return false;

  const int nAttribs = inNode.nAttribute();
  for (int i1 = 0; i1 < nAttribs; ++i1)
  {
    inNode.deleteAttribute(0);
  }


  return true;
}

// -------------------------------------------


bool
missionx::Utils::xml_copy_node_attributes (const IXMLNode & sNode, IXMLNode& tNode, const bool flag_includeClearData)
{

  if (sNode.isEmpty())
    return false;

  if (tNode.isEmpty ())
    tNode = IXMLNode ();

  const int nAttribs = sNode.nAttribute();
  for (int i1 = 0; i1 < nAttribs; ++i1)
  {
    const IXMLAttr attrib = sNode.getAttribute(i1);
    tNode.addAttribute(attrib.sName, attrib.sValue);
  }

  if (flag_includeClearData)
  {
    const int nClear = sNode.nClear();
    for (int i1 = 0; i1 < nClear; ++i1)
    {
      IXMLClear c = sNode.getClear(i1);
      tNode.addClear(c.sValue, c.sOpenTag, c.sCloseTag);
    }
  } // end add clear

  return true;
}

// -------------------------------------------

bool
missionx::Utils::xml_copy_node_attributes_excluding_black_list (const IXMLNode & sNode, IXMLNode& tNode, std::set<std::string>* inExclude, const bool flag_includeClearData)
{
  if (sNode.isEmpty () || tNode.isEmpty ())
    return false;

  const int nAttribs = sNode.nAttribute();
  for (int i1 = 0; i1 < nAttribs; ++i1)
  {
    IXMLAttr attrib = sNode.getAttribute(i1);

    // small function to decide if attribute should be copied or not. Skip the ones that are not like "name" in most cases because we do not want to override them
    const auto lmbda_is_attribute_valid_for_copy = [&](const IXMLAttr& attrib) {
      if (inExclude != nullptr) // check if in set
      {
        if (!inExclude->contains(attrib.sName)) // if not found then return true
          return true;
        else
          return false;
      }

      return true;
    };

    if (lmbda_is_attribute_valid_for_copy(attrib))
    {
      tNode.updateAttribute(&attrib, &attrib);
    }
  }

  if (flag_includeClearData)
  {
    const int nClear = sNode.nClear();
    for (int i1 = 0; i1 < nClear; ++i1)
    {
      const IXMLClear c = sNode.getClear(i1);
      tNode.addClear(c.sValue, c.sOpenTag, c.sCloseTag);
    }
  } // end add clear

  return true;
}

// -------------------------------------------

bool
missionx::Utils::xml_copy_specific_attributes_using_white_list (const IXMLNode & sNode, IXMLNode& tNode, std::set<std::string>* inWhiteList, const bool flag_includeClearData)
{
  if (sNode.isEmpty () || tNode.isEmpty ())
    return false;

  const int nAttribs = sNode.nAttribute();
  for (int i1 = 0; i1 < nAttribs; ++i1)
  {
    IXMLAttr attrib = sNode.getAttribute(i1);

    // small program to decide if attribute should be copied or not. Skip the ones that are not like "name" in most cases because we do not want to override them
    const auto lmbda_is_attribute_valid_for_copy = [&](const IXMLAttr& attrib) {
      if (inWhiteList != nullptr) // check if in set
      {
        if (inWhiteList->contains(attrib.sName))
          return true;
        else
          return false;
      }

      return true;
    };

    if (lmbda_is_attribute_valid_for_copy(attrib))
    {
      tNode.updateAttribute(&attrib, &attrib);
    }
  }

  if (flag_includeClearData)
  {
    int nClear = sNode.nClear();
    for (int i1 = 0; i1 < nClear; ++i1)
    {
      IXMLClear c = sNode.getClear(i1);
      tNode.addClear(c.sValue, c.sOpenTag, c.sCloseTag);
    }
  } // end add clear

  return true;
}

// -------------------------------------------

void
missionx::Utils::xml_clear_node_attributes_excluding_list ( IXMLNode &sNode, const std::string &inExcludeList_s, const bool flag_includeClearData )
{
  const auto vecExcludeList = mxUtils::split_v2 ( inExcludeList_s, ",", false );

  if (!sNode.isEmpty())
  {
    const int nAttribs = sNode.nAttribute ();
    for (int i1 = 0; i1 < nAttribs; ++i1)
    {
      const IXMLAttr attrib = sNode.getAttribute ( i1 );
      if ( mxUtils::isStringExistsInVec ( vecExcludeList, attrib.sName ) )
        continue;

      sNode.updateAttribute ( "", attrib.sName, i1 );
    }

    if ( flag_includeClearData )
    {
      const int nClear = sNode.nClear ();
      for ( int i1 = 0; i1 < nClear; ++i1 )
      {
        IXMLClear c = sNode.getClear ( i1 );
        sNode.addClear ( c.sValue, c.sOpenTag, c.sCloseTag );
      }
    } // end handle clear node
  }
}

// -------------------------------------------

IXMLNode
missionx::Utils::xml_get_node_from_node_tree_IXMLNode (const IXMLNode & pNode, const std::string &inSearchedElementName, const bool flag_returnCopy)
{
  IXMLNode node = IXMLNode::emptyIXMLNode;

  // check root node is valid
  if (pNode.isEmpty())
    return IXMLNode::emptyIXMLNode;

  // Check if child node exists
  node = pNode.getChildNode(inSearchedElementName.c_str());
  if (!node.isEmpty())
  {
    if (flag_returnCopy)
      return node.deepCopy();
    else
      return node;
  }

  // search in child nodes recursively
  const int nChilds = pNode.nChildNode();
  for (int i1 = 0; i1 < nChilds; ++i1)
  {
    IXMLNode cNode = pNode.getChildNode(i1);
    node           = Utils::xml_get_node_from_node_tree_IXMLNode(cNode, inSearchedElementName, flag_returnCopy);
    if (!node.isEmpty())
    {
      if (flag_returnCopy)
        return node.deepCopy();
      else
        return node;
    }
  }

  return node.deepCopy(); // return empty node
}

// -------------------------------------------

IXMLNode missionx::Utils::xml_get_node_pointer_from_node_tree_by_attrib_name_and_value_IXMLNode(IXMLNode& pNode, const std::string &inSearchedElementName, const std::string &inAttribName, const std::string &attribValue, const bool flag_searchAllAttributesWithName)
{
  return xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(pNode, inSearchedElementName, inAttribName, attribValue, false, flag_searchAllAttributesWithName);
}

// -------------------------------------------

IXMLNode
missionx::Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(IXMLNode& pNode, const std::string &inSearchForElementName, const std::string &inAttribName, const std::string &inAttribValue, const bool flag_returnCopy, const bool flag_searchAllAttributesWithName)
{
  // check root node is valid
  if (pNode.isEmpty())
    return IXMLNode::emptyIXMLNode;

  // Check if parent node itself is the correct node
  // check attributes
  const int nAttributes = pNode.nAttribute();
  #ifndef RELEASE
  [[maybe_unused]]
  std::string attrib_value;
  #endif

  for (int iLoop = 0; iLoop < nAttributes; ++iLoop)  // loop over all attributes regardless their name
  {
    const auto attrib = pNode.getAttribute(iLoop);
    if (inAttribName == attrib.sName)
    {
      // We separate the filter logic to correctly handle cases where we just want to search the "first attribute" vs "any attribute with name 'x' and value 'y' "
      if (inAttribValue == attrib.sValue)
      {
        #ifndef RELEASE
        attrib_value = attrib.sValue;
        #endif
        return (flag_returnCopy) ? pNode.deepCopy() : pNode;
      }
    }
    else
    {
      continue; // skip to next attribute
    }

    // v24.05.1 Decide if to exit after we found attrib with the same name or repeat search for same attribute name (maybe there are duplications)
    if (!flag_searchAllAttributesWithName)
      break; // exit loop
  } // exit loop over all attributes


  // search in child nodes recursively
  const int nodes_child_i = pNode.nChildNode();
  for (int i1 = 0; i1 < nodes_child_i; ++i1)
  {
    IXMLNode cNode = pNode.getChildNode(i1);
    IXMLNode node  = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(cNode
                                                                                         , inSearchForElementName
                                                                                         , inAttribName
                                                                                         , inAttribValue
                                                                                         , flag_returnCopy
                                                                                         , flag_searchAllAttributesWithName);
    if (!node.isEmpty())
      return node;
  }

  return IXMLNode::emptyIXMLNode;
}

// -------------------------------------------

std::string
missionx::Utils::xml_get_attribute_value (const IXMLNode &pNode, const std::string &attribName, bool& flag_found)
{
  flag_found = false;
  std::string value;
  value.clear();

  if (pNode.isEmpty ()) // v3.0.253.9.1
    return value;

  const int nAttrib = pNode.nAttribute();
  for (int i1 = 0; i1 < nAttrib; ++i1)
  {
    if (const IXMLAttr attrib = pNode.getAttribute (i1)
      ; attribName == attrib.sName)
    {
      flag_found = true;
      value      = attrib.sValue;

      return value;
    }
  }

  return value;
}

// -------------------------------------------

int
missionx::Utils::xml_find_node_location (const IXMLNode & pNode, const std::string &tagNameToSearch)
{
  const int nodes_child_i = pNode.nChildNode();
  for (int i1 = 0; i1 < nodes_child_i; ++i1)
  {
    if (IXMLNode cNode = pNode.getChildNode (i1)
      ; cNode.isEmpty())
    {
      if (std::string name = cNode.getName ()
        ; tagNameToSearch == name)
        return i1;
    }
  }

  return 0;
}

// -------------------------------------------

bool
missionx::Utils::xml_add_cdata(IXMLNode& node, const std::string &cdataString)
{
  if (node.isEmpty())
    return false;

  /// v3.0.219.13 delete all clear elements before adding CDATA
  const int nClear = node.nClear();
  for (int i1 = 0; i1 < nClear; ++i1)
    node.deleteClear(0); // v3.0.241.8 instead of i1, always delete 0 because it probably shifts

  [[maybe_unused]]
  IXMLClear* c = node.addClear(cdataString.c_str(), nullptr); // , "<![CDATA[", "]]>"); // v3.0.241.8 add to the start
  return true;
}

// ----------------------------------------------------------------


void missionx::Utils::add_xml_comment(IXMLNode& node, const std::string &inCommentString)
{
  if (!node.isEmpty())
  {
    node.addClear(inCommentString.c_str(), "<!--", "-->");
  }
}

// ----------------------------------------------------------------

bool
missionx::Utils::xml_search_and_set_node_text(IXMLNode& parentNode, const std::string& inTagName, const std::string &inTextValue, const std::string &inValueType, const bool flag_force_adding_missing_element)
{
  if (parentNode.isEmpty())
    return false;

  auto nodePtr = xml_get_node_from_node_tree_IXMLNode(parentNode, inTagName, false); // return pointer of the element

  if (nodePtr.isEmpty() && flag_force_adding_missing_element)
  {
    nodePtr = parentNode.addChild(inTagName.c_str());
  }

  assert(!nodePtr.isEmpty() && "Failed to create node element");
  nodePtr.updateAttribute(inValueType.c_str(), mxconst::get_ATTRIB_TYPE().c_str(), mxconst::get_ATTRIB_TYPE().c_str());

  return xml_set_text(nodePtr, inTextValue);
}

// -------------------------------------------

void
missionx::Utils::xml_delete_all_text_subnodes(IXMLNode& node)
{
  if (!node.isEmpty())
  {
    for (int i1 = node.nText() - 1; i1 >= 0; --i1)
    {
      node.deleteText(i1);
    }
  }
}

// ----------------------------------------------------------------

bool
missionx::Utils::xml_set_text(IXMLNode& node, const std::string &inDefaultValue)
{
  if (node.isEmpty())
    return false;
  else
  {
    xml_delete_all_text_subnodes(node); // delete all text strings before writing new value
    node.addText(inDefaultValue.c_str());
    return true;
  }

  return false;
}

// ----------------------------------------------------------------

std::string
missionx::Utils::xml_get_text (const ITCXMLNode & node, const std::string &inDefaultValue)
{
  const IXMLNode xNode = node.deepCopy();
  return Utils::xml_get_text(xNode, inDefaultValue);
}

// ----------------------------------------------------------------

std::string
missionx::Utils::xml_get_text (const IXMLNode & node, const std::string &inDefaultValue)
{
  // return (node.getText() == NULL) ? inDefaultValue : std::string(node.getText());
  return (node.nText() < 1) ? inDefaultValue : std::string(node.getText());
}


// -------------------------------------------
int
missionx::Utils::getRandomIntNumber (const int inMin, const int inMax)
{
  if (inMin > inMax)
    return inMin;

  // Seed with a real random value, if available
  std::random_device              rd;
  std::mt19937                    gen(rd()); // Standard mersenne_twister_engine seeded with rd()
  std::uniform_int_distribution<> dis(inMin, inMax);

  return dis(gen);
}

// ----------------------------------------------------------------

void
missionx::Utils::xml_delete_attribute(IXMLNode& node, std::set<std::string>& inSetAttributes, const std::string &inTagName)
{

  if (node.isEmpty())
    return;

  std::string       tagNameToSearch         = inTagName;
  bool              flag_canSearchAndDelete = true;
  const std::string tagName                 = node.getName();

  if (inTagName.empty())
  {
    tagNameToSearch         = tagName;
    flag_canSearchAndDelete = true;
  }

  else if (inTagName == tagName)
    flag_canSearchAndDelete = true;

  // Can we conduct search and delete ?
  if (flag_canSearchAndDelete)
  {
    std::multimap<std::string, int> multiDelAttribList;
    multiDelAttribList.clear ();
    constexpr bool hasMore  = true;
    int  counter  = 0;
    int  nAttribs = node.nAttribute();

    while (hasMore && counter < nAttribs)
    {
      std::string name = node.getAttributeName(counter);
      if (inSetAttributes.contains(name))
      {

        node.deleteAttribute(name.c_str());
        // do not change counter location since the stack decreased in size
      }
      else
        ++counter;

      nAttribs = node.nAttribute(); // This is a MUST. we need to update attribute counter since we delete as we test
    }

  } // end if can search and delete


  //// Drill down to childrens if tag name is different than inTagName
  if (inTagName != tagName)
  {
    int node_child_i = node.nChildNode();
    for (int i1 = 0; i1 < node_child_i; ++i1)
    {
      IXMLNode cNode = node.getChildNode(i1);
      Utils::xml_delete_attribute(cNode, inSetAttributes, inTagName);
    }
  } // end drill to element that their tag is different from searched tag.
}

// -------------------------------------------

void
missionx::Utils::xml_add_comment(IXMLNode& node, const std::string &inCommentString)
{
  if (!node.isEmpty())
  {
    node.addClear(inCommentString.c_str(), "<!--", "-->");
  }
}

// -------------------------------------------

void missionx::Utils::xml_delete_all_subnodes(IXMLNode& pNode, const std::string &inSubNodeName, const bool inDelClear_b)
{
  std::string err;
  err.clear();

  if (pNode.isEmpty())
    return;

  // loop over all child nodes
  int node_child_i = (inSubNodeName.empty()) ? pNode.nChildNode() : pNode.nChildNode(inSubNodeName.c_str());
  for (int i1 = (node_child_i - 1); i1 >= 0; --i1)
  {
    auto node = (inSubNodeName.empty())? pNode.getChildNode(i1) : pNode.getChildNode(inSubNodeName.c_str(), i1);
    node.deleteNodeContent();
  }

  if (inDelClear_b)
  {
    node_child_i = pNode.nClear();
    for (int i1 = node_child_i - 1; i1 >= 0; --i1)
      pNode.deleteClear(i1);
  }

}

// -------------------------------------------

void
missionx::Utils::xml_delete_all_subnodes_except(IXMLNode& pNode, const std::string &inSubNodeName, const bool inDelClear_b, const std::string& exceptElementWithTagAndAttribAndValue)
{


  std::string exceptTag, andExceptWithAttribName, andExceptWithAttribValue;
  if (auto vecExceptionTag = Utils::split (exceptElementWithTagAndAttribAndValue, ',')
    ; vecExceptionTag.size() > 2)
  {
    exceptTag = vecExceptionTag.at(0);
    andExceptWithAttribName  = vecExceptionTag.at(1);
    andExceptWithAttribValue = vecExceptionTag.at(2);
  }
  else
    exceptTag = andExceptWithAttribName = andExceptWithAttribValue = "";

  if (pNode.isEmpty())
    return;

  // loop over all child nodes, delete all who are not in the exception rule
  int node_child_i = (inSubNodeName.empty()) ? pNode.nChildNode() : pNode.nChildNode(inSubNodeName.c_str());
  for (int i1 = (node_child_i - 1); i1 >= 0; --i1)
  {
    auto node = (inSubNodeName.empty()) ? pNode.getChildNode(i1) : pNode.getChildNode(inSubNodeName.c_str(), i1);

    const std::string val  = Utils::readAttrib(node, andExceptWithAttribName, "");
    #ifndef RELEASE
    [[maybe_unused]]
    const auto name = node.getName ();
    #endif

    if (!node.isEmpty() && (exceptTag == node.getName()
        && !andExceptWithAttribName.empty() && !andExceptWithAttribValue.empty()
        && andExceptWithAttribValue == Utils::readAttrib(node, andExceptWithAttribName, "")  )
       )
    {
      continue; // skip this element, do not delete
    }
    else
      node.deleteNodeContent();
  }

  if (inDelClear_b)
  {
    node_child_i = pNode.nClear();
    for (int i1 = node_child_i - 1; i1 >= 0; --i1)
      pNode.deleteClear(i1);
  }
}

// -------------------------------------------

double
missionx::Utils::getRandomRealNumber (const double inMin, const double inMax)
{
  if (inMin > inMax)
    return inMin;

  // Seed with a real random value, if available
  std::random_device               rd;
  std::mt19937                     gen(rd()); // Standard mersenne_twister_engine seeded with rd()
  std::uniform_real_distribution<> dis(inMin, inMax);

  return dis(gen);
}

IXMLNode missionx::Utils::xml_create_node_from_string (const std::string& inStringNode)
{
  IXMLDomParser iDom;
  IXMLResults   pResults;
  const auto    xChildNode = iDom.parseString (inStringNode.c_str (), "", &pResults);

  return xChildNode.deepCopy();
}

IXMLNode missionx::Utils::xml_create_message (const std::string& inMsgName, const std::string &inText)
{
  //const missionx::mxconst mx_const; // v25.04.2
  IXMLNode xMsg = Utils::xml_get_node_from_XSD_map_as_acopy(mxconst::get_ELEMENT_MESSAGE ());
  xMsg.updateAttribute(inMsgName.c_str(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str());

  if (inText.empty())
    return xMsg;

  IXMLNode mix = Utils::xml_get_or_create_node_ptr(xMsg, mxconst::get_ELEMENT_MIX(), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE(), mxconst::get_CHANNEL_TYPE_TEXT());
  if (!mix.isEmpty())
    Utils::xml_add_cdata(mix, inText);


  return xMsg;
}

// -------------------------------------------

bool missionx::Utils::xml_update_message_text(IXMLNode& pNode, const std::string& inMsgName, const std::string &inText)
{
  //const missionx::mxconst mx_const; // v25.04.2
  IXMLNode xMsg = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(pNode, mxconst::get_ELEMENT_MESSAGE (), mxconst::get_ATTRIB_NAME(), inMsgName, false); // return pointer to message
  if (xMsg.isEmpty())
    return false;

  IXMLNode mix = Utils::xml_get_or_create_node_ptr(xMsg, mxconst::get_ELEMENT_MIX(), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE(), mxconst::get_CHANNEL_TYPE_TEXT()); //
  if (mix.isEmpty())
    return false;
  else
    Utils::xml_add_cdata(mix, inText);

  return true;
}



// -------------------------------------------

std::string
missionx::Utils::convert_string_to_24_min_numbers (const std::string &inTimeIn24Hfortmat, int& outHour, int& outMinutes, int& outCycles)
{
  outCycles = -1; // -1 means that plugin will have to calculate by itself

  std::string err;
  err.clear();

  // split inTimeIn24Hfortmat
  auto        listTimeSplit = Utils::splitStringToList (inTimeIn24Hfortmat, ":");
  std::string hours_s;
  hours_s.clear ();

  if (!listTimeSplit.empty()) // > 0
  {
    hours_s = listTimeSplit.front(); // vec[0]
    listTimeSplit.pop_front();
    if (!hours_s.empty())
      outHour = Utils::stringToNumber<int>(hours_s);
  }

  if (!listTimeSplit.empty()) // > 1
  {
    std::string min_s = listTimeSplit.front (); // vec[1]
    listTimeSplit.pop_front();
    if (!min_s.empty())
      outMinutes = Utils::stringToNumber<int>(min_s);
  }

  if (!listTimeSplit.empty()) // > 2
  {
    std::string cycles_s = listTimeSplit.front ();
    listTimeSplit.pop_front();
    if (!cycles_s.empty())
      outCycles = Utils::stringToNumber<int>(cycles_s);
  }

  if (hours_s.empty())
    err = "Hours provided is not in 24H format, please fix. Current format: " + inTimeIn24Hfortmat + ". Supported format: \"[h24]:[min]:[cycles]\" ";


  return err;
}

// -------------------------------------------

IXMLNode
missionx::Utils::xml_get_or_create_node_ptr(IXMLNode& pNode, const std::string &tagChildNodeName_s, const std::string &with_attrib_name, const std::string &with_attrib_value)
{
  assert(!pNode.isEmpty()); // crash if node is empty

  auto lmbda_get_node_ptr = [&]() {
    if (with_attrib_name.empty())
      return pNode.getChildNode(tagChildNodeName_s.c_str());

    return pNode.getChildNodeWithAttribute(tagChildNodeName_s.c_str(), with_attrib_name.c_str(), with_attrib_value.c_str()); // only search for the first node with these options
  };

  IXMLNode node_ptr = lmbda_get_node_ptr();

  if (node_ptr.isEmpty())
  {
    // v3.0.241.1 search node in our internal mapping
    node_ptr = Utils::xml_get_node_from_XSD_map_as_acopy(tagChildNodeName_s); // v3.0.301 use the function to instantiate the XDS node if not already done

    // make sure that child node belongs to the parent node (pNode)
    if (node_ptr.isEmpty()) // if we did not find the <tag> in "Utils::xml_xMainXSDNode()" then create one
      node_ptr = pNode.addChild(tagChildNodeName_s.c_str());
    else
      pNode.addChild(node_ptr);

    // v3.0.301 B3 if we searched by attrib re-update the child node so we won't need to handle it from the calling function?
    if (with_attrib_name.empty() == false && with_attrib_value.empty() == false)
      node_ptr.updateAttribute(with_attrib_value.c_str(), with_attrib_name.c_str(), with_attrib_name.c_str());
  }

  assert(!node_ptr.isEmpty()); // crash if node is empty

  return node_ptr;
}

// -------------------------------------------

IXMLNode
missionx::Utils::xml_get_or_create_node(IXMLNode pNode, const std::string &tagChildNodeName_s, const bool& in_flag_return_copy)
{
  assert(!pNode.isEmpty()); // crash if node is empty

  IXMLNode node = pNode.getChildNode(tagChildNodeName_s.c_str());
  if (node.isEmpty())
    node = pNode.addChild(tagChildNodeName_s.c_str());

  assert(!node.isEmpty()); // crash if node is empty

  if (in_flag_return_copy)
    return node.deepCopy();

  return node; // pointer
}

// -------------------------------------------

std::string
missionx::Utils::xml_read_cdata_node (const ITCXMLNode &inNode, const std::string &default_value)
{
  return missionx::Utils::xml_read_cdata_node(inNode.deepCopy(), default_value);
}

// -------------------------------------------

std::string
missionx::Utils::xml_read_cdata_node (const IXMLNode &inNode, std::string default_value)
{
  if (inNode.isEmpty())
    return default_value;

  // v3.305.4 new code, should resolve cases where we read <!-- as <![CDATA
  auto node_child_i = inNode.nClear();
  for (int i = 0; i < node_child_i; ++i)
  {
    auto clearNode = inNode.getClear(i);
    if (std::string(clearNode.sOpenTag) == "<![CDATA[")
      return clearNode.sValue;
  }

  //const std::string str = (inNode.nClear() > 0) ? inNode.getClear().sValue : default_value; // v3.305.4 cause a bug with there are comments in the same <leg>.
  return default_value;
}


// -------------------------------------------


std::string
missionx::Utils::DegreesMinutes(double ang, unsigned int num_dec_places, const bool flag_format)
{
  bool neg(false);
  if (ang < 0.0)
  {
    neg = true;
    ang = -ang;
  }

  const int    deg  = static_cast<int> (ang);
  double frac = ang - static_cast<double> (deg);

  frac *= 60.0;

  const int min = static_cast<int> (frac);

  frac = (frac - static_cast<double> (min)) * 100.0;



  std::ostringstream oss;

  if (neg)
  {
    oss << "-";
  }

  //  TODO: allow user to define delimiters separating
  //        degrees, minutes, and seconds.
  oss.setf(std::ios::fixed, std::ios::floatfield);

  if (flag_format)
    oss << deg << DEG_SIM;
  else
    oss << deg;


  if (flag_format)
  {
    oss.width(2);
    oss.fill('0');
    oss << min << "\'";
  }
  else
  {
    oss.width(2);
    oss.fill('0');
    oss << min;
    oss << static_cast<int> (frac);
  }


  return oss.str();
}

// -------------------------------------------

std::string
missionx::Utils::DegreesMinutesSeconds(double ang, const unsigned int num_dec_places, const bool flag_format)
{
  bool neg(false);
  if (ang < 0.0)
  {
    neg = true;
    ang = -ang;
  }

  const int deg  = static_cast<int> (ang);
  double    frac = ang - static_cast<double> (deg);

  frac *= 60.0;

  int min = static_cast<int> (frac);

  frac = frac - static_cast<double> (min);

  // fix the DDD MM 60 case
  // TODO: nearbyint isn't always available (Visual C++, for example)
  double sec = nearbyint(frac * 600000.0);
  sec /= 10000.0;

  if (sec >= 60.0)
  {
    min++;
    sec -= 60.0;
  }

  std::ostringstream oss;

  if (neg)
  {
    oss << "-";
  }

  //  TODO: allow user to define delimiters separating
  //        degrees, minutes, and seconds.
  oss.setf(std::ios::fixed, std::ios::floatfield);

  if (flag_format)
    oss << deg << DEG_SIM;
  else
    oss << deg;

  oss.fill('0');
  if (flag_format)
  {
    oss.width(2);
    oss << min << "\'";
  }
  else
    oss << min;

  if (num_dec_places == 0)
  {
    if (flag_format)
      oss.width(2);

    oss.precision(0);
  }
  else
  {
    oss.width(num_dec_places + static_cast<unsigned int> (3));
    oss.precision(num_dec_places);
  }
  if (flag_format)
    oss << sec << "\"";
  else
    oss << sec;

  return oss.str();
}

// -------------------------------------------

std::string
missionx::Utils::DegreesMinutesSecondsLat (const double ang, const unsigned int num_dec_places, const bool flag_format)
{
  std::string lat(DegreesMinutesSeconds(ang, num_dec_places, flag_format));

  if (lat[0] == '-')
  {
    lat.erase(0, 1);

    if (flag_format)
      lat += std::string(" S");
    else
      lat = std::string("S") + lat;
  }
  else
  {
    if (flag_format)
      lat += std::string(" N");
    else
      lat = std::string("N") + lat;
  }

  if (flag_format)
    lat = std::string(" ") + lat;

  return lat;
}

// -------------------------------------------

std::string
missionx::Utils::DegreesMinutesSecondsLon (const double ang, const unsigned int num_dec_places, const bool flag_format)
{
  std::string lon(DegreesMinutesSeconds(ang, num_dec_places, flag_format));

  if (lon[0] == '-')
  {
    lon.erase(0, 1);
    if (flag_format)
      lon += std::string(" W");
    else
      lon = std::string("W") + lon;
  }
  else
  {
    if (flag_format)
      lon += std::string(" E");
    else
      lon = std::string("E") + lon;
  }

  if (fabs(ang) < 100.0)
  {
    lon = std::string("0") + lon;
  }

  return lon;
}

// -------------------------------------------

std::string
missionx::Utils::DegreesMinutesSecondsLat_XP (const double ang, const unsigned int num_dec_places, const bool flag_format)
{
  constexpr size_t MAX_FORMAT_LAT_LENGTH = 6;
  constexpr char   PADDING_CHAR          = '0';

  std::string lat = Utils::DegreesMinutes(ang, num_dec_places, flag_format);

  const bool flag_negative_angle = (lat[0] == '-') ? true : false;
  #ifndef RELEASE
  missionx::Log::logMsgNoneCR(",  " + lat);
  #endif
  if (flag_negative_angle)
    lat.erase(0, 1);


  if (fabs(ang) < 10.0) // fill zeros
    lat.insert(0, "0");

  if (flag_negative_angle)
  {
    lat = std::string("S") + lat;
  }
  else
  {
    lat = std::string("N") + lat;
  }

  // result = ang2_s;
  if (lat.length() >= MAX_FORMAT_LAT_LENGTH)
    return lat.substr(0, MAX_FORMAT_LAT_LENGTH);

  // pad Zeros to the end of the string
  lat.insert(lat.end(), (MAX_FORMAT_LAT_LENGTH - lat.length()), PADDING_CHAR);
  return lat;
}

// -------------------------------------------

std::string
missionx::Utils::DegreesMinutesSecondsLon_XP (const double ang, const unsigned int num_dec_places, const bool flag_format)
{
  constexpr size_t MAX_FORMAT_LON_LENGTH = 7;
  constexpr char   PADDING_CHAR          = '0';
  std::string lon = Utils::DegreesMinutes(ang, num_dec_places, flag_format);

  const bool flag_negative_angle = (lon[0] == '-') ? true : false;

#ifndef RELEASE
  missionx::Log::logMsgNoneCR("[lon] " + lon);
#endif

  if (flag_negative_angle)
    lon.erase(0, 1);

  // fill lzeros for the longitude (must contain 3 digits)
  if (fabs(ang) < 100.0)
  {
    if (fabs(ang) < 10.0) // add only zero respectively to angle (we need to fill with leading zeros)
      lon.insert(0, "00");
    else
      lon.insert(0, "0");
  }

  if (flag_negative_angle)
  {
    lon = std::string("W") + lon;
  }
  else
  {
    lon = std::string("E") + lon;
  }

  if (lon.length() >= MAX_FORMAT_LON_LENGTH)
    return lon.substr(0, MAX_FORMAT_LON_LENGTH);

  // pad Zeros to the end of the string. Do not move this code before prev statement or Mission-X will crash.
  lon.insert(lon.end(), (MAX_FORMAT_LON_LENGTH - lon.length()), PADDING_CHAR);

  return lon;
}

// -------------------------------------------

std::string
missionx::Utils::db_extract_list_into_sql_string (const std::list<std::string> &inList, const char inPrePost_char)
{
  std::string result;
  bool        flag_first_time = true;
  for (const auto& col : inList)
  {
    if (flag_first_time)
    {
      result = (inPrePost_char == '\0') ? col : inPrePost_char + col + inPrePost_char;
    }
    else
    {
      result.append(", ");
      result.append(std::string((inPrePost_char == '\0') ? col : inPrePost_char + col + inPrePost_char));
    }
    flag_first_time = false;
  }
  return result;
}

// -------------------------------------------

void
missionx::Utils::CalcWinCoords (const int inWinWidth, const int inWinHeight, const int inWinPad, int inColPad, int& left, int& top, int& right, int& bottom)
{
  // Screen coordinates
  int screenLeft, screenTop;
  XPLMGetScreenBoundsGlobal(&screenLeft, &screenTop, nullptr, nullptr);

  // Coordinates of our window
  left   = screenLeft + inWinPad;
  right  = left + inWinWidth;
  top    = screenTop - inWinPad;
  bottom = top - inWinHeight;
}

// -------------------------------------------

void
missionx::Utils::getWinCoords(int& left, int& top, int& right, int& bottom)
{
  left = top = right = bottom = 0;
  XPLMGetScreenBoundsGlobal(&left, &top, &right, &bottom);
}

// -------------------------------------------
// https://stackoverflow.com/questions/2419562/convert-seconds-to-days-minutes-and-seconds
std::string
missionx::Utils::format_number_as_hours(const double& inSeconds)
{
  auto            n   = static_cast<long long> (inSeconds); // convert to non decimal number
  const long long day = n / (24 * 3600);

  n                    = n % (24 * 3600);
  const long long hour = n / 3600;

  n %= 3600;
  const long long minutes = n / 60;

  n %= 60;
  const long long seconds = n;

  const std::string format_s = ((day > 0) ? mxUtils::formatNumber<long long> (day) + " " : "") + mxUtils::formatNumber<long long> (hour) + ":" + mxUtils::formatNumber<long long> (minutes) + ":" + mxUtils::formatNumber<long long> (seconds);

  return format_s;
}

// -------------------------------------------

std::string
missionx::Utils::getNavType_Translation (const XPLMNavType inType)
{
  switch (inType)
  {
    case xplm_Nav_Airport:
      return "airport";
      break;
    case xplm_Nav_NDB:
      return "NDB";
      break;
    case xplm_Nav_VOR:
      return "VOR";
      break;
    case xplm_Nav_Fix:
      return "Fix";
      break;
    case xplm_Nav_DME:
      return "DME";
      break;
    case xplm_Nav_LatLon:
      return "lat/lon";
      break;
    default:
      break;

  } // end switch

  return "";
}

// -------------------------------------------

void
missionx::Utils::load_cb(const char* real_path, void* ref)
{
  auto* dest = static_cast<XPLMObjectRef *> (ref);
  if (*dest == nullptr)
  {
    *dest = XPLMLoadObject(real_path);
  }
}

// -------------------------------------------

void
missionx::Utils::load_cb_dummy(const char* real_path, void* ref)
{

}

// -------------------------------------------

bool
missionx::Utils::isStringIsValidArithmetic (const std::string& inArithmetic)
{
  // loop over each character until you find "-,+,*,/"
  const std::string operators    = "+-*/"; //
  bool              isFirst      = true;
  std::string       num_s; // holds the number we are concatenating
  std::vector<int>  vecNumbers_i;
  std::vector<char> vecOperators_c;

  const auto lmbda_convert_string_to_integer_number = [inOperator = operators](std::string_view inNumber_s) {

  };

  for (const auto& c : inArithmetic)
  {
    if (std::isdigit(c))
      num_s += c;
    else if (operators.find(c) != std::string::npos)
    { // we have operator
      switch (c)
      {
        case '-':
          if (isFirst)
            num_s += c;
          else
          {
            vecNumbers_i.emplace_back(mxUtils::stringToNumber<int>(num_s));
            vecOperators_c.emplace_back('+'); // since we keep "-" with the number, we just need to add
            num_s = c;
          }
          break;
        case '+': // ignore
          if (isFirst)
            continue;
          else
          {
            vecNumbers_i.emplace_back(mxUtils::stringToNumber<int>(num_s));
            vecOperators_c.emplace_back(c); // since we keep "-" with the number, we just need to add
            num_s = c;
          }
          break;
        case '*':
        case '/':
          if (isFirst)
            return false; // not valid arithmetic
          else
          {
            vecNumbers_i.emplace_back(mxUtils::stringToNumber<int>(num_s));
            vecOperators_c.emplace_back(c); //
            num_s.clear();
          }
          break;
        default:
          return false; // unsupported operator
          break;
      } // end isFirst switch
    }
    else
      return false; // not a digit or operator, maybe a character

    isFirst = false;
  }


  return true;
}

bool
missionx::Utils::position_plane_in_ICAO(std::string inICAO, float lat, float lon, const float currentPlaneLat, const float currentPlanelon, const bool flag_FindNearestAirportIfIcaoIsNotValid)
{
  [[maybe_unused]]
  bool flag_positioned_plane_in_ICAO = false;
  #ifndef RELEASE
  const std::string_view FAILED_POSITIONING_VU = "Plugin might fail to position plane. Please position the plane in the starting icao and then try again.";
  #endif

  {
    XPLMNavRef navRef = XPLMFindNavAid(nullptr, inICAO.c_str(), nullptr, nullptr, nullptr, xplm_Nav_Airport);
    if (inICAO.empty() || navRef == XPLM_NAV_NOT_FOUND)
    {
      Log::logMsg("Failed to find airport with ICAO: " + inICAO);
      if ( flag_FindNearestAirportIfIcaoIsNotValid )
      {
        #ifndef RELEASE
        Log::logMsg("Search for nearest airport to target lat/lon");
        #endif // !RELEASE
        if (lat != 0.0f && lon != 0.0f)
        {

          if (XPLMNavRef local_navRef = XPLMFindNavAid (nullptr, nullptr, &lat, &lon, nullptr, xplm_Nav_Airport)
            ; local_navRef == XPLM_NAV_NOT_FOUND)
          {
            if (Utils::calcDistanceBetween2Points_nm(currentPlaneLat, currentPlanelon, lat, lon) > 100) // if target is more than 100nm from target
            {
              #ifndef RELEASE
              XPLMSpeakString(FAILED_POSITIONING_VU.data());
              #endif // !RELEASE
              Log::logMsg("Failed to find airport with ICAO: " + inICAO);
              return false;
            }
          }
          else
          {
            char ID[32]{ 0 };
            XPLMGetNavAidInfo(local_navRef, nullptr, &lat, &lon, nullptr, nullptr, nullptr, ID, nullptr, nullptr);
            inICAO = std::string(ID);
            if (inICAO.empty())
            {
              #ifndef RELEASE
              XPLMSpeakString(FAILED_POSITIONING_VU.data());
              #endif // !RELEASE

              return false;
            }
            else
            {
              flag_positioned_plane_in_ICAO = true;
              XPLMPlaceUserAtAirport(inICAO.c_str());
            }
          }
        }
        else
        {
          Log::logMsg("No alternative lat/lon provided so can't position plane. Fix the starting ICAO or provide a valid <location_adjust> lat/lon values. ");
          return false;
        }
      } // end flag_FindNearestAirportIfIcaoIsNotValid = true
      else
      {
        Log::logMsg("No ICAO by the name: " + inICAO +" found, Fix the starting ICAO.");
        return false;
      }
    } // end XPLM_NAV_NOT_FOUND
    else
    {
      flag_positioned_plane_in_ICAO = true;
      XPLMPlaceUserAtAirport(inICAO.c_str()); // position plane in ICAO
    }
  }


  return true;
}


// -------------------------------------------


std::string
missionx::Utils::get_time_as_string()
{
  // https://stackoverflow.com/questions/16357999/current-date-and-time-as-string
  const auto t  = std::time (nullptr);
  const auto tm = *std::localtime(&t);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
  // #ifndef RELEASE
  // auto str = oss.str();
  // return str;
  // #endif // !RELEASE

  return oss.str();
}

missionx::mx_clock_time_strct
missionx::Utils::get_os_time()
{
  // https://cplusplus.com/reference/ctime/tm/

  missionx::mx_clock_time_strct osTime;

  const std::chrono::system_clock::time_point now      = std::chrono::system_clock::now();
  const time_t                                tt       = std::chrono::system_clock::to_time_t(now);
  const tm                                    local_tm = *localtime(&tt);

  osTime.year           = local_tm.tm_year + missionx::mx_clock_time_strct::CHRONO_START_YEAR;
  osTime.month          = local_tm.tm_mon + 1;
  osTime.dayInMonth     = local_tm.tm_mday;
  osTime.dayInYear      = local_tm.tm_yday; // starts in 0..355, same as in x-plane dataref
  osTime.hour           = local_tm.tm_hour;
  osTime.minutes        = local_tm.tm_min;
  osTime.seconds_in_day = local_tm.tm_sec + (osTime.hour * 60 * 60);

  return osTime;
}

// -------------------------------------------


std::string
missionx::Utils::get_hash_string (const std::string &inValue)
{
  return mxUtils::formatNumber<size_t>(std::hash<std::string>{}(inValue));
}

// -------------------------------------------

void
missionx::Utils::xml_print_node(const IXMLNode& inNode, const bool bThread)
{
  if (inNode.isEmpty() == false)
  {
    IXMLRenderer xmlWriter;
    if (bThread) // v3.303.8 added thread support
      Log::logMsgThread(xmlWriter.getString(inNode));
    else
      Log::logMsg(xmlWriter.getString(inNode));

    xmlWriter.clear();
  }
}

// -------------------------------------------

std::string
missionx::Utils::xml_get_node_content_as_text (const IXMLNode & inNode)
{

  if (inNode.isEmpty() == false)
  {
    IXMLRenderer xmlWriter;
    const std::string text =  xmlWriter.getString(inNode);
    xmlWriter.clear();

    return text;
  }

  return "";
}

// -------------------------------------------

std::vector<IXMLNode>
missionx::Utils::xml_get_all_nodes_pointer_with_tagName (const IXMLNode & inRootNode, const std::string_view inTagName, const std::string_view inTagNameToIgnore)
{
  std::vector<IXMLNode> vec_xmlNodes_p;
  std::map<std::string, IXMLNode> map_vec_xmlNodes_p;

  // loop over all sub elements that are not "inTagName" and search same child nodes
  const auto node_child_i = inRootNode.nChildNode();
  for (int i1 = 0; i1 < node_child_i; ++i1)
  {
    if (IXMLNode childP = inRootNode.getChildNode (i1)
      ; !childP.isEmpty())
    {
      if (const auto tagName = childP.getName ()
        ; inTagNameToIgnore == tagName)
        continue; // skip ignored tags, maybe to resolve duplications when coping elements

      std::vector<IXMLNode> vec_nodesChild = Utils::xml_get_all_child_nodes_recurs_with_tagName (childP, inTagName);
      std::ranges::for_each (vec_nodesChild, [&vec_xmlNodes_p] (const auto &node) { vec_xmlNodes_p.emplace_back (node); });

      }

  } // end loop


  return vec_xmlNodes_p;
}

// -------------------------------------------

void
missionx::Utils::xml_add_node_to_parent_with_duplicate_filter(IXMLNode& inOutParentNode, const IXMLNode & inNode, const std::string_view inChildTagNameToFilter, const std::string_view inAttribNameHoldsFilterRule)
{
  if (inNode.isEmpty())
    return;


  if (inAttribNameHoldsFilterRule.empty())
    inOutParentNode.addChild(inNode.deepCopy());
  else
  {
    const auto attrib_value_new_node = Utils::readAttrib (inNode, inAttribNameHoldsFilterRule.data (), "");
    const auto p_vecChild_nodes      = Utils::xml_get_all_child_nodes_recurs_with_tagName (inOutParentNode, inChildTagNameToFilter);
    bool bFoundDuplicate = false;
    for (const auto& pNode : p_vecChild_nodes)
    {
      if (const auto attrib_value_child = Utils::readAttrib (pNode, inAttribNameHoldsFilterRule.data (), "")
        ; attrib_value_child == attrib_value_new_node)
      {
        #ifndef RELEASE
        Log::logMsgThread("[" + std::string(__func__) + "] Found duplicate node <" + inNode.getName() + "> with attribute: '" + inAttribNameHoldsFilterRule.data() + "' and value: " + attrib_value_new_node);
        #endif // !RELEASE

        bFoundDuplicate = true;
        break;
      }
    }

    if (!bFoundDuplicate)
      inOutParentNode.addChild(inNode.deepCopy());
  }
}

// -------------------------------------------


std::vector<mx_score_strct>
missionx::Utils::xml_extract_scoring_subElements (const IXMLNode & pNode, const std::string& inSubElement)
{
  const std::vector<std::string>   vecAttributes = { "best", "good", "average" };
  std::vector<mx_score_strct> result;
  if (pNode.isEmpty() || pNode.getChildNode(inSubElement.c_str()).isEmpty())
  {
    return result;
  }
  else
  {
    auto node = pNode.getChildNode(inSubElement.c_str());
    for ( const auto &attribName : vecAttributes )
    {
      if (const std::string attrib_value_s = Utils::readAttrib (node, attribName, "")
        ; attrib_value_s.empty())
      {
        Log::logMsg("Invalid score for: " + inSubElement);
        result.clear();
        break;
      }
      else
      {
        if (const auto vecValues = Utils::split (attrib_value_s, '|')
          ; vecValues.size() >2)
        {
          missionx::mx_score_strct attribute_scoring;
          attribute_scoring.min = mxUtils::stringToNumber<float>(vecValues.at(0), 8);
          attribute_scoring.max = mxUtils::stringToNumber<float>(vecValues.at(1), 8);
          attribute_scoring.score = mxUtils::stringToNumber<float>(vecValues.at(2), 6);

          result.emplace_back(attribute_scoring);
        }
        else
        {
          Log::logMsg( fmt::format("Invalid attribute_scoring values for attribute: {}. Must have 3 numbers, current attribute value is: {}", attribName, attrib_value_s) );
          result.clear();
          break;
        }
      }

    }
  }


  return result;
}

// -------------------------------------------

float
missionx::Utils::getScoreAfterAnalyzeMinMax(const std::vector<mx_score_strct>& inVecParsedScores, const double& inMin, const double& inMax)
{
  float result = 0.0f;
  if (inVecParsedScores.size() < 3)
    return result;
  else
  {
    bool foundMax = false;
    bool foundMin = false;
    for (const auto &[min, max, score] : inVecParsedScores)
    {

      if ( !foundMin && inMin>=min && inMin < max)
      {
        result += score;
        foundMin = true;
      }

      if (!foundMax && inMax > min && inMax <= max)
      {
        result += score;
        foundMax = true;
      }

      if (foundMin && foundMax)
        break;
    }
  }


  return result;
}

// -------------------------------------------

std::string
missionx::Utils::getAndFixStartingDayValue(const std::string& inStarting_day)
{
  // check starting day is a number without hyphen (-)
  if (!inStarting_day.empty() && mxUtils::is_number(inStarting_day) == false)
  {
    // remove from first space
    return inStarting_day.substr(0, inStarting_day.find_first_of(' '));
  }

  return inStarting_day;
}

// -------------------------------------------

IXMLNode
missionx::Utils::xml_add_child(IXMLNode& inParent, const std::string& inTagName, const std::string& inInitAttribName, const std::string& inInitAttribValue, const std::string& inTextValue)
{
  if (inTagName.empty())
    return IXMLNode::emptyIXMLNode;

  if (inParent.isEmpty())
    return IXMLNode::emptyIXMLNode;

  auto cNode = inParent.addChild(inTagName.c_str());
  if (!cNode.isEmpty())
  {
    if (!inInitAttribName.empty())
      cNode.updateAttribute(inInitAttribValue.c_str(), inInitAttribName.c_str(), inInitAttribName.c_str());

    if (!inTextValue.empty())
    {
      Utils::xml_set_text(cNode, inTextValue);
    }
  }

  return cNode;
}

// -------------------------------------------

IXMLNode
missionx::Utils::xml_add_child(IXMLNode& inParent, const std::string& inTagName, const std::string& inTextValue)
{
  return Utils::xml_add_child(inParent, inTagName, "", "", inTextValue);
}

// -------------------------------------------

IXMLNode
missionx::Utils::xml_add_error_child(IXMLNode& inParent, const std::string& inTextValue, const std::string& inTagName)
{
  return Utils::xml_add_child(inParent, inTagName, "", "", inTextValue);
}

// -------------------------------------------

IXMLNode
missionx::Utils::xml_add_warning_child(IXMLNode& inParent, const std::string& inTextValue, const std::string& inTagName)
{
  return Utils::xml_add_child(inParent, inTagName, "", "", inTextValue);
}

// -------------------------------------------

IXMLNode
missionx::Utils::xml_add_info_child(IXMLNode& inParent, const std::string& inTextValue, const std::string& inTagName)
{
  return Utils::xml_add_child(inParent, inTagName, "", "", inTextValue);
}


// -------------------------------------------

std::vector<IXMLNode>
missionx::Utils::xml_get_all_child_nodes_recurs_with_tagName(IXMLNode& inParent, const std::string_view inTagName)
{
  std::vector<IXMLNode> vec_nodesToReturn;
  const auto            nodes_child_i = inParent.nChildNode();

  #ifndef RELEASE
    [[maybe_unused]]
    auto tagName = inParent.getName(); // DEBUG
  #endif // !RELEASE

  // check if parent is same as searched <TAG> name
  if (inTagName == inParent.getName())
    vec_nodesToReturn.emplace_back(inParent);
  else
  {
    for (int i1 = 0; i1 < nodes_child_i; ++i1)
    {
      auto node = inParent.getChildNode(i1);

      if (inTagName == node.getName())
        vec_nodesToReturn.emplace_back(node);
      else
      {
        if (std::vector<IXMLNode> vec_nodesChild = Utils::xml_get_all_child_nodes_recurs_with_tagName (node, inTagName)
          ; !vec_nodesChild.empty())
          std::ranges::for_each (vec_nodesChild, [&vec_nodesToReturn](const auto& node) { vec_nodesToReturn.emplace_back(node); });
      }
    }
  } // end else if parent Node is == TagName

  return vec_nodesToReturn;
}

// -------------------------------------------

void
missionx::Utils::read_external_sql_query_file(std::map<std::string, std::string>& mapQueries, const std::string& inRootNodeName, const std::string& inFilePath)
{
  std::string err;

  const std::string &SQL_FOLDER_FILE_S = inFilePath;
  IXMLDomParser      domSql;
  const IXMLNode     xQueries = domSql.openFileHelper (SQL_FOLDER_FILE_S.c_str (), inRootNodeName.c_str (), &err).deepCopy (); // parse xml into ITCXMLNode
  for (int i1 = 0; i1 < xQueries.nChildNode() && err.empty(); ++i1)
  {
    IXMLNode    node = xQueries.getChildNode(i1);
    std::string name = Utils::readAttrib(node, mxconst::get_ATTRIB_NAME(), "");
    if (name.empty())
      continue;

    std::string text = Utils::xml_read_cdata_node(node, "");
    if (text.empty())
      continue;

    mapQueries[name] = text; // override older value or create new one
  }
  domSql.clear();
}

// -------------------------------------------

std::vector<std::string>
missionx::Utils::read_external_categories(const std::string& inRootNodeName, const std::string& inFilePath)
{
  std::string              err;
  std::vector<std::string> vec_emptyCategories;
  IXMLDomParser            domSql;

  vec_emptyCategories.clear ();

  const IXMLNode xCargo = domSql.openFileHelper(inFilePath.c_str(), inRootNodeName.c_str(), &err).deepCopy(); // parse xml into ITCXMLNode
  if (!err.empty())
  {
    Log::logMsgErr(err);
    return vec_emptyCategories;
  }

  IXMLNode nItems = xCargo.getChildNode(mxconst::get_ELEMENT_CARGO_CATEGORIES().c_str());

  const std::string sCategories= Utils::xml_get_text( nItems );


  return mxUtils::split(Utils::xml_get_text( nItems ), ',');
}

// -------------------------------------------

IXMLNode
missionx::Utils::read_external_blueprint_items(const std::string& inRootNodeName, const std::string& inSearchTagName, const std::string &inSubCategoryType, const bool inThread, const bool inCaseSensitive, const std::string& inFilePath)
{
  std::string   err;
  IXMLDomParser domSql;

  const IXMLNode xRoot = domSql.openFileHelper(inFilePath.c_str(), inRootNodeName.c_str(), &err).deepCopy(); // parse xml into ITCXMLNode
  if (!err.empty() || xRoot.isEmpty())
  {
    Log::logMsgErr(err);
    return IXMLNode::emptyIXMLNode;
  }


  int nodes_child_i = xRoot.nChildNode(mxconst::get_ELEMENT_ITEM_BLUEPRINTS().c_str());
  for (int iLoop1 = 0; iLoop1 < nodes_child_i; ++iLoop1)
  {
    auto      node        = xRoot.getChildNode(mxconst::get_ELEMENT_ITEM_BLUEPRINTS().c_str(), iLoop1);
    const int nAttributes = node.nAttribute();

    for (int iLoop2 = 0; iLoop2 < nAttributes; ++iLoop2) // loop over all attributes regardless of their name
    {
      const auto attrib = node.getAttribute(iLoop2);
      if (mxconst::get_ATTRIB_TYPE() == attrib.sName && mxUtils::compare( inSubCategoryType, attrib.sValue, inCaseSensitive) ) // insensitive test of text value
      {
        #ifndef RELEASE
        Utils::xml_print_node(node, inThread);
        #endif // !RELEASE

        return node.deepCopy();
      }
    } // end loop over all node attributes

  }

  return IXMLNode::emptyIXMLNode;
}




// -------------------------------------------

IXMLNode
missionx::Utils::xml_get_node_from_XSD_map_as_acopy (const std::string &inNodeName)
{
  if (Utils::xml_xMainXSDNode.isEmpty())
    Utils::prepare_static_XSD();

  return Utils::xml_xMainXSDNode.getChildNode(inNodeName.c_str()).deepCopy();
}

// -------------------------------------------

bool
missionx::Utils::is_it_an_airport(const std::string &inICAO)
{
  return (XPLMFindNavAid (nullptr, inICAO.c_str(), nullptr, nullptr, nullptr, xplm_Nav_Airport) != XPLM_NAV_NOT_FOUND);
}

// -------------------------------------------

void
missionx::Utils::prepare_static_XSD()
{
  const std::string missionx_xsd_map = R"(
<MAPPING> 
  <MISSION name="" title="" version="301" designer_mode="" />

  <CONVERSION />

  <mission_info mission_image_file_name="" plane_desc="" estimate_time="" difficulty="" other_settings="" scenery_settings="" />

  <global_settings>
    <folders sound_folder_name="sound" obj3d_folder_name="obj"  />
    <start_time day_in_year="" hours="" min="" />
    <base_weights_kg pilot="" passengers="0" storage="" />
    <position auto_position_plane="true" />
  </global_settings> 

  <briefer starting_icao="" starting_leg="" >
    <location_adjust lat="" long="" elev_ft="0" heading_psi="" pause_after_location_adjust="" starting_speed_mt_sec="" start_cold_and_dark="" />       
  </briefer>     

  <leg name="" title="" next_leg="" >  
    <start_leg_message name="" />     
    <link_to_objective name="" /> 
    <desc/>     
    <post_leg_message name="" /> 
  </leg> 
   
  <objective name="" /> 
  <task name="" base_on_trigger="" base_on_script="" eval_success_for_n_sec="" mandatory="" force_evaluation=""/>         
 
  <trigger name="" type="rad" rearm="" post_script="" > 
    <conditions plane_on_ground="" cond_script="" /> 
    <outcome message_name_when_fired="" message_name_when_left="" script_name_when_fired="" script_name_when_left="" commands_to_exec_when_fired="" commands_to_exec_when_left="" dataref_to_modify_when_fired=""  dataref_to_modify_when_left=""  set_task_as_success="" reset_task_state="" /> 
 
    <loc_and_elev_data>  
      <radius length_mt="" /> 
      <point lat="" long="" />   
      <elevation_volume  elev_lower_upper_ft="" />     
    </loc_and_elev_data> 
  </trigger> 
 
  <mix track_type="" sound_file=""  sound_vol="" /> 

  <message name="" >
      <mix track_type="text"  mute_xplane_narrator="" hide_text="" override_seconds_to_display_text="" override_seconds_calc_per_line="" label="radio" label_color="yellow" /> 
      <mix track_type="comm" sound_file=""  sound_vol="" /> 
      <mix track_type="back" sound_file=""  sound_vol="" /> 
  </message> 

  <inventory name="" type="rad" > 
    <item name="" barcode="" quantity="1"  weight_kg="0"/> 
    <loc_and_elev_data> 
      <radius length_mt="100" /> 
      <point lat="" long=""/> 
    </loc_and_elev_data> 
  </inventory> 
   
  <item name="" barcode="" quantity="0"  weight_kg="0"/> 
   
  <point lat="" long=""/> 

  <location lat="" lon="" elev_ft=""  elev_above_ground_ft=""/> 
	<conditions distance_to_display_nm="10" keep_until_leg="" cond_script=""/>
	<tilt heading_psi="0" pitch="0" roll="0"/>
  
  <template_markers_obj3d>
			<obj3d name="marker01" file_name="marker_five_parts_02.obj">
				<conditions distance_to_display_nm="10" keep_until_leg="" cond_script=""/>
				<location lat="" long="" elev_ft="0" elev_above_ground_ft=""/>
				<tilt heading_psi="0" pitch="0" roll="0"/>
			</obj3d>
			<obj3d name="marker02" file_name="mx_arrow_up_15m.obj">
				<conditions distance_to_display_nm="10" keep_until_leg="" cond_script=""/>
				<location lat="" long="" elev_ft="0" elev_above_ground_ft=""/>
				<tilt heading_psi="0" pitch="0" roll="0"/>
			</obj3d>
			<obj3d name="marker03" file_name="mx_arrow_down_15m.obj">
				<conditions distance_to_display_nm="10" keep_until_leg="" cond_script=""/>
				<location lat="" long="" elev_ft="0" elev_above_ground_ft=""/>
				<tilt heading_psi="0" pitch="0" roll="0"/>
			</obj3d>
			<obj3d name="marker04" file_name="mx_tall_marker_down_arrow_15x60.obj">
				<conditions distance_to_display_nm="10" keep_until_leg="" cond_script=""/>
				<location lat="" long="" elev_ft="0" elev_above_ground_ft=""/>
				<tilt heading_psi="0" pitch="0" roll="0"/>
			</obj3d>
			<obj3d name="marker05" file_name="mx_arrow_down_60m.obj">
				<conditions distance_to_display_nm="10" keep_until_leg="" cond_script=""/>
				<location lat="" long="" elev_ft="0" elev_above_ground_ft=""/>
				<tilt heading_psi="0" pitch="0" roll="0"/>
			</obj3d>
			<obj3d name="marker06" file_name="mx_arrow_up_60m.obj">
				<conditions distance_to_display_nm="10" keep_until_leg="" cond_script=""/>
				<location lat="" long="" elev_ft="0" elev_above_ground_ft=""/>
				<tilt heading_psi="0" pitch="0" roll="0"/>
			</obj3d>
			<obj3d name="marker07" file_name="mx_static_text_40m.obj">
				<conditions distance_to_display_nm="10" keep_until_leg="" cond_script=""/>
				<location lat="" long="" elev_ft="0" elev_above_ground_ft=""/>
				<tilt heading_psi="0" pitch="0" roll="0"/>
			</obj3d>
			<obj3d name="marker08" file_name="mx_text_rotation_40m.obj">
				<conditions distance_to_display_nm="10" keep_until_leg="" cond_script=""/>
				<location lat="" long="" elev_ft="0" elev_above_ground_ft=""/>
				<tilt heading_psi="0" pitch="0" roll="0"/>
			</obj3d>
			<obj3d name="marker09" file_name="mx_text_rotation_smooth01_40m.obj">
				<conditions distance_to_display_nm="10" keep_until_leg="" cond_script=""/>
				<location lat="" long="" elev_ft="0" elev_above_ground_ft=""/>
				<tilt heading_psi="0" pitch="0" roll="0"/>
			</obj3d>
			<obj3d name="marker10" file_name="mx_text_rotation_smooth01_250m.obj">
				<conditions distance_to_display_nm="10" keep_until_leg="" cond_script=""/>
				<location lat="" long="" elev_ft="0" elev_above_ground_ft=""/>
				<tilt heading_psi="0" pitch="0" roll="0"/>
			</obj3d>
			<obj3d name="marker11" file_name="mx_x_rotation_smooth_42m.obj">
				<conditions distance_to_display_nm="10" keep_until_leg="" cond_script=""/>
				<location lat="" long="" elev_ft="0" elev_above_ground_ft=""/>
				<tilt heading_psi="0" pitch="0" roll="0"/>
			</obj3d>
			<obj3d name="marker12" file_name="mx_x_rotation_smooth_128m.obj">
				<conditions distance_to_display_nm="10" keep_until_leg="" cond_script=""/>
				<location lat="" long="" elev_ft="0" elev_above_ground_ft=""/>
				<tilt heading_psi="0" pitch="0" roll="0"/>
			</obj3d>
			<obj3d name="marker13" file_name="mx_arrow_down_5m.obj">
				<conditions distance_to_display_nm="10" keep_until_leg="" cond_script=""/>
				<location lat="" long="" elev_ft="0" elev_above_ground_ft=""/>
				<tilt heading_psi="0" pitch="0" roll="0"/>
			</obj3d>
			<obj3d name="marker14" file_name="mx_arrow_down_narrow_5m.obj">
				<conditions distance_to_display_nm="10" keep_until_leg="" cond_script=""/>
				<location lat="" long="" elev_ft="0" elev_above_ground_ft=""/>
				<tilt heading_psi="0" pitch="0" roll="0"/>
			</obj3d>
			<obj3d name="marker15" file_name="mx_static_text_8m.obj">
				<conditions distance_to_display_nm="10" keep_until_leg="" cond_script=""/>
				<location lat="" long="" elev_ft="0" elev_above_ground_ft=""/>
				<tilt heading_psi="0" pitch="0" roll="0"/>
			</obj3d>
			<obj3d name="marker16" file_name="mx_text_rotation_smooth01_8m.obj">
				<conditions distance_to_display_nm="10" keep_until_leg="" cond_script=""/>
				<location lat="" long="" elev_ft="0" elev_above_ground_ft=""/>
				<tilt heading_psi="0" pitch="0" roll="0"/>
			</obj3d>
			<obj3d name="marker17" file_name="mx_x_rotation_smooth_10m.obj">
				<conditions distance_to_display_nm="10" keep_until_leg="" cond_script=""/>
				<location lat="" long="" elev_ft="0" elev_above_ground_ft=""/>
				<tilt heading_psi="0" pitch="0" roll="0"/>
			</obj3d>
  </template_markers_obj3d>


  <display_object name="" instance_name="" />

  <end_mission>
    <success_image file_name="success.png"/>
    <success_msg><![CDATA[well done pilot. Have a nice day and see you tomorrow. ]]></success_msg>
    <success_sound sound_file="" sound_vol=""/>
    <fail_image file_name="fail.png"/>
    <fail_msg><![CDATA[You failed !!!]]></fail_msg>
    <fail_sound sound_file="" sound_vol=""/>
  </end_mission>

  <scoring>
    <pitch best="-10|15|0.5" good="-15|18|0.4" average="-20|20|0.25" />
    <roll best="-25|25|0.5" good="-30|30|0.4" average="-35|35|0.25" />
    <gforce best="-0.5|1.55|0.5" good="1.55|1.82|0.4" average="1.82|1.95|0.25" />
    <center_line best="-1.3|1.3|0.5" good="-1.8|1.8|0.35" average="-2.3|2.3|0.25" />
  </scoring>  

</MAPPING>
)";


  IXMLResults xResult;
  missionx::Utils::xml_xMainXSDNode = missionx::Utils::xml_iDomXSD.parseString(missionx_xsd_map.c_str(), mxconst::get_MAPPING_ROOT_DOC().c_str(), &xResult);

  #ifndef RELEASE
  if (missionx::Utils::xml_xMainXSDNode.isEmpty())
  {
    const std::string translateError = IXMLRenderer::getErrorMessage(xResult.errorCode);
    Log::logMsgNone("[XSD] error in generating internal mapping: " + translateError + ", line: " + mxUtils::formatNumber<long long>(xResult.nLine) + ", column: " + mxUtils::formatNumber<int>(xResult.nColumn) + " \n");
  }
  else
  {
    IXMLRenderer render;
    Log::logMsgNone("Internal Mapping:\n================\n" + std::string(render.getString(missionx::Utils::xml_xMainXSDNode)) + "\n");
  }
  #endif

} // end prepare_static_XSD()

// -------------------------------------------

std::string
missionx::Utils::get_earth_nav_dat_file()
{
  const std::string custom_dat_nav_data_path = "Custom Data/earth_nav.dat";
  const std::string default_nav_data_path    = "Resources/default data/earth_nav.dat";

  std::ifstream infs;
  bool          bFoundCustom = false;
  infs.open(custom_dat_nav_data_path.c_str(), std::ios::in);
  if (infs.is_open())
    bFoundCustom = true;

  if (infs.is_open())
    infs.close();

  return (bFoundCustom) ? custom_dat_nav_data_path : default_nav_data_path;
}

// -------------------------------------------

std::string
missionx::Utils::get_nav_dat_cycle()
{
  const std::string earth_nav_dat_file = get_earth_nav_dat_file ();
  std::ifstream     infs;
  std::cin.tie (nullptr);

#ifndef RELEASE
  missionx::Log::logMsgThread("[Utils Read ILS] Nav data file: " + earth_nav_dat_file);
#endif // !RELEASE

  // iterate for five rows and try to find "cycle nnnn," string
  infs.open(earth_nav_dat_file.c_str(), std::ios::in);
  if (infs.is_open())
  {
    int         counter_i = 0;
    std::string line;
    while ((getline(infs, line) && counter_i < 4))
    {
      counter_i++;
      // search for ".... 1802,"
      // although it is lame, it is still the same for xp11 and Navigraph
      if (const auto psik = line.find (',')
        ; psik != std::string::npos && psik > 4)
      {
        constexpr int CYCLE_CHARS = 4; // how many chars define the CYCLE
        if (const std::string c = line.substr (psik - CYCLE_CHARS, CYCLE_CHARS)
          ; mxUtils::is_digits (c))
        {
          infs.close ();
          return c;
        }
      }
    }
  }
  if (infs.is_open())
    infs.close();

  return mxconst::get_DEFAULT_CYCLE();
  ;
}

// -------------------------------------------

// -------------------------------------------

std::string
missionx::Utils::getJsonValue(nlohmann::json js, const std::string& key, std::string outDefaultValue)
{
  if (!js.is_discarded() && js.contains(key))
  {
    //if (js[key].is_null())
    //  return outDefaultValue;

    if (js[key].is_string())
      return js[key].get<std::string>();
  }

  return outDefaultValue;
}


// -------------------------------------------


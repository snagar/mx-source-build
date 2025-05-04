#ifndef UTILS_H_
#define UTILS_H_
#pragma once

/**************


**************/
#include <deque>
#include <random>
#include <set>
#include <unordered_set>
#include <filesystem>

#include <nlohmann/json.hpp>

#include "dataref_const.h"
#include "MxUtils.h"

#include "../io/IXMLParser.h"
#include "../io/Log.hpp"
#include "fmod.hpp"
//#include "fmod_errors.h"

// using namespace std;
using namespace missionx;

#ifdef __cplusplus
extern "C"
{
#endif

  //#define UNIX_EOL 13            /* Decimal code of Carriage Return char */
  //#define LF 10            /* Decimal code of Line Feed char */
  //#define EOF_MARKER 26    /* Decimal code of DOS end-of-file marker */
  //#define MAX_REC_LEN 2048 /* Maximum size of input buffer */

#ifdef __cplusplus
}
#endif

namespace missionx
{

// https://stackoverflow.com/questions/21892934/how-to-assert-if-a-stdmutex-is-locked
class mutex : public std::mutex
{
public:
  void lock()
  {
    std::mutex::lock();
    m_holder = std::this_thread::get_id();
  }



  void unlock()
  {
    m_holder = std::thread::id();
    std::mutex::unlock();
  }


  /**
   * @return true iff the mutex is locked by the caller of this method. */
  bool locked_by_caller() const { return m_holder == std::this_thread::get_id(); }


private:
  std::atomic<std::thread::id> m_holder;
};

} // namespace missionx

// -------------------------------------------
// -------------------------------------------


namespace missionx
{

static dataref_const drefConst;

class Utils : public mxUtils
{
private:
  static ITCXMLNode xml_xMainXSDNode; // = iDom.openFileHelper(pluginDataFullFileNameDir.c_str(), mxconst::get_MISSION_ELEMENT().c_str(), &errMsg);
  static missionx::mutex s_CalcDistMutex; // v3.305.2 Used with the sorting trigger function to optimize evaluation performace

public:
  Utils();
  virtual ~Utils();

  static int seqTimerFunc;

  // v3.0.241.1 moved from data_manager class.
  ///// XML Mission-X mission XSD like factory
  static IXMLDomParser xml_iDomXSD;
  static IXMLNode xml_get_node_from_XSD_map_as_acopy (const std::string &inNodeName);
  static void     prepare_static_XSD(); // v3.0.241.1

  using mxUtils::formatNumber;
  using mxUtils::getPrecision;
  using mxUtils::isElementExists;
  using mxUtils::stringToNumber;


  //static std::string logFilePath;

  // From Sandy Barboar plugin example: TestQuaternions.c
  typedef struct _QUATERNION
  {
    double w = 0.0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
  } QUATERNION;


  // From Sandy Barboar plugin example: TestQuaternions.c
  typedef struct _HPR
  {
    float Heading = 0.0f;
    float Pitch   = 0.0f;
    float Roll    = 0.0f;
  } HPR;



  ////////////////////// Convert Units of Measure ////////////////

  static missionx::mx_units_of_measure translateStringToUnits(std::string& inVal);
  static float                         convertNmTo(const float& nm, mx_units_of_measure from);

  static float convertToNm (const float & v, mx_units_of_measure from);

  // static FMOD_RESULT check_fmod_result(FMOD_RESULT result);


  // TEMPLATES

  // -------------------------------------------

  template<class Container>
  static bool getElementAsDuplicateFromMap(Container& inMap, const typename Container::key_type& key, typename Container::mapped_type& outMappedType)
  {
    bool                            found = false;
    typename Container::mapped_type dummy;
    typename Container::const_iterator iter = inMap.find(key);
    if (iter != inMap.end())
    {
      found         = true;
      outMappedType = inMap[key];
    }

    return found;
  }


  // -------------------------------------------


  template<class Container>
  static void addElementToMap(Container& inMap, const typename Container::key_type& key, typename Container::mapped_type inElement)
  {
    inMap.insert(std::make_pair(key, inElement));
  }

  // -------------------------------------------

  template<class Container>
  static bool addElementToMap(Container& inMap, const typename Container::key_type& key, typename Container::mapped_type inElement, std::string& errText)
  {
    bool success = true;
    errText.clear();

    // check if exists
    typename Container::const_iterator iter = inMap.find(key);
    if (iter == inMap.end())
      inMap.insert(make_pair(key, inElement));
    else
    {
      success = false;
      errText = "Key: " + key + ", is already present. Change key name.";
    }

    return success;
  } // end template


  // -------------------------------------------

  template<class Container>
  static void cloneMap(Container& inSource, Container& inTarget)
  {

    // loop over source and insert into target map
    inTarget.clear();

    for (auto a : inSource)
    {
      Utils::addElementToMap(inTarget, a.first, a.second);
    }


  } // end template


 // -------------------------------------------


  // Convert String into Number with precision
  template<typename N>
  static N stringToNumber(const std::string& s, const int precision)
  {
    std::istringstream is(s);
    N                  n;
    std::string        num;
    num.clear();
    for (int i = 0; i < precision; i++)
    {
      char c = '\0';
      is.get(c);
      num += c;
    }
    std::istringstream isNum(num);
    isNum >> n;

    // is >> std::setprecision(precision) >> std::setiosflags(ios::fixed) >> n;
    return n;
  }


 // -------------------------------------------


  /**
    replaceChar1WithChar2
    Replaces a character with another character or string of characters
    Uses: remove the LF and UNIX_EOL from strings
  */
  //static std::string replaceChar1WithChar2(const std::string inString, const char charToReplace, const std::string newChar);

  static std::string replaceChar1WithChar2_v2(const std::string& inString, char charToReplace, const std::string &newChar);

  // -------------------------------------------
  /**
  replaceCharsWithString
  Replaces set of characters, one by one, with another string of characters
  Uses: remove the LF and UNIX_EOL from strings
  */
  static std::string replaceCharsWithString(const std::string& outString, const std::string& charsToReplace, const std::string &newChar);

  // -------------------------------------------
  static std::string replaceStringWithOtherString(std::string inStringToModify, const std::string& inStringToReplace, const std::string& inNewString, bool flag_forAllOccurences = false, size_t skip_occurence = 0);

  // -------------------------------------------

  static std::string getXPlaneInstallFolder()
  {
    char sys_path[2048]; // v3.303.8 resize to 2048

    XPLMGetSystemPath(sys_path);

    return std::string(sys_path);

  }

  // -------------------------------------------
  static std::string getRelativePluginsPath()
  {
    return std::string("Resources") + XPLMGetDirectorySeparator() + "plugins";
  }

  // -------------------------------------------
  static std::string getXplaneFullPluginsPath()
  {
    std::string result;
    result.clear();
    char path[2048];
    XPLMGetSystemPath(path);
    result = std::string(path);
    result += std::string("Resources") + XPLMGetDirectorySeparator() + "plugins";
    return result;
  }

  // -------------------------------------------

  static std::string getPluginDirectoryWithSep(const std::string& pluginDirName = missionx::PLUGIN_DIR_NAME)
  {
    std::string result;
    result.clear();

    result = getXplaneFullPluginsPath();
    result += XPLMGetDirectorySeparator() + pluginDirName + XPLMGetDirectorySeparator();

    return result;
  }

  // -------------------------------------------

  static std::string getMissionxCustomSceneryFolderPath_WithSep (const bool isRelative = false)
  {
    std::string customMissionxPath;
    customMissionxPath.clear();

    char sysPath[1024] = {};

    XPLMGetSystemPath (sysPath);

    if (isRelative)
      customMissionxPath.append("Custom Scenery").append(XPLMGetDirectorySeparator()).append("missionx").append(XPLMGetDirectorySeparator());
    else
      customMissionxPath.append(sysPath).append("Custom Scenery").append(XPLMGetDirectorySeparator()).append("missionx").append(XPLMGetDirectorySeparator());

    return customMissionxPath;
  }



  // -------------------------------------------
  static std::string getCustomSceneryRelativePath()
  {
    static std::string str;
    str = "Custom Scenery";
    str.append(XPLMGetDirectorySeparator());

    return str;
  }

  // -------------------------------------------
  //
  // thread safe function (same as calcDistanceBetween2Points_nm)
  double static calcDistanceBetween2Points_nm_ts(const double              gFromLat,
                                              const double              gFromLong,
                                              const double              gTargetLat,
                                              const double              gTargetLong,
                                              const mx_units_of_measure inReturnInUnits = mx_units_of_measure::nm); // calculate distance between 2 points but thread safe

  // -------------------------------------------
  double static calcDistanceBetween2Points_nm(const double              gFromLat,
                                              const double              gFromLong,
                                              const double              gTargetLat,
                                              const double              gTargetLong,
                                              const mx_units_of_measure inReturnInUnits = mx_units_of_measure::nm); // calculate distance between 2 points in Nauticle Miles

  // -------------------------------------------
  double static calcBearingBetween2Points(const double gFromLat, const double gFromLong, const double gTargetLat, const double gTargetLong);

  // -------------------------------------------
  // provide two points and elevation between them. Function will retrieve the slope angle
  double static calcSlopeBetween2PointsWithGivenElevation(const double gFromLat, const double gFromLong, const double gTargetLat, const double gTargetLong, const double relativeElevInFeet);

  // -------------------------------------------

  float static calcElevBetween2Points_withGivenAngle_InFeet(const float distanceBetweenPointsInFeet, const float slopeAngle);

  // -------------------------------------------
  /**
   * findCoordinateBasedOnDistanceAndAngle function return a Point based on location.
   * inLat, inLon: plane latitude/longitude
   * inHdg: the function angles are (clockwise): 0, 270, 180, 90
   *        x-plane angles (clockwise): 0, 90, 180, 270
   *        In order to compensate for the un-identical measures, we use 360 - Angle in order to be on par with X-Plane angles.
   *        The Headings must be in PSI and not in magnetic PSI.
   *        PSI: sim/flightmodel/position/psi
   */

  void static calcPointBasedOnDistanceAndBearing_2DPlane(double& outLat, double& outLon, const double inLat, const double inLon, const float inHdg_deg, const double inDistance_nm);
  static const missionx::mxVec2d getPointBasedOnDistanceAndBearing_2DPlane(const double inLat, const double inLon, const float inHdg, const double inDistance);

  // -------------------------------------------

  std::vector<std::string> static buildSentenceFromWords(const std::vector<std::string>& inTokens, const size_t allowedSentenceLength, const size_t maxLinesAllowed /*, size_t *outRealLinesCount*/);

  // -------------------------------------------

  // extract unit from a string by checking the lats two characters symbols
  static bool extractUnitsFromString(const std::string& inNumWithUnits, std::string& outNumber, std::string& outUnit);

  // -------------------------------------------

  std::map<int, std::string> static splitStringToMap(const std::string& inString, const std::string& delimateChars); // returns set of key,values. Key is seq number starting from 0... while value is string


  // -------------------------------------------

  std::list<std::string> static splitStringToList(const std::string& inString, const std::string& delimateChars);

  // -------------------------------------------

  // Extract base string from a string. Good to extract file name without the extension "[file].exe", for example.
  static std::string extractBaseFromString(const std::string& inFullFileName, const std::string &delimiter = ".", std::string* outRestOfString = nullptr);

  // -------------------------------------------

  // Extract base string from a string. Good to extract file name without the PATH, for example.
  static std::string extractLastFromString(std::string inFullFileName, const std::string &delimiter = ".", std::string* outRestOfString = nullptr);

  // -------------------------------------------

  // Simple template to split string into numeric vector
  template<class T>
  std::vector<T> static splitStringToNumbers(const std::string& inString, const std::string& delimateChar)
  {
    // flag_isNumber = true;
    std::vector<T>           vecResult;
    std::vector<std::string> vecSplitResult;

    vecSplitResult.clear();
    vecSplitResult = mxUtils::split_v2(inString, delimateChar);
    for (auto s : vecSplitResult)
    {
      if (Utils::is_number(s))
      {
        T val = static_cast<T> (Utils::stringToNumber<T> (s, s.length ())); // v3.303.9.1 changed length from "6" to "s.length()"
        vecResult.emplace_back(val);
      }
    }

    return vecResult;
  }


  // -------------------------------------------
  // Function receives min|max as string, a pointer to datatype and optionally a default low value if no "low|high" values were sent. Default is Zero.
  // The function won't check validity of type, it is up for the programmer to do this.
  // example usage: getRand<double>( "min|max", &out ); or getRand<double>( "min|max", &out, 1000 ); // 1000 = default low value if not provided
  template<class N>
  static bool getRand (const std::string inMinMaxToSplit, N* outRandVal, N inDefaultLow = static_cast<N> (0))
  {
    N low;
    N high;
    N t;

    bool result = false;

    if (inMinMaxToSplit.empty())
      return result; // fail


    std::vector<N> vecValues = splitStringToNumbers<N>(inMinMaxToSplit, "|");

    int iSize = vecValues.size();
    if (iSize > 0)
    {
      low = vecValues.at(0);
      if (iSize > 1)
        high = vecValues.at(1);
      else
        high = inDefaultLow;

      // fix if high !> low
      if (high < low)
      { // switch values
        t    = high;
        low  = high;
        high = t;
      }

      // produce random numbers
      std::random_device               rd;        // Will be used to obtain a seed for the random number engine
      std::mt19937                     gen(rd()); // Standard mersenne_twister_engine seeded with rd()
      std::uniform_real_distribution<> dis(low, high);
      (*outRandVal) = dis(gen); // generate rand number

      result = true;
    }


    return result;
  }

  // -------------------------------------------
  // This function split words into sentences, while trying to keep the line width in boundaries.

  // v3.0.223.1 main function to construct sentences with length in mind.
  static void sentenceTokenizerWithBoundaries(std::deque<std::string>& outDeque, const std::string& inString, const std::string &delimiterChar, size_t width = 0, const std::string &in_special_LF_char = ";");

  // convert the std::deque sentence to vector, where vector is used. TODO: Consider using the deque only function instead of the vector one.
  static std::vector<std::string> sentenceTokenizerWithBoundaries (const std::string &inString, const std::string& delimiterChar, size_t width = 0, const std::string &in_special_LF_char = ";");

  // v3.0.223.1 add the new string to the sentence string. Take into consideration limiting length and too long strings.
  static std::string add_word_to_line(std::deque<std::string>& outDeque, const std::string &inCurrentLine_s, const std::string &inNewWord_s, const int inMaxLineLength_i, const bool flag_force_new_line = false);

  // -------------------------------------------

  std::vector<std::string> static tokenizer(const std::string &inString, char delimate_char, size_t width);

  // -------------------------------------------

  double static lRound(double x);

  // -------------------------------------------
  static void convert_qArr_to_Quaternion(const float in_q_arr[4], QUATERNION q);
  // -------------------------------------------

  /// Sandy Barbour
  static void QuaternionToHPR (const QUATERNION &quaternion, HPR* phpr)
  {
    double local_w = quaternion.w;
    double local_x = quaternion.x;
    double local_y = quaternion.y;
    double local_z = quaternion.z;

    double sq_w = local_w * local_w;
    double sq_x = local_x * local_x;
    double sq_y = local_y * local_y;
    double sq_z = local_z * local_z;

    phpr->Heading = static_cast<float> (atan2 (2.0 * (local_x * local_y + local_z * local_w), (sq_x - sq_y - sq_z + sq_w))) * RadToDeg;
    phpr->Pitch   = static_cast<float> (asin (-2.0 * (local_x * local_z - local_y * local_w))) * RadToDeg;
    phpr->Roll    = static_cast<float> (atan2 (2.0 * (local_y * local_z + local_x * local_w), (-sq_x - sq_y + sq_z + sq_w))) * RadToDeg;
  }

  /// Sandy Barbour
  /// Convert location info to quanterian to locate plane in Graphical Sphare
  //---------------------------------------------------------------------------
  /***

  psi' = pi / 360 * psi
  theta' = pi / 360 * theta
  phi' = pi / 360 * phi
  q[0] =  cos(psi') * cos(theta') * cos(phi') + sin(psi') * sin(theta') * sin(phi')
  q[1] =  cos(psi') * cos(theta') * sin(phi') - sin(psi') * sin(theta') * cos(phi')
  q[2] =  cos(psi') * sin(theta') * cos(phi') + sin(psi') * cos(theta') * sin(phi')
  q[3] = -cos(psi') * sin(theta') * sin(phi') + sin(psi') * cos(theta') * cos(phi')

  */
  void static HPRToQuaternion(HPR hpr, QUATERNION* pquaternion)
  {
    double local_Heading = hpr.Heading * DegToRad; // DegToRad is a constant
    double local_Pitch   = hpr.Pitch * DegToRad;
    double local_Roll    = hpr.Roll * DegToRad;

    double Cosine1 = cos(local_Roll / 2);
    double Cosine2 = cos(local_Pitch / 2);
    double Cosine3 = cos(local_Heading / 2);
    double Sine1   = sin(local_Roll / 2);
    double Sine2   = sin(local_Pitch / 2);
    double Sine3   = sin(local_Heading / 2);

    pquaternion->w = Cosine1 * Cosine2 * Cosine3 + Sine1 * Sine2 * Sine3;
    pquaternion->x = Sine1 * Cosine2 * Cosine3 - Cosine1 * Sine2 * Sine3;
    pquaternion->y = Cosine1 * Sine2 * Cosine3 + Sine1 * Cosine2 * Sine3;
    pquaternion->z = Cosine1 * Cosine2 * Sine3 - Sine1 * Sine2 * Cosine3;
  }


  // -------------------------------------------

  static float calc_heading_base_on_plane_move(double& inCurrentLat, const double & inCurrentLong, double& inTargetLat, const double & inTargetLong, const int & inTargetHeading);

  // -------------------------------------------

  template<class T>
  static T readNodeNumericAttrib(const ITCXMLNode& node, const std::string& inAttribName, const T defaultValue)
  {
    if (node.isEmpty())
      return defaultValue;

    T returnValue = defaultValue; // init with default value

    if (char exists = node.isAttributeSet (inAttribName.c_str ()))
    {
      const std::string       tv          = node.getAttribute(inAttribName.c_str());

      if (const std::string attribValue = Utils::trim (tv)
        ; !attribValue.empty())
      {
        // check if value is a numeric
        if (Utils::is_number(attribValue))
        {
          returnValue = Utils::stringToNumber<T>(attribValue);
        }
      }
    }

    return returnValue; // calling function will have to cast to the correct numeric type (int/float...)
  }

  // -------------------------------------------

  static double      readNumericAttrib(const ITCXMLNode& node, const std::string &attribName, const double defaultValue);                                                   // v3.0.213.7
  static double      readNumericAttrib(const ITCXMLNode& node, const std::string &attribName, const std::string &attribName2, const double defaultValue);                    // v3.0.223.1
  static bool        readBoolAttrib(const ITCXMLNode& node, const std::string &attribName, bool defaultValue);                                                                          // v3.0.221.15 rc4
  static std::string readAttrib (const ITCXMLNode & node, const std::string &attribName, std::string defaultValue, bool needStringTrim = true);                                           // v3.0.213.7
  static std::string readAttrib (const ITCXMLNode & node, const std::string &attribOptionName1, const std::string &attribOptionName2, std::string defaultValue, bool needStringTrim);            // v3.0.221.15rc5
  static std::string readAttrib(const IXMLNode& node, const std::string& attribOptionName1, const std::string& attribOptionName2, std::string defaultValue, bool needStringTrim = true); // v3.0.223.4
  static std::string readAttrib(const IXMLNode& node, const std::string &attribOptionName1, const std::string &defaultValue, bool needStringTrim = true); // v3.303.11


  // -------------------------------------------
  template<class T>
  static T read_tagTextAsNumericValue(IXMLNode node, const T defaultValue)
  {
    if (node.isEmpty())
      return defaultValue;

    std::string text_s = std::string(node.getText());

    if (text_s.empty())
      return defaultValue;
    else
    {
      // check if value is a numeric
      if (Utils::is_number(text_s))
        return Utils::stringToNumber<T>(text_s);
      else
        return defaultValue;
    }

    return defaultValue; // calling function will have to cast to the correct numeric type (int/float...)
  }

  // -------------------------------------------

  // This function set the string based attribute values.
  static bool xml_set_attribute_in_node_asString(IXMLNode& inNode, const std::string& attribName, const std::string& attribValue, const std::string &inElementName = "")
  {
    return missionx::Utils::xml_search_and_set_attribute_in_IXMLNode(inNode, attribName, attribValue, inElementName);
  }

  // The template will search for a node with same attribute name and tag name (if provided) and will set it.
  // The search is recursive. If it found the tag but no attribute is present, then it will add it and stop searching.
  template<class T>
  static bool xml_set_attribute_in_node(IXMLNode& inNode, const std::string& attribName, T attribValue, const std::string& inElementName = "")
  {
    std::string val_s;

    if (std::is_integral_v<decltype(attribValue)>) // ::value will return true or false. Is this a number or bool ?
    {
      if (std::is_same_v<decltype(attribValue), bool>)
      {
        val_s = (attribValue) ? "true" : "false";
      }
      else
      {
        val_s = Utils::formatNumber<T>(attribValue);
      }
    }
    else if (std::is_floating_point_v<decltype(attribValue)>)
    {
      // find decimal part
      double      intpart_d;
      double      fractpart_d              = modf (attribValue, &intpart_d);
      std::string fract_s                  = Utils::formatNumber<double>(fractpart_d);
      const auto  lmbda_get_decimal_length = [&]() { return ((fractpart_d == 0.0) ? 0 : static_cast<int> (fract_s.length () - 1)); };

      val_s = mxUtils::formatNumber<T>(attribValue, static_cast<int> (lmbda_get_decimal_length ()));
    }


    return missionx::Utils::xml_search_and_set_attribute_in_IXMLNode(inNode, attribName, val_s, inElementName);
  }
  // -------------------------------------------

  static bool xml_search_and_set_attribute_in_IXMLNode(IXMLNode& inNode, const std::string& attribName, const std::string& attribValue, const std::string &inElementName = missionx::EMPTY_STRING); // v3.0.217.4


  // -------------------------------------------
  static bool xml_copy_nodes_from_one_parent_to_another_IXMLNode(IXMLNode& outTargetNode, const IXMLNode & inSourceNode, const std::string& nameOfChildTag = "", bool bIsDeepCopy = false);  // v3.0.303 move sub nodes from one element to the target node

  // -------------------------------------------

  static bool xml_add_node_to_element_IXMLNode(IXMLNode& rootNode, IXMLNode& inNodeToAdd, const std::string& nameOfParentElementToAddTo = missionx::EMPTY_STRING); // v3.0.217.4

         // -------------------------------------------
         // Function returns random node. IMPORTANT: this will remove the node from source node tree, if we add the result node to an other element.
  static IXMLNode xml_get_node_randomly_by_name_IXMLNode (const IXMLNode & rootNode, const std::string &inTagToSearch, std::string& outErr, bool flag_removePicked = false); // v3.0.217.4 return random XML node from N child nodes with same tag name

  // -------------------------------------------
  // Function returns random node based on the attribute name and value and not the tag name.
  // IMPORTANT: this will return the pointer to the node. It is up to you to decide if to duplicate the result or to use the pointer, hence it will remove it from the source node.
  static IXMLNode xml_get_node_ptr_randomly_by_attrib_and_value (const IXMLNode & rootNode, const std::string &inTagToSearch, const std::string &inAttribName, const std::string &inAttribValue); // v24.06.1

  // -------------------------------------------
  // Function returns random node. IMPORTANT: this will remove the node from source node tree, if we add the result node to an other element.
  static IXMLNode xml_get_node_randomly_by_name_and_distance_IXMLNode (const IXMLNode &    rootNode, const std::string &inTagToSearch,
                                                                      double       inLat,
                                                                      double       inLon,
                                                                      std::string& outErr,
                                                                      double       inMinDistance     = 0.0,
                                                                      double       inMaxDistance     = 0.0,
                                                                      bool         flag_removePicked = false); // v3.0.217.4 return random XML node from N child nodes with same tag name

  // -------------------------------------------

  static void xml_delete_empty_nodes(IXMLNode& rootNode);

  // -------------------------------------------
  // search for attribute value using attribute name and optional tag name.
  // return empty string if does not find.
  static std::string xml_get_attribute_value_drill (const IXMLNode & node, const std::string &inAttribName, bool& flag_found, const std::string &inTagName = missionx::EMPTY_STRING);

  // -------------------------------------------
  // Copy source Node attributes to target attributes
  static bool xml_delete_all_node_attribute(IXMLNode& inNode); // v3.305.3
  //static bool xml_clear_all_node_attribute_values(IXMLNode& inNode); // v25.02.1 deprecated, use: "xml_clear_node_attributes_excluding_list()" instead. // v3.305.3
  static bool xml_copy_node_attributes (const IXMLNode & sNode, IXMLNode& tNode, bool flag_includeClearData = false);
  static bool xml_copy_node_attributes_excluding_black_list (const IXMLNode & sNode, IXMLNode& tNode, std::set<std::string>* inExclude = nullptr, bool flag_includeClearData = false);   // the blacklist is for excluding attribute names
  static bool xml_copy_specific_attributes_using_white_list (const IXMLNode & sNode, IXMLNode& tNode, std::set<std::string>* inWhiteList = nullptr, bool flag_includeClearData = false); // the white list is for specific attribute list to copy
  static void xml_clear_node_attributes_excluding_list ( IXMLNode &sNode, const std::string &inExcludeList_s = "", bool flag_includeClearData = false ); // v25.02.1 the blacklist is for excluding attribute names

  // -------------------------------------------

  // search and return a copy of a specific childElement from element tree. If you want to work directly on element node, then use: "flag_returnCopy = false".
  // This function do not traverse the whole tree, only the first level child nodes
  static IXMLNode xml_get_node_from_node_tree_IXMLNode (const IXMLNode & pNode, const std::string &inSearchedElementName, bool flag_returnCopy = true);

  static IXMLNode xml_get_node_pointer_from_node_tree_by_attrib_name_and_value_IXMLNode(IXMLNode& pNode, const std::string &inSearchedElementName, const std::string &inAttribName, const std::string &attribValue, bool flag_searchAllAttributesWithName = false); // v3.0.301 B3 Helper function so we only get the XML pointer and not a copy by mistake.
  static IXMLNode xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(IXMLNode& pNode, const std::string &inSearchedElementName, const std::string &inAttribName, const std::string &attribValue, bool flag_returnCopy = true, bool flag_searchAllAttributesWithName = false); // v24.05.1 added searchAllAttributes flag

  // -------------------------------------------
  // return attribute value as string. If not found return empty string and flag_found is set to false
  static std::string xml_get_attribute_value (const IXMLNode &pNode, const std::string &attribName, bool& flag_found);

  // -------------------------------------------
  static int xml_find_node_location (const IXMLNode & pNode, const std::string &tagNameToSearch);

  // -------------------------------------------
  static bool xml_add_cdata(IXMLNode& node, const std::string &cdataString);

  // -------------------------------------------

  static void add_xml_comment(IXMLNode& node, const std::string &inCommentString = " ++++++++++++++++++++++++ " /*, int pos = -1*/); // v3.0.301 moved from RandomEngine

  // -------------------------------------------
  static bool        xml_search_and_set_node_text(IXMLNode&          parentNode,
                                                  const std::string& inTagName, const std::string &inTextValue                       = "", const std::string &inValueType                       = "6",
                                                  bool               flag_force_adding_missing_element = false); // search sub element with "inTagName" tag and set its TEXT "clear element" with the "inTextValue".
  static void        xml_delete_all_text_subnodes(IXMLNode& node);                                 // set TEXT clear element and returns true if it was success
  static bool        xml_set_text(IXMLNode& node, const std::string &inDefaultValue = "");                // set TEXT clear element and returns true if it was success
  static std::string xml_get_text (const IXMLNode & node, const std::string &inDefaultValue = "");                // This function wraps the "IXMLNode.getText()" to solve unwanted crash when there is no <node>text</nod> available.
  static std::string xml_get_text (const ITCXMLNode & node, const std::string &inDefaultValue = "");              // This function wraps the "IXMLNode.getText()" to solve unwanted crash when there is no <node>text</nod> available.

  // -------------------------------------------

  static void xml_delete_attribute(IXMLNode& node, std::set<std::string>& inSetAttributes, const std::string &inTagName = "");
  // -------------------------------------------

  static void xml_add_comment(IXMLNode& node, const std::string &inCommentString);

  // -------------------------------------------
  // delete all sub-nodes with certain tag name. If empty, then delete all sub-nodes. It will check only the root level.
  static void xml_delete_all_subnodes(IXMLNode& pNode, const std::string &inSubNodeName = "", bool inDelClear_b = false);
  static void xml_delete_all_subnodes_except(IXMLNode& pNode, const std::string &inSubNodeName = "", bool inDelClear_b = false, const std::string& exceptElementWithTagAndAttribAndValue = "" ); // delete all sub-nodes with certain tag name. If empty then delete all subnodes

  // -------------------------------------------


  // generate random numbers
  //// random related
  static int getRandomIntNumber(int inMin, int inMax); // TODO: convert to a template ?

  // -------------------------------------------

  static double getRandomRealNumber(double inMin, double inMax); // TODO: convert to a template ?

  // -------------------------------------------

  static IXMLNode xml_create_node_from_string (const std::string& inStringNode);

  // -------------------------------------------
  static IXMLNode xml_create_message (const std::string& inMsgName, const std::string &inText); // v3.0.301
  static bool xml_update_message_text(IXMLNode &pNode, const std::string& inMsgName, const std::string &inText); // v3.0.301
  // -------------------------------------------

  static std::string convert_string_to_24_min_numbers (const std::string &inTimeIn24Hfortmat, int& outHour, int& outMinutes, int& outCycles); // returns error string. Format: HH24:MIN:Cycles

  // -------------------------------------------

  // Search for a child node. If it does not exist add it to the parent and return the node.
  static IXMLNode xml_get_or_create_node_ptr(IXMLNode& pNode, const std::string &tagChildNodeName_s, const std::string &with_attrib_name = "", const std::string &with_attrib_value = "");

  // Search for a child node. If it does not exist create a node and return a copy or a pointer to the node. Default is to return a copy.
  static IXMLNode xml_get_or_create_node(IXMLNode pNode, const std::string &tagChildNodeName_s, const bool& in_flag_return_copy = true); // v3.0.241.10 b3 added return a copy of the Node or a pointer.

  // -------------------------------------------
  static std::string xml_read_cdata_node (const ITCXMLNode &inNode, const std::string &default_value);
  static std::string xml_read_cdata_node (const IXMLNode &inNode, std::string default_value);

  // -------------------------------------------
  //=================================================================
  /*
   * dms.h:  Version 0.02
   * Created by James A. Chappell <rlrrlrll@gmail.com>
   * http://www.storage-b.com/c/16
   * Created 23 August 2005
   *
   * History:
   * 23-aug-2005  created
   * 25-apr-2008  added latitude/longitude conversions
   * 04-sep-2016  update degrees symbol
   *
   */
  //=================================================================
  static const char* DEG_SIM; // = "\u00B0";
  static std::string DegreesMinutes(double ang, unsigned int num_dec_places = 2, bool flag_format = true);        //  Convert decimal degrees to degrees, minutes and seconds
  static std::string DegreesMinutesSeconds(double ang, unsigned int num_dec_places = 2, bool flag_format = true); //  Convert decimal degrees to degrees, minutes and seconds
  static std::string DegreesMinutesSecondsLat(double ang, unsigned int num_dec_places = 2, bool flag_format = true);
  static std::string DegreesMinutesSecondsLon(double ang, unsigned int num_dec_places = 2, bool flag_format = true);
  static std::string DegreesMinutesSecondsLat_XP(double ang, unsigned int num_dec_places = 0, bool flag_format = false); // convert but without special characters and spaces
  static std::string DegreesMinutesSecondsLon_XP(double ang, unsigned int num_dec_places = 0, bool flag_format = false); // convert but without special characters and spaces

  // -------------------------------------------

  static std::string db_extract_list_into_sql_string (const std::list<std::string> &inList, char inPrePost_char = '\0');

  // -------------------------------------------

  static void CalcWinCoords(int inWinWidth, int inWinHeight, int inWinPad, int inColPad, int& left, int& top, int& right, int& bottom); // Calculate window's standard coordinates
  static void getWinCoords(int& left, int& top, int& right, int& bottom);                                                               // get window's standard coordinates


  // -------------------------------------------
  static bool is_it_an_airport(const std::string &inICAO);

  static std::string get_earth_nav_dat_file();
  static std::string get_nav_dat_cycle();
  // -------------------------------------------

  template<typename T>
  static void deque_erase_item_at_index(std::deque<T>& outDeque, const int inItemIndex_toDelete)
  {
    auto iter    = outDeque.begin();
    int  counter = 0;
    while (iter != outDeque.end())
    {
      if (counter == inItemIndex_toDelete)
      {
        outDeque.erase(iter);
        break;
      }

      ++iter;
      counter++;
    }

  } // end deque_erase_item_at_index


  // -------------------------------------------
  static std::string format_number_as_hours(const double& inSeconds);

  // -------------------------------------------
  static std::string getNavType_Translation(XPLMNavType inType);
  // -------------------------------------------

  static void load_cb(const char* real_path, void* ref);
  static void load_cb_dummy(const char* real_path, void* ref); // used in conjunction with XPLMLookupObjects in data_manager class.

  // -------------------------------------------

  static bool isStringIsValidArithmetic (const std::string& inArithmetic); // this function is only relevant when you want to test if a specific string has a valid lat/lon string or if string is only a valid arithmetic but it does so in a simplified manner.

  // -------------------------------------------
  template<typename T>
  static T getNodeText_type_1_5(IXMLNode& inParentNode_ptr, const std::string& tagToSearch, T defaultValueIfNotExists)
  {
    T    result_T = defaultValueIfNotExists;
    if (auto node = Utils::xml_get_node_from_node_tree_IXMLNode (inParentNode_ptr, tagToSearch, false)
      ; node.isEmpty())
      return defaultValueIfNotExists;
    else
    {
      const std::string text       = Utils::xml_get_text(node, mxconst::get_NO_SETUP_TEXT());
      switch (int val_type_i = Utils::readNodeNumericAttrib<int> (node, mxconst::get_ATTRIB_TYPE(), -1))
      {
        case 1: // bool
        {
          bool result_b;
          if (Utils::isStringBool(text, result_b))
            return result_b;
          else
            return defaultValueIfNotExists;
        }
        break;
        case 2: // char
          if (mxconst::get_NO_SETUP_TEXT() == text)
            return defaultValueIfNotExists;
          else
            return static_cast<T> (text.front ());
          break;
        case 3: // int
          if (mxUtils::is_number(text))
            return static_cast<T> (mxUtils::stringToNumber<int> (text));
          else
            return defaultValueIfNotExists;
          break;
        case 4: // float
          if (mxUtils::is_number(text))
            return static_cast<T> (mxUtils::stringToNumber<float> (text, text.length ()));
          else
            return defaultValueIfNotExists;
          break;
        case 5: // double
          if (mxUtils::is_number(text))
            return static_cast<T> (mxUtils::stringToNumber<double> (text, text.length ()));
          else
            return defaultValueIfNotExists;
          break;
        default:
          return defaultValueIfNotExists;
          break;
      }
    }

    return result_T;
  }


  // -------------------------------------------
  // In this function we read the setup node text which is type="6"
  static std::string getNodeText_type_6 (const IXMLNode & inParentNode_ptr, const std::string& tagToSearch, std::string defaultValueIfNotExists)
  {
    std::string result_s = defaultValueIfNotExists;
    if (auto node = Utils::xml_get_node_from_node_tree_IXMLNode (inParentNode_ptr, tagToSearch, false)
      ; node.isEmpty())
      return defaultValueIfNotExists;
    else
    {
      return Utils::xml_get_text(node, defaultValueIfNotExists);
    }

    return result_s;
  }


  // -------------------------------------------

  static bool position_plane_in_ICAO(std::string inICAO, float lat, float lon, float currentPlaneLat, float currentPlanelon, bool flag_FindNearestAirportIfIcaoIsNotValid = false);

  // -------------------------------------------

  static std::string get_time_as_string();
  // -------------------------------------------
  static missionx::mx_clock_time_strct get_os_time(); // https://cplusplus.com/reference/ctime/tm/ and https://stackoverflow.com/questions/15957805/extract-year-month-day-etc-from-stdchronotime-point-in-c
  // -------------------------------------------
  static std::string get_hash_string (const std::string &inValue);
  // -------------------------------------------
  static void xml_print_node(const IXMLNode& inNode, bool bThread = false); // v3.303.8 added thread support
  // -------------------------------------------
  static std::string xml_get_node_content_as_text (const IXMLNode & inNode, const std::string &inDefaultText = ""); // v3.303.9.1 Return the node as TEXT, mainly used for debug.
  // -------------------------------------------
  static std::vector<IXMLNode> xml_get_all_nodes_pointer_with_tagName (const IXMLNode & inRootNode, std::string_view inTagName, std::string_view inTagNameToIgnore = ""); // this is the main function that will call xml_get_all_child_nodes_recurs_with_tagName
  // -------------------------------------------
  static void xml_add_node_to_parent_with_duplicate_filter(IXMLNode& inOutParentNode, const IXMLNode & inNode, std::string_view inChildTagNameToFilter, std::string_view inAttribNameHoldsFilterRule = mxconst::get_ATTRIB_NAME()); // this is the main function that will call xml_get_all_child_nodes_recurs_with_tagName
  // -------------------------------------------
  static std::vector<mx_score_strct> xml_extract_scoring_subElements (const IXMLNode & pNode, const std::string& inSubElement); // v3.303.8.3
  static float                       getScoreAfterAnalyzeMinMax(const std::vector<mx_score_strct>& inVecParsedScores, const double& inMin, const double& inMax); // v3.303.8.3
  // -------------------------------------------

  static std::string getAndFixStartingDayValue(const std::string& inStarting_day);

  // -------------------------------------------

  static IXMLNode xml_add_child(IXMLNode& inParent, const std::string& inTagName, const std::string& inInitAttribName, const std::string& inInitAttribValue, const std::string& inTextValue = ""); // v3.305.3 used in load formatting
  static IXMLNode xml_add_child(IXMLNode &inParent, const std::string& inTagName, const std::string& inTextValue); // v3.305.3 Short version to concentrate on the Text part
  static IXMLNode xml_add_error_child(IXMLNode& inParent, const std::string& inTextValue, const std::string& inTagName = mxconst::get_ELEMENT_ERROR());                                                                           // v3.305.3 Short version to concentrate on the Text part
  static IXMLNode xml_add_warning_child(IXMLNode& inParent, const std::string& inTextValue, const std::string& inTagName = mxconst::get_ELEMENT_WARNING());                                                                           // v3.305.3 Short version to concentrate on the Text part
  static IXMLNode xml_add_info_child(IXMLNode& inParent, const std::string& inTextValue, const std::string& inTagName = mxconst::get_ELEMENT_INFO());                                                                           // v3.305.3 Short version to concentrate on the Text part

  // -------------------------------------------

  static void read_external_sql_query_file(std::map<std::string, std::string>& mapQueries, const std::string& inRootNodeName, const std::string& inFilePath = "Resources/plugins/missionx/libs/sql.xml"); // the function will read the external <query> content and override the container
  static std::vector<std::string> read_external_categories(const std::string& inRootNodeName, const std::string& inFilePath = fmt::format ("Resources/plugins/missionx/libs/{}", missionx::CARGO_DATA_FILE) ); // v24.05.1
  static IXMLNode                 read_external_blueprint_items(const std::string& inRootNodeName, const std::string& inSearchTagName, const std::string &inSubCategoryType, bool inThread = false, bool inCaseSensitive = true, const std::string& inFilePath = fmt::format( "Resources/plugins/missionx/libs/{}", missionx::CARGO_DATA_FILE) ); // v24.05.1

  ////////////////////////////////////////
  //////////////// JSON Related functions
  ///////////////////////////////////////
  
  static std::string getJsonValue(nlohmann::json js, const std::string& key, std::string outDefaultValue);

  // -------------------------------------------

  template<typename T>
  static T getJsonValue(nlohmann::json js, const std::string& key, const nlohmann::detail::value_t inType, T outDefaultValue)
  {
    // bool bFoundKey = false;
    if (!js.is_discarded() && js.contains(key))
    {
      // bFoundKey = true;
      if (js[key].is_null())
        return outDefaultValue;

      //  null,              ///< null value
      //  object,          ///< object (unordered set of name/value pairs)
      //  array,           ///< array (ordered collection of values)
      //  string,          ///< string value
      //  boolean,         ///< boolean value
      //  number_integer,  ///< number value (signed integer)
      //  number_unsigned, ///< number value (unsigned integer)
      //  number_float,    ///< number value (floating-point)
      //  binary,          ///< binary array (ordered collection of bytes)
      //  discarded        ///< discarded by the parser callback function*/

      switch (inType)
      {
        case nlohmann::detail::value_t::null:
        {
          return outDefaultValue;
        }
        break;
        case nlohmann::detail::value_t::object:
        {
          if (js[key].is_object())
            return js[key];
        }
        break;
        case nlohmann::detail::value_t::array:
        {
          if (js[key].is_array())
            return js[key];
        }
        break;
        case nlohmann::detail::value_t::boolean:
        {
          if (js[key].is_boolean())
            return static_cast<T> (js[key].get<bool> ());
        }
        break;
        case nlohmann::detail::value_t::number_integer:
        case nlohmann::detail::value_t::number_unsigned:
        case nlohmann::detail::value_t::number_float:
        {
          if (js[key].is_number())
            return static_cast<T> (js[key].get<double> ());
        }
        break;
        default:
          break;
      }
    }

    return outDefaultValue;
  }


  // -------------------------------------------
  // -------------------------------------------


  // -------------------------------------------

 private:
  static std::vector<IXMLNode> xml_get_all_child_nodes_recurs_with_tagName(IXMLNode& inParent, std::string_view inTagName);     // this function can be called recursively
  // -------------------------------------------


};



class TimerFunc
{
public:
  TimerFunc(); 

  TimerFunc (const std::string &inFilename, const std::string &inSourceFuncName, bool inThread);
  

  TimerFunc (const std::string &inFilename, const std::string &inSourceFuncName, const std::string& info, bool inThread);


  ~TimerFunc();

private:
  void stop() const;
  void init(const std::string& inFilename, const std::string &inSourceFuncName, bool inThread);

  bool isThread{ false };
  int  seq{};
  constexpr static const int FILENAME_WIDTH_CHAR{ 16 };
  constexpr static const int FUNCNAME_WIDTH_CHAR{ 51 };

  std::chrono::time_point<std::chrono::high_resolution_clock> m_startTimepoint;
  std::string                                        sourceFuncName;
  std::string                                        sourceFileName;
};



} // namespace
#endif /*UTILS_H_*/

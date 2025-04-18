#ifdef MAC
#if __has_include(<filesystem>) && (!defined(__MAC_OS_X_VERSION_MIN_REQUIRED) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 101500)
#define GHC_USE_STD_FS
#include <filesystem>
namespace fs = std::filesystem;
#endif

#ifndef GHC_USE_STD_FS
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#endif
#else // Linux and IBM
#include <filesystem>
namespace fs = std::filesystem;
#endif

#include "system_actions.h"
#include "../core/data_manager.h"
#include "../logic/dataref_param.h"
#include <algorithm>
#include <string.h>
#include <vector>

#include "Log.hpp"


namespace missionx
{
//missionx::mxProperties missionx::system_actions::pluginSetupOptions;
missionx::mx_base_node missionx::system_actions::pluginSetupOptions;
}

missionx::system_actions::system_actions() {}


missionx::system_actions::~system_actions() {}

/***
Copy file:
inSourceFilePath: obsolete file location + file name.
inTargetFilePath: obsolete destination file location + file name.
outError: Holds error message.
**/
bool
missionx::system_actions::copy_file(std::string inSourceFilePath, std::string inTargetFilePath, std::string& outError)
{
  //// https://gehrcke.de/2011/06/reading-files-in-c-using-ifstream-dealing-correctly-with-badbit-failbit-eofbit-and-perror/

  // use <filesystem> library instead
  // https://www.cppstories.com/2017/08/cpp17-details-filesystem/
  // https://en.cppreference.com/w/cpp/filesystem/copy_file
  // https://en.cppreference.com/w/cpp/filesystem/copy_options

  fs::path sourceFileAndPath = inSourceFilePath;
  fs::path targetFileAndPath = inTargetFilePath;
  std::error_code errCode;

  const auto copyOptions = fs::copy_options::overwrite_existing;

  if (fs::is_regular_file(sourceFileAndPath))
  {
    if (fs::copy_file(sourceFileAndPath, targetFileAndPath, copyOptions, errCode))
      return true;
    else
    {
      outError = mxUtils::formatNumber<int>((errCode.value())) + ": " + errCode.message();
#ifndef RELEASE
      Log::logMsg(outError);
#endif // !RELEASE

      return false;    
    }

  }
  else
  {
    outError = "Failed to copy file: " + inSourceFilePath + ", to: " + inTargetFilePath;
#ifndef RELEASE
    Log::logMsg(outError);
#endif // !RELEASE

    return false;
  }

  return true;
}



bool
missionx::system_actions::save_datarefs_with_savepoint(std::string inFileAndPath, std::string inTargetFileAndPath, std::string& outError)
{
  // 1. open dataref
  // 2. read each line
  // 2.1 check if valid dataref (use Dataref class)
  outError.clear();
  // bool success = true;
  unsigned int drefLineCounter    = 0;
  unsigned int drefLineProccessed = 0;

  std::ifstream fin;
  fin.open(inFileAndPath.c_str());
  if (fin.fail() || fin.bad())
  {
    outError = "[Error] Fail to open source file: \"" + inFileAndPath + "\"";
    return false;
  }

  std::ofstream fout;
  fout.open(inTargetFileAndPath.c_str(), std::ofstream::out);
  // check open success
  if (fout.fail() || fout.bad())
  {
    outError = "[Error] Fail to find/create target file: [" + inTargetFileAndPath + "]. Skip operation !!!";
    return false;
  }

  std::vector<std::string> vecDrefLineSplit;
  // Read each line, check dataref and write to target file
  if (fout)
  {
    bool                           writeActionOK = true;
    const std::vector<std::string> vecValidTypes = { { "int" }, { "float" }, { "double" }, { "int[" }, { "float[" } };

    const std::vector<std::string> vecValidCategoriesPath = { { "sim/aircraft/controls" },
                                                              { "sim/aircraft/gear" },
                                                              { "sim/aircraft/weight" },
                                                              { "sim/aircraft/specialcontrols" },
                                                              { "sim/cockpit/autopilot" },
                                                              { "sim/cockpit/engine" },
                                                              { "sim/cockpit/radios" },
                                                              { "sim/cockpit/switches" },
                                                              { "sim/cockpit/weapons" },
                                                              { "sim/flightmodel/controls" },
                                                              { "sim/flightmodel/cyclic" },
                                                              { "sim/flightmodel/failures" },
                                                              { "sim/flightmodel/forces" },
                                                              { "sim/flightmodel/misc" },
                                                              { "sim/flightmodel/position" },
                                                              { "sim/flightmodel/transmissions" },
                                                              { "sim/flightmodel/weight" },
                                                              { "sim/operation/failures" },
                                                              { "sim/time" },
                                                              { "sim/weather" },
                                                              { "sim/flightmodel2/controls" },
                                                              { "sim/flightmodel2/wing/" } }; // end vector initialization
    // { "sim/flightmodel/engine" } // DO NOT USE ENGINE

    std::string line = "";
    while (writeActionOK)
    {
      // READ DATAREF KEY
      std::getline(fin, line);
      line = Utils::trim(line);

      drefLineCounter++;

      if (fin.eof()) // reached end of file ?
      {
        Log::logMsgNone("[Finish Filter Dataref Lines Read: " + Utils::formatNumber<int>(drefLineCounter) + ", Processed: " + Utils::formatNumber<int>(drefLineProccessed));
        writeActionOK = false;
      }
      else if (fin.good())
      {

        // skip if empty
        if (line.empty())
          continue;

        // v3.303.8 skip DEPRECATED - DO NOT USE or DEPRECATED datarefs
        if (line.find("DEPRECATED") != line.npos)
          continue;

        // check if line starts with "s" "/sim/xxx"
        const char firstChar = line.front();
        if (firstChar != 's')
          continue; // skip line

        // split line
        vecDrefLineSplit.clear();
        vecDrefLineSplit = mxUtils::split_v2(line, "\t");

        if (vecDrefLineSplit.empty())
          continue;

        line = vecDrefLineSplit.at(0);
        // bool foundKey = false;

        // simple Lambda to better represent the code "for (auto s : vecValidCategoriesPath)"
        const auto lmbda_partial_string_found = [&](const std::string& inCategoryString) { return line.find(inCategoryString) != std::string::npos; }; // if inCategoryString is partialy found in "line" then should be valid
        const bool foundKey                   = any_of(begin(vecValidCategoriesPath), end(vecValidCategoriesPath), lmbda_partial_string_found);

        if (!foundKey)
          continue;

        // skip if line is not in Filtered categories
        // check dataref
        bool isValid = true;

        std::string drefValue;
        drefValue.clear();

        if (vecDrefLineSplit.size() < 3)
          continue;

        missionx::dataref_param dref;
        dref.key = vecDrefLineSplit.front(); // not using "setAndInitializeKey()" since it triggers the "initDatarefInfo()" function

        std::string type     = vecDrefLineSplit.at(1);
        std::string writable = vecDrefLineSplit.at(2);

        isValid = dref.initDataRefInfo();
        if (isValid)
        {

//#ifndef RELEASE
////        This was a debug test of specific datarefs
//          if ((dref.key.compare("sim/flightmodel/weight/m_fuel1") == 0) || (dref.key.compare("sim/flightmodel/weight/m_fuel2") == 0) || (dref.key.compare("sim/flightmodel/weight/m_fuel3") == 0))
//          {
//            if (writable.compare("y") == 0) // v3.303.8
//            {
//              Log::logMsgNone("Dataref: [" + dref.key + "] is writable. Value: " + mxUtils::formatNumber<double>(dref.getValue(), 6));
//            }
//        
//          }
//#endif // !RELEASE


          // check if array and if designer defined specific cell in its name
          if (dref.arraySize > 0) // check array dataref
          {
            if (dref.evaluateArray()) // this function can change "isParamReadyToBeUsed" to FALSE hence it won't be available.
            {
              dref.arrayElementPicked = 0; // 0 = read/pick all values
              dref.readDatarefValue_into_missionx_plugin();
              drefValue = "{" + dref.pStrArrayValue + "}";
            }
            else
            {
              Log::logMsgNone("Fail eval array: " + dref.key);
              continue;
            }


          } // end if is array
          else
          {
            dref.readDatarefValue_into_missionx_plugin();
            drefValue = "{" + Utils::formatNumber(dref.getValue <double>()) + "}";
          }
          isValid = dref.flag_paramReadyToBeUsed;
        }
        else
        {
          Log::logMsgNone("Fail init: " + dref.key);
          isValid = false;
        } // if initStatic succeed


        // write to filtered file
        if (isValid)
        {
          line += "\t" + type + "\t" + writable + ((drefValue.empty()) ? drefValue : "\t" + drefValue);
          fout << line << std::endl;
          if (!fout.good())
          {
            outError      = "[Error] Fail to write to file: [" + inTargetFileAndPath + "] while writing it.";
            writeActionOK = false;
          }
          else
            drefLineProccessed++;
        }
      }
      else
      {
        outError      = "[Error] Fail to read from file: [" + inFileAndPath + "] while coping it.";
        writeActionOK = false;
      }

      line.clear();
      if (!writeActionOK)
      {
        return false;
      }

    } // end while loop over file lines

    fout << fout.eof(); // manually add EOF at the end of file

  } // end if "fout" is valid
  else
  {
    outError = "[Error]Output stream is not functioning as expected. Skipping copy command.";
    return false;
  }

  Log::logMsgNone("Count DataRef.txt lines: " + Utils::formatNumber<int>(drefLineCounter));
  return true;
}


bool
missionx::system_actions::save_acf_datarefs_with_savepoint_v2(const std::string& inTargetFileAndPath)
{
  bool writeActionOK = true;

  char outFileName[512]{ 0 };
  char outPathAndFile[2048]{ 0 };
  const std::string writable{ "y" };
  std::string       dref_key_s;
  std::string       type_s;

  XPLMGetNthAircraftModel(XPLM_USER_AIRCRAFT, outFileName, outPathAndFile); // we will only return the file name

  fs::path acf_path = outPathAndFile;
  fs::path acf_folder = acf_path.parent_path();
  if ( fs::exists( acf_folder ) && fs::is_directory(acf_folder))
  {
    fs::path cache_file = acf_folder / mxconst::get_FILE_CUSTOM_ACF_CACHED_DATAREFS_NAME();
    if (fs::is_regular_file(cache_file))
    {
      std::ifstream fin;      
      fin.open(cache_file.c_str());
      if (fin.fail() || fin.bad())
      {
        Log::logMsg("[Error] Fail to open checkpoint dataref file - " + cache_file.string());
        return false;
      }
      else
      {

        std::ofstream fout;
        fout.open(inTargetFileAndPath.c_str(), std::ofstream::out);
        if (fout.fail() || fout.bad())
        {
          Log::logMsg("[Error] Fail to find/create target file: [" + inTargetFileAndPath + "]. Skip operation !!!");
          return false;
        }

        if (fout)
        {
          while (std::getline(fin, dref_key_s)) // read each line in from the file
          {
            if (!fin.good())
            {
              Log::logMsg("[Error] Problem reading mission savepoint dataref file - " + cache_file.string());
              return false;
            }

            if (fin.eof())
            {
              break;
            }

            dref_key_s = mxUtils::trim(dref_key_s);
            // skip if empty
            if (dref_key_s.empty() || dref_key_s.length() < 8) // min dataref char length
              continue;

            std::string             drefValue;
            missionx::dataref_param dref(dref_key_s); // initialize and set the value of the dataref
            bool                    isValid = dref.flag_paramReadyToBeUsed;

            // write to file
            if (isValid)
            {
              type_s = mxUtils::mx_translateDrefTypeToString(dref.getDataRefType());
              if (dref.arraySize > 0) // check array dataref
              {
                type_s += mxUtils::formatNumber<int>(dref.arraySize) + "]";

                if (dref.evaluateArray()) // this function can change "isParamReadyToBeUsed" to FALSE hence it won't be available.
                {
                  dref.arrayElementPicked = 0; // 0 = read/pick all values
                  dref.readDatarefValue_into_missionx_plugin();
                  drefValue = "{" + dref.pStrArrayValue + "}";
                }
                else
                {
                  Log::logMsgNone("Fail eval array: " + dref.key);
                  continue;
                }


              } // end if is array
              else
              {
                dref.readDatarefValue_into_missionx_plugin();
                drefValue = "{" + Utils::formatNumber<double>(dref.getValue<double>()) + "}";
              }
              isValid = dref.flag_paramReadyToBeUsed;

            }
            else
            {
              Log::logMsgNone("Fail init: " + dref.key);
              isValid = false;
            } // if initStatic succeed

            if (isValid)
            {
              std::string line = dref.key + "\t" + type_s + "\t" + writable + ((drefValue.empty()) ? drefValue : "\t" + drefValue);
              fout << line << "\n";
              if (!fout.good())
              {
                Log::logMsg("[Error] Fail to write to file: [" + inTargetFileAndPath + "] while writing dataref data.");
                writeActionOK = false;
              }
            }

            if (!writeActionOK)
            {
              if (fout.is_open())
                fout.close();

              return false;
            }

          } // end while read file loop

          if (fout.is_open())
            fout.close();

        } // end if fout stream was open

      } // end fin
      if (fin.is_open())
        fin.close();
    
    } // end if fs::is_regular_file()
  } // end if acf folder exists

  return true;
}


// -----------------------------------


std::set<std::string>
missionx::system_actions::search_datarefs_in_obj_file(fs::path inFile)
{
  const size_t          min_line_chars = 25;
  std::ifstream         file_toRead;
  std::set<std::string> setDatarefs;

  std::ios_base::sync_with_stdio(false);
  std::cin.tie(nullptr);

  file_toRead.open(inFile.string(), std::ios::in); // read the file
  if (file_toRead.is_open())
  {
    std::string line;
    std::string line_s;

    while (getline(file_toRead, line))
    {
      line_s.clear();

      line = mxUtils::trim(line);
      if (line.length() < min_line_chars)
        continue;

      line_s = mxUtils::stringToLower(line);

      if (line_s.find("anim_") != 0)
        continue;


      std::vector<std::string> tokens = mxUtils::split_v2(line);

      if (tokens.empty())
        continue;

      const auto lastVal_s = tokens.back();

      if (lastVal_s.find("CMND=") == 0) // represent command flag, for example in aerobask Phenom 300
        continue;

      if (mxUtils::countCharsInString(lastVal_s, '/') > 0)
      {
        setDatarefs.insert(lastVal_s);
      }


    } // end loop over file lines


    if (file_toRead.bad())
      perror("error while reading file");
  }
  else // fail to open file
  {
    Log::logAttention((std::string("[Fail parse aptdat] Fail to open file: ") + inFile.string()).c_str(), true);
  }
  return setDatarefs;
}



bool
missionx::system_actions::read_saved_mission_dataref_file(std::string inFileAndPath, std::string& outError, const bool bIsCustomDataref)
{
  // 1. open saved file
  // 2. read each line
  // 2.1 check if valid dataref (use Dataref class)
  outError.clear();
//  unsigned int             drefLineCounter = 0;
  std::vector<std::string> vecDrefLineSplit;
  std::vector<std::string> vecValues;
  std::string              line;
  line.clear(); // holds the line from file
  std::string key;
  key.clear();
  std::string drefValue; // holds only the value of the key stored in file

#ifndef RELEASE
  if (bIsCustomDataref) // v3.303.9.1
    Log::logMsg(">>>> Opening Saved Custom Datarefs.");
  else 
    Log::logMsg(">>>> Opening Saved Datarefs.");
#endif
  std::ifstream fin;
  fin.open(inFileAndPath.c_str());


  // v3.0.152
  int fileNameLength = (int)inFileAndPath.size();

  // v3.0.215.6 fixed a loading bug when folder string length exceed 110 characters. The last character split code did not use: "inFileAndPath.size()" and that caused a crash
  const std::string displayFileNameInMessage = mxconst::get_QM() + ((fileNameLength < 110) ? inFileAndPath : std::string(inFileAndPath.substr(0, 30)).append(" ... ").append(inFileAndPath.substr(inFileAndPath.size() - 60))) + mxconst::get_QM();

  if (fin.fail() || fin.bad())
  {
    outError = "[Error] Fail to open checkpoint dataref file - " + displayFileNameInMessage;
    return false;
  }


  while (std::getline(fin, line))
  {
    if (!fin.good())
    {
      outError = "[Error] Problem reading mission savepoint dataref file - " + displayFileNameInMessage;
      return false;
    }

    if (fin.eof())
    {
      break;
    }

    line = Utils::trim(line);
//    drefLineCounter++;

    // skip if empty
    if (line.empty())
      continue;

    // check if line starts with "s" "sim/xxx"
    char firstChar = line.front();
    if ( false == bIsCustomDataref && firstChar != 's') // v3.303.9.1 skip only if this is x-plane dataref and not custom dataref
      continue; // skip line

    // split line
    vecDrefLineSplit.clear();
    vecValues.clear();

    vecDrefLineSplit = mxUtils::split_v2(line, "\t");

    if (vecDrefLineSplit.empty() || vecDrefLineSplit.size() < 4)
      continue;

    // store localy the split values
    key = vecDrefLineSplit.at(system_actions::MX_LOAD_KEY_POS_IN_VEC);

    //#ifndef RELEASE // debug purpose 
    //if (key.compare("sim/weather/region/cloud_base_msl_m") == 0)
    //  int i = 0;
    //#endif

    drefValue = vecDrefLineSplit.at(system_actions::MX_LOAD_VALUE_POS_IN_VEC); // holds value

    // clear values
    drefValue = Utils::replaceCharsWithString(drefValue, "{}", "");
    drefValue = Utils::trim(drefValue);
    vecValues = mxUtils::split_v2(drefValue, ","); // if array then we will be ready with the values

    if (vecValues.empty())
      continue;

    // create dataref_param
    // initialize key
    // if not array, then set value
    // if array then prepare array and call setarray function
    dataref_param dref(key);
    if (dref.dataRefType == xplmType_IntArray)
    {
      // Option A: cons: magnetos switch reset to 0
      missionx::data_manager::apply_dataref_based_on_key_value_strings(key, drefValue);

      //// Options B:
      //dref.out_vecArrayIntValues.clear();
      //for (auto s : vecValues)
      //  dref.out_vecArrayIntValues.push_back(Utils::stringToNumber<int>(s));
      // dref.flag_individual_value_copy_inTheArray = true; // v3.305.3
      // dataref_param::set_dataref_value_into_xplane(dref, true); // v3.305.3 added custom value flag

//#ifndef RELEASE
//      std::string format = "[" + dref.key + "] [";
//      for (auto s : vecValues)
//        format += s + " ";
//
//      format += "]";
//
//#endif

    }
    else if (dref.dataRefType == xplmType_FloatArray)
    {
      //// Option A: cons: magnetos switch reset to 0
      missionx::data_manager::apply_dataref_based_on_key_value_strings(key, drefValue);

      //// Option B:
      //dref.out_vecArrayFloatValues.clear();
      //for (auto s : vecValues)
      //  dref.out_vecArrayFloatValues.push_back(Utils::stringToNumber<float>(s));
      // dref.flag_individual_value_copy_inTheArray = true; // v3.305.3
      // dataref_param::set_dataref_value_into_xplane(dref, true); // v3.305.3 added custom value flag

//#ifndef RELEASE
//      std::string format = "[" + dref.key + "] [";
//      for (auto s : vecValues)
//        format += s + " ";
//
//      format += "]";
//#endif
    }
    else if (dref.dataRefType == xplmType_Int || dref.dataRefType == xplmType_Float || dref.dataRefType == xplmType_Double)
    {
      const double d = Utils::stringToNumber<double>(drefValue);
      dref.setValue(d);

      dataref_param::set_dataref_values_into_xplane(dref);

    }
    else
    {
      Log::logMsgErr("Dataref might not be supported: " + mxconst::get_QM() + key + mxconst::get_QM());
    }

  } // loop until eof


  return true;
} // read_saved_mission_dataref_file



std::string
missionx::system_actions::getOptionFileAndPath()
{
  char              path[1024]   = { '\0' };
  const std::string fileName     = "missionx_pref_v3.xml";
  std::string       fullPathName = "";

  XPLMGetPrefsPath(path);
  XPLMExtractFileAndPath(path);

  fullPathName = std::string(path) + XPLMGetDirectorySeparator() + fileName;


  return fullPathName;
}



IXMLNode
missionx::system_actions::load_plugin_options()
{
  const std::string fullPathName = system_actions::getOptionFileAndPath();
  std::string       errMsg;
  errMsg.clear();


  IXMLDomParser iDom;
  ITCXMLNode    xMissionxNode = iDom.openFileHelper(fullPathName.c_str(), mxconst::get_MISSIONX_ROOT_DOC().c_str(), &errMsg);

  if (errMsg.empty())
  {
    return xMissionxNode.deepCopy();
  }

  Log::logXPLMDebugString(errMsg + "\n");
  IXMLNode xOldOptions                      = iDom.openFileHelper(fullPathName.c_str(), mxconst::get_ELEMENT_OPTIONS_CAPITAL_LETTERS().c_str(), &errMsg).deepCopy();
  return system_actions::create_new_plugin_preference_file(xOldOptions); // save the options, but it also creates if there are none
}





IXMLNode
missionx::system_actions::add_overpass_urls()
{
  // parse default URL

  if (missionx::data_manager::xMissionxPropertiesNode.isEmpty())
  {
    IXMLDomParser iDom;
    IXMLResults   parse_result_strct;
    auto          xOverpass = iDom.parseString(mxconst::get_OVERPASS_XML_URLS().c_str(), mxconst::get_ELEMENT_OVERPASS().c_str(), &parse_result_strct).deepCopy(); // parse xml into ITCXMLNode
    if (!xOverpass.isEmpty())
    {
      return xOverpass;
    }
  }
  else
  {
    return missionx::data_manager::xMissionxPropertiesNode.deepCopy();
  }

  return IXMLNode();
}

void
missionx::system_actions::store_plugin_options()
{


  if (missionx::data_manager::xMissionxPropertiesNode.isEmpty())
    missionx::data_manager::xMissionxPropertiesNode = create_new_plugin_preference_file();
  else
  {
    const auto        fileVer_s          = Utils::readAttrib(missionx::data_manager::xMissionxPropertiesNode, mxconst::get_ATTRIB_MXFEATURE(), mxUtils::formatNumber<int>(missionx::MX_FEATURES_VERSION));
    const std::string fullPathToPrefFile = system_actions::getOptionFileAndPath();


    /* Build XML in Memory */
    IXMLNode     xMainNode;
    IXMLRenderer xmlWriter;

    auto xLocalMissionxNode = missionx::data_manager::xMissionxPropertiesNode.deepCopy();

    // remove <setup> because we will use system_action setup node instead
    if (auto setupNode = xLocalMissionxNode.getChildNode(mxconst::get_ELEMENT_SETUP().c_str())
        ;!setupNode.isEmpty())
      setupNode.deleteNodeContent();

    if (xLocalMissionxNode.nChildNode(mxconst::get_ELEMENT_OVERPASS().c_str()) == 0)
      xLocalMissionxNode.addChild(add_overpass_urls());

    if (xLocalMissionxNode.nChildNode(mxconst::get_ELEMENT_SCORING().c_str()) == 0)
      xLocalMissionxNode.addChild( Utils::xml_get_node_from_XSD_map_as_acopy (mxconst::get_ELEMENT_SCORING()) );

    // add <setup> main node
    xLocalMissionxNode.addChild(missionx::system_actions::pluginSetupOptions.node.deepCopy());

    // v24.03.1 <notes> element should be the last one
    auto xNotes = xLocalMissionxNode.getChildNode(mxconst::get_ELEMENT_NOTES().c_str());
    if (!xNotes.isEmpty())
    {
      auto xNotesTemp = xNotes.deepCopy();
      xNotes.deleteNodeContent();
      xLocalMissionxNode.addChild(xNotesTemp);
    }

    // replace <MISSIONX> node in data manager with the new constructed one. This is not performance friendly but it has minimal to no impact
    missionx::data_manager::xMissionxPropertiesNode = IXMLNode::emptyNode (); // v25.03.3 replaced "delete nodes"
    missionx::data_manager::xMissionxPropertiesNode = xLocalMissionxNode.deepCopy();

    [[maybe_unused]] IXMLErrorInfo xmlLoadErr = xmlWriter.writeToFile(xLocalMissionxNode, fullPathToPrefFile.c_str());

    // test the file
    bool saveOk = true;

    std::string errMsg;
    errMsg.clear();

    IXMLDomParser iDom;
    iDom.openFileHelper(fullPathToPrefFile.c_str(), mxconst::get_MISSIONX_ROOT_DOC().c_str(), &errMsg);
    if (!errMsg.empty())
    {
      Log::logMsg("[Error] " + errMsg);
      saveOk = false;
    }

    Log::logMsg(std::string("Save options status: ") + ((saveOk) ? "success." : "failed. "));

  } // end store missionx::data_manager::xMissionxPropertiesNode


} // store_plugin_options



IXMLNode
missionx::system_actions::create_new_plugin_preference_file(IXMLNode inOldOptionsNode)
{
  const std::string fullPathToPrefFile = system_actions::getOptionFileAndPath();
  const auto        fileVer_s          = Utils::readAttrib(system_actions::pluginSetupOptions.node, mxconst::get_ATTRIB_MXFEATURE(), "0");

  /* Build XML in Memory */
  IXMLNode     xMainNode;
  IXMLRenderer xmlWriter;

  xMainNode = IXMLNode::createXMLTopNode("xml", TRUE);
  xMainNode.addAttribute("version", "1.0");
  xMainNode.addAttribute("encoding", "iso-8859-1");
  auto xMissionxNode = xMainNode.addChild(mxconst::get_MISSIONX_ROOT_DOC().c_str());

  [[maybe_unused]] const bool b1 = Utils::xml_set_attribute_in_node_asString(xMissionxNode, mxconst::get_ATTRIB_MXFEATURE(), fileVer_s, mxconst::get_MISSIONX_ROOT_DOC()); // pick the version from last option load

  // Construct the Overpass URLs
  auto xURLs = add_overpass_urls();

  if (!xURLs.isEmpty())
    xMissionxNode.addChild(xURLs);

  system_actions::pluginSetupOptions.node = xMissionxNode.addChild(mxconst::get_ELEMENT_SETUP().c_str());
  assert(system_actions::pluginSetupOptions.node.isEmpty() == false && "failed creating <setup> node for preference file");

  system_actions::pluginSetupOptions.node.updateAttribute(fileVer_s.c_str(), mxconst::get_ATTRIB_MXFEATURE().c_str(), mxconst::get_ATTRIB_MXFEATURE().c_str());
  system_actions::pluginSetupOptions.node = system_actions::pluginSetupOptions.node.deepCopy();

  xmlWriter.writeToFile(xMainNode, fullPathToPrefFile.c_str());

  // test the file
  bool saveOk = true;

  std::string errMsg;
  errMsg.clear();

  IXMLDomParser iDom;
  iDom.openFileHelper(fullPathToPrefFile.c_str(), mxconst::get_MISSIONX_ROOT_DOC().c_str(), &errMsg);
  if (!errMsg.empty())
  {
    Log::logMsg("[Error] " + errMsg);
    saveOk = false;
  }

  Log::logMsg(std::string("Tried to create Mission-X preference file. Create status: ") + ((saveOk) ? "success." : "failed. "));

  return xMissionxNode.deepCopy(); // return a copy of the XML preference file
}



std::set<std::string>
missionx::system_actions::search_datarefs_in_acf_file(const std::string& inSourceFile)
{
  fs::path inFile = inSourceFile;

  const size_t          min_cahrs = 8;
  std::ifstream         file_aptDat;
  std::set<std::string> setDatarefs;

  std::ios_base::sync_with_stdio(false);
  std::cin.tie(nullptr);

  file_aptDat.open(inFile.string(), std::ios::in); // read the file

  if (file_aptDat.is_open())
  {
    std::string line;
    std::string line_s;

    while (getline(file_aptDat, line))
    {
      line_s = mxUtils::stringToLower(line);
      const size_t strLength = line_s.length();

      if (min_cahrs < strLength)
      {
        auto datarefPos_i    = line_s.find("dataref");
        auto pos_first_space = line_s.find(" ", datarefPos_i);
        if (datarefPos_i != line_s.npos && datarefPos_i < (strLength - 1) && pos_first_space != std::string::npos)
        {
          const std::string dataref_s = mxUtils::trim(line.substr(pos_first_space, 256)); // only copy the last 256 chars
          if (dataref_s.find("sim/") == 0) // skip default dataref names
            continue;

          setDatarefs.emplace(dataref_s);
        }
      } // end if string length is longer than 8 characters
        
    } // end while getLine
    if (file_aptDat.bad())
     Log::logXPLMDebugString (">>> Error while reading file: " + inSourceFile);
  }
  else // fail to open file
  {
    Log::logAttention("Fail to open the file: " + inSourceFile);    
  }

  return setDatarefs;
}

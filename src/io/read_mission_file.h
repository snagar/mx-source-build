#ifndef READ_MISSION_FILE_H
#define READ_MISSION_FILE_H

/**************

First Implementation: 14-jan-2016


**************/


#include <vector>

#include "IXMLParser.h"
#include "../core/xx_mission_constants.hpp"

#include "Log.hpp"
#include "../data/Trigger.h"
#include "../data/Waypoint.h"
//#include "../inv/Item.hpp"
#include "../core/embeded_script/base_script.hpp"

//#include "../core/embeded_script/script_manager.h"

using namespace missionx;
// using namespace mxconst;

namespace missionx
{

class read_mission_file
{
private:
  // static char pluginDir[1024];
  static std::string errMsg;

  static std::vector<std::string> vecErrors;

  static IXMLNode dummy_top_node;

  static IXMLRenderer xmlWriter;

public:
  read_mission_file();

  static bool initOK;
  static void addError(std::string inError);


  // Specific element readings
  static void readGlobalSettings ( const ITCXMLNode & xParent); // , std::string& inPathToRootMissionFolder);
  static void readBriefer(ITCXMLNode& xParent);
  static void readScriptElement(ITCXMLNode& xParent, std::map<std::string, missionx::base_script>& inMapScripts, std::string& inScriptFolder);
  static void addIncludeFiles(std::map<std::string, missionx::base_script>& inMapScripts); // should be called after "readScriptElement" and "readFlightLegElements"

  static void readXPlaneDataElement(ITCXMLNode& xParent);
  static void readDatarefElement(ITCXMLNode& xParent);

  static void readObjectiveAndTaskElements(ITCXMLNode& xParent);
  static void readFlightPlanElements(std::map<std::string, missionx::Waypoint>&        inMapHoldsFlightLegs,
                                     IXMLNode&                                     inXmlRootChoices,
                                     ITCXMLNode&                                   xParent,
                                     std::string&                                  inPathToRootMissionFolder,
                                     std::map<std::string, missionx::base_script>& inMapScripts,
                                     std::string&                                  inScriptFolder);

  static void readTriggers(ITCXMLNode& xParent);
  static void postReadTriggers(std::map<std::string, missionx::Trigger>& inMapTriggers);
  static void readInventories ( const ITCXMLNode & xParent);                                      // read inventory information
  //static bool readItem(ITCXMLNode& xItem, missionx::Item& outItem, bool canLink = true); // read item information. return tru if item is valid
  static void readMessages(ITCXMLNode& xParent);

  static void                             readEndMission(ITCXMLNode& xParent, std::string& inPathToMissionBrieferFolder);
  static missionx::mx_location_3d_objects readLocationElement(ITCXMLNode& xNode);
  static bool                             readPointElement(ITCXMLNode& xNode, missionx::Point& inPoint); // return true if Point is valid
  static missionx::mx_location_3d_objects readConditionElement(ITCXMLNode& xNode);

  static void read3DObjects(ITCXMLNode& xParent);

  static void readGPS(ITCXMLNode& xParent);

  static void readChoices(IXMLNode& inXmlRootChoices, ITCXMLNode& xParent);

  // Function Members //
  static bool load_mission_file ( const std::string &inPathAndFileName, std::string inPathToRootMissionFolder );

  // validations
  static bool post_load_validations(const IXMLNode& xMainNode, const std::string& inPathAndFileName = "");
  static void post_load_calc_story_message_timings(IXMLNode xMainNode);

  static bool loadSavePoint();

  static std::string getErrMsg() { return errMsg; } // v3.0.152


  // v3.0.241.1 for the new way we read and manipulate the XML data file
  template<class Container>
  static bool load_saved_elements(Container& inMap, ITCXMLNode& xMainNode, const std::string& pName, const std::string& cName, const bool isMandatory, std::string& errText, std::string attribHoldsKeyName = "") // v3.303.10 added "attribHoldsKeyName". When reading instance, we need to use the instance name and not the obnj3d Name
  {
    constexpr bool success = true;
    errText.clear();


    //// read the root element (pName) from which you want to read the other sub elements ////
    ITCXMLNode xParent = xMainNode.getChildNode(pName.c_str());
    if (xParent.isEmpty() && isMandatory)
    {
      errText = "No <" + pName + "> elements were found in save file. Abort savepoint loading !!!";
      return false;
    }
    else
    {
      const int nChilds = xParent.nChildNode(cName.c_str());
      if (nChilds < 1 && isMandatory) // validate
      {
        errText = "No <" + pName + ">  elements were found in save file. Abort savepoint loading !!!";
        initOK  = false;
        return false;
      }

      // loop over child elements (cName) Elements
      for (int i1 = 0; i1 < nChilds; i1++)
      {
        ITCXMLNode xChild = xParent.getChildNode(cName.c_str(), i1);
        if (xChild.isEmpty())
          continue;

        typename Container::mapped_type inElement;
        inElement.node = xChild.deepCopy();
        if (inElement.parse_node())
        {
          const std::string key = (attribHoldsKeyName.empty()) ? inElement.getName() : Utils::readAttrib(inElement.node, attribHoldsKeyName, ""); // v3.303.11 extended for special cases like <display_object> who are instances
          if (!key.empty())
            Utils::addElementToMap(inMap, key, inElement);

          inMap[key].prepareCueMetaData();

          Log::logMsg(fmt::format("read and parsed: {}:{}", cName, key) ); // debug
        }
        else
          addError(errMsg);

      } // end loop sub elements

    } // end if Parent element exists

    return success;
  }
};

} // end namespace

#endif // READ_MISSION_FILE_H

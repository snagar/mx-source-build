#ifndef LISTDIR_H_
#define LISTDIR_H_

/**************

Updated: 24-nov-2012

Done: Nothing to change


ToDo:


**************/

#include <cstdio>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <vector>

#include "BrieferInfo.h"
#include "TemplateFileInfo.hpp"
// #include "../core/xx_mission_constants.hpp"
// #include "Log.hpp"
//#include"../core/thread/base_thread.hpp"

using namespace missionx;

namespace missionx
{



/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

class ListDir //: public base_thread
{

public:
  ListDir(void);
  ~ListDir(void);


  // void stop_plugin();
  ////// End Thread AptData ////////

  static bool getListOfFiles(const char* inPath, std::map<std::string, std::string>& mapFiles, const std::string& filter);    // store list of files in map
  static bool getListOfDirectories(const char* inPath, std::map<std::string, std::string>* mapFiles, const std::string& filter); // store list of directories in map

  static bool getListOfFilesAsFullPath(const char* inPath, std::list<std::string>& outListOfFiles, const std::string& filter, bool in_flag_add_front = false); // store list of files as full path


  // Read mission folders and validate they have "briefer" folder in them as valid structure.
  // Later function will validate {mission_folder_name}.xml
  // read briefer information for Load screen. Images will be read separately.
  // outMapBrieferInfo & outMapBrieferInfoLocator must have same number of elements
  static void readMissionsBrieferInfo(const std::string& inPath, std::map<std::string, BrieferInfo>& outMapBrieferInfo, std::map<int, std::string>& outMapBrieferInfoLocator);

  static bool readExtScriptFile(const std::string& inPathAndScriptName, std::string* outContent);

  static bool read_all_templates(const std::string& inPluginTemplateFolder, const std::string& inCustomMissionFolder, std::map<std::string, TemplateFileInfo>& outMapTemplateFilesInfo); // v3.0.241.10 b2
  static bool readTemplateFilesInfo(const std::string& inPath, std::map<std::string, TemplateFileInfo>& outMapTemplateFilesInfo, int inSeq, bool flag_is_custom_folder = false, std::string in_main_mission_folder_name_s = "");
  // backup of the original code before using read_template_file() in readTemplateFilesInfo()
  static bool readTemplateFilesInfo_v1(const std::string& inPath, std::map<std::string, TemplateFileInfo>& outMapTemplateFilesInfo, int inSeq, bool flag_is_custom_folder = false, std::string in_main_mission_folder_name_s = "");

  // v25.02.1 read the template file information and return error text if any.
  static std::string read_template_file(const std::string& inPath, const std::string& fileName, missionx::TemplateFileInfo &fileInfo, const bool &flag_is_custom_folder = false, const std::string &in_main_mission_folder_name_s = "", const bool &isThread = false); // v25.02.1 Read only one template file information
  static bool parse_replace_options_into_template (missionx::TemplateFileInfo &fileInfo);
  // readFontMetadata: returns all lines that starts with: "inStartsWith" string, except the search string.
  // If ini file is not found it will try to create one but it will use an in memory default text of that "fonts.ini" file.
  static std::vector<std::string> readFontMetadata(const std::string& inStartsWith, int howMany, const std::string& inDefaultValue, const std::string& inAlternativeContent);

private:
  static const char* CN_FONT_METADATA_FILE; //  = "./Resources/plugins/missionx/libs/fonts/fonts.ini";

};


} // namespace

#endif // LISTDIR_H_

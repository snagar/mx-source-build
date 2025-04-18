#ifndef SYSTEMACTIONS_H_
#define SYSTEMACTIONS_H_
#pragma once

/***

Main purpose of this CLSAS is to do System actions on ALL supported platforms.

1. Copy/Paste (ascii/binary)
2.

***/

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>

#include <filesystem>
namespace fs = std::filesystem;


#include <assert.h>

#include "IXMLParser.h"
#include "../data/mxProperties.hpp"
#include "../core/mx_base_node.h"



namespace missionx
{


class system_actions
{
public:
  system_actions();
  virtual ~system_actions();

  // MX Options
  //static missionx::mxProperties pluginSetupOptions; // holds plugin options saved in preferences file
  static missionx::mx_base_node pluginSetupOptions; // holds plugin options saved in preferences file

  /***
  Copy file:
  inSourceFilePath: absolute file location + file name.
  inTargetFilePath: absolute destination file location + file name.
  outError: Holds error message.
  https://gehrcke.de/2011/06/reading-files-in-c-using-ifstream-dealing-correctly-with-badbit-failbit-eofbit-and-perror/
  **/

  static bool copy_file(std::string inSourceFilePath, std::string inTargetFilePath, std::string& outError); // implemented in v3.0.201

  /*init_missionx_log_file: Reset the log file so we will write into a blank file. */
  //static void init_missionx_log_file(std::string inFileAndPath);

  /*write_missionx_log_file: Write message to X-Plane log and to Mission-X log.*/
  //static bool write_missionx_log_file(std::string inFileAndPath, std::string& inMsg, std::string& outError);

  // read dataref file and filter by write
  static bool save_datarefs_with_savepoint(std::string inFileAndPath, std::string inTargetFileAndPath, std::string& outError);
  static bool save_acf_datarefs_with_savepoint_v2(const std::string & inTargetFileAndPath);                                                // v3.303.9.1
  //static bool save_acf_datarefs_with_savepoint(std::string inFileAndPath, std::string inTargetFileAndPath, std::string& outError); // v3.303.9.1

  // read missions checkpoint dataref file and apply in x-plane
  static bool read_saved_mission_dataref_file(std::string inFileAndPath, std::string& outError, const bool bIsCustomDataref=false);

  // get full path and options file
  static std::string getOptionFileAndPath();

  // load plugin options
  static IXMLNode load_plugin_options();

  static IXMLNode add_overpass_urls();
  static void     store_plugin_options();                                                         // returns the ROOT node
  static IXMLNode create_new_plugin_preference_file(IXMLNode inOldOptionsNode = IXMLNode::emptyNode()); // returns the ROOT node

  //static std::set<std::string> gather_custom_acf_datarefs(fs::path inFile);
  static std::set<std::string> search_datarefs_in_acf_file(const std::string& inSourceFile);
  static std::set<std::string> search_datarefs_in_obj_file(fs::path inFile);

private:
  static constexpr const int MX_LOAD_KEY_POS_IN_VEC   = 0;
  static constexpr const int MX_LOAD_VALUE_POS_IN_VEC = 3;

};

}
#endif // SYSTEMACTIONS_H_

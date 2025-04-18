#include "../mission.h"
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

#include "ListDir.h"
#include <assert.h>
#include <cctype>
#include <chrono>
#include <iostream>
#include <fstream>
#include <vector>



#include "IXMLParser.h"
#include "../core/Utils.h"


namespace missionx
{
const char* ListDir::CN_FONT_METADATA_FILE = "./Resources/plugins/missionx/libs/fonts/fonts.ini";
}

// -----------------------------------

ListDir::ListDir(void) = default;

// -----------------------------------

// ListDir::~ListDir(void) {}

// -----------------------------------


bool
missionx::ListDir::getListOfFiles(const char* inPath, std::map<std::string, std::string>& mapFiles, const std::string& filter)
{

  fs::path path = inPath;
  const auto filterLength = filter.length();

  if (fs::directory_entry(path).is_directory())
  {
    for (const auto& entry : fs::directory_iterator(path))
    {
      // std::cout << entry.path() << std::endl;
      auto       fileName = entry.path().filename().string();
      const auto found    = fileName.find(filter.c_str(), fileName.length() - filterLength); // check last 4 characters if ".xml" or ".txt" or what ever
      if (found != std::string::npos)
        mapFiles.insert(make_pair(fileName, entry.path().string()));
    }
  }
  else
    return false; // we did not receive a folder path

  return true;
}

// -----------------------------------

bool
missionx::ListDir::getListOfDirectories(const char* inPath, std::map<std::string, std::string>* mapFiles, const std::string& filter)
{

  if (fs::path path = inPath
      ; fs::directory_entry(path).is_directory())
  {
    for (const auto& entry : fs::directory_iterator(path))
    {
      if (entry.is_directory())
      {
        auto fileName = entry.path().filename().string();

#ifndef RELEASE
        const auto found = fileName.find(filter.c_str()); // check filter
        if (found != std::string::npos)
#else
        if (fileName.find(filter) != std::string::npos)
#endif // !RELEASE
          mapFiles->insert(make_pair(fileName, entry.path().string()));
      }
    }
  }
  else
    return false; // we did not receive a folder path

  return true;
}


// -----------------------------------

bool
missionx::ListDir::getListOfFilesAsFullPath(const char* inPath, std::list<std::string>& outListOfFiles, const std::string& filter, bool in_flag_add_front)
{
  std::map<std::string, std::string> mapFiles;
  if (getListOfFiles(inPath, mapFiles, filter))
  {
    if (in_flag_add_front) // v3.303.14 added missing implementation of this flag
    {
      for (const auto& path_s : mapFiles | std::views::values)
        outListOfFiles.emplace_front (path_s);
    }
    else {
      for (const auto& path_s : mapFiles | std::views::values)
        outListOfFiles.emplace_back(path_s);
    }
  }
  else
    return false;

  return true;
}


// -----------------------------------

void
missionx::ListDir::readMissionsBrieferInfo(const std::string& inCustomMissionFolderName, std::map<std::string, BrieferInfo>& outMapBrieferInfo, std::map<int, std::string>& outMapBrieferInfoLocator)
{
  //  bool structureSuccess = false;
  std::string                        folderName;
  std::map<std::string, std::string> mapListFiles;

  int seq = 0;
  outMapBrieferInfo.clear();
  outMapBrieferInfoLocator.clear();

  Log::logMsg("\n===> Start Reading Mission Files <===\n"); // v3.0.255.4.1

  if (fs::directory_entry(inCustomMissionFolderName).is_directory()) // if parentDir fail initialization
  {
    // loop over all files and check if they are V3 ready.
    // file that can't be opened will be removed
    if (getListOfDirectories(inCustomMissionFolderName.c_str(), &mapListFiles, missionx::EMPTY_STRING)) // valid folder list
    {
      std::string errMsg;
      std::string attrib;
      for (const auto& path_s : mapListFiles | std::views::values)
      {

        if (const std::string brieferFolder_s = std::string(path_s).append( mxconst::get_FOLDER_SEPARATOR() ).append(mxconst::get_BRIEFER_FOLDER())
            ; fs::is_directory(brieferFolder_s)) // if fail initialization folder then skip
        {
          BrieferInfo brieferInfo;

          errMsg.clear();
          attrib.clear();

          // initialize brieferInfo
          brieferInfo.pathToMissionPackFolderInCustomScenery = path_s; // v3.0.156
          brieferInfo.pathToMissionFile       = brieferFolder_s;

          // v3.0.161 Read all XML files in "brieferInfo.pathToMissionFile" folder - supports more than 1 file in briefer folder
          std::map<std::string, std::string> mapMissionFiles;
          mapMissionFiles.clear();
          if (getListOfFiles(brieferInfo.pathToMissionFile.c_str(), mapMissionFiles, ".xml"))
          {

            for (const auto& fileName : mapMissionFiles | std::views::keys)
            {

              brieferInfo.mission_filename = fileName; // +".xml"; map holds the file + extension

              // debug
              Log::logMsg("Reading File: " + fileName); // v3.0.200

              //// Read XML mission file
              IXMLDomParser iDom;
              ITCXMLNode    xParent = iDom.openFileHelper(brieferInfo.getFullMissionXmlFilePath().c_str(), mxconst::get_MISSION_ELEMENT().c_str(), &errMsg);

              // check if mission file is present. If error string is empty, file is present
              if (errMsg.empty()) // if file exists
              {
                /* read MISSION attributes */
                attrib = Utils::readAttrib(xParent, "version", "0");

                const auto lmbda_check_mission_file_has_supported_version = [&]() {

                  std::vector<std::string> vecSupportedVersions = Utils::split(attrib, ',');

                  // v24.12.2 Check supported file version.
                  const bool bResult = std::ranges::any_of(vecSupportedVersions, [&](const auto& item)
                  {
                    if (auto supportedVersion_i = mxUtils::stringToNumber<int>(item); // init
                        std::ranges::find(missionx::lsSupportedMissionFileVersions, supportedVersion_i) != missionx::lsSupportedMissionFileVersions.end() )
                      return true;

                    return false;
                  });

                  return bResult;
                }; // end lambda to check if version is valid

                if (const bool flag_fileVersionIsValid = lmbda_check_mission_file_has_supported_version(); // init
                    flag_fileVersionIsValid
                   ) // skip if not correct version
                {
                  ITCXMLNode xBriefer;
                  ITCXMLNode xBrieferInfoNode;

                  // validate that mission file has at least the briefer element
                  xBriefer = xParent.getChildNode(mxconst::get_ELEMENT_BRIEFER().c_str());
                  if (xBriefer.isEmpty())
                    continue;

                  brieferInfo.missionName = Utils::readAttrib(xParent, mxconst::get_ATTRIB_NAME(), fileName); // v3.0.153

                  brieferInfo.missionDesc = (xBriefer.nClear() > 0) ? xBriefer.getClear().sValue : "Please add Briefer description!!!";
                  brieferInfo.setBrieferDescription(brieferInfo.missionDesc);


                  brieferInfo.missionTitle = Utils::readAttrib(xParent, mxconst::get_ATTRIB_TITLE(), brieferInfo.missionName);
                  brieferInfo.missionTitle = Utils::replaceChar1WithChar2_v2(brieferInfo.missionTitle, ';', " "); // remove any ";"

                  // read <mission_info>
                  xBrieferInfoNode = xParent.getChildNode(mxconst::get_ELEMENT_MISSION_INFO().c_str());
                  if (!xBrieferInfoNode.isEmpty())
                  {
                    // fetch information on mission image file
                    mxTextureFile texture;
                    texture.fileName = Utils::readAttrib(xBrieferInfoNode, mxconst::get_ATTRIB_MISSION_IMAGE_FILE_NAME(), "briefer.png");
                    texture.filePath = brieferFolder_s;
                    Utils::addElementToMap(brieferInfo.mapImages, fileName, texture); // mFile.first = root mission folder name

                    brieferInfo.planeTypeDesc    = Utils::readAttrib(xBrieferInfoNode, mxconst::get_ATTRIB_PLANE_DESC(), EMPTY_STRING);
                    brieferInfo.difficultyDesc   = Utils::readAttrib(xBrieferInfoNode, mxconst::get_ATTRIB_DIFFICULTY(), EMPTY_STRING);
                    brieferInfo.estimateTimeDesc = Utils::readAttrib(xBrieferInfoNode, mxconst::get_ATTRIB_ESTIMATE_TIME(), EMPTY_STRING);
                    brieferInfo.weatherDesc      = Utils::readAttrib(xBrieferInfoNode, mxconst::get_ATTRIB_WEATHER_SETTINGS(), EMPTY_STRING);
                    brieferInfo.scenery_settings = Utils::readAttrib(xBrieferInfoNode, mxconst::get_ATTRIB_SCENERY_SETTINGS(), EMPTY_STRING);
                    brieferInfo.written_by       = Utils::readAttrib(xBrieferInfoNode, mxconst::get_ATTRIB_WRITTEN_BY(), EMPTY_STRING);
                    brieferInfo.other_settings   = Utils::readAttrib(xBrieferInfoNode, mxconst::get_ATTRIB_OTHER_SETTINGS(), EMPTY_STRING);
                    brieferInfo.missionTitle     = Utils::readAttrib(xBrieferInfoNode, mxconst::get_ATTRIB_TITLE(), brieferInfo.missionTitle); // v3.0.255.4.1 allow designer to define special title in mission_info that will override the MISSION attributes

                    brieferInfo.node = xBrieferInfoNode.deepCopy(); // v3.0.255.4 store the information for use in template/mission pick screen. We will use the <options> sube element to add "include" options to the XML mission file
                  }


                  Utils::addElementToMap(outMapBrieferInfo, fileName, brieferInfo);
                  seq++;
                  Utils::addElementToMap(outMapBrieferInfoLocator, seq, fileName);
                } // end if flag_fileVersionIsValid
                else
                {
                  Log::logMsg( fmt::format("Mission File: {}, is not in the correct version. File version: {}, expected version: {}", fileName, attrib, data_manager::mission_file_supported_versions) );
                }
              }
              else // print debug info
              {
                Log::logMsg(errMsg); // v3.0.200
              }
            }
          }
          else
          {
            Log::logMsg(errMsg); //
          }

        } // if dir is present
        else
        {
          Log::logMsg("Folder: " + path_s + ", is not v3 valid mission. Skipping...");
        }

      } // end loop over folders

      Log::logMsg("\n===> End Reading Mission Files <===\n"); // v3.0.255.4.1

    } // end read folders in "custom scenery/missionx"
  }
  else
  {
#ifndef RELEASE
    Log::logMsg("[ListDir ERROR] Fail open folder information: " + inCustomMissionFolderName);
#endif // !RELEASE
  }
}


// -----------------------------------

bool
missionx::ListDir::read_all_templates(const std::string& inPluginTemplateFolder, const std::string& inCustomMissionFolder, std::map<std::string, TemplateFileInfo>& outMapTemplateFilesInfo)
{
  std::string                        folderName;
  std::map<std::string, std::string> mapListFiles;

  readTemplateFilesInfo(inPluginTemplateFolder, outMapTemplateFilesInfo, static_cast<int>(outMapTemplateFilesInfo.size()), false);

  // point to a folder
  mapListFiles.clear();

  if (getListOfDirectories(inCustomMissionFolder.c_str(), &mapListFiles, missionx::EMPTY_STRING)) // valid folder list
  {
    std::string preferred_name;

    for (const auto &[folderName_s, path_s] : mapListFiles)
    {
      readTemplateFilesInfo(path_s, outMapTemplateFilesInfo, static_cast<int>(outMapTemplateFilesInfo.size()), true, folderName_s);
    }
  } // end get list of folders

  return true;
}


// -----------------------------------
bool
missionx::ListDir::readTemplateFilesInfo(const std::string& inPath, std::map<std::string, missionx::TemplateFileInfo>& outMapTemplateFilesInfo, int inSeq, bool flag_is_custom_folder, std::string in_main_mission_folder_name_s)
{
  bool                               result = true;
  std::string                        errMsg;
  std::string                        folderName;
  std::string                        failedTemplatesFilesList_s;
  std::map<std::string, std::string> mapListFiles;

  errMsg.clear();
  mapListFiles.clear();
  int seq = inSeq;

  //// point to a folder
  if (fs::directory_entry(inPath).is_directory()) // if parentDir fail initialization
  {

    // loop over all files and check if they are V3 ready.
    // file that can't be open will be removed
    if (getListOfFiles(inPath.c_str(), mapListFiles, ".xml")) // valid folder list
    {
      // int counterOfWrongFormatFiles = 0; // add

      for (const auto& fileName : mapListFiles | std::views::keys)
      {
        missionx::TemplateFileInfo fileInfo;

        fileInfo.fileName = fileName;

        if (flag_is_custom_folder)
        {
          if (mxconst::get_TEMPLATE_FILE_NAME() != fileInfo.fileName) // if file name is not "template.xml"
          {
            // Log::logMsg("Skipping XML File: " + fileName + " - not a template.xml" + mxconst::get_UNIX_EOL()); // debug
            Log::logMsg(fmt::format(R"(Skipping XML File: "{}" - not a template.xml\n)", fileName)); // debug
            continue;
          }
        }

        // Call read_template_file
        errMsg = read_template_file(inPath, fileName, fileInfo, flag_is_custom_folder, in_main_mission_folder_name_s);
        if (!errMsg.empty())
        {
          failedTemplatesFilesList_s += (failedTemplatesFilesList_s.empty() ? "" : ", ");
          failedTemplatesFilesList_s += fileInfo.getAbsoluteTemplateXmlFilePath();
          continue;
        }

        // check if template is valid and store it or not
        const std::string key_s = (fileInfo.missionFolderName.empty() ? fileInfo.fileName : fileInfo.missionFolderName);
        assert(!key_s.empty());
        if (!key_s.empty())
        {
          ++seq;
          fileInfo.seq = seq;
          Utils::addElementToMap(outMapTemplateFilesInfo, key_s, fileInfo);
        }

      } // end loop over mapListFiles

      // send message if found a template with wrong template version
      if (!failedTemplatesFilesList_s.empty())
      {
        const std::string message = fmt::format(R"(The following templates are invalid, either has wrong format version, or missing mandatory information: {})", failedTemplatesFilesList_s);
        Log::logMsgWarn(message);
      }

    } // end get List of files
  }
  else
  {
    #ifndef RELEASE
    Log::logMsg("[ListDir ERROR] Fail open folder information: " + inPath);
    #endif // !RELEASE

    return false; // skip
  }

  return result;
}

// -----------------------------------
bool
missionx::ListDir::readTemplateFilesInfo_v1(const std::string& inPath, std::map<std::string, missionx::TemplateFileInfo>& outMapTemplateFilesInfo, int inSeq, bool flag_is_custom_folder, std::string in_main_mission_folder_name_s)
{
  bool                               result = true;
  std::string                        errMsg;
  std::string                        folderName;
  std::map<std::string, std::string> mapListFiles;
//
//   errMsg.clear();
//   mapListFiles.clear();
//   int seq = inSeq;
//
//   //// point to a folder
//   if (fs::directory_entry(inPath).is_directory()) // if parentDir fail initialization
//   {
//
//     // loop over all files and check if they are V3 ready.
//     // file that can't be open will be removed
//     if (getListOfFiles(inPath.c_str(), mapListFiles, ".xml")) // valid folder list
//     {
//       int counterOfWrongFormatFiles = 0; // add
//
//       for (const auto& fileName : mapListFiles | std::views::keys)
//       {
//         missionx::TemplateFileInfo fileInfo;
//
//         fileInfo.fileName = fileName;
//
//         if (flag_is_custom_folder)
//         {
//           if (mxconst::get_TEMPLATE_FILE_NAME() != fileInfo.fileName) // if file name is not "template.xml"
//           {
//             // Log::logMsg("Skipping XML File: " + fileName + " - not a template.xml" + mxconst::get_UNIX_EOL()); // debug
//             Log::logMsg(fmt::format(R"(Skipping XML File: "{}" - not a template.xml\n)", fileName)); // debug
//             continue;
//           }
//         }
//
//         // debug
//         // Log::logMsg("Reading File: " + inPath + "/" + fileName + mxconst::get_UNIX_EOL());
//         Log::logMsg(fmt::format(R"(Reading File: "{}/{}"\n)", inPath, fileName));
//
//         fileInfo.imageFile.filePath = fileInfo.filePath = inPath;
//
//         fileInfo.fullFilePath = inPath + std::string(XPLMGetDirectorySeparator()).append(fileName); // absolute path with file name
//
//         //// Read XML mission file
//         IXMLDomParser iDom;
//         ITCXMLNode    xParent = iDom.openFileHelper(fileInfo.fullFilePath.c_str(), mxconst::get_TEMPLATE_ROOT_DOC().c_str(), &errMsg);
//         ITCXMLNode    xRoot;
//
//         // check if mission file is present. If error string is empty, file is present
//         if (errMsg.empty()) // if file exists
//         {
//           /* read MISSION attributes */
//
//           if (const std::string attrib = Utils::readAttrib(xParent, "version", "0"); missionx::RANDOM_TEMPLATE_VER != attrib) // skip if not correct version
//           {
//             ++counterOfWrongFormatFiles;
//             // Log::logMsgErr("Template file: " + fileName + ", is not in the correct version - " + missionx::RANDOM_TEMPLATE_VER + ", currently it is: " + attrib + ". Check documentation");
//             Log::logMsgErr(fmt::format(R"(Template file: "{}", is not in the correct version - "{}", currently it is: "{}". Check documentation)", fileName, missionx::RANDOM_TEMPLATE_VER, attrib));
//             continue;
//           }
//
//
//           auto xTemplateMissionInfoNode = xParent.getChildNode(mxconst::get_ELEMENT_MISSION_INFO().c_str());
//           if (xTemplateMissionInfoNode.isEmpty())
//           {
//             Log::logMsgErr("Fail to load template file: " + fileName + ". No <mission_info> element found. skipping...");
//             Log::logMsg(errMsg);
//             continue;
//           }
//
//           fileInfo.node = xTemplateMissionInfoNode.deepCopy();
//           // prepare image information
//           fileInfo.imageFile.fileName = Utils::readAttrib(fileInfo.node, mxconst::get_ATTRIB_TEMPLATE_IMAGE_FILE_NAME(), "");
//           fileInfo.imageFile.filePath = fileInfo.getPath() + ((flag_is_custom_folder) ? XPLMGetDirectorySeparator() + mxconst::get_BRIEFER_FOLDER() + XPLMGetDirectorySeparator() : ""); // v3.0.241.10 b2 if this a custom folder, then image should reside in "briefer" folder.
//
//           // read short_desc
//           if (std::string short_desc_s = Utils::readAttrib(fileInfo.node, mxconst::get_ATTRIB_SHORT_DESC(), ""); !short_desc_s.empty())
//           {
//             fileInfo.prepareSentenceBasedOnString(short_desc_s); // v3.303.14
//           }
//
//           // validate a template image name is present
//           if (!fileInfo.imageFile.fileName.empty() || fileInfo.fileName == mxconst::get_RANDOM_TEMPLATE_BLANK_4_UI()) // v3.0.241.9 modified to handle the special template file: "template_blank_4_ui.xml" that we will remove from map later.
//           {
//             ++seq;
//             fileInfo.seq = seq;
//
//             const auto lmbda_get_key_to_store_and_finalize_fileInfo = [&]()
//             {
//               if (flag_is_custom_folder)
//               {
//                 fileInfo.missionFolderName = in_main_mission_folder_name_s; //
//                 return in_main_mission_folder_name_s;
//               }
//
//               return fileInfo.fileName; // v3.303.14 replaced returning "fileName" from external loop
//             };
//
//             const std::string key_s = lmbda_get_key_to_store_and_finalize_fileInfo();
//
//             assert(!key_s.empty());
//
//             // v3.0.255.4.1 initial options vector, this will be used with ImGui::Combo
//             {
//               IXMLDomParser iDomReplaceOptions;
//               ITCXMLNode    xREPLACE_OPTIONS = iDomReplaceOptions.openFileHelper(fileInfo.fullFilePath.c_str(), mxconst::get_ELEMENT_TEMPLATE_REPLACE_OPTIONS().c_str(), &errMsg);
//               if (errMsg.empty())
//               {
//                 fileInfo.vecReplaceOptions_s.clear();
//                 fileInfo.mapOptionsInfo.clear(); // v24.12.2
//
//                 if (!xREPLACE_OPTIONS.isEmpty())
//                 {
//                   fileInfo.nodeReplaceOptions             = xREPLACE_OPTIONS.deepCopy();
//                   const auto    nChilds                   = fileInfo.nodeReplaceOptions.nChildNode(mxconst::get_ELEMENT_OPT().c_str()); // check if we have <opt >
//                   constexpr int compIndex                 = -1;                                                                   // v24.12.2 compatibility Index
//                   fileInfo.mapOptionsInfo[compIndex].name = "Compatibility Options";                                              // v24.12.2 init
//                   if (nChilds > 0)
//                   {
//                     for (int i = 0; i < nChilds; ++i)
//                     {
//                       auto nodeOPT = fileInfo.nodeReplaceOptions.getChildNode(mxconst::get_ELEMENT_OPT().c_str(), i); // read <opt>
//                       if (std::string name = Utils::readAttrib(nodeOPT, mxconst::get_ATTRIB_NAME(), ""); !name.empty())
//                       {
//                         fileInfo.vecReplaceOptions_s.emplace_back(name);                           // store opt name
//                         fileInfo.mapOptionsInfo[compIndex].vecReplaceOptions_s.emplace_back(name); // v24.12.2
//
//                         if (fileInfo.longest_text_length_i < static_cast<int>(name.length())) // v3.0.255.4.1 store longest string size, used in UI
//                         {
//                           fileInfo.longest_text_length_i                           = static_cast<int>(name.length());
//                           fileInfo.mapOptionsInfo[compIndex].longest_text_length_i = static_cast<int>(name.length()); // v24.12.2
//                           fileInfo.mapOptionsInfo[compIndex].longestTextInVector_s = name;                            // v24.12.2
//                         }
//                       }
//                     }
//                     // Store the address of all "opt names" to use with the UI
//                     fileInfo.mapOptionsInfo[compIndex].refresh_vecReplaceOptions_char();
//                   }
//
//
//                   // v24.12.2 Read all <options> sub groups
//                   const auto nOptionsChilds = fileInfo.nodeReplaceOptions.nChildNode(mxconst::get_ELEMENT_OPTION_GROUP().c_str()); // check if we have <option>
//                   for (int i = 0; i < nOptionsChilds; ++i)
//                   {
//                     auto              nodeOptionGroup = fileInfo.nodeReplaceOptions.getChildNode(mxconst::get_ELEMENT_OPTION_GROUP().c_str(), i); // read <option>
//                     const std::string option_name     = Utils::readAttrib(nodeOptionGroup, mxconst::get_ATTRIB_NAME(), "");
//                     if (!option_name.empty())
//                     {
//                       fileInfo.mapOptionsInfo[i].name = option_name;
//
//                       const auto nOptChilds = nodeOptionGroup.nChildNode(mxconst::get_ELEMENT_OPT().c_str()); // check if we have <opt >
//                       for (int i2 = 0; i2 < nOptChilds; ++i2)
//                       {
//                         auto nodeOPT = nodeOptionGroup.getChildNode(mxconst::get_ELEMENT_OPT().c_str(), i2); // read <opt>
//                         if (std::string opt_name = Utils::readAttrib(nodeOPT, mxconst::get_ATTRIB_NAME(), ""); !opt_name.empty())
//                         {
//                           fileInfo.mapOptionsInfo[i].vecReplaceOptions_s.emplace_back(opt_name);                      // store opt name
//                           if (fileInfo.mapOptionsInfo[i].longest_text_length_i < static_cast<int>(opt_name.length())) // store longest string size, used in UI
//                           {
//                             fileInfo.mapOptionsInfo[i].longest_text_length_i = static_cast<int>(opt_name.length());
//                             fileInfo.mapOptionsInfo[i].longestTextInVector_s = opt_name; // v24.12.2
//                           }
//                         }
//                       }
//                       // Store the address of all "opt names" to use with the UI
//                       fileInfo.mapOptionsInfo[i].refresh_vecReplaceOptions_char();
//
//                       if (fileInfo.mapOptionsInfo[i].vecReplaceOptions_s.empty())
//                       {
//                         fileInfo.mapOptionsInfo.erase(i);
//                         continue;
//                       }
//                     }
//                   }
//                   // END v24.12.2 gathering "options"
//                 }
//               }
//               else
//               {
//                 Log::logMsg("[ListDir] FYI: No " + mxconst::get_ELEMENT_TEMPLATE_REPLACE_OPTIONS() + " was found. This is valid and not an error.");
//               }
//
//             } // end anonymous block
//
//
//             if (!key_s.empty())
//               Utils::addElementToMap(outMapTemplateFilesInfo, key_s, fileInfo);
//           }
//         }
//         else
//         {
//           Log::logMsgErr(errMsg);
//           result = false;
//         }
//       } // end loop over mapListFiles
//
//       // send message if found a template with wrong template version
//       if (counterOfWrongFormatFiles > 0)
//       {
//         std::string message = std::string("Found ") + ((counterOfWrongFormatFiles == 1) ? "a template " : Utils::formatNumber<int>(counterOfWrongFormatFiles) + " templates ") + " with wrong version.";
//         XPLMSpeakString(message.c_str());
//       }
//
//     } // end get List of files
//   }
//   else
//   {
// #ifndef RELEASE
//     Log::logMsg("[ListDir ERROR] Fail open folder information: " + inPath);
// #endif // !RELEASE
//
//     return false; // skip
//   }

  return result;
}


// -----------------------------------


std::string
ListDir::read_template_file(const std::string& inPath, const std::string& fileName, missionx::TemplateFileInfo& fileInfo, const bool& flag_is_custom_folder, const std::string& in_main_mission_folder_name_s, const bool& isThread)
{
  const std::string file_path = std::string(inPath) + "/" + fileName;
  std::string       errMsg;

  Log::logMsg(fmt::format(R"(Reading File: "{}/{}"\n)", inPath, fileName));

  fileInfo.imageFile.filePath = fileInfo.filePath = inPath;

  fileInfo.fullFilePath = inPath + std::string(XPLMGetDirectorySeparator()).append(fileName); // absolute path with file name

  //// Read XML mission file
  IXMLDomParser iDom;
  ITCXMLNode    xParent = iDom.openFileHelper(fileInfo.fullFilePath.c_str(), mxconst::get_TEMPLATE_ROOT_DOC().c_str(), &errMsg);
  ITCXMLNode    xRoot;

  // check if mission file is present. If error string is empty, file is present
  if (errMsg.empty()) // if file exists
  {
    /* read MISSION attributes */

    if (const std::string attrib = Utils::readAttrib(xParent, "version", "0"); missionx::RANDOM_TEMPLATE_VER != attrib) // skip if not correct version
    {
      errMsg = fmt::format(R"(Template file: "{}", is not in the correct version - "{}", currently it is: "{}". Check documentation.)", file_path, missionx::RANDOM_TEMPLATE_VER, attrib);
      Log::logMsgErr(errMsg, isThread);
      return errMsg;
    }

    const auto xTemplateMissionInfoNode = xParent.getChildNode(mxconst::get_ELEMENT_MISSION_INFO().c_str());
    if (xTemplateMissionInfoNode.isEmpty())
    {
      errMsg = "Fail to load template file: " + fileName + ". No <mission_info> element found. skipping...";
      Log::logMsgErr(errMsg, isThread);
      // continue;
      return errMsg;
    }

    fileInfo.node = xTemplateMissionInfoNode.deepCopy();
    // prepare image information
    fileInfo.imageFile.fileName = Utils::readAttrib(fileInfo.node, mxconst::get_ATTRIB_TEMPLATE_IMAGE_FILE_NAME(), "");

    // read short_desc
    if (std::string short_desc_s = Utils::readAttrib(fileInfo.node, mxconst::get_ATTRIB_SHORT_DESC(), ""); !short_desc_s.empty())
    {
      fileInfo.prepareSentenceBasedOnString(short_desc_s); // v3.303.14
    }

    // validate a template image name is present
    if (!fileInfo.imageFile.fileName.empty() || fileInfo.fileName == mxconst::get_RANDOM_TEMPLATE_BLANK_4_UI()) // v3.0.241.9 modified to handle the special template file: "template_blank_4_ui.xml" that we will remove from map later.
    {
      if (flag_is_custom_folder)
      {
        fileInfo.missionFolderName = in_main_mission_folder_name_s;
        fileInfo.imageFile.filePath = fileInfo.getPath() + XPLMGetDirectorySeparator() + mxconst::get_BRIEFER_FOLDER() + XPLMGetDirectorySeparator(); // v25.02.1 if this a custom folder, then image should reside in "briefer" folder.
      }

      // v3.0.255.4.1 initial options vector, this will be used with ImGui::Combo
      parse_replace_options_into_template(fileInfo);

    }
  }
  else
  {
    Log::logMsgErr(errMsg);
    return errMsg;
  }

  return errMsg; // empty error message should be success
}

// -----------------------------------

bool
ListDir::parse_replace_options_into_template(missionx::TemplateFileInfo& fileInfo)
{
  std::string errMsg;

  IXMLDomParser iDomReplaceOptions;
  ITCXMLNode    xREPLACE_OPTIONS = iDomReplaceOptions.openFileHelper(fileInfo.fullFilePath.c_str(), mxconst::get_ELEMENT_TEMPLATE_REPLACE_OPTIONS().c_str(), &errMsg);
  if (errMsg.empty())
  {
    fileInfo.vecReplaceOptions_s.clear();
    fileInfo.mapOptionsInfo.clear(); // v24.12.2

    if (!xREPLACE_OPTIONS.isEmpty())
    {
      fileInfo.nodeReplaceOptions             = xREPLACE_OPTIONS.deepCopy ();
      const auto    nChilds                   = fileInfo.nodeReplaceOptions.nChildNode (mxconst::get_ELEMENT_OPT().c_str ()); // check if we have <opt >
      constexpr int compIndex                 = -1; // v24.12.2 compatibility Index
      if (nChilds > 0)
      {
        fileInfo.mapOptionsInfo[compIndex].name = "Legacy Options"; // v24.12.2 init
        for (int i = 0; i < nChilds; ++i)
        {
          auto nodeOPT = fileInfo.nodeReplaceOptions.getChildNode(mxconst::get_ELEMENT_OPT().c_str(), i); // read <opt>
          if (std::string name = Utils::readAttrib(nodeOPT, mxconst::get_ATTRIB_NAME(), ""); !name.empty())
          {
            fileInfo.vecReplaceOptions_s.emplace_back(name);                           // store opt name
            fileInfo.mapOptionsInfo[compIndex].vecReplaceOptions_s.emplace_back(name); // v24.12.2

            if (fileInfo.longest_text_length_i < static_cast<int>(name.length())) // v3.0.255.4.1 store longest string size, used in UI
            {
              fileInfo.longest_text_length_i                           = static_cast<int>(name.length());
              fileInfo.mapOptionsInfo[compIndex].longest_text_length_i = static_cast<int>(name.length()); // v24.12.2
              fileInfo.mapOptionsInfo[compIndex].longestTextInVector_s = name;                            // v24.12.2
            }
          }
        }
        // Store the address of all "opt names" to use with the UI
        fileInfo.mapOptionsInfo[compIndex].refresh_vecReplaceOptions_char();
      }


      // v24.12.2 Read all child <option_group> nodes
      const auto nOptionsChilds = fileInfo.nodeReplaceOptions.nChildNode(mxconst::get_ELEMENT_OPTION_GROUP().c_str()); // check if we have <option>
      for (int i = 0; i < nOptionsChilds; ++i)
      {
        auto              nodeOptionGroup = fileInfo.nodeReplaceOptions.getChildNode(mxconst::get_ELEMENT_OPTION_GROUP().c_str(), i); // read <option>
        const std::string option_name     = Utils::readAttrib(nodeOptionGroup, mxconst::get_ATTRIB_NAME(), "");
        if (!option_name.empty())
        {
          fileInfo.mapOptionsInfo[i].name = option_name;

          const auto nOptChilds = nodeOptionGroup.nChildNode(mxconst::get_ELEMENT_OPT().c_str()); // check if we have <opt >
          for (int i2 = 0; i2 < nOptChilds; ++i2)
          {
            auto nodeOPT = nodeOptionGroup.getChildNode(mxconst::get_ELEMENT_OPT().c_str(), i2); // read <opt>
            if (std::string opt_name = Utils::readAttrib(nodeOPT, mxconst::get_ATTRIB_NAME(), ""); !opt_name.empty())
            {
              fileInfo.mapOptionsInfo[i].vecReplaceOptions_s.emplace_back(opt_name);                      // store opt name
              if (fileInfo.mapOptionsInfo[i].longest_text_length_i < static_cast<int>(opt_name.length())) // store longest string size, used in UI
              {
                fileInfo.mapOptionsInfo[i].longest_text_length_i = static_cast<int>(opt_name.length());
                fileInfo.mapOptionsInfo[i].longestTextInVector_s = opt_name; // v24.12.2
              }
            }
          }
          // Store the address of all "opt names" to use with the UI
          fileInfo.mapOptionsInfo[i].refresh_vecReplaceOptions_char();

          if (fileInfo.mapOptionsInfo[i].vecReplaceOptions_s.empty())
            fileInfo.mapOptionsInfo.erase(i);
        } // end if not option is empty

      } // end loop over nOptionsChilds
      // END v24.12.2 gathering "options"
    }
  }
  else
  {
    Log::logMsg("[ListDir] FYI: No " + mxconst::get_ELEMENT_TEMPLATE_REPLACE_OPTIONS() + " was found. This is valid and not an error.");
  }

  return true;
}

// -----------------------------------

std::vector<std::string>
missionx::ListDir::readFontMetadata(const std::string& inStartsWith, int howMany, const std::string& inDefaultValue, const std::string& inAlternativeContent)
{
  std::stringstream        buffer; // will hold the pointer to the data string (from file or memory)
  std::vector<std::string> vecResult;
  vecResult.clear();
  fs::path                 path = ListDir::CN_FONT_METADATA_FILE;
  bool     bReadMetaFile = false;
  if (fs::directory_entry(path).is_regular_file())
  {
    std::ifstream file(ListDir::CN_FONT_METADATA_FILE, std::ios::in);
    if (file)
    {
      buffer << file.rdbuf();
      file.close();
      bReadMetaFile = true;
    }

  } // end if font metadata exists
  else
  {
    // create default file
    std::ofstream file(path.string());
    if (file) {
      file << inAlternativeContent;
    }
  }


  if (bReadMetaFile == false)
    buffer << inAlternativeContent;

  // loop over buffer
  bool bContinue = true;

  int         iFoundCounter = 0;
  std::string line;

  while (bContinue && std::getline(buffer, line))
  {
    if (line.find(inStartsWith) == 0)
    {
      bool bFileExists = true;
      // validate existence
      const std::string line_vu = mxUtils::rtrim( line.substr(inStartsWith.length()) );

      if (inStartsWith == "font=") // Special case to handle font files vs the other options. Ugly but works.
      {
        fs::path fontPath = line_vu; // remove any special characters that might fail file recognition.
        if (fs::directory_entry(fontPath).is_regular_file() == false)
          bFileExists = false;
      }

      if (bFileExists)
      {
        iFoundCounter++;
        vecResult.emplace_back(line_vu); // return only the value of the startWith
        if (howMany && howMany >= iFoundCounter)
          bContinue = false;
      } // end file exists
    }   // end if found string
  }     // end while loop


  if (vecResult.empty() && !inDefaultValue.empty())
    vecResult.emplace_back(inDefaultValue);

  return vecResult;
}


// -----------------------------------


bool
ListDir::readExtScriptFile(const std::string& inPathAndScriptName, std::string* outContent)
{
  if (outContent != nullptr)
  {
    if (std::ifstream ifs(inPathAndScriptName)
        ; ifs) // if exists
    {
      const std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
      (*outContent) = content;

      return true;
    }
  }

  return false;
}

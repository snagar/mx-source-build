#ifndef TEMPLATEFILEINFO_H_
#define TEMPLATEFILEINFO_H_

#pragma once

#include "../core/xx_mission_constants.hpp"
#include "../io/IXMLParser.h"
#include "../ui/core/TextureFile.h"
#include "../core/Utils.h"
#include "../core/vr/mxvr.h"

namespace missionx
{

class TemplateFileInfo //: public missionx::mxProperties
{
public:
  TemplateFileInfo()
  {
    node               = IXMLNode::emptyIXMLNode;
    nodeReplaceOptions = IXMLNode::emptyIXMLNode; // v3.0.255.4.1

    this->seq = mxconst::INT_UNDEFINED; // no locator is present in missionx::data_manager::mapGenerateMissionTemplateFilesLocator
  }


  ~TemplateFileInfo() {}

  int      seq; // v3.0.241.9 holds same sequence number as map file locator so it will be easier to erase if needed. If init with -1 then it means that there is no locator
  IXMLNode node;
  IXMLNode nodeReplaceOptions;                       // v3.0.255.4.1 holds the root element <REPLACE_OPTIONS>
  size_t   size_of_vecReplaceOptions_n{ (size_t)0 }; // v3.0.255.4 holds vecReplaceOptions_s size

  missionx::mxTextureFile                              imageFile;
  std::string                                          fileName;
  std::string                                          fileProposedName;  // v3.0.241.10 b2 holds the folder name
  std::string                                          missionFolderName; // v3.0.241.10 b2
  std::string                                          filePath;
  std::string                                          fullFilePath;
  std::string                                          desc_from_vector_with_tabs_s;                    // v3.0.251.1 // holds the description with '\t' but without the "\n\r".
  std::string                                          description;                                     // v3.0.251.1 will hold generated mission description (this is initialized by the plugin)
  std::vector<std::string>                             vecSentences;                                    // holds template mission info "short_desc" as lines (this is defined by designer)
  std::vector<std::string>                             vecImguiSentences;                               // holds template mission info "short_desc" as lines
  std::vector<std::string>                             vecReplaceOptions_s{};                           // v24.12.2 TODO: deprecate // v3.0.255.4 add <opt> support in <mission_info>, will allow designer to create different include txt file to inject into an XML file.
  std::map<int, missionx::mx_option_info>              mapOptionsInfo{};                                // v24.12.2 add <options> support in <mission_info>, will allow designer to create different include txt file to inject into an XML file.
  //std::unordered_map<int, missionx::mx_ui_option_info> mapUserOptionsPicked;                            // v24.12.2 used by Random Engine, we should consider consolidate into "mapOptionsInfo", but only after it will work.
  int                                                  longest_text_length_i{ mxconst::INT_UNDEFINED }; // v3.0.255.4.1

  //int user_pickedReplaceOptions_i = { mxconst::INT_UNDEFINED }; // v24.12.2 deprecated, we use the "multi-option" container path instead. // will be initialized by the plugin before calling random engine mission::flcPRE() "imgui_generate_random_mission_file"

  

  void prepareSentenceBasedOnString(const std::string& inString) //, int inLineWidth, int maxLines)
  {
    this->vecSentences.clear();
    this->vecImguiSentences.clear();

    if (!inString.empty())
    {
      std::string desc_s = inString;
      desc_s             = Utils::replaceChar1WithChar2_v2(desc_s, '\n', ";");
      desc_s             = Utils::replaceChar1WithChar2_v2(desc_s, '\r', ";");
      desc_s             = Utils::replaceChar1WithChar2_v2(desc_s, '\t', " ");

      this->vecSentences = Utils::sentenceTokenizerWithBoundaries(desc_s, mxconst::get_SPACE(), ((missionx::mxvr::vr_display_missionx_in_vr_mode) ? missionx::MAX_CHARS_IN_RANDOM_LINE_3D : missionx::MAX_CHARS_IN_RANDOM_LINE_2D), ";");

      desc_s = Utils::replaceChar1WithChar2_v2(desc_s, '\n', ";");
      desc_s = Utils::replaceChar1WithChar2_v2(desc_s, '\r', ";");

      this->vecImguiSentences = Utils::sentenceTokenizerWithBoundaries(desc_s, mxconst::get_SPACE(), 65, ";");

      // v3.0.251.1 store also the vector string and see how it will render
      if (!vecImguiSentences.empty())
      {
        for (auto& s : this->vecImguiSentences)
        {
          this->desc_from_vector_with_tabs_s += s + "\n";
        }
      }
      else
        this->desc_from_vector_with_tabs_s = "No description was defined for this file";
    }
  }


  std::string getAbsoluteTemplateXmlFilePath()
  {
    std::string path;
    path.clear();

    path = this->fullFilePath;

    return path; //
  }


  std::string getPath()
  {
    return this->filePath; 
  }

  std::string getFileName()
  {
    return this->fileName;
  }

  std::string getTemplateImageFileName()
  {
    return this->imageFile.fileName;
  }


}; // end class

} // namespace

#endif

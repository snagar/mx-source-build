/*
 * BrieferInfo.h
 *
 *  Created on: Jan 7, 2012
 *      Author: snagar
 */


#ifndef BRIEFERINFO_H_
#define BRIEFERINFO_H_

#include "IXMLParser.h"
#include "../ui/core/TextureFile.h"

// removed in v3.303.14
//#include "../core/Utils.h"
//#include "../core/base_c_includes.h"
//#include "../core/base_xp_include.h"
//#include "../core/xx_mission_constants.hpp"


namespace missionx
{
class BrieferInfo
{
public:
  IXMLNode node{ IXMLNode::emptyIXMLNode }; // v3.0.255.4 will store a copy of the briefer_info element

  BrieferInfo();
  BrieferInfo(std::string path, std::string inFilename, std::string title, std::string desc);
//  virtual ~BrieferInfo();

  std::string              missionName; // v3.0.153
  std::string              missionTitle;
  std::string              missionDesc;
  std::vector<std::string> vecSentences;

  std::string planeTypeDesc;
  std::string estimateTimeDesc;
  std::string weatherDesc;
  std::string difficultyDesc;
  std::string scenery_settings;
  std::string written_by;
  std::string other_settings;

  std::string mission_filename;
  std::string pathToMissionFile;
  std::string pathToMissionPackFolderInCustomScenery;

  std::map<std::string, mxTextureFile> mapImages; // at startup we only read the briefer image, but when we press start we can load all mission specific images. We store the filename +


  // Function will remove special characters and split into sentences based on "BRIEFERINFO_SENTENCE_LENGTH" constant.
  void setBrieferDescription(std::string inDesc);

  std::string getFullMissionXmlFilePath();

  void init();
};

}


#endif /* BRIEFERINFO_H_ */

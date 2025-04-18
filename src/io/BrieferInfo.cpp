/*
 * BrieferInfo.cpp
 *
 *  Created on: Jan 7, 2012
 *      Author: snagar
 */

/**************

Updated: 24-nov-2012

Done: Nothing to change


ToDo:


**************/

#include "BrieferInfo.h"
#include "../core/Utils.h"
#include "../core/vr/mxvr.h"

missionx::BrieferInfo::BrieferInfo()
{
  init();
}

missionx::BrieferInfo::BrieferInfo(std::string path, std::string inFilename, std::string title, std::string desc)
{
  init();
  this->pathToMissionFile = path;
  this->mission_filename  = inFilename;
  this->missionTitle      = title;
  this->missionDesc       = desc;
}


//missionx::BrieferInfo::~BrieferInfo()
//{
//  // TODO Auto-generated destructor stub
//}

void
missionx::BrieferInfo::setBrieferDescription(std::string inDesc)
{
  this->missionDesc = inDesc;

  this->vecSentences.clear();
  this->vecSentences = Utils::sentenceTokenizerWithBoundaries(this->missionDesc, mxconst::get_SPACE(), ((missionx::mxvr::vr_display_missionx_in_vr_mode) ? missionx::MAX_CHARS_IN_BRIEFER_LINE_3D : missionx::MAX_CHARS_IN_BRIEFER_LINE_2D), ";");
}

std::string
missionx::BrieferInfo::getFullMissionXmlFilePath()
{
  {
    if (!pathToMissionFile.empty())
    {
      const char sep = (*XPLMGetDirectorySeparator());
      const char c   = pathToMissionFile.back();
      if (c == sep)
        return std::string(pathToMissionFile) + mission_filename;
      else
        return std::string(pathToMissionFile) + XPLMGetDirectorySeparator() + mission_filename;
    }

    return EMPTY_STRING;
  }
}

void
missionx::BrieferInfo::init()
{
  missionTitle.clear();
  missionDesc.clear();
  mission_filename.clear();
  pathToMissionFile.clear();

  this->difficultyDesc.clear();
  this->estimateTimeDesc.clear();
  this->weatherDesc.clear();
  this->vecSentences.clear();
  this->other_settings.clear();
  this->pathToMissionPackFolderInCustomScenery.clear();
  this->mapImages.clear();
  this->scenery_settings.clear();
  this->written_by.clear();
}

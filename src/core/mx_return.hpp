/*
 * mx_return.hpp
 *
 *  Created on: Dec 09, 2024
 *      Author: snagar
 * 
 * Used for more robust information moving between functions.
 */

#ifndef MX_RETURN_H_
#define MX_RETURN_H_

#include <string>
#include <unordered_map>
#include <fmt/core.h>

namespace missionx
{


class mx_return
{
private:
  int errIndex  = 0;
  int infoIndex = 0;

public:
  bool result{ false };
  std::string string_value; // v25.04.2 added the option to initialize a text value to use later on.

  std::unordered_map<int, std::string> errMsges;
  std::unordered_map<int, std::string> infoMsges;

  mx_return()
  {
    result    = false;
    errIndex  = 0;
    infoIndex = 0;
    errMsges.clear();
    infoMsges.clear();
  }

  mx_return(bool inInitResult)
  {
    this->result = inInitResult;
    errMsges.clear();
    infoMsges.clear();
  }

  int getErrIndex() { return errIndex; }
  int getInfoIndex() { return infoIndex; }

  int addErrMsg(const std::string& inText)
  {
    ++errIndex;
    this->errMsges[errIndex] = inText;
    return errIndex;
  }

  int addInfoMsg(const std::string& inText)
  {
    ++infoIndex;
    this->infoMsges[infoIndex] = inText;
    return infoIndex;
  }

  std::string getErrorsAsText()
  {
    std::string text;
    text.clear();
    for (const auto& [key, value] : this->errMsges)
      text.append(fmt::format("{}: {}", key, value));

    return text;
  }

  std::string getInfoAsText()
  {
    std::string text;
    text.clear();
    for (const auto& [key, value] : this->infoMsges)
      text.append(fmt::format("{}: {}", key, value));

    return text;
  }
};

} // missionx namespace
#endif // MX_RETURN_H_
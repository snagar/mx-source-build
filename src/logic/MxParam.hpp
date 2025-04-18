#ifndef MXPARAM_H_
#define MXPARAM_H_

#pragma once

#include "../core/mx_base_node.h"
#include <assert.h>

using namespace missionx;

namespace missionx
{

/*
 MxParam represent parameters in x-plane, which should only be "double" based.
*/
class MxParam : public mx_base_node
{
private:
  double      paramNumValue{ 0.0 };
  std::string paramStringValue;

public:
  MxParam() { init(); }
  ~MxParam(){}


  std::string errReason;               // any initialization error should be written to this variable
  std::string pStrArrayValue;          // Will hold full array values delimited by comma ","
  bool        flag_paramReadyToBeUsed; // ALSO use for dataref array that might not be set correctly. FALSE = not usable

  // -----------------------------------


  // members
  void init()
  {
    name.clear();
    paramStringValue.clear();

    setValue<double>(0.0); // set property value to double initialization.
    errReason.clear();
    flag_paramReadyToBeUsed = false;

    if (!mxconst::get_ELEMENT_DATAREF().empty())
      this->node.updateName(mxconst::get_ELEMENT_DATAREF().c_str());
  }

  // -----------------------------------

  void setParamStringValue(const std::string& inValue)
  {
    this->paramStringValue = inValue;
  }

  // -----------------------------------

  std::string getParamStringValue() { return this->paramStringValue; }

  // -----------------------------------

  void setName(const std::string& inVal) { this->name = inVal; }

  // -----------------------------------

  template<typename T>
  void setValue(T inValue)
  {
    this->paramNumValue = static_cast<double> (inValue);
  }

  // -----------------------------------

  template<typename T>
  T getValue()
  {
    return (T)this->paramNumValue;
  }

  // -----------------------------------

  std::string to_string() const
  {
    const std::string format = "[\"" + this->name + "\", valNum: \"" + mxUtils::formatNumber<double>(this->paramNumValue) + "\" | valString: " + this->paramStringValue + "\" ]\n";

    return format;
  }

  // -----------------------------------

private:
};

} // missionx

#endif

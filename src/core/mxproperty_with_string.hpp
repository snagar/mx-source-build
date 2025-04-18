#ifndef PROPERTY_WITH_STRING_H_
#define PROPERTY_WITH_STRING_H_

#pragma once

#include "mxproperty.hpp"
using namespace missionx;

namespace missionx
{

//class mxPropertyWithString : public missionx::mxProperty
//{
////private:
////  std::string s;
////
////public:
////  // make base-class members from mxProperty available
////  using mxProperty::getValue;
////  using mxProperty::setValue;
////  using mxProperty::to_string;
////
////  mxPropertyWithString() { s.clear(); }
////
////
////  std::string getValue() { return s; }
////
////  void clearStr() { s.clear(); }
////
////
////
////  void setValue(const char* inVal)
////  {
////    std::string str = std::string((inVal));
////    setValue(str);
////  }
////
////  void setValue(std::string inVal)
////  {
////    this->s   = inVal;
////    type_enum = mx_property_type::MX_STRING;
////    resetUnion(); // mxProperty
////  }
////
////  std::string to_string()
////  {
////    std::string result;
////    result.clear();
////
////    if (type_enum == missionx::mx_property_type::MX_UNKNOWN)
////      return result;
////
////    if (type_enum == missionx::mx_property_type::MX_STRING)
////      result += getValue();
////    else
////      result += mxProperty::to_string();
////
////    return result;
////  }
//};

} // missionx

#endif
#ifndef PROPERTY_H_
#define PROPERTY_H_
#pragma once
//#include <variant>

#include "MxUtils.h"
//#include "XPLMUtilities.h" // v3.303.14 removed
#include "xx_mission_constants.hpp"
#include <assert.h>
#include <iostream>
#include <string>

using namespace missionx;
// using namespace mxconst;

namespace missionx
{


class mxProperty : public mxUtils
{
private:
  union
  {
    bool   b;
    char   c;
    int    i;
    double d;
  };

  std::string s;

  // c++17 ?
  // std::variant <bool, char, int, double, std::string> v;

public:
  mxProperty()
  {
    resetUnion();
    s.clear();
    type_enum = mx_property_type::MX_UNKNOWN;
  }; // v3.0.255.4.2 changed type = mx_property_type::MX_DOUBLE to mx_property_type::MX_UNKNOWN
  ~mxProperty(){};

  // typedef enum : uint8_t { MX_UNKNOWN, MX_BOOL, MX_CHAR, MX_INT, MX_FLOAT, MX_DOUBLE, MX_STRING } mx_property_type;
  missionx::mx_property_type type_enum;

  std::string getValue() { return s; }

  template<class T>
  T getValue()
  {

    if (std::is_arithmetic<T>::value) // ::value will return true or false
    {
      if (type_enum == mx_property_type::MX_CHAR)
        return (T)c;
      else if (type_enum == mx_property_type::MX_INT)
        return (T)i;
      else if (type_enum == mx_property_type::MX_FLOAT)
        return (T)d;
      else if (type_enum == mx_property_type::MX_DOUBLE)
        return (T)d;
      else if (type_enum == mx_property_type::MX_BOOL)
        return (T)b;
    }

    return (T)0;
  }


  void resetUnion() { d = 0.0; }

  void setValue(bool inVal)
  {
    this->b   = inVal;
    type_enum = missionx::mx_property_type::MX_BOOL;
  }

  void setValue(char inVal)
  {
    this->c   = inVal;
    type_enum = missionx::mx_property_type::MX_CHAR;
  }


  void setValue(int inVal)
  {
    this->i   = inVal;
    type_enum = missionx::mx_property_type::MX_INT;
  }

  void setValue(float inVal)
  {
    this->d   = (double)inVal;
    type_enum = missionx::mx_property_type::MX_FLOAT;
  }

  void setValue(double inVal)
  {
    this->d   = inVal;
    type_enum = missionx::mx_property_type::MX_DOUBLE;
  }

    void setValue(std::string inVal)
  {
    this->s   = inVal;
    type_enum = mx_property_type::MX_STRING;
    resetUnion(); // mxProperty
  }

   void setValue(const char* inVal)
  {
    std::string str = std::string((inVal));
    setValue(str);
  }

  std::string getBoolAsString() { return ((b) ? mxconst::get_MX_TRUE() : mxconst::get_MX_FALSE()); }

  std::string to_string()
  {
    std::string result;
    result.clear();

    if (type_enum == missionx::mx_property_type::MX_UNKNOWN)
      return result;
    else if (type_enum == missionx::mx_property_type::MX_BOOL)
      return ((b) ? mxconst::get_MX_TRUE() : mxconst::get_MX_FALSE());
    else if (type_enum == missionx::mx_property_type::MX_CHAR)
      result += getValue<char>();
    else if (type_enum == missionx::mx_property_type::MX_INT)
      result += formatNumber<int>(getValue<int>());
    else if (type_enum == missionx::mx_property_type::MX_FLOAT)
    {
      int precision = (this->d == 0.0) ? 1 : getPrecision<float>((float)d);
      result += formatNumber<float>((float)d, precision);
    }
    else if (type_enum == missionx::mx_property_type::MX_DOUBLE)
    {
      int precision = (this->d == 0.0) ? 1 : getPrecision<double>(d);
      result += formatNumber<double>(d, precision);
    }
    else if (type_enum == missionx::mx_property_type::MX_STRING)
      result += s;


    return result;
  }



};

} // and namespace missionx

#endif

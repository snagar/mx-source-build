#ifndef BASE_DATA_CLASS_H_
#define BASE_DATA_CLASS_H_
#pragma once

#include <map>
#include "../core/Utils.h"
#include "../core/mxproperty.hpp"

using namespace missionx;
// using namespace mxconst;

/*
mxProperties is mainly use with the embedded script class.
It holds properties from different classes, like tasks, triggers, goal etc and inject it into MY-BASIC when needed.
*/
namespace missionx
{


class mxProperties : public missionx::mxProperty
{

public:
  std::map<std::string, missionx::mxProperty> mapProperties;
  mxProperties()
  {
    mapProperties.clear();
  }

  // -----------------------------------

  mxProperties(mxProperties const&) = default;


  // -----------------------------------
  void clone(mxProperties& inProperties)
  {
    // we do not copy the XML Node: node. Need better investigation regarding implications.
    mapProperties.clear();
#ifdef APL
    this->mapProperties.clear();
    for (auto p : inProperties.mapProperties)
      Utils::addElementToMap(this->mapProperties, p.first, p.second);
#else
    this->mapProperties = inProperties.mapProperties;
#endif
  }

  // OPERATOR
  mxProperties& operator=(mxProperties& inProp) { this->clone(inProp); return *this; }

  // -----------------------------------
  // -----------------------------------
  // -----------------------------------
  void clear()
  {
    if (mapProperties.empty())
      return;

    mapProperties.clear();
  }

  // -----------------------------------
  template<typename T>
  mxProperty setProperty(const std::string& inPropertyName, const T& inElement)
  {

    const auto p = mapProperties.find(inPropertyName);
    if (p != mapProperties.end())
    {
      auto* ptrProperty = &p->second; // mxPropertyWithString
      ptrProperty->setValue(inElement);
      return (*ptrProperty);
    }

    mxProperty new_property;
    new_property.setValue(inElement);
    this->mapProperties.insert(make_pair(inPropertyName, new_property));
    return new_property;
  }


  // -----------------------------------

  /* modify MX_STRING type property */
  mxProperty setStringProperty(const std::string& inKey, const std::string& inNewValue)
  {
    if (trim(inKey).empty())
    {
      Log::logMsgErr("[setProperty]Found EMPTY property key!!! Notify Developer !!!");
      assert(!trim(inKey).empty() && "EMPTY string key in mxProperties");
    }

    return setProperty<std::string>(inKey, inNewValue);

  }

  // -----------------------------------

  mxProperty setBoolProperty(const std::string& inPropertyName, const bool inValue)
  {

    const auto p = mapProperties.find(inPropertyName);
    if (p != mapProperties.end())
    {
      auto* ptrProperty = &p->second; // mxProperty
      ptrProperty->setValue(inValue);
      return (*ptrProperty);
    }

    mxProperty new_property;
    new_property.setValue (inValue);
    this->mapProperties.insert (make_pair (inPropertyName, new_property));
    return new_property;

  }

  // -----------------------------------

  mxProperty setBoolProperty(const std::string& inPropertyName, std::string inValue)
  {
    bool b = false;
    auto emptyProperty = mxProperty();

    if (inValue.empty())
    {
      Log::logMsgErr("[add bool property] Value is empty.");
      return emptyProperty; // skip action with no error
    }
    else if (inPropertyName.empty())
    {
      Log::logMsgErr("[Error add bool property] Property name is empty.");
      return emptyProperty; // skip action with no error
    }

    // convert to lower case
    inValue = stringToLower(trim(inValue));

    // try to guess value and convert to int/bool
    // check length = 1
    // check if "true"/"false"/"yes"/"no"

    if (bool result = isStringBool (inValue, b))
      return setBoolProperty(inPropertyName, b);

    return emptyProperty;
  }

  // -------------------------------------------

  mxProperty setIntProperty(const std::string& inPropertyName, const int inValue, const bool failToDeductValue = false)
  {
    auto emptyProperty = mxProperty ();

    const auto p = mapProperties.find(inPropertyName);
    if (p != mapProperties.end())
    {
      if (!failToDeductValue)
      {
        auto* ptrProperty = &p->second; // mxPropertyWithString
        ptrProperty->setValue(inValue);
        return (*ptrProperty);
      }
      else
        return emptyProperty;
    }

    mxProperty new_property;
    new_property.setValue (inValue);
    this->mapProperties.insert (make_pair (inPropertyName, new_property));

    return new_property;

  }
  // -------------------------------------------

  mxProperty setNumberProperty(const std::string& inPropertyName, const double inValue, const bool failToDeductValue = false)
  {

    const auto p = mapProperties.find(inPropertyName);
    if (p != mapProperties.end())
    {
      if (!failToDeductValue)
      {
        auto* ptrProperty = &p->second; // mxPropertyWithString
        ptrProperty->setValue(inValue);

        return (*ptrProperty);
      }
    }

    mxProperty new_property;
    new_property.setValue (inValue);
    this->mapProperties.insert (make_pair (inPropertyName, new_property));

    return new_property;
  }

  // -----------------------------------

  bool hasProperty(const std::string& inPropName) { return isElementExists(this->mapProperties, inPropName); }


  // -----------------------------------
  void removeProperty(const std::string& inPropName)
  {
    if (hasProperty(inPropName))
      this->mapProperties.erase(inPropName);

    std::set<std::string> attributesToDelete = { inPropName }; // v3.0.303.6
    //Utils::xml_delete_attribute(this->node, attributesToDelete); // v3.0.303.6
  }


  // -----------------------------------

  std::string getPropertyValue(const std::string& key, std::string& outError)
  {
    outError.clear();

    if (const auto iter = mapProperties.find (key)
      ; iter != mapProperties.end())
    {
      mxProperty p = iter->second;
      if (p.type_enum == missionx::mx_property_type::MX_STRING)
      {
        return p.getValue(); // string
      }
      outError = "Property: [" + key + "] is not STRING. Fix Name or Notify Developer!!!";
    }

    // return a value that is "arithmetic based" or "string based" and represent EMPTY.
    return "";
  }

  // -----------------------------------
  // -----------------------------------


};


}

#endif

#pragma once
#ifndef MX_BASE_NODE_H_
#define MX_BASE_NODE_H_

#include "Utils.h"

using namespace missionx;

namespace missionx
{
// Node holds NAME of the element
// Also holds all its attributes in format: attrib_name
class mx_base_node : public missionx::mxUtils
{
public:
  mx_base_node (const mx_base_node &other)
    : missionx::mxUtils (other)
    , name (other.name)
    , node (other.node)
    , mapText (other.mapText)
    // , mx_const (other.mx_const)
  {
  }
  mx_base_node (mx_base_node &&other) noexcept
    : missionx::mxUtils (std::move (other))
    , name (std::move (other.name))
    , node (std::move (other.node))
    , mapText (std::move (other.mapText))
    // , mx_const (std::move (other.mx_const))
  {
  }
  mx_base_node &operator= (const mx_base_node &other)
  {
    if (this == &other)
      return *this;
    missionx::mxUtils::operator= (other);
    name     = other.name;
    node     = other.node.deepCopy();
    mapText  = other.mapText;
    // mx_const = other.mx_const;
    return *this;
  }
  mx_base_node &operator= (mx_base_node &&other) noexcept
  {
    if (this == &other)
      return *this;
    missionx::mxUtils::operator= (std::move (other));
    name     = std::move (other.name);
    node     = std::move (other.node);
    mapText  = std::move (other.mapText);
    // mx_const = std::move (other.mx_const);
    return *this;
  }

private:

public:
  std::string name;
  [[nodiscard]] std::string getName() const { return name; }
  [[nodiscard]] std::string getBaseNodeName() const { return getName(); } // v25.04.2
  void        setName(const std::string& inVal) { name = inVal; } // v3.305.1 for better compatibility with mxProperties class
  void        setBaseNodeName(const std::string &inVal) { setName( inVal ); } // v25.04.2 added a unique function to also set the local name

  IXMLNode                            node{IXMLNode::emptyIXMLNode}; // v24.06.1 initialize with emptyIXMLNode
  std::map <std::string, std::string> mapText; // store special text that might not feet in xml attribute
  missionx::mxconst mx_const;

  mx_base_node();

  explicit mx_base_node(const std::string& inTageName);

  // -------------------------------------------
  void initBaseNode();

  // -------------------------------------------
  IXMLNode addChild(const std::string& inTagName, const std::string& inInitAttribName, const std::string& inInitAttribValue, const std::string& inTextValue); // v3.305.3 used in load formating
  IXMLNode addChildText(const std::string& inTagName, const std::string& inTextValue); // v25.04.2 Add a sub node and set its TEXT value without attributes.
  IXMLNode getChild(const std::string& inTagName); // v25.04.2 get the sub-node.
  std::string getChildTextValue(const std::string& inTagName, const std::string &inDefaultValue = ""); // v25.04.2 get TEXT value of a sub-node. Do not confuse with attributes. <node>TEXT</node>

  // -------------------------------------------
  void searchAndSetStringProperty_inNodeTree(const std::string& inAttribName, const std::string& attribValue, const std::string &inElementName = "");

  // -------------------------------------------
  void setStringProperty(const std::string& inAttribName, const std::string& attribValue, bool in_flagStoreInNode = true);
  // -------------------------------------------
  void setNodeStringProperty(const std::string& inAttribName, const std::string & attribValue, bool in_flagStoreInNode = true); // I'll have to re-use this function once I'll move all "data" classes to use mx_base_node.  
  // -------------------------------------------
  
  std::string getNodeStringProperty(const std::string& inAttribName,  const std::string & attribDefaultValue = "", bool in_getTextFromContainerAndThenFromNode = false); 
  bool        getBoolValue(const std::string& inAttribName, bool attribDefaultValue = false);

  // -------------------------------------------
  // functions that do not store text internaly
  void setNodeStringProperty(const std::string& inAttribName, const std::string& attribValue, IXMLNode& inNode_ptr);
  void setNodeStringProperty_drillDown(const std::string& inAttribName, const std::string& attribValue, IXMLNode& inNode_ptr, const std::string& inTagNameToUpdate);

  // -------------------------------------------
  // set attribute in the current element (not in a sub element)
  std::string getAttribStringValue(std::string inAttribName, std::string inDefaultValue, std::string& outErr) // IXMLNode& inParentNodeToSearchFrom = IXMLNode::emptyIXMLNode, const std::string& inTagNameToUpdate = "")
  {
    outErr.clear();
    return Utils::readAttrib(this->node, inAttribName, inDefaultValue);
  }

  // v3.303.14 should replace getAttribStringValue()
  std::string getStringAttributeValue(std::string inAttribName, std::string inDefaultValue) // IXMLNode& inParentNodeToSearchFrom = IXMLNode::emptyIXMLNode, const std::string& inTagNameToUpdate = "")
  {
    return Utils::readAttrib(this->node, inAttribName, inDefaultValue);
  }

  // -------------------------------------------

  void setNumberProperty(const std::string& inAttribName, const std::string& attribValue);
  void setBoolProperty(const std::string& inAttribName, const std::string& attribValue);
  void setBoolProperty(const std::string& inAttribName, const bool& attribValue);

  // -------------------------------------------

  template <class T>
  void setNumberProperty(const std::string& inAttribName, const T &attribValue)
  {
    if (!this->node.isEmpty())
    {
      this->setStringProperty(inAttribName, mxUtils::formatNumber<T>(attribValue));
    }
  }

  // -------------------------------------------

  template<class T>
  T getAttribNumericValue(std::string inAttribName, T inDefaultValue, std::string& outErr) // IXMLNode& inParentNodeToSearchFrom = IXMLNode::emptyIXMLNode, const std::string& inTagNameToUpdate = "")
  {
    outErr.clear();

    if (!node.isEmpty())
    {
      if (std::is_arithmetic<T>::value)
      {
        std::string val_s = Utils::readAttrib(this->node, inAttribName, mxUtils::formatNumber<T>(inDefaultValue));
        if (mxUtils::is_number(val_s))
          return (T)mxUtils::stringToNumber<T>(val_s, val_s.length());
        else
        {
          outErr = "NaN";
          return (T)inDefaultValue;
        }
      }
    }
    else
    {
      outErr = "Node is empty.";
      return (T)inDefaultValue;
    }

    outErr = "Node is empty.";
    return (T)inDefaultValue;
  }

  // -------------------------------------------

  template<class T>
  T getAttribNumericValue(std::string inAttribName, T inDefaultValue) 
  {
    return Utils::readNodeNumericAttrib<T>(this->node, inAttribName, inDefaultValue);
  }

  // -------------------------------------------

  template<class T>
  void setNodeProperty(const std::string inAttribName, T attribValue)
  {
    if (!node.isEmpty())
      Utils::xml_set_attribute_in_node<T>(node, inAttribName, attribValue, node.getName());
  }
  // -------------------------------------------

  template<class T>
  void setNodeProperty(IXMLNode & inParentNodeToSearchFrom, std::string inAttribName, T attribValue, const std::string& inTagNameToUpdate = "")
  {
    if (!inParentNodeToSearchFrom.isEmpty() )
      Utils::xml_set_attribute_in_node<T>(inParentNodeToSearchFrom, inAttribName, attribValue, inTagNameToUpdate);
    else if (!node.isEmpty())
      Utils::xml_set_attribute_in_node<T>(node, inAttribName, attribValue, node.getName());
  }

  // -------------------------------------------
  // Search and set an attribute in an element (can be a sub element)
  template<class T>
  void searchAndSetNodeProperty_inNodeTree(std::string inAttribName, T attribValue, std::string inElementName = "")
  {
    if (!node.isEmpty())
      Utils::xml_set_attribute_in_node<T>(node, inAttribName, attribValue, (inElementName.empty() ? node.getName() : inElementName)); // If inElementName is empty fallback to current element tag name
  }

  // -------------------------------------------
  // setNodeStringProperty sets the XML node with a numeric/bool value and also uses xml_search_and_set_attribute_in_IXMLNode() to set the value at the Node level.
  // It differs from Utils::xml_set_attribute_in_node() since it also created the sub-node if it is not exists.
  template<class T>
  void setSetupNodeProperty(std::string inTagName, T attribValue) //, IXMLNode& inParentNode_ptr = IXMLNode::emptyIXMLNode)
  {
    std::string                val_s;
    missionx::mx_property_type val_type_enum = missionx::mx_property_type::MX_UNKNOWN;

    if (std::is_integral<decltype(attribValue)>::value) // ::value will return true or false. Is this a number or bool ?
    {
      if (std::is_same<decltype(attribValue), bool>::value)
      {
        val_s = (attribValue) ? "true" : "false";
        // we could also write:
        val_type_enum = missionx::mx_property_type::MX_BOOL;
      }
      else
      {
        val_type_enum = missionx::mx_property_type::MX_INT;
        val_s         = mxUtils::formatNumber<T>(attribValue);
      }
    }
    else if (std::is_floating_point<decltype(attribValue)>::value)
    {
      val_type_enum = missionx::mx_property_type::MX_DOUBLE;
      val_s = mxUtils::formatNumber<T>(attribValue);
    }

    if (!this->node.isEmpty())
    {
      // search for a sub node with the element name {inTagName}
      const std::string val_type_s = mxUtils::formatNumber<int>((int)val_type_enum);
      Utils::xml_search_and_set_node_text(this->node, inTagName, val_s, val_type_s, true);
    }
  }

  // -------------------------------------------
  [[nodiscard]] std::string get_node_as_text() const
  {
    if (this->node.isEmpty())
      return "";

    IXMLRenderer      xmlWriter;
    const std::string text = xmlWriter.getString(this->node);
    xmlWriter.clear();

    return text;
    
  }
  // -------------------------------------------

  // wrappers to the "preference file node"
  template<typename T>
  T getNodeText_type_1_5(const std::string& tagToSearch, T defaultValueIfNotExists)
  {
    return Utils::getNodeText_type_1_5<T>(this->node, tagToSearch, defaultValueIfNotExists);
  }
  // -------------------------------------------
  [[nodiscard]] std::string getNodeText_type_6 (const std::string& tagToSearch, const std::string &defaultValueIfNotExists) const
  {
    return Utils::getNodeText_type_6 (this->node, tagToSearch, defaultValueIfNotExists);
  }

  // -------------------------------------------
};

}

#endif // MX_BASE_NODE_H_

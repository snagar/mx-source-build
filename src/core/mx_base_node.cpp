#include "mx_base_node.h"

missionx::mx_base_node::mx_base_node()
{
  initBaseNode();
}

// -----------------------------------

missionx::mx_base_node::mx_base_node(const std::string& inTageName)
{
  if (!inTageName.empty())
  {
    initBaseNode();
    node.updateName(inTageName.c_str());
  }
}

// -----------------------------------

void
missionx::mx_base_node::initBaseNode()
{
  // if (!mxconst::get_ELEMENT_NODE().empty ())
  node = IXMLNode::createXMLTopNode(mxconst::get_ELEMENT_NODE().c_str());

}

// -----------------------------------

IXMLNode
missionx::mx_base_node::addChild (const std::string &inTagName, const std::string &inInitAttribName, const std::string &inInitAttribValue, const std::string &inTextValue)
{
  if (!inTagName.empty () && !inInitAttribName.empty ())
    return Utils::xml_add_child (this->node, inTagName, inInitAttribName, inInitAttribValue, inTextValue);

  return IXMLNode::emptyNode ();
}

// -----------------------------------

IXMLNode
mx_base_node::addChildText (const std::string &inTagName, const std::string &inTextValue)
{
  if (!inTagName.empty () && !inTextValue.empty ())
    return Utils::xml_add_child (this->node, inTagName, inTextValue);

  return IXMLNode::emptyNode ();
}

// -----------------------------------

IXMLNode
mx_base_node::getChild (const std::string &inTagName)
{
  if (!inTagName.empty ())
    return this->node.getChildNode (inTagName.c_str ());

  return IXMLNode::emptyNode ();
}

// -----------------------------------

std::string
mx_base_node::getChildTextValue (const std::string &inTagName, const std::string &inDefaultValue)
{
  if (!inTagName.empty ())
  {
    const auto child_node = this->node.getChildNode (inTagName.c_str ());
    return Utils::xml_get_text (child_node);
  }

  return inDefaultValue;
}

// -----------------------------------

void
missionx::mx_base_node::searchAndSetStringProperty_inNodeTree(const std::string& inAttribName, const std::string& attribValue, const std::string &inElementName)
{
  if (!node.isEmpty() && !inAttribName.empty ()) // If inElementName is empty fallback to current element tag name
    Utils::xml_search_and_set_attribute_in_IXMLNode(node, inAttribName, attribValue, (inElementName.empty() ? node.getName() : inElementName)); 
}

// -----------------------------------

void
missionx::mx_base_node::setStringProperty(const std::string& inAttribName, const std::string& attribValue, const bool in_flagStoreInNode)
{
  if (!inAttribName.empty ())
    this->setNodeStringProperty(inAttribName, attribValue, in_flagStoreInNode);
}

// -----------------------------------

void
missionx::mx_base_node::setNodeStringProperty(const std::string& inAttribName,  const std::string& attribValue, const bool in_flagStoreInNode)
{
  if (!node.isEmpty() && !inAttribName.empty ())
  {
    if (in_flagStoreInNode)
      node.updateAttribute(attribValue.c_str(), inAttribName.c_str(), inAttribName.c_str());

    if (!inAttribName.empty())
      this->mapText[inAttribName] = attribValue;
  }
}

// -----------------------------------

std::string
missionx::mx_base_node::getNodeStringProperty(const std::string& inAttribName, const std::string& attribDefaultValue, const bool in_getTextFromContainerAndThenFromNode)
{
  if (in_getTextFromContainerAndThenFromNode && mxUtils::isElementExists(this->mapText, inAttribName))
    return this->mapText[inAttribName];
  else if (!this->node.isEmpty())
    return Utils::readAttrib(this->node, inAttribName, attribDefaultValue);
  
  return attribDefaultValue;
}

bool
missionx::mx_base_node::getBoolValue(const std::string& inAttribName, const bool attribDefaultValue)
{
  return Utils::readBoolAttrib(this->node, inAttribName, attribDefaultValue);
}

// -----------------------------------

void
missionx::mx_base_node::setNodeStringProperty(const std::string& inAttribName, const std::string& attribValue, IXMLNode& inNode_ptr)
{
  if (!inNode_ptr.isEmpty() && !inAttribName.empty ())
    inNode_ptr.updateAttribute(attribValue.c_str(), inAttribName.c_str(), inAttribName.c_str());
}

// -----------------------------------

void
missionx::mx_base_node::setNodeStringProperty_drillDown(const std::string& inAttribName, const std::string& attribValue, IXMLNode& inNode_ptr, const std::string& inTagNameToUpdate)
{
  if (!inNode_ptr.isEmpty() && !inAttribName.empty ())
    Utils::xml_search_and_set_attribute_in_IXMLNode(inNode_ptr, inAttribName, attribValue, inTagNameToUpdate);
}

// -----------------------------------

void
missionx::mx_base_node::setNumberProperty(const std::string& inAttribName, const std::string& attribValue)
{
  if (mxUtils::is_number(attribValue) && !inAttribName.empty ())
    this->setStringProperty(inAttribName, attribValue);
  else
    Log::logMsg("NaN: Attribute: " + inAttribName + " has value: " + attribValue + " which is not identified as a number.", true);
}

// -----------------------------------

void
missionx::mx_base_node::setBoolProperty(const std::string& inAttribName, const std::string& attribValue)
{
  bool bValueOutcome = false;
  if (mxUtils::isStringBool(attribValue, bValueOutcome) && !inAttribName.empty ())
  {
    this->setStringProperty(inAttribName, attribValue);
  }
  else
    Log::logMsgErr("Attribute: " + inAttribName + ", has non bool value: " + attribValue, true);
}

// -----------------------------------

void
missionx::mx_base_node::setBoolProperty(const std::string& inAttribName, const bool& attribValue)
{
  if (!inAttribName.empty ())
    this->setNodeProperty<bool>(inAttribName, attribValue);
}


// -----------------------------------
// -----------------------------------
// -----------------------------------

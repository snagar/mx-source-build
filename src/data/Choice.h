#ifndef CHOICE_H_
#define CHOICE_H_

#include "../../src/core/Utils.h"

namespace missionx
{

class Choice
{
private:
public:
  Choice();
//  ~Choice();

  class option_data
  {
  public:
    int         key_i;
    bool        flag_hidden; // default is false
    std::string text;
    IXMLNode    xOption_ptr;

    option_data()
    {
      key_i         = -1;
      flag_hidden = false;
      text.clear();
    }

    option_data(IXMLNode& inNode, int inKey)
    {
      this->key_i         = inKey;
      this->text        = Utils::readAttrib(inNode, mxconst::get_ATTRIB_TEXT(), "");
      this->flag_hidden = Utils::readBoolAttrib(inNode, mxconst::get_ATTRIB_HIDE(), false);
      xOption_ptr       = inNode;
    }
  };

  IXMLNode              xChoice_ptr;
  std::vector<IXMLNode> vecXmlOptions;
  int                   nVecSize_i;
  std::string           currentChoiceName_beingDisplayed_s; // holds current displayed choice
  std::string           prepareNewChoiceName_s;             // holds the new choice name to prepare and display
  std::string           currentChoiceTitle_s;
  std::string           last_choice_name_s;

  int optionPicked_key_i; // holds the picked options. We will use it during "mission::preFLC()"

  std::map<int, option_data> mapOptions; // key=sequence number, value=option data display information

  void init();

  // parseChoiceNode() is being called from missionx_data::prepare_choice_options(string)
  bool parseChoiseNode(IXMLNode& inNode);

  // We want to read this once and use it during draw iteration.
  // Each option change, we should execute the "prepare" function
  void prepareDisplayedOptions();

  // is Choice class is set with XML node and its vectors are ready ?
  bool is_choice_set();

  IXMLNode& get_picked_option_node_ptr()
  {
    if (Utils::isElementExists(this->mapOptions, this->optionPicked_key_i))
      return this->mapOptions[this->optionPicked_key_i].xOption_ptr;

    return IXMLNode::emptyIXMLNode;
  }
  void reset_option_picked_key() { this->optionPicked_key_i = -1; }

private:
};

}


#endif

#include "Choice.h"

missionx::Choice::Choice()
{
  last_choice_name_s.clear();
  this->init();
}


//missionx::Choice::~Choice() {}

// -----------------------------------



void
missionx::Choice::init()
{
  this->xChoice_ptr = IXMLNode::emptyNode();
  this->currentChoiceName_beingDisplayed_s.clear();
  this->prepareNewChoiceName_s.clear();
  this->currentChoiceTitle_s.clear();
  this->mapOptions.clear();
  this->vecXmlOptions.clear();
  this->nVecSize_i         = static_cast<int> (this->vecXmlOptions.size ());
  this->optionPicked_key_i = -1;
}

// -----------------------------------



bool
missionx::Choice::parseChoiseNode(IXMLNode& inNode)
{

  if (!inNode.isEmpty())
  {
    this->xChoice_ptr                        = inNode; // pointer to the choice element to display
    this->currentChoiceName_beingDisplayed_s = Utils::readAttrib(inNode, mxconst::get_ATTRIB_NAME(), "");
    this->currentChoiceTitle_s               = Utils::readAttrib(inNode, mxconst::get_ATTRIB_TEXT(), "");

    // read all options
    const int nOptions = inNode.nChildNode(mxconst::get_ELEMENT_OPTION().c_str());
    this->vecXmlOptions.clear();
    this->mapOptions.clear();

    // fill vector
    for (int i1 = 0; i1 < nOptions; ++i1)
      this->vecXmlOptions.push_back(inNode.getChildNode(mxconst::get_ELEMENT_OPTION().c_str(), i1));

    this->prepareDisplayedOptions();

    this->nVecSize_i = (int)this->vecXmlOptions.size();

    if (nOptions)
      this->last_choice_name_s = this->currentChoiceName_beingDisplayed_s; // store last choice only if it has options

    return true;
  }


  return false;
}

// -----------------------------------


void
missionx::Choice::prepareDisplayedOptions()
{
  this->mapOptions.clear();
  int counter = 0;
  for (auto& node : this->vecXmlOptions)
  {
    option_data data(node, counter);
    Utils::addElementToMap(this->mapOptions, counter, data);

    ++counter;
  }
}

// -----------------------------------


bool
missionx::Choice::is_choice_set()
{
  return (!this->xChoice_ptr.isEmpty() && !this->currentChoiceName_beingDisplayed_s.empty()); // we do not test if options exists, since the lack of options is possible
}

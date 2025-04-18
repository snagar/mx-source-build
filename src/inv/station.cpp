#include "station.h"

#include "../core/data_manager.h"
#include "Inventory.h"

namespace missionx
{
station::station()
{
  init();
}


station::station(const int& inIndex, const std::string& inName)
  : station()
{

  station_indx = inIndex;
  this->name   = inName;
}


bool
station::parse_node (const IXMLNode & inNode)
{
  if (!this->node.isEmpty())
    this->node.deleteNodeContent();

  this->node = inNode;

  return parse_node();
}


bool
station::parse_node()
{
  // mainly extract "station_name" and "station_id"
  if (this->node.isEmpty())
    return false;

  #ifndef RELEASE
  Utils::xml_print_node (this->node);
  #endif

  this->name               = this->getStringAttributeValue(mxconst::get_ATTRIB_NAME(), "");
  this->station_indx       = this->getAttribNumericValue<int>(mxconst::get_ATTRIB_ID(), -1); // -1 is invalid
  this->max_allowed_weight = this->getAttribNumericValue<float>(mxconst::get_ATTRIB_WEIGHT_KG(), 0.0f);

  // if (station_indx < 0 || this->name.empty())
  if (station_indx < 0
    || ( this->name.empty ()
        && missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i != missionx::XP11_COMPATIBILITY
        && !(mxUtils::mx_between <missionx::mx_mission_state_enum> (missionx::data_manager::missionState, missionx::mx_mission_state_enum::mission_is_being_loaded_from_the_original_file, missionx::mx_mission_state_enum::mission_is_running ) )
       )
     )
  {
    this->init();
    return false;
  }

  this->total_weight_in_station_f = this->calc_total_weight();
  this->total_items_in_station_i  = this->calc_total_item_groups_in_station();

  return true;
}


void
station::set_name(const std::string& inName)
{
  this->setName(inName); // mx_base_node
  this->setStringProperty(mxconst::get_ATTRIB_NAME(), inName, true);
}


mx_return
station::add_item(IXMLNode & inSourceItemNodePtr, const int& inQuantity)
{
  mx_return retInfo(false); // init the return class

  if (inSourceItemNodePtr.isEmpty())
  {
    retInfo.addErrMsg("Item Node is empty.");
    return retInfo;
  }

  const auto item_barcode = Utils::readAttrib(inSourceItemNodePtr, mxconst::get_ATTRIB_BARCODE(), "");
  if (item_barcode.empty())
  {    
    const int iMsgIndex = retInfo.addErrMsg("Item is without a name, will skip its move to station.\n" + Utils::xml_get_node_content_as_text(inSourceItemNodePtr));
    missionx::Log::logMsg(retInfo.errMsges[iMsgIndex]);
    return retInfo;
  }

  const auto sourceQuantity = Utils::readNodeNumericAttrib<int>(inSourceItemNodePtr, mxconst::get_ATTRIB_QUANTITY(), 0);
  const auto bIsMandatory = Utils::readBoolAttrib(inSourceItemNodePtr, mxconst::get_ATTRIB_MANDATORY(), false);

  // this->node represent a pointer to a <station> root node. We add the item as a sub node to the <station> node.
  if (auto childNode_ptr = Utils::xml_get_node_pointer_from_node_tree_by_attrib_name_and_value_IXMLNode(this->node, mxconst::get_ELEMENT_ITEM(), mxconst::get_ATTRIB_BARCODE(), item_barcode)
      ;childNode_ptr.isEmpty())
  {
    auto newItemNodePtr = inSourceItemNodePtr.deepCopy();
    newItemNodePtr.updateAttribute(mxUtils::formatNumber<int>(inQuantity).c_str(), mxconst::get_ATTRIB_QUANTITY().c_str(), mxconst::get_ATTRIB_QUANTITY().c_str());
    this->node.addChild(newItemNodePtr);
  }
  else
  {
    const auto quantity = Utils::readNodeNumericAttrib<int>(childNode_ptr, mxconst::get_ATTRIB_QUANTITY(), 0);
    const int target_quantity = (quantity + inQuantity >= 0)? quantity + inQuantity : 0 ; // debug
    childNode_ptr.updateAttribute(mxUtils::formatNumber<int>( target_quantity ).c_str(), mxconst::get_ATTRIB_QUANTITY().c_str(), mxconst::get_ATTRIB_QUANTITY().c_str());
    // add mandatory info
    if (bIsMandatory)
    {
      const std::string sSourceAllowedInventories = Utils::readAttrib(inSourceItemNodePtr, mxconst::get_ATTRIB_TARGET_INVENTORY(), "");
      const std::string sAllowedInventories = Utils::readAttrib(childNode_ptr, mxconst::get_ATTRIB_TARGET_INVENTORY(), sSourceAllowedInventories);

      childNode_ptr.updateAttribute(sAllowedInventories.c_str(), mxconst::get_ATTRIB_TARGET_INVENTORY().c_str(), mxconst::get_ATTRIB_TARGET_INVENTORY().c_str());
      childNode_ptr.updateAttribute(mxUtils::formatNumber<bool>(bIsMandatory).c_str(), mxconst::get_ATTRIB_MANDATORY().c_str(), mxconst::get_ATTRIB_MANDATORY().c_str());
    }
  }

  // Update the source item quantity.
  const int new_source_quantity_i = (sourceQuantity - inQuantity >= 0)? sourceQuantity - inQuantity : 0 ;
  inSourceItemNodePtr.updateAttribute(fmt::format("{}", (new_source_quantity_i)).c_str(), mxconst::get_ATTRIB_QUANTITY().c_str(), mxconst::get_ATTRIB_QUANTITY().c_str());

  this->calc_total_item_groups_in_station();
  this->calc_total_weight();
  return (retInfo.result = true);
}


float
station::calc_total_weight()
{
  this->total_weight_in_station_f = 0.0f;
  const auto iNodes = this->node.nChildNode(mxconst::get_ELEMENT_ITEM().c_str());
  for (int iLoop = 0; iLoop < iNodes; ++iLoop)
  {
    auto itemNode = this->node.getChildNode(mxconst::get_ELEMENT_ITEM().c_str(), iLoop);
    this->total_weight_in_station_f += (Utils::readNodeNumericAttrib<int>(itemNode, mxconst::get_ATTRIB_QUANTITY(), 0) * Utils::readNodeNumericAttrib<float>(itemNode, mxconst::get_ATTRIB_WEIGHT_KG(), 0.0f) );
  }
  return this->total_weight_in_station_f;
}

int
station::calc_total_item_groups_in_station()
{
  return this->total_items_in_station_i = this->node.nChildNode(mxconst::get_ELEMENT_ITEM().c_str());
}



} // end namespace missionx
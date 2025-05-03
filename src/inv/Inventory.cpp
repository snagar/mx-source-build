/*
 * Inventory.cpp
 *
 *  Created on: Nov 12, 2018
 *      Author: snagar
 */

#include "Inventory.h"

#include <fmt/core.h>
#include <filesystem>
namespace fs = std::filesystem;

#include "../core/data_manager.h"

namespace missionx
{

int  missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i {0}; // v24.12.2 "0" means no special compatibility, use the "station" layout

// Inventory::~Inventory()
//{
//  // TODO Auto-generated destructor stub
//}


Inventory::Inventory()
  : Trigger()
{
  init();
}


Inventory::Inventory(const std::string &inTagName, const std::string& inName) : Inventory()
{
  mx_base_node::initBaseNode();
  this->node.updateName(inTagName.c_str());

  this->setName(inName);
  this->setStringProperty(mxconst::get_ATTRIB_NAME(), inName);
}


Inventory::Inventory(const ITCXMLNode& inNode)
{
  this->node = inNode.deepCopy();
  init();
}


Inventory::Inventory(const missionx::Inventory& inInventory)  : Trigger(inInventory) {
  this->clone(inInventory);
}


void
Inventory::clone(const missionx::Inventory& inInventory)
{
  this->reset_inventory_content();
  this->mapStations = inInventory.mapStations;
  this->map_acf_station_names = inInventory.map_acf_station_names;

  // #ifndef RELEASE
  // Utils::xml_print_node (inInventory.node);
  // #endif

  this->parse_node(inInventory.node);
}


void
Inventory::read_xp11_style_inventory_items()
{
  const auto nChild = this->node.nChildNode(mxconst::get_ELEMENT_ITEM().c_str());
  Log::logMsg("[read inventory: " + this->name + " items]"); // v3.305.3 deprecated

  // this->mapItems.clear(); // we rebuild the item information
  int itemCounter = 0;
  for (int i1 = 0; i1 < nChild; ++i1)
  {
    if (auto xItem = this->node.getChildNode(mxconst::get_ELEMENT_ITEM().c_str(), itemCounter); !xItem.isEmpty())
    {
      std::string itemName = Utils::readAttrib(xItem, mxconst::get_ATTRIB_NAME(), "");

      const std::string barcode = Utils::readAttrib(xItem, mxconst::get_ATTRIB_BARCODE(), "");
      if ( barcode.empty() )
      {
        Log::logMsgErr("[parse item in Inventory] Found item with no Barcode. please fix: \n" + Utils::xml_get_node_content_as_text(xItem) );
        continue;
      }

      if (itemName.empty())
      {
        itemName = barcode;
        this->setNodeStringProperty(mxconst::get_ATTRIB_NAME(), itemName, xItem);
      }

      if (missionx::Inventory::parse_item_node(xItem)) // if item is valid
      {
        ++itemCounter;
      }
      else // remove item from XML element
      {
        xItem.deleteNodeContent(); // remove from <inventory>
      }

    } // end (!xItem.isEmpty())
  } // end loop over all Item elements in Inventory

}


bool
Inventory::parse_node(const IXMLNode& inNode)
{
  this->node = inNode.deepCopy();
  return this->parse_node();
}


bool
Inventory::parse_item_node(IXMLNode& inNode)
{
  assert(!inNode.isEmpty()); // abort if node was not set

  if (inNode.isEmpty())
    return false;

  const std::string barcode_s = Utils::readAttrib(inNode, mxconst::get_ATTRIB_BARCODE(), "");
  if (barcode_s.empty())
  {
    Log::logMsgErr("[parse item] Found item with no Barcode. please fix");
    return false;
  }

  std::string name_s      = Utils::readAttrib(inNode, mxconst::get_ATTRIB_NAME(), barcode_s); // v24.12.2 replaced default value with barcode text.
  const int   quantity_i  = static_cast<int>(Utils::readNumericAttrib(inNode, mxconst::get_ATTRIB_QUANTITY(), 0.0));
  const float weight_kg_f = static_cast<float>(Utils::readNumericAttrib(inNode, mxconst::get_ATTRIB_WEIGHT_KG(), 0.0));

  name_s = (name_s.empty()) ? barcode_s : name_s;

  Utils::xml_set_attribute_in_node_asString (inNode, mxconst::get_ATTRIB_NAME(), name_s);
  Utils::xml_set_attribute_in_node_asString (inNode, mxconst::get_ATTRIB_BARCODE(), barcode_s);
  Utils::xml_set_attribute_in_node <int>(inNode, mxconst::get_ATTRIB_QUANTITY(), quantity_i);
  Utils::xml_set_attribute_in_node <float>(inNode, mxconst::get_ATTRIB_WEIGHT_KG(), weight_kg_f);

  return true;
}



bool
Inventory::parse_node()
{
  assert(!this->node.isEmpty()); // fail if Point node was not initialized

  bool         isInventoryValid = true;
  std::string  tag              = this->node.getName();
  this->isPlane                 = (tag == mxconst::get_ELEMENT_PLANE()); // v24.12.2

  const std::string invName = (tag == mxconst::get_ELEMENT_PLANE()) ? mxconst::get_ELEMENT_PLANE() : Utils::readAttrib(this->node, mxconst::get_ATTRIB_NAME(), ""); // this should solve the <plane> inventory special tag name while all else are <inventory>
  std::string       type    = mxUtils::stringToLower(Utils::readAttrib(this->node, mxconst::get_ATTRIB_TYPE(), ""));                                                 // type in lower case

  // read inventory sub elements that are not shared with plane inventory element
  if (!isPlane)
  {
    int nChild = 0;
    if (invName.empty() + type.empty())
    {
      Log::logMsgErr("Found Inventory: [" + invName + "] element without name or type. Skipping definitions. Please fix or remove element from file !!!");
      return false;
    }
    // check Inventory type is valid
    if ((mxconst::get_TRIG_TYPE_RAD() != type) && (mxconst::get_TRIG_TYPE_POLY() != type))
    {
      Log::logMsgErr("Inventory: \"" + invName + "\", Has a none valid type: \"" + type + "\". Can be [rad] or [poly]. Fix Inventory settings. skipping...");
      return false;
    }


    // READ location information
    auto xLocAndElev = this->node.getChildNode(mxconst::get_ELEMENT_LOC_AND_ELEV_DATA().c_str());
    if (xLocAndElev.isEmpty())
    {
      Log::logMsgErr("Inventory \"" + invName + "\", has no location information. Please fix and try again. skipping Inventory...");
      return false;
    }

    // read area elements based on type
    // rad elements
    bool isRadiusType = false;
    if (mxconst::get_TRIG_TYPE_RAD() == type)
    {
      isRadiusType = true;
      // read radius + 1 point
      this->xRadius = xLocAndElev.getChildNode(mxconst::get_ELEMENT_RADIUS().c_str(), 0); // pick only 1 radius;
      if (this->xRadius.isEmpty())
      {
        Log::logMsgErr("Inventory \"" + invName + "\", has missing 'radius' setting value to 50m.");
        xRadius = xLocAndElev.addChild(mxconst::get_ELEMENT_RADIUS().c_str());
      }

      this->setNodeProperty<double>(this->xRadius, mxconst::get_ATTRIB_LENGTH_MT(), Utils::readNumericAttrib(this->xRadius, mxconst::get_ATTRIB_LENGTH_MT(), 50.0)); // update property and xml attribute

    } // end radius handling

    // Read Points
    nChild = xLocAndElev.nChildNode(mxconst::get_ELEMENT_POINT().c_str());
    if (nChild > 0)
    {
      if (isRadiusType)
        nChild = 1; // make sure we will only read 1 point for radius and ignore the rest

      for (int i1 = 0; i1 < nChild; ++i1)
      {
        auto  xPoint = xLocAndElev.getChildNode(mxconst::get_ELEMENT_POINT().c_str(), i1);
        Point p(xPoint.deepCopy());
        if (p.parse_node())
          this->addPoint(p); // valid Point. Add to Inventory.
        else
        {
          Log::logMsgErr("Inventory \"" + invName + "\", has invalid 'point' information. Skipping inventory... !!!");
          isInventoryValid = false;
        }
      } // end loop over points
    }
    else // no points found
    {
      Log::logMsgErr("Inventory \"" + invName + "\", has missing 'point' location information. Skipping inventory...");
      isInventoryValid = false;
    }

    this->calcCenterOfArea();

  } // end handle inventory that is not plane


  if (isInventoryValid)
  {
    // Store inventory name
    if (this->isPlane)
    {
      this->setNodeStringProperty(mxconst::get_ATTRIB_NAME(), mxconst::get_ELEMENT_PLANE());
      this->name = mxconst::get_ELEMENT_PLANE(); // v3.303.11
    }
    else
      this->name = invName;


    this->setNodeStringProperty(mxconst::get_ATTRIB_TYPE(), type); // rad | poly

  } // Finish items, and end if inventory is valid after reading special inventory elements.



  if (isInventoryValid)
  {
    // Read all items in inventory (if any), based on Inventory Layout and type
    if ( (missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i == missionx::XP11_COMPATIBILITY) + (this->name != mxconst::get_ELEMENT_PLANE()) )
    {
      Utils::xml_delete_all_subnodes(this->node, mxconst::get_ELEMENT_STATION(), false); // delete all stations
      this->read_xp11_style_inventory_items(); // Reads and store all items under <inventory>.
    }
    else if (this->isPlane ) // v24.12.2 parse stations and initialize station containers
    {
      Utils::xml_delete_all_subnodes(this->node, mxconst::get_ELEMENT_ITEM(), false); // delete <item>s

      const auto nStationNodes = this->node.nChildNode(mxconst::get_ELEMENT_STATION().c_str());
      for (int iStationLoop = 0; iStationLoop < nStationNodes; ++iStationLoop)
      {
        missionx::station acf_station; // = lmbda_get_station(iStationLoop); // return existing station or a new one.
        acf_station.node = this->node.getChildNode(mxconst::get_ELEMENT_STATION().c_str(), iStationLoop);

        if (acf_station.parse_node())
        {
          // check if station index is valid.
          if (mxUtils::isElementExists(this->mapStations, acf_station.station_indx ))
          {
            // Force station name to be same as we read from the "acf" file, even if it is different in the mission file.
            if (auto cargo_station = this->mapStations[acf_station.station_indx]; !cargo_station.getName().empty())
              acf_station.set_name(cargo_station.getName());
            else if (cargo_station.getName().empty() && acf_station.getName().empty())
              acf_station.set_name(fmt::format("{}", cargo_station.station_indx)); // use mission file station name
          }

          this->map_acf_station_names[acf_station.station_indx] = acf_station.getName(); // used with UI for easier access. Consider to use only "mapStations".
          this->mapStations[acf_station.station_indx]           = acf_station;
        }
      }
    }
    // end v24.12.2

    this->applyPropertiesToLocal();
  }
  else
  {
    Log::logMsg("inventory is NOT VALID. Please check: \n" + std::string(Utils::xml_get_node_content_as_text ( this->node) ) );
  }

  // v24.12.2 Search for mandatory items and validate it.
  if (auto xResult = gather_and_validate_mandatory_items(this->node, 0); !xResult.result)
    Log::log_to_missionx_log( xResult.getErrorsAsText() );

  return true;
}


void
Inventory::init ()
{
  this->name.clear ();
  this->node.updateName (mxconst::get_ELEMENT_INVENTORY ().c_str ()); // v24.12.2
  setStringProperty (mxconst::get_ATTRIB_PLANE_ON_GROUND (), mxconst::get_MX_YES ()); // v3.0.217.7


  this->getCueInfo_ptr ().cueType = missionx::mx_cue_types::cue_inventory; // v3.0.213.7 try to fix load checkpoint lack of information

  // this->mapItems.clear(); // v24.12.2
  this->mapStations.clear (); // v24.12.2
  this->map_acf_station_names.clear (); // v24.12.2
}

// -----------------------------------

void
Inventory::init_plane_inventory ()
{
  this->reset_inventory_content();
  this->node.updateName (mxconst::get_ELEMENT_PLANE().c_str ());
  this->name = mxconst::get_ELEMENT_PLANE();
}


// -----------------------------------


bool
Inventory::isItemExistsInInventory(const std::string& inBarcodeName)
{
  return this->get_item_exists(this->node, inBarcodeName);
}


// -----------------------------------


int
Inventory::get_item_quantity(const std::string& inBarcodeName)
{
  if ((missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i == missionx::XP11_COMPATIBILITY) + (this->name != mxconst::get_ELEMENT_PLANE()))
  { // get item quantity from external inventory or from inventory with xp11 layout set
    if (const auto node_ptr = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(this->node, mxconst::get_ELEMENT_ITEM(), mxconst::get_ATTRIB_BARCODE(), inBarcodeName, false); !node_ptr.isEmpty())
      return Utils::readNodeNumericAttrib<int>(node_ptr, mxconst::get_ATTRIB_QUANTITY(), 0);
  }
  else
  { // search items in <station>s and add their quantities
    int quantity = 0;
    for (auto& acf_station : this->mapStations | std::views::values)
    {
      if (const auto node_ptr = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode(acf_station.node, mxconst::get_ELEMENT_ITEM(), mxconst::get_ATTRIB_BARCODE(), inBarcodeName, false); !node_ptr.isEmpty())
        quantity += Utils::readNodeNumericAttrib<int>(node_ptr, mxconst::get_ATTRIB_QUANTITY(), 0);
    } // end loop over all stations
    return quantity;
  } // end else if "inventory layout" is XP11 compatible

  return -1; // -1 = does not exist.
}


// -----------------------------------


bool
Inventory::get_item_exists(const IXMLNode& pNode, const std::string& inBarcodeName, const int& nLevel)
{
  bool      bFound  = false;
  const int nChilds = pNode.nChildNode();

  for (int iLoop = 0; (iLoop < nChilds) * (!bFound); ++iLoop)
  {
    const auto cNode = pNode.getChildNode (iLoop);
    if (const std::string node_name = cNode.getName ()
      ; (missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i != missionx::XP11_COMPATIBILITY) * (node_name == mxconst::get_ELEMENT_STATION()) * (nLevel == 0) )
      bFound = this->get_item_exists(cNode, inBarcodeName, (nLevel + 1));
    else if (node_name == mxconst::get_ELEMENT_ITEM())
      bFound = (Utils::readNodeNumericAttrib<int>(cNode, mxconst::get_ATTRIB_QUANTITY(), 0) > 0);
  }

  return bFound;
}


// -----------------------------------


IXMLNode
Inventory::get_item_node_ptr(const std::string& barcode, const int &inStation_id)
{
  // check if barcode exists
  if ( this->get_item_exists(this->node, barcode) )
  {
    // check if we are testing PLANE inventory, and we are in XP12 compatible mode (<station>)
    if ((opt_forceInventoryLayoutBasedOnVersion_i != missionx::XP11_COMPATIBILITY) * (this->name == mxconst::get_ELEMENT_PLANE()))
    {
      // check if station exists
      if (mxUtils::isElementExists(this->mapStations, inStation_id))
      {
        return Utils::xml_get_node_pointer_from_node_tree_by_attrib_name_and_value_IXMLNode(this->mapStations[inStation_id].node, mxconst::get_ELEMENT_ITEM(), mxconst::get_ATTRIB_BARCODE(), barcode, false);
      }// end is station is valid
    }

    // retrieve the first item node with same barcode
    return Utils::xml_get_node_pointer_from_node_tree_by_attrib_name_and_value_IXMLNode(this->node, mxconst::get_ELEMENT_ITEM(), mxconst::get_ATTRIB_BARCODE(), barcode, false);

  } // end if there is any item with "barcode" in this inventory

  return IXMLNode::emptyIXMLNode;
}


// -----------------------------------


bool
Inventory::add_an_item_to_a_station(IXMLNode & inout_sourceItemNode, const int inStationId, const int inQuantity)
{
  // Lambda
  const auto lmbda_get_station = [&](const int &in_station_id)
  {
    if (mxUtils::isElementExists(this->mapStations, in_station_id))
    {
      return in_station_id;
    }

    if (!this->mapStations.empty())
    {
      return this->mapStations.at(0).station_indx;
    }
    return -1; // no valid station found. Something might be wrong.
  };

  // Search item barcode in station
  if (const auto station_id = lmbda_get_station(inStationId)
      ; station_id > -1 )
  {
    return (this->mapStations[station_id].add_item(inout_sourceItemNode, inQuantity)).result;
  }

  Log::log_to_missionx_log(fmt::format(R"(Received Station Index: "{}" might be invalid for current Inventory: "{}", check your code.)", inStationId, this->name));
  return false;
}


// -----------------------------------


mx_return
Inventory::add_item(IXMLNode& inSourceItemNodePtr, const int& inQuantity, const int &inStation)
{
    mx_return retInfo(false); // init the return class

  if (inSourceItemNodePtr.isEmpty())
  {
    retInfo.addErrMsg("Item Node is empty.");
    return retInfo;
  }

  // get barcode and do validation
  const auto item_barcode = Utils::readAttrib(inSourceItemNodePtr, mxconst::get_ATTRIB_BARCODE(), "");
  if (item_barcode.empty())
  {
    const int iMsgIndex = retInfo.addErrMsg("Item is without a name, will skip its move to station.\n" + Utils::xml_get_node_content_as_text(inSourceItemNodePtr));
    missionx::Log::logMsg(retInfo.errMsges[iMsgIndex]);
    return retInfo;
  }


  if ((missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i == missionx::XP11_COMPATIBILITY) + (this->name != mxconst::get_ELEMENT_PLANE()) )
  {
    const auto sourceQuantity = std::abs( Utils::readNodeNumericAttrib<int>(inSourceItemNodePtr, mxconst::get_ATTRIB_QUANTITY(), 0) );

    if (sourceQuantity < std::abs(inQuantity))
    {
      retInfo.addErrMsg(fmt::format("Source item quantity: '{}' is lower than the requested one: '{}'.", sourceQuantity, inQuantity));
      return (retInfo.result = false);
    }

    // get target node pointer
    auto target_item_ptr = Utils::xml_get_node_pointer_from_node_tree_by_attrib_name_and_value_IXMLNode(this->node, mxconst::get_ELEMENT_ITEM(), mxconst::get_ATTRIB_BARCODE(), item_barcode);
    if (target_item_ptr.isEmpty())
    {
      // Add the item based on source
      target_item_ptr = this->node.addChild(inSourceItemNodePtr.deepCopy());
      assert (!target_item_ptr.isEmpty() && fmt::format("[{}] Target Item node failed creation. Notify developer.", __func__ ).c_str()  );
      Utils::xml_set_attribute_in_node<int>(target_item_ptr, mxconst::get_ATTRIB_QUANTITY(), 0);
    }
    // read target quantity
    const auto targetQuantity = std::abs( Utils::readNodeNumericAttrib<int>(target_item_ptr, mxconst::get_ATTRIB_QUANTITY(), 0) );
    // set "source quantity"
    Utils::xml_set_attribute_in_node<int>(inSourceItemNodePtr, mxconst::get_ATTRIB_QUANTITY(), sourceQuantity - inQuantity );

    // set and return target quantity
    return (retInfo.result = Utils::xml_set_attribute_in_node<int>(target_item_ptr, mxconst::get_ATTRIB_QUANTITY(), targetQuantity + inQuantity) );
  } // end XP11 Compatibility

  // Call XP12 logic
  return (retInfo.result = this->add_an_item_to_a_station(inSourceItemNodePtr, inStation, inQuantity) );

}


// -----------------------------------


void
Inventory::discardItem(const std::string& inBarcode, int inQuantity, const int& inStationId)
{
  missionx::mx_base_node itemNodePtr;
  itemNodePtr.node = this->get_item_node_ptr(inBarcode, inStationId);

  if (! itemNodePtr.node.isEmpty() )
  {
    inQuantity              = (int)std::abs(inQuantity);
    const int node_quantity = Utils::readNodeNumericAttrib<int>(itemNodePtr.node, mxconst::get_ATTRIB_QUANTITY(), 0);

    // if the calculation is less than 0 then reset to zero.
    itemNodePtr.setNumberProperty(mxconst::get_ATTRIB_QUANTITY(), ((node_quantity - inQuantity < 0) ? 0 : node_quantity - inQuantity) );
  }
}

// -----------------------------------

void
Inventory::applyPropertiesToLocal()
{
  assert(!this->node.isEmpty()); // fail to apply if node is not set

  this->cueType   = missionx::mx_cue_types::cue_inventory;
  this->isEnabled = true; // always

  this->trigElevType = missionx::mx_trigger_elev_type_enum::on_ground;
  this->setNodeProperty<bool>(mxconst::get_ATTRIB_ENABLED(), Utils::readBoolAttrib(this->node, mxconst::get_ATTRIB_ENABLED(), true));

  this->setNodeStringProperty(mxconst::get_ATTRIB_PLANE_ON_GROUND(), mxconst::get_MX_YES()); // this is deterministic. Inventory is always on ground or on surface we land on.
  this->setNodeProperty<int>(mxconst::get_PROP_TRIG_ELEV_TYPE(), (int)this->trigElevType); // v3.0.241.1 update both property and XML node if we want
}

// -----------------------------------

void
Inventory::storeCoreAttribAsProperties()
{
  this->setBoolProperty(mxconst::get_ATTRIB_ENABLED(), true);
  this->setStringProperty(mxconst::get_ATTRIB_PLANE_ON_GROUND(), mxconst::get_MX_YES()); // v3.0.217.7
}

// -----------------------------------

void
Inventory::reset_inventory_content()
{
  mx_base_node::initBaseNode();
  init();
}

// -----------------------------------

bool
Inventory::move_item_to_station(missionx::station& inout_station, IXMLNode inSourceItemNodePtr, IXMLNode inTargetInventoryPtr, const std::string& in_barcode, const int& inQuantity)
{
  int      targetQuantity_i   = 1;
  IXMLNode targetItemNode_ptr = Utils::xml_get_node_pointer_from_node_tree_by_attrib_name_and_value_IXMLNode(inTargetInventoryPtr, mxconst::get_ELEMENT_ITEM(), mxconst::get_ATTRIB_BARCODE(), in_barcode);

  // Calculate how many items in store after move
  if (targetItemNode_ptr.isEmpty())                                                     // if not available we need to create it
    targetItemNode_ptr = inTargetInventoryPtr.addChild(inSourceItemNodePtr.deepCopy()); // create item
  else
    targetQuantity_i = Utils::readNodeNumericAttrib<int>(targetItemNode_ptr, mxconst::get_ATTRIB_QUANTITY(), 0) + 1;

  // Calculate source move
  auto quantity_i = Utils::readNodeNumericAttrib<int>(inSourceItemNodePtr, mxconst::get_ATTRIB_QUANTITY(), 0);
  --quantity_i; // now we have one item less in plane

  //// update nodes
  Utils::xml_set_attribute_in_node<int>(inSourceItemNodePtr, mxconst::get_ATTRIB_QUANTITY(), quantity_i, mxconst::get_ELEMENT_ITEM());      // set plane item quantity
  Utils::xml_set_attribute_in_node<int>(targetItemNode_ptr, mxconst::get_ATTRIB_QUANTITY(), targetQuantity_i, mxconst::get_ELEMENT_ITEM()); // set external item inventory quantity

  inout_station.calc_total_weight();

  return true;
}

// -----------------------------------

std::vector<float>
Inventory::get_inventory_station_weights_as_vector(const int& in_max_array_size)
{
  std::vector<float> vecStationWeight(in_max_array_size);
  for (int index = 0; const auto& acf_station : this->mapStations | std::views::values)
  {
    if (index < in_max_array_size)
      vecStationWeight[index] = acf_station.total_weight_in_station_f;

    index++;
  }

  return vecStationWeight;
}


// -----------------------------------

std::string
Inventory::get_inventory_station_weights_as_string(const int& in_max_array_size, int& out_stations_array_size)
{
  std::stringstream ss;
  out_stations_array_size = 0;
  const std::string empty_s; // empty string
  const std::string comma_s = ",";

  std::ranges::for_each(this->mapStations,
                        [&](auto& station)
                        {
                          ss << (out_stations_array_size == 0 ? empty_s : comma_s) << station.second.total_weight_in_station_f;
                          ++out_stations_array_size;
                        });

  return ss.str();
}

// -----------------------------------

int
Inventory::get_stations_number() const
{
  return static_cast<int>( this->mapStations.size() );
}

// -----------------------------------

missionx::Inventory
Inventory::mergeAcfAndPlaneInventories (missionx::Inventory inTargetInventory, missionx::Inventory &inSourceInventory)
{
  #ifndef RELEASE
  Utils::xml_print_node (inTargetInventory.node);
  Utils::xml_print_node (inSourceInventory.node);
  #endif

  if (missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i == missionx::XP11_COMPATIBILITY)
    return inSourceInventory;

  // loop over ".acf" <station>s and add the plane items into those stations
  const auto nStations = inSourceInventory.node.nChildNode (mxconst::get_ELEMENT_STATION().c_str ());
  for (const auto &station : inSourceInventory.mapStations | std::views::values)
  {
    // check acf has same station id
    if (!mxUtils::isElementExists (inTargetInventory.mapStations, station.station_indx))
    {
      Log::log_to_missionx_log (fmt::format ("Station [{}], has an id: {} that is not in the 'acf' file. Will skip this station information.", station.name, station.station_indx));
      continue;
    }

    // loop over all station <items> and move them to the acf inventory.
    const auto nItems = station.node.nChildNode (mxconst::get_ELEMENT_ITEM().c_str ());
    for (int i = 0; i < nItems; ++i)
    {
      if (auto itemNode = station.node.getChildNode (mxconst::get_ELEMENT_ITEM().c_str (), i); !itemNode.isEmpty ())
      {
        // check if acf has item with same barcode
        const auto itemBarcode_s  = Utils::readAttrib (itemNode, mxconst::get_ATTRIB_BARCODE(), "");
        const auto itemQuantity_i = Utils::readNodeNumericAttrib<int> (itemNode, mxconst::get_ATTRIB_QUANTITY(), 0);

        // add if acf does not have <item>
        if (auto target_node_ptr = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode (inTargetInventory.mapStations[station.station_indx].node, mxconst::get_ELEMENT_ITEM(), mxconst::get_ATTRIB_BARCODE(), itemBarcode_s, false); target_node_ptr.isEmpty ())
        {
          inTargetInventory.mapStations[station.station_indx].node.addChild (itemNode.deepCopy ());
          #ifndef RELEASE
          Log::logMsg (">>>>>>>>>>>>>>>>");
          Utils::xml_print_node (inTargetInventory.node);
          Log::logMsg ("<><><><><><><>");
          Utils::xml_print_node (inTargetInventory.mapStations[station.station_indx].node);
          Log::logMsg ("<<<<<<<<<<<<<<<<");
          #endif
        }
        else
        { // add to an existing item
          const auto targetQuantity_i = Utils::readNodeNumericAttrib<int> (target_node_ptr, mxconst::get_ATTRIB_QUANTITY(), 0);
          target_node_ptr.updateAttribute (fmt::format ("{}", (targetQuantity_i + itemQuantity_i)).c_str (), mxconst::get_ATTRIB_QUANTITY().c_str (), mxconst::get_ATTRIB_QUANTITY().c_str ());
        }

      } // end check if plane <item> is valid
    } // end loop over all "plane"s items
  } // end loop over all "plane"s stations

  return inTargetInventory;
}

// -----------------------------------


IXMLNode
Inventory::mergeAcfAndPlaneInventories (IXMLNode inTargetInventory, const IXMLNode &inSourceInventory)
{
#ifndef RELEASE
  Utils::xml_print_node (inSourceInventory);
#endif

  if (missionx::Inventory::opt_forceInventoryLayoutBasedOnVersion_i == missionx::XP11_COMPATIBILITY)
    return inSourceInventory.deepCopy ();


  // loop over ".acf" <station>s and add the plane items into those stations
  const auto nTargetStations = inTargetInventory.nChildNode (mxconst::get_ELEMENT_STATION().c_str ());

  const auto nSourceStations = inSourceInventory.nChildNode (mxconst::get_ELEMENT_STATION().c_str ());
  for (int iStationLoop = 0; iStationLoop < nSourceStations; ++iStationLoop)
  {
    auto       source_station_node = inSourceInventory.getChildNode (mxconst::get_ELEMENT_STATION().c_str (), iStationLoop);
    const auto station_name        = Utils::readAttrib (source_station_node, mxconst::get_ATTRIB_NAME(), "");
    const auto station_id          = Utils::readAttrib (source_station_node, mxconst::get_ATTRIB_ID(), "");

    // check acf has same station id
    if (source_station_node.isEmpty () || source_station_node.nChildNode (mxconst::get_ELEMENT_ITEM().c_str ()) <= 0)
    {
      Log::log_to_missionx_log (fmt::format ("Station [{}], has an id: {} that is not in the 'acf' file. Will skip this station information.", station_name, station_id));
      continue;
    }

    // loop over all station <items> and move them to the acf inventory.
    const auto nItems = source_station_node.nChildNode (mxconst::get_ELEMENT_ITEM().c_str ());
    for (int i2 = 0; i2 < nItems; ++i2)
    {
      if (auto itemNode = source_station_node.getChildNode (mxconst::get_ELEMENT_ITEM().c_str (), i2); !itemNode.isEmpty ())
      {
        // check if acf has item with same barcode
        const auto itemBarcode_s  = Utils::readAttrib (itemNode, mxconst::get_ATTRIB_BARCODE(), "");
        const auto itemQuantity_i = Utils::readNodeNumericAttrib<int> (itemNode, mxconst::get_ATTRIB_QUANTITY(), 0);

        // add if acf does not have <item>
        if (auto target_item_node_ptr = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode (inTargetInventory, mxconst::get_ELEMENT_ITEM(), mxconst::get_ATTRIB_BARCODE(), itemBarcode_s, false); target_item_node_ptr.isEmpty ())
        {
          if (nTargetStations > 0)
          {
            IXMLNode target_station_node = IXMLNode::emptyIXMLNode;
            if (nTargetStations >= iStationLoop)
              target_station_node = inTargetInventory.getChildNode (mxconst::get_ELEMENT_STATION().c_str (), iStationLoop);
            else
              target_station_node = inTargetInventory.getChildNode (mxconst::get_ELEMENT_STATION().c_str ()); // pick 0

            target_station_node.addChild (itemNode.deepCopy ()); // add item to target station
          }
          else
            inTargetInventory.addChild (itemNode.deepCopy ()); // add to the root inventory node.
        }
        else
        { // add to an existing item
          const auto targetQuantity_i = Utils::readNodeNumericAttrib<int> (target_item_node_ptr, mxconst::get_ATTRIB_QUANTITY(), 0);
          target_item_node_ptr.updateAttribute (fmt::format ("{}", (targetQuantity_i + itemQuantity_i)).c_str (), mxconst::get_ATTRIB_QUANTITY().c_str (), mxconst::get_ATTRIB_QUANTITY().c_str ());
        }

      } // end check if plane <item> is valid
    } // end loop over all "plane"s items
  } // end loop over all "plane"s stations

#ifndef RELEASE
  Log::logMsg (">>>>>> Final >>>>>>");
  Utils::xml_print_node (inTargetInventory);
  Log::logMsg ("<<<<<<<<<<<<<<<<<<<");
#endif

  return inTargetInventory.deepCopy ();
}

// -----------------------------------

void
Inventory::parse_max_weight_line_and_station_name (const std::string &line, IXMLNode &pNode, const missionx::enums::mx_acf_line_type_enum in_line_type)
{
  std::istringstream       iss (line);

  const std::vector<std::string> vecTokens  = mxUtils::split_skipEmptyTokens (line);
  const auto               tokenCount = vecTokens.size ();

  const auto lmbda_get_station_from_pNode =[] (IXMLNode &parent_node, const std::string &in_station_id)
  {
    auto local_node_station_ptr = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode (parent_node, mxconst::get_ELEMENT_STATION(), mxconst::get_ATTRIB_ID(), in_station_id, false);
    if (local_node_station_ptr.isEmpty ())
    {
      local_node_station_ptr = parent_node.addChild (mxconst::get_ELEMENT_STATION().c_str ());
      local_node_station_ptr.updateAttribute (in_station_id.c_str (), mxconst::get_ATTRIB_ID().c_str (), mxconst::get_ATTRIB_ID().c_str ());
    }

    return local_node_station_ptr;
  };


  switch (in_line_type)
  {
    case missionx::enums::mx_acf_line_type_enum::max_weight_line:
    {
      const std::string &lastToken = vecTokens.back ();

      // Test number or scientific number
      float      v_third_token_num_value = 0.0; // weight kg
      const bool bIsNumberic             = mxUtils::isNumeric (lastToken);
      const bool bIsSceintific           = mxUtils::isScientific (lastToken); // in the LR airbus, some station had scientific weight values.

      if (!bIsNumberic && bIsSceintific)
        v_third_token_num_value = static_cast<float> (mxUtils::convertScientificToDecimal (lastToken));
      else
        v_third_token_num_value = mxUtils::stringToNumber<float> (lastToken);


      if ((bIsNumberic + bIsSceintific) && vecTokens.size () > 2)
      {
        if (const auto pos = vecTokens.at (1).find_last_of ('/'); pos != std::string::npos)
        {
          if (const std::string v_station_index_str = vecTokens.at (1).substr (pos + 1); mxUtils::is_number (v_station_index_str))
          {
            IXMLNode node_station_ptr = lmbda_get_station_from_pNode (pNode, v_station_index_str); //   pNode.addChild (mxconst::get_ELEMENT_STATION().c_str ());
            assert (!node_station_ptr.isEmpty () && fmt::format ("[{}] Failed to create <station> node for weight attribute.", __func__).c_str ());

            // Utils::xml_set_attribute_in_node_asString (node_station_ptr, mxconst::get_ATTRIB_ID(), v_station_index_str, mxconst::get_ELEMENT_STATION());
            Utils::xml_set_attribute_in_node<float> (node_station_ptr, mxconst::get_ATTRIB_WEIGHT_KG(), v_third_token_num_value, mxconst::get_ELEMENT_STATION());
          }
        }
      }
    } // end case max_weight_line
    break;
    case missionx::enums::mx_acf_line_type_enum::station_name_line:
    {
      if (vecTokens.size () > 2)
      {
        if (const auto pos = vecTokens.at (1).find_last_of ('/')
            ; pos != std::string::npos)
        {
          if (const std::string v_station_index_str = vecTokens.at (1).substr (pos + 1)
            ; mxUtils::is_number(v_station_index_str))
          {
            // int v_station_index = std::stoi(v_station_index_str);
            std::string v_station_name;
            for (auto i = static_cast<size_t>(2); i < tokenCount; ++i)
            {
              if (!v_station_name.empty())
                v_station_name += " ";
              v_station_name += vecTokens.at(i);
            }

            IXMLNode node_station_ptr = lmbda_get_station_from_pNode (pNode, v_station_index_str); //   pNode.addChild (mxconst::get_ELEMENT_STATION().c_str ());
            assert (!node_station_ptr.isEmpty () && fmt::format ("[{}] Failed to create <station> node for station name attribute.", __func__).c_str ());

            // Utils::xml_set_attribute_in_node_asString (node_station_ptr, mxconst::get_ATTRIB_ID(), v_station_index_str, mxconst::get_ELEMENT_STATION());
            Utils::xml_set_attribute_in_node_asString (node_station_ptr, mxconst::get_ATTRIB_NAME(), v_station_name, mxconst::get_ELEMENT_STATION());
          } // end is number
        } // end found "/"
      } // end vecToken > 2
    } // end station_name_line
    break;
    default:;
  } // end switch
}


// -----------------------------------


void
Inventory::gather_acf_cargo_data (Inventory &inout_current_plane_inventory, const bool in_plane_was_changed_b)
{
  auto startTimer = std::chrono::steady_clock::now ();

  std::array<std::string, 2> arr_text = { "P acf/_fixed_max", "P acf/_fixed_name" };

  char outFileName[512]{ 0 };
  char outPathAndFile[2048]{ 0 };
  XPLMGetNthAircraftModel (XPLM_USER_AIRCRAFT, outFileName, outPathAndFile); // we will only return the file name
  fs::path acf_file = outPathAndFile;
  std::ios_base::sync_with_stdio (false);
  std::cin.tie (nullptr);

  auto new_acf_inventory = Inventory (mxconst::get_ELEMENT_PLANE(), mxconst::get_ELEMENT_PLANE());

  #ifndef RELEASE
  Log::log_to_missionx_log (fmt::format ("Local inventory before reading [{}] file.\n==========>\n{}\n<=========", acf_file.string (), Utils::xml_get_node_content_as_text (new_acf_inventory.node)));
  #endif

  if (fs::exists (acf_file) && fs::is_regular_file (acf_file))
  {
    std::ifstream file_toRead;
    file_toRead.open (acf_file.string (), std::ios::in); // read the file
    if (file_toRead.is_open ())
    {
      int                  stations_i = 0;
      std::map<int, float> mapMaxWeights;
      std::string          line;
      char                 ch{ '\0' };
      int                  char_counter_i = 0;
      while (file_toRead.get (ch))
      {
        if ((char_counter_i > 17) + (ch == '\n'))
        {
          if (ch != '\n')
          {
            std::string restOfLine;
            std::getline (file_toRead, restOfLine);
            line.append (ch + restOfLine); // append to "line" the last character we read + the rest of line.
          }
          if (line.starts_with (arr_text[0])) // "P acf/_fixed_max"
            Inventory::parse_max_weight_line_and_station_name (line, new_acf_inventory.node, missionx::enums::mx_acf_line_type_enum::max_weight_line);
          else if (line.starts_with (arr_text[1])) // "P acf/_fixed_name"
            Inventory::parse_max_weight_line_and_station_name (line, new_acf_inventory.node, missionx::enums::mx_acf_line_type_enum::station_name_line);

          line.clear ();
          char_counter_i = 0; // reset counter
        }
        else
        {
          line += ch;
          ++char_counter_i;
        }
      }

      // remove nodes with weight <= 0
      auto nStations = new_acf_inventory.node.nChildNode(mxconst::get_ELEMENT_STATION().c_str ());
      for (int iLoop = nStations - 1; iLoop >= 0; --iLoop)
      {
        const auto station_name  = Utils::readAttrib( new_acf_inventory.node.getChildNode (mxconst::get_ELEMENT_STATION().c_str (), iLoop), mxconst::get_ATTRIB_NAME(), "");
        const auto weight_f = Utils::readNodeNumericAttrib <float>( new_acf_inventory.node.getChildNode (mxconst::get_ELEMENT_STATION().c_str (), iLoop), mxconst::get_ATTRIB_WEIGHT_KG(), 0.0f);
        if (weight_f <= 0.0f)
          new_acf_inventory.node.getChildNode (mxconst::get_ELEMENT_STATION().c_str (), iLoop).deleteNodeContent ();
      }

      #ifndef RELEASE
      Utils::xml_print_node (new_acf_inventory.node);
      #endif
      new_acf_inventory.parse_node();

    } // finish read acf file
  } // end evaluate file exists

  #ifndef RELEASE
  Log::log_to_missionx_log (fmt::format ("ACF Inventory Information:\n=================>\n [{}]", new_acf_inventory.get_node_as_text ())); // debug v24.12.2
  #endif // !RELEASE


  // Ignore inventory information if we loaded from checkpoint
  if (data_manager::missionState >= missionx::mx_mission_state_enum::mission_is_being_loaded_from_the_original_file && data_manager::missionState != missionx::mx_mission_state_enum::mission_loaded_from_savepoint)
  {
    // if (in_plane_was_changed_b || (data_manager::missionState >= missionx::mx_mission_state_enum::mission_is_being_loaded_from_the_original_file && data_manager::missionState <= missionx::mx_mission_state_enum::mission_loaded_from_the_original_file))
    if ( in_plane_was_changed_b || mxUtils::mx_between<missionx::mx_mission_state_enum>(data_manager::missionState, missionx::mx_mission_state_enum::mission_is_being_loaded_from_the_original_file, missionx::mx_mission_state_enum::mission_loaded_from_the_original_file) )
    {
      IXMLNode new_node = Inventory::mergeAcfAndPlaneInventories (new_acf_inventory.node, inout_current_plane_inventory.node);
      inout_current_plane_inventory.init_plane_inventory ();
      inout_current_plane_inventory.parse_node (new_node);
      Utils::xml_print_node (inout_current_plane_inventory.node);
    }
  }


  #ifndef RELEASE
  Log::log_to_missionx_log (fmt::format ("Final Inventory Information:\n=================>\n [{}]", inout_current_plane_inventory.get_node_as_text ())); // debug v24.12.2
  #endif // !RELEASE

  auto endTimer   = std::chrono::steady_clock::now ();
  auto diff_cache = endTimer - startTimer;
  auto duration   = std::chrono::duration<double, std::milli> (diff_cache).count ();
  Log::logMsg (">> parsed: " + acf_file.string () + ", Duration: " + Utils::formatNumber<double> (duration, 3) + "ms (" + Utils::formatNumber<double> ((duration * 0.001), 3) + "sec) ");
}

// -----------------------------------


inline missionx::mx_return
Inventory::gather_and_validate_mandatory_items( const IXMLNode &pNode, const int &inLevel )
{
  missionx::mx_return result(true);

  if (inLevel == 0)
  {
    this->map_mandatory_items.clear();
    this->map_mandatory_items_ptr.clear();
    this->map_non_mandatory_items.clear();
  }

  // crawl over all items and search for items with mandatory flag
  const int nChilds = pNode.nChildNode();
  for ( int iLoop = 0; iLoop < nChilds; ++iLoop )
  {
    auto cNode_ptr = pNode.getChildNode(iLoop);
    // Check <station>
    if ( !cNode_ptr.isEmpty() && cNode_ptr.getName() == mxconst::get_ELEMENT_STATION() && inLevel == 0 )
    {
      auto tmpResult = gather_and_validate_mandatory_items(cNode_ptr, (inLevel + 1));
      if (!tmpResult.result) // if there is "barcode" conflict, store it
      {
        result.result = false;
        // merge error
        result.errMsges.merge(tmpResult.errMsges);
      }
    }
    // Check <item>
    else if ( (!cNode_ptr.isEmpty()) && (cNode_ptr.getName() == mxconst::get_ELEMENT_ITEM()) )
    {
      const std::string barcode_s = Utils::readAttrib(cNode_ptr, mxconst::get_ATTRIB_BARCODE(), "");

      if (Utils::readBoolAttrib(cNode_ptr, mxconst::get_ATTRIB_MANDATORY(), false) && !barcode_s.empty())
      {
        // test if barcode exists in the non-mandatory map
        if (mxUtils::isElementExists(this->map_non_mandatory_items, barcode_s))
        {
          result.result = false; // Found "non-mandatory" and "mandatory" items with same barcode.
          result.addErrMsg(fmt::format("Found 'mandatory' and 'non-mandatory' items with same barcode: {}. This is an invalid state, a mandatory item must have a unique barcode string. Mission inventory behavior might produce wrong outcome.", barcode_s));

          // TODO: consider resting the mandatory item attributes.
        }

        // add node pointer if not exists
        if (!mxUtils::isElementExists(this->map_mandatory_items, barcode_s))
        {
          this->map_mandatory_items[barcode_s]     = 0;         // initialize
          this->map_mandatory_items_ptr[barcode_s] = cNode_ptr; // store mandatory pointer
        }
        // Mark Inventory with "mandatory item" flag.
        this->map_mandatory_items[barcode_s] += 1;
        this->flag_has_mandatory_item = true;
        
      }
      else
      { 
        // Add non mandatory item to the map
        if (!mxUtils::isElementExists(this->map_non_mandatory_items, barcode_s))
          this->map_non_mandatory_items[barcode_s] = 0; // initialize
         
        this->map_non_mandatory_items[barcode_s] += 1;

        // check if non-mandatory item barcode exists in the "map_mandatory_items", since it is not valid
        if (mxUtils::isElementExists(this->map_mandatory_items, barcode_s) )
        {
          result.result = false; // Found "non-mandatory" and "mandatory" items with same barcode.
          result.addErrMsg(fmt::format("Found 'non-mandatory' and 'mandatory' items with same barcode: {}. This is an invalid state, a mandatory item must have a unique barcode string. Mission inventory behavior might produce wrong outcome.", barcode_s) );

          // TODO: consider resting the mandatory item attributes.
        }
      }
    }
  } // end loop


  return result;
}


} /* namespace missionx */

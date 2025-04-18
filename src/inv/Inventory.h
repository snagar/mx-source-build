/*
 * Inventory.h
 *
 *  Created on: Nov 12, 2018
 *      Author: snagar
 */

#ifndef INV_INVENTORY_H_
#define INV_INVENTORY_H_

#include "../data/Trigger.h"
//#include "Item.hpp"
#include "station.h" // v24.12.2

namespace missionx
{

class Inventory : public Trigger
{
private:
  bool isPlane{ false }; // v24.12.2
  void clone(const missionx::Inventory& inInventory);
  void read_xp11_style_inventory_items(); // Read the items that directly under the "<inventory>" and skip any <station>.

public:
  using Trigger::Trigger;

  static int opt_forceInventoryLayoutBasedOnVersion_i; // v24.12.2

  std::map<int, missionx::station>          mapStations;             // v24.12.2 will hold station information
  std::map<int, std::string>                map_acf_station_names;   // v24.12.2 will hold station id + name   
  std::unordered_map<std::string, int>      map_mandatory_items;     // v24.12.2 mandatory items: Holds <barcode, counter>
  std::unordered_map<std::string, int>      map_non_mandatory_items; // v24.12.2 mandatory items: Holds <barcode, counter> of non mandatory barcodes
  std::unordered_map<std::string, IXMLNode> map_mandatory_items_ptr; // v24.12.2 mandatory items ptr, <barcode, node pointer>


  Inventory();
  Inventory(const std::string& inTagName, const std::string& inName);
  explicit Inventory(const ITCXMLNode& inNode);
  Inventory(const missionx::Inventory& inInventory);


  missionx::Inventory& operator=(const Inventory& inInventory)
  {
    this->clone(inInventory);
    return *this;
  }

  // virtual ~Inventory();

  bool parse_node();                       // v3.0.241.1
  bool parse_node(const IXMLNode& inNode); // v24.12.2
  bool parse_item_node(IXMLNode& inNode);

  bool getIsPlane() const { return this->isPlane; } // v24.12.2

  void init();                    // clear class data, keeps node.
  void init_plane_inventory();    // v25.03.1 clear plane inventory
  void reset_inventory_content(); // v24.12.2 clear nodes and init() class data.

  bool isItemExistsInInventory(const std::string& inBarcodeName);

  // The function will return true, if there is no conflict between mandatory and regular items.
  // Check the "errors" container after the function completes.
  missionx::mx_return gather_and_validate_mandatory_items(const IXMLNode& pNode, const int& inLevel);
  int                 flag_has_mandatory_item{ false };


  void discardItem(const std::string& inBarcode, int inQuantity, const int& inStationId); // v24.12.2 Used if there is no "to inventory"
  void applyPropertiesToLocal();                                                          // read properties and apply to local variables.

  // checkpoint
  void storeCoreAttribAsProperties();

  // v24.12.2
  static bool        move_item_to_station(missionx::station& inout_station, IXMLNode inSourceItemNodePtr, IXMLNode inTargetInventoryPtr, const std::string& barcode, const int& inQuantity = 1);
  std::vector<float> get_inventory_station_weights_as_vector(const int& in_max_array_size);
  std::string        get_inventory_station_weights_as_string(const int& in_max_array_size, int& out_stations_array_size);
  int                get_stations_number() const;

  // v24.12.2 - get item node based on barcode. Compatible with XP11/XP12
  IXMLNode get_item_node_ptr(const std::string& barcode, const int& inStation_id = -1);

  // Supports XP11 / XP12 inventory layout.
  // Can return "-1,0 and N". "-1, means no node was found".
  int get_item_quantity(const std::string& inBarcodeName);

  // Return if item exists in current inventory. Supports XP11/XP12 inventory layout.
  // @nLevel should always start with Zero, and it used only in recursion calls.
  bool      get_item_exists(const IXMLNode& pNode, const std::string& inBarcodeName, const int& nLevel = 0);
  bool      add_an_item_to_a_station(IXMLNode& inout_sourceItemNode, const int inStationId, const int inQuantity); // add an item to a station. If station id < 0, then add to station with index 0.
  mx_return add_item(IXMLNode& inSourceItemNodePtr, const int& inQuantity, const int& inStation = -1);


  // merge two inventories based on the "acf" file. This is relevant only when using <station> based inventories
  static missionx::Inventory mergeAcfAndPlaneInventories (missionx::Inventory inTargetInventory, missionx::Inventory &inSourceInventory); // This is a dedicated function
  static IXMLNode mergeAcfAndPlaneInventories ( IXMLNode inTargetInventory, const IXMLNode &inSourceInventory); // This is a dedicated function
  // END v24.12.2

  // v25.03.1
  // v25.03.1 parse station weight and add <station> to the parent node
  static void parse_max_weight_line_and_station_name(const std::string& line, IXMLNode &pNode, const missionx::enums::mx_acf_line_type_enum in_line_type);
  static void parse_station_name_line(const std::string& line, std::map<int, std::string>& mapStationNames);                         // v24.12.2

  static void gather_acf_cargo_data (Inventory &inout_current_plane_inventory, bool in_plane_was_changed_b = false); // v25.03.1 this function will run in the main flight callback.

};


} /* namespace missionx */

#endif /* INV_INVENTORY_H_ */

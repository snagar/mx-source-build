/*
 * cargo.h
 *
 *  Created on: Dec 09, 2024
 *      Author: snagar
 */

#ifndef STATION_H
#define STATION_H

#include "../core/Utils.h"
#include "../core/mx_base_node.h"
#include "../io/IXMLParser.h"

namespace missionx
{
/////////////////////////////////////
//// MX_STATION CLASS
/////////////////////////////////////
class station : public missionx::mx_base_node
{
  using mx_base_node::mx_base_node;

public:
  // We will store the "ITEM" information inside the "station" node.
  // The station name we will store in: "base_node->name"
  int   station_indx = 0;
  int   total_items_in_station_i{ 0 }; // total items
  float total_weight_in_station_f{ 0.0f }; // total weight of the station
  float max_allowed_weight{ 0.0 };

  // std::unordered_map<std::string, missionx::Item> mapItems;
  // std::unordered_map<std::string, IXMLNode> mapItemNodes;

  station();
  station(const int &inIndex, const std::string &inName);

  // ~station();

  void init()
  {
    mx_base_node::initBaseNode();

    station_indx              = 0;
    total_items_in_station_i  = 0;
    total_weight_in_station_f = 0.0f;
    max_allowed_weight        = 0.0f;

    if (!mxconst::get_ELEMENT_STATION().empty ())
      this->node.updateName(mxconst::get_ELEMENT_STATION().c_str());
  }

  bool parse_node (const IXMLNode & inNode);

  bool parse_node();

  [[nodiscard]] float get_total_weight() const { return total_weight_in_station_f; }
  auto  get_station_node_ptr() { return node; }

  void set_name(const std::string &inName);

  mx_return add_item(IXMLNode & inSourceItemNodePtr, const int& inQuantity);

  float calc_total_weight();
  int calc_total_item_groups_in_station();

};


} /* namespace missionx */

#endif /* STATION_H */

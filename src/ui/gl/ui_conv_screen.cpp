//
// Created by saar.nagar on 4/15/2026.
//

#include "ui_conv_screen.h"
#include "../../io/ListDir.h"



namespace missionx
{
std::map<std::string, std::string> ui_conv_screen::read_fpln_files()
{
  const std::string                  pathToRead = "Output/FMS plans";
  std::map<std::string, std::string> mapListOfFiles;
  std::string                        filter = ".lnmpln";
  missionx::ListDir::getListOfFiles (pathToRead.c_str (), mapListOfFiles, filter);
  return mapListOfFiles;
}

// ----------------------------

void ui_conv_screen::draw_conv_popup_datarefs(IXMLNode& inXpData)
{
    auto         win_size_vec2 = ImGui::GetWindowSize (); // ImGui::GetContentRegionAvail();
  const auto   modal_w       = mxUiGetContentWidth ();
  const auto   modal_h       = ImGui::GetWindowHeight ();
  const ImVec2 modal_center (modal_w * 0.5f, modal_h * 0.5f);
  const ImVec2 vec2_multiLine_dim = ImVec2 (win_size_vec2.x - 25.0f, 120.0f);

  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
  ImGui::TextColored (missionx::color::color_vec4_yellow, R"(In this screen you can add datarefs to the <xpdata> main element.
You have to enter correct XML elements. Example:
<dataref name="gearForce" key="sim/flightmodel/forces/faxil_gear"/>
Other option is to add this information after generating the mission file, if you so prefer.)");
  this->mxUiReleaseLastFont ();

  // close window button
  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ()); // v3.303.14
  ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
  ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgray);
  ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
  ImGui::NewLine ();
  ImGui::SameLine (win_size_vec2.x - 90.0f);
  if (ImGui::Button ("Close", ImVec2 (70.0f, 0.0f)))
    ImGui::CloseCurrentPopup ();
  ImGui::PopStyleColor (3);

  ImGui::BeginGroup ();
  ImGui::BeginChild ("MultiLineTextEdit", ImVec2 (0.0f, vec2_multiLine_dim.y + 10.0f));
  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ()); // v3.303.14
  ImGui::InputTextMultiline ("##xpdataMultiLine", this->strct_conv_layer.buff_dataref, sizeof (this->strct_conv_layer.buff_dataref), vec2_multiLine_dim);
  this->mxUiReleaseLastFont ();
  ImGui::EndChild ();
  ImGui::EndGroup ();

  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ()); // v3.303.14
  ImGui::TextColored (missionx::color::color_vec4_beige, "%zu of %zu", std::string (this->strct_conv_layer.buff_dataref).length (), sizeof (this->strct_conv_layer.buff_dataref));

  // Parse and store button
  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ()); // v3.303.14
  ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
  ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgray);
  ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
  if (ImGui::Button ("Parse and Store", ImVec2 (110.0f, 0.0f)))
  {
    IXMLDomParser iDomTemplate;
    IXMLResults   parse_result_strct;

    std::string xpdata_s    = "<" + mxconst::get_ELEMENT_XPDATA () + "> " + std::string (this->strct_conv_layer.buff_dataref) + "</" + mxconst::get_ELEMENT_XPDATA () + ">"; // holds the user entered XML string
    IXMLNode    xpdata_node = iDomTemplate.parseString (xpdata_s.c_str (), mxconst::get_ELEMENT_XPDATA ().c_str (), &parse_result_strct).deepCopy (); // parse xml into ITCXMLNode
    if (parse_result_strct.errorCode == IXMLError_None)
    {
      Utils::xml_delete_all_subnodes (inXpData, mxconst::get_ELEMENT_DATAREF ());
      for (int i1 = 0; i1 < xpdata_node.nChildNode (mxconst::get_ELEMENT_DATAREF ().c_str ()); ++i1)
        inXpData.addChild (xpdata_node.getChildNode (mxconst::get_ELEMENT_DATAREF ().c_str (), i1).deepCopy ()); // add all <dataref> childs to the global xpdata


      this->set_bottom_message_line1 ("<xpdata> information was stored", 8);

      #ifndef RELEASE
      Log::logMsg ("Valid <xpdata>:\n" + xpdata_s);
      #endif // !RELEASE
    }
    else
    {
      std::string err_s = std::string (IXMLPullParser::getErrorMessage (parse_result_strct.errorCode)) + " [line/col:" + mxUtils::formatNumber<long long> (parse_result_strct.nLine) + "/" + mxUtils::formatNumber<int> (parse_result_strct.nColumn) + "]";
      this->set_bottom_message_line1 (err_s, 8);
      #ifndef RELEASE
      Log::logMsg (err_s);
      #endif // !RELEASE

      Log::logMsg ("Not valid <xpdata>:\n" + xpdata_s);
    }
  }
  ImGui::PopStyleColor (3);

  this->mxUiResetAllFontsToDefault (); // v3.303.14
  this->mx_add_tooltip (missionx::color::color_vec4_yellow, "The XML string you entered must be valid or it won't be stored");

  // v3.305.1
  this->add_message_text ();
}

// ----------------------------

void ui_conv_screen::draw_conv_popup_globalSettings(IXMLNode& inOutGlobalSettingsNode)
{
  constexpr const static float min_multiLineWidth_px = 5000.0f;
  auto                         win_size_vec2         = ImGui::GetWindowSize (); // ImGui::GetContentRegionAvail();
  const auto                   modal_w               = mxUiGetContentWidth ();
  const auto                   modal_h               = ImGui::GetWindowHeight ();
  const ImVec2                 modal_center (modal_w * 0.5f, modal_h * 0.5f);

  // const ImVec2 vec2_multiLine_dim = ImVec2((float)sizeof(this->strct_conv_layer.buff_globalSettings) + 10.0f, 200.0f);
  // Calculate the multi text width as a function of the text length relative to windows width. Minimal width is "current context windows" width.
  // const float  buff_length        = (float)std::string(this->strct_conv_layer.buff_globalSettings).length();
  const float  text_width_px      = ImGui::CalcTextSize (this->strct_conv_layer.buff_globalSettings).x;
  const ImVec2 vec2_multiLine_dim = ImVec2 (((text_width_px + 2000.0f) < min_multiLineWidth_px) ? min_multiLineWidth_px : text_width_px + min_multiLineWidth_px, 200.0f);

  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
  ImGui::TextColored (missionx::color::color_vec4_yellow, R"(In this screen you can edit the <global_settings> main element.
You have to enter correct XML elements.
Other option is to add this information after generating the mission file, if you so prefer.)");
  this->mxUiReleaseLastFont ();


  // close window button
  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
  ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
  ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgray);
  ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
  ImGui::NewLine ();
  ImGui::SameLine (win_size_vec2.x - 90.0f);
  if (ImGui::Button ("Close", ImVec2 (70.0f, 0.0f)))
    ImGui::CloseCurrentPopup ();
  ImGui::PopStyleColor (3);
  this->mxUiReleaseLastFont ();


  ///// Draw multi line
  ImGui::BeginGroup ();

  // ImGuiID child_id = ImGui::GetID("GlobalSettings##MultiLineTextEdit");
  // ImGui::BeginChild(child_id, ImVec2(0.0f, vec2_multiLine_dim.y), true, ImGuiWindowFlags_HorizontalScrollbar);

  ImGui::BeginChild ("MultiLineTextEdit", ImVec2 (0.0f, vec2_multiLine_dim.y + 10.0f), ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysHorizontalScrollbar);

  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TEXT_REG ());
  ImGui::InputTextMultiline ("##texDataMultiLine", this->strct_conv_layer.buff_globalSettings, sizeof (this->strct_conv_layer.buff_globalSettings), vec2_multiLine_dim);
  this->mxUiReleaseLastFont ();

  ImGui::EndChild ();
  ImGui::EndGroup ();
  ///// End draw multi line

  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
  ImGui::TextColored (missionx::color::color_vec4_beige, "%zu of %zu", std::string (this->strct_conv_layer.buff_globalSettings).length (), sizeof (this->strct_conv_layer.buff_globalSettings));

  // Parse and store button
  this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
  ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
  ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgray);
  ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
  if (ImGui::Button ("Parse and Store", ImVec2 (110.0f, 0.0f)))
  {
    IXMLDomParser iDomTemplate;
    IXMLResults   parse_result_strct;

    std::string xml_data_s  = "<" + mxconst::get_GLOBAL_SETTINGS () + "> " + std::string (this->strct_conv_layer.buff_globalSettings) + "</" + mxconst::get_GLOBAL_SETTINGS () + ">"; // holds the user entered XML string
    IXMLNode    xResultNode = iDomTemplate.parseString (xml_data_s.c_str (), mxconst::get_GLOBAL_SETTINGS ().c_str (), &parse_result_strct).deepCopy (); // parse xml into ITCXMLNode
    if (parse_result_strct.errorCode == IXMLError_None)
    {
      Utils::xml_delete_all_subnodes (inOutGlobalSettingsNode, "", true);
      for (int i1 = 0; i1 < xResultNode.nChildNode (); ++i1)
        inOutGlobalSettingsNode.addChild (xResultNode.getChildNode (i1).deepCopy ()); // add all <sub elements> childs to the global_settings element


      this->set_bottom_message_line1 ("<global_settings> information was stored", missionx::DEFAULT_MESSAGE_TIME_I);

#ifndef RELEASE
      Log::logMsg ("Valid <global_settings>:\n" + xml_data_s);
#endif // !RELEASE
    }
    else
    {
      std::string err_s = std::string (IXMLPullParser::getErrorMessage (parse_result_strct.errorCode)) + " [line/col:" + mxUtils::formatNumber<long long> (parse_result_strct.nLine) + "/" + mxUtils::formatNumber<int> (parse_result_strct.nColumn) + "]";
      this->set_bottom_message_line1 (err_s, DEFAULT_MESSAGE_TIME_I);
#ifndef RELEASE
      Log::logMsg (err_s);
#endif // !RELEASE

      Log::logMsg ("Not valid <global_settings>:\n" + xml_data_s);
    }
  }
  ImGui::PopStyleColor (3);

  this->mxUiResetAllFontsToDefault (); // v3.303.14
  this->mx_add_tooltip (missionx::color::color_vec4_yellow, "The XML string you entered must be valid or it won't be stored.");

  this->add_message_text ();
}

// ----------------------------

void ui_conv_screen::draw_conv_popup_flight_leg_detail(missionx::mx_local_fpln_strct& inLegData)
{
  auto         win_size_vec2 = ImGui::GetWindowSize (); // ImGui::GetContentRegionAvail();
  const auto   modal_w       = mxUiGetContentWidth ();
  const auto   modal_h       = ImGui::GetWindowHeight ();
  const ImVec2 modal_center (modal_w * 0.5f, modal_h * 0.5f);
  const ImVec2 vec2_multiLine_dim = ImVec2 (win_size_vec2.x - 50.0f, 40.0f);

  // draw only the picked row data and not for every row
  if (this->strct_conv_layer.way_row_picked_i == inLegData.indx)
  {
    this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ()); // v3.303.14
    // close window button
    ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
    ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgray);
    ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
    if (ImGui::Button ("Close", ImVec2 (120.0f, 0.0f)))
      ImGui::CloseCurrentPopup ();
    ImGui::PopStyleColor (3);

    // Title
    ImGui::SameLine (modal_center.x - (ImGui::CalcTextSize (inLegData.getName ().c_str ()).x / 2.0f)); // position text in the middle of the screen
    ImGui::TextColored (missionx::color::color_vec4_lime, "%s", inLegData.getName ().c_str ());

    int ii = 0; // i: tracks the array buffer, so we have to hardcode its value after each CollapsingHeader.

    // welcome
    ImGui::TextColored (missionx::color::color_vec4_beige, "Enter information when en-route to: %s. Try not to overdo with information.", inLegData.getName ().c_str ());
    ImGui::TextColored (missionx::color::color_vec4_yellow, "En-Route Description:");

    this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_SMALL ()); // v3.303.14
    ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_purple);
    ImGui::PushStyleColor (ImGuiCol_FrameBg, missionx::color::color_vec4_beige);
    if (ImGui::InputTextMultiline ("###flightLegDesciption", inLegData.buff_arr[ii], sizeof (inLegData.buff_arr[ii]), vec2_multiLine_dim, ImGuiInputTextFlags_None))
    {
      Utils::xml_add_cdata (inLegData.xLeg, inLegData.buff_arr[ii]);
    }
    ImGui::PopStyleColor (2);

    ImGui::NewLine ();


    ImGui::PushStyleColor (ImGuiCol_Header, missionx::color::color_vec4_orangered);
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, missionx::color::color_vec4_tomato);
    ImGui::PushStyleColor (ImGuiCol_HeaderActive, missionx::color::color_vec4_orangered);
    {
      ++ii; // 1 - 4
      this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_SMALL ()); // v3.303.14
      if (ImGui::CollapsingHeader ("Actions at the start/end of the Flight Leg"))
      {
        this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_SMALL ()); // v3.303.14

        // START
        ImGui::TextColored (missionx::color::color_vec4_yellow, "[optional] Send message at the start of the Flight Leg: ");
        if (ImGui::InputTextMultiline ("###messageSentOnFlightLegStart", inLegData.buff_arr[ii], sizeof (inLegData.buff_arr[ii]), vec2_multiLine_dim, ImGuiInputTextFlags_None))
        {
          IXMLNode nodeChild, targetNode; // the targetNode is the node we are going to do work on
          IXMLNode node = Utils::xml_get_or_create_node_ptr (inLegData.xLeg, mxconst::get_ELEMENT_START_LEG_MESSAGE ()); // this is our main node
          assert (node.isEmpty () == false && "Failed creating node.");

          if (node.nChildNode (mxconst::get_ELEMENT_MESSAGE ().c_str ()) == 0)
          {
            nodeChild = Utils::xml_get_or_create_node_ptr (node, mxconst::get_ELEMENT_MESSAGE ());
            assert (nodeChild.isEmpty () == false && "Failed creating nodeChild <message> for start_leg_message.");

            const std::string val = inLegData.attribName + "_start_message";
            Utils::xml_set_attribute_in_node_asString (nodeChild, mxconst::get_ATTRIB_NAME (), val);
            node.updateAttribute (val.c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
          }
          else
            nodeChild = node.getChildNode (mxconst::get_ELEMENT_MESSAGE ().c_str ());

          assert (nodeChild.isEmpty () == false && "Failed retrieving nodeChild.");
          targetNode = Utils::xml_get_or_create_node_ptr (nodeChild, mxconst::get_ELEMENT_MIX (), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE (), mxconst::get_CHANNEL_TYPE_TEXT ());
          Utils::xml_add_cdata (targetNode, inLegData.buff_arr[ii]);

          #ifndef RELEASE
          Utils::xml_print_node (inLegData.xLeg);
          #endif // !RELEASE
        }

        ImGui::NewLine ();
        ++ii;

        ImGui::TextColored (missionx::color::color_vec4_yellow, "[optional] Commands to run at the start of the Flight Leg: ");
        if (ImGui::InputTextWithHint ("###commandsAtStartOfLeg", "sim/command/xxx", inLegData.buff_arr[ii], sizeof (inLegData.buff_arr[ii]), ImGuiInputTextFlags_None))
        {
          IXMLNode node = Utils::xml_get_or_create_node_ptr (inLegData.xLeg, mxconst::get_ELEMENT_FIRE_COMMANDS_AT_LEG_START ());
          node.updateAttribute (inLegData.buff_arr[ii], mxconst::get_ATTRIB_COMMANDS ().c_str ());
        }

        ImGui::NewLine ();


        // END
        ++ii;
        ImGui::TextColored (missionx::color::color_vec4_yellow, "[optional] Send message when arriving to the waypoint: ");
        if (ImGui::InputTextMultiline ("###messageSentatEndOfFlightLeg", inLegData.buff_arr[ii], sizeof (inLegData.buff_arr[ii]), vec2_multiLine_dim, ImGuiInputTextFlags_None))
        {

          IXMLNode nodeChild, targetNode; // the targetNode is the node we are going to do work on
          IXMLNode node = Utils::xml_get_or_create_node_ptr (inLegData.xLeg, mxconst::get_ELEMENT_END_LEG_MESSAGE ()); // this is our main node
          assert (node.isEmpty () == false && "Failed creating node <end_leg_message>");

          if (node.nChildNode (mxconst::get_ELEMENT_MESSAGE ().c_str ()) == 0)
          {
            nodeChild = Utils::xml_get_or_create_node_ptr (node, mxconst::get_ELEMENT_MESSAGE ());
            assert (nodeChild.isEmpty () == false && "Failed creating nodeChild <element_message>");

            const std::string val = inLegData.attribName + "_end_message";
            Utils::xml_set_attribute_in_node_asString (nodeChild, mxconst::get_ATTRIB_NAME (), val);
            node.updateAttribute (val.c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
          }
          else
            nodeChild = node.getChildNode (mxconst::get_ELEMENT_MESSAGE ().c_str ());

          assert (nodeChild.isEmpty () == false && "Failed retrieving nodeChild.");
          targetNode = Utils::xml_get_or_create_node_ptr (nodeChild, mxconst::get_ELEMENT_MIX (), mxconst::get_ATTRIB_MESSAGE_MIX_TRACK_TYPE (), mxconst::get_CHANNEL_TYPE_TEXT ());
          Utils::xml_add_cdata (targetNode, inLegData.buff_arr[ii]);
        }
        ImGui::NewLine ();
        ++ii;

        ImGui::TextColored (missionx::color::color_vec4_yellow, "[optional] Commands to run when arriving to the waypoint: ");
        if (ImGui::InputTextWithHint ("###commandsAtEndOfLeg", "sim/command/xxx", inLegData.buff_arr[ii], sizeof (inLegData.buff_arr[ii]), ImGuiInputTextFlags_None))
        {
          IXMLNode node = Utils::xml_get_or_create_node_ptr (inLegData.xLeg, mxconst::get_ELEMENT_FIRE_COMMANDS_AT_LEG_END ());
          node.updateAttribute (inLegData.buff_arr[ii], mxconst::get_ATTRIB_COMMANDS ().c_str ());
        }

      } // Tasks at start/end of the Flight Leg



      // Waypoint Radius, Elevation & 3D Marker Settings
      ii = 5; // we start counting from 0
      this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_SMALL ()); // v3.303.14
      if (ImGui::CollapsingHeader ("Waypoint Radius, Elevation & 3D Marker settings when reaching the waypoint"))
      {
        static const std::vector<const char *> vecElevOptions = { "Ignore (plane in physical area)", "On Ground", "Restrict to min/max elevation", "lower than...", "above than...", "Highest allowed elev above ground", "Lowest allowed elev above ground" };
        this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_SMALL ()); // v3.303.14

        //// Trigger Area Radius ////
        ImGui::TextColored (missionx::color::color_vec4_greenyellow, "Pick Waypoint radius of effect (in meters): ");
        ImGui::SameLine (0.0f, 5.0f);
        if (ImGui::InputInt ("Waypoint area radius", &inLegData.target_trig_strct.radius_of_trigger_mt, 100))
        {
          if (inLegData.target_trig_strct.radius_of_trigger_mt <= 0)
            inLegData.target_trig_strct.radius_of_trigger_mt = 100;
          if (inLegData.target_trig_strct.radius_of_trigger_mt > 500000)
            inLegData.target_trig_strct.radius_of_trigger_mt = 500000;
        }

        ImGui::NewLine ();

        //// Target elevation rules ////
        ImGui::TextColored (missionx::color::color_vec4_greenyellow, "Elevation you want the plane to be when reaching the waypoint (overrides waypoint table on ground option): ");
        ImGui::SetNextItemWidth (350.0f);
        if (ImGui::Combo ("###ComboWhereDoYouWantPlane", &inLegData.target_trig_strct.elev_combo_picked_i, vecElevOptions.data (), static_cast<int> (vecElevOptions.size ())))
          inLegData.target_trig_strct.elev_lower_upper.clear ();

        // Show/Hide 3D Marker checkbox
        ImGui::SameLine (0.0f, 30.0f);
        ImGui::Checkbox ("Display 3D Marker##displayMarkerInHeader", &inLegData.flag_add_marker);

        // Display sliders/options based on user pick
        switch (inLegData.target_trig_strct.elev_combo_picked_i)
        {
          case 0: // ignore
            inLegData.target_trig_strct.elev_rule_s.clear ();
            inLegData.target_trig_strct.flag_on_ground = false;
            break;
          case 1: // onGround
            inLegData.target_trig_strct.elev_rule_s    = mxconst::get_MX_TRUE ();
            inLegData.target_trig_strct.flag_on_ground = true;
            break;
          case 2: // min/max
          {
            inLegData.target_trig_strct.elev_rule_s    = mxconst::get_MX_FALSE ();
            inLegData.target_trig_strct.flag_on_ground = false;

            ImGui::TextColored (missionx::color::color_vec4_greenyellow, "Pick min/max elevation volume (-4000ft..15000ft) ");
            if (ImGui::InputInt ("Min. Elevation###MinElevationTrig", &inLegData.target_trig_strct.elev_min, 100))
            {
              if (inLegData.target_trig_strct.elev_min >= inLegData.target_trig_strct.elev_max)
                inLegData.target_trig_strct.elev_max = inLegData.target_trig_strct.elev_min + 1000;
              else if (inLegData.target_trig_strct.elev_min > 20000)
                inLegData.target_trig_strct.elev_min = 20000;
              else if (inLegData.target_trig_strct.elev_min < -4000)
                inLegData.target_trig_strct.elev_min = -4000;
            }
            ImGui::SameLine ();
            ImGui::TextUnformatted (" .. ");
            ImGui::SameLine ();
            if (ImGui::InputInt ("Max. Elevation###MaxElevationTrig", &inLegData.target_trig_strct.elev_max, 100))
            {
              if (inLegData.target_trig_strct.elev_max <= inLegData.target_trig_strct.elev_min)
                inLegData.target_trig_strct.elev_min = inLegData.target_trig_strct.elev_max - 1000;
              else if (inLegData.target_trig_strct.elev_max < 1000)
                inLegData.target_trig_strct.elev_max = 1000;
              else if (inLegData.target_trig_strct.elev_max > 150000)
                inLegData.target_trig_strct.elev_max = 150000;
            }
            inLegData.target_trig_strct.elev_lower_upper = mxUtils::formatNumber<int> (inLegData.target_trig_strct.elev_min) + "|" + mxUtils::formatNumber<int> (inLegData.target_trig_strct.elev_max);
          }
          break;
          case 3: // lower than
          {
            inLegData.target_trig_strct.elev_rule_s = mxconst::get_MX_FALSE ();
            ImGui::TextColored (missionx::color::color_vec4_greenyellow, "Pick upper elevation (plane should fly below it): ");

            ImGui::SameLine (0.0f, 30.0f);
            if (ImGui::InputInt ("###InputUpperElevSlider", &inLegData.target_trig_strct.slider_elev_value_i, 100))
              inLegData.target_trig_strct.elev_lower_upper = "--" + mxUtils::formatNumber<int> (inLegData.target_trig_strct.slider_elev_value_i);

            ImGui::SameLine (0.0f, 30.0f);
            ImGui::TextColored (missionx::color::color_vec4_white, "%s", inLegData.target_trig_strct.elev_lower_upper.c_str ());
          }
          break;
          case 4: // above than
          {
            inLegData.target_trig_strct.elev_rule_s = mxconst::get_MX_FALSE ();
            ImGui::TextColored (missionx::color::color_vec4_greenyellow, "Pick lowest elevation (plane should fly above it): ");

            ImGui::SameLine (0.0f, 30.0f);
            if (ImGui::InputInt ("###InputLowestElevSlider", &inLegData.target_trig_strct.slider_elev_value_i, 100))
              inLegData.target_trig_strct.elev_lower_upper = "++" + mxUtils::formatNumber<int> (inLegData.target_trig_strct.slider_elev_value_i);

            ImGui::SameLine (0.0f, 30.0f);
            ImGui::TextColored (missionx::color::color_vec4_white, "%s", inLegData.target_trig_strct.elev_lower_upper.c_str ());
          }
          break;
          case 5: // highest allowed elev above ground
          {
            inLegData.target_trig_strct.elev_rule_s = mxconst::get_MX_FALSE ();
            ImGui::TextColored (missionx::color::color_vec4_beige, "Pick elevation above ground - plane should fly below that elevation:");

            if (ImGui::InputInt ("###RelativeElevAboveGround_flyBelow", &inLegData.target_trig_strct.slider_elev_value_i, 100))
              inLegData.target_trig_strct.elev_lower_upper = "---" + mxUtils::formatNumber<int> (inLegData.target_trig_strct.slider_elev_value_i);

            ImGui::SameLine (0.0f, 10.0f);
            ImGui::TextColored (missionx::color::color_vec4_white, "%s", inLegData.target_trig_strct.elev_lower_upper.c_str ());
          }
          break;
          case 6: // lowest allowed elevation above ground
          {
            inLegData.target_trig_strct.elev_rule_s = mxconst::get_MX_FALSE ();
            ImGui::TextColored (missionx::color::color_vec4_beige, "Pick elevation above ground - plane should fly above that elevation:");

            if (ImGui::InputInt ("###RelativeElevAboveGround_flyAbove", &inLegData.target_trig_strct.slider_elev_value_i, 100))
              inLegData.target_trig_strct.elev_lower_upper = "+++" + mxUtils::formatNumber<int> (inLegData.target_trig_strct.slider_elev_value_i);

            ImGui::SameLine (0.0f, 10.0f);

            ImGui::TextColored (missionx::color::color_vec4_white, "%s", inLegData.target_trig_strct.elev_lower_upper.c_str ());
          }
          break;
          default: ;
        } // end switch



        if (inLegData.flag_add_marker)
        {
          ImGui::NewLine ();
          ImGui::TextColored (missionx::color::color_vec4_greenyellow, "How do you want to set the 3D Marker:");

          ImGui::TextUnformatted ("Marker Type: ");
          ImGui::SameLine ();
          ImGui::SetNextItemWidth (250.0f);
          ImGui::Combo ("###3DMarkerTypes", &inLegData.marker_type_i, mxconst::get_vecMarkerTypeOptions ().data (), static_cast<int> (mxconst::get_vecMarkerTypeOptions ().size ()));

          ImGui::TextUnformatted ("Distance to display marker in Nautical Miles (2-50): ");
          ImGui::SameLine ();
          ImGui::SetNextItemWidth (100.0f);
          if (ImGui::InputFloat ("##Distance to display marker", &inLegData.radius_to_display_3D_marker_in_nm_f, 0.5, 1.0, "%.2f")) // minimum 2.0 nm
          {
            if (inLegData.radius_to_display_3D_marker_in_nm_f < 2.0f)
              inLegData.radius_to_display_3D_marker_in_nm_f = 2.0f;
            else if (inLegData.radius_to_display_3D_marker_in_nm_f > 50.0f)
              inLegData.radius_to_display_3D_marker_in_nm_f = 50.0f;
          }
        }
      } // elevation & 3D Marker header


      // Triggers
      ii = 5;
      {
        this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_SMALL ()); // v3.303.14
        if (ImGui::CollapsingHeader ("Triggers / Events during en-route"))
        {
          this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_SMALL ()); // v3.303.14
          subDraw_ui_xTrigger_main (inLegData, this->strct_conv_layer.flag_refreshTriggerListFrom_xNode, inLegData.indx, this->strct_conv_layer.mapOfGlobalTriggers, this->strct_conv_layer.vecGlobalTriggers_names);
        }
      }
      ii = 5;
    } // end pushStyle
    ImGui::PopStyleColor (3);

    this->mxUiResetAllFontsToDefault (); // v3.303.14
  }
}

// ----------------------------

void ui_conv_screen::draw_conv_popup_briefer(missionx::mx_local_fpln_strct& inLegData)
{
  auto         win_size_vec2 = ImGui::GetContentRegionAvail ();
  const ImVec2 modal_center (win_size_vec2.x * 0.5f, win_size_vec2.y * 0.5f);

  if (this->strct_conv_layer.way_row_picked_i == inLegData.indx)
  {
    // int ii = 0;
    inLegData.iCurrentBuf = 0;

    this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ()); // v3.303.14
    // close window button
    ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
    ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgray);
    ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
    if (ImGui::Button ("Close", ImVec2 (120, 0)))
      ImGui::CloseCurrentPopup ();
    ImGui::PopStyleColor (3);

    // Title
    constexpr static const char *title_s = "Briefer and Mission Information";
    ImGui::SameLine (modal_center.x - (ImGui::CalcTextSize (title_s).x / 2.0f));
    ImGui::TextColored (missionx::color::color_vec4_lime, title_s);

    // welcome
    ImGui::TextColored (missionx::color::color_vec4_beige, "Enter the mission information (what user sees when they click on the mission image) and Enter the briefer description");
    ImGui::NewLine ();

    ImGui::PushStyleColor (ImGuiCol_Header, missionx::color::color_vec4_orangered);
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, missionx::color::color_vec4_tomato);
    ImGui::PushStyleColor (ImGuiCol_HeaderActive, missionx::color::color_vec4_orangered);
    {
      this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_SMALL ()); // v3.303.14
      if (ImGui::CollapsingHeader ("< Mission Info >"))
      {
        ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_purple);
        ImGui::PushStyleColor (ImGuiCol_FrameBg, missionx::color::color_vec4_beige);
        this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_SMALL ()); // v3.303.14
        ImGui::TextColored (missionx::color::color_vec4_yellow, "Written By: ");
        ImGui::SameLine ();
        if (ImGui::InputTextWithHint ("###WrittenBy", "== your name ==", inLegData.buff_arr[inLegData.iCurrentBuf], sizeof (inLegData.buff_arr[inLegData.iCurrentBuf]), ImGuiInputTextFlags_None))
        {
          this->strct_conv_layer.xConvInfo.updateAttribute (inLegData.buff_arr[inLegData.iCurrentBuf], mxconst::get_ATTRIB_WRITTEN_BY ().c_str (), mxconst::get_ATTRIB_WRITTEN_BY ().c_str ()); // 0
        }
        ++inLegData.iCurrentBuf;

        ImGui::SameLine (0.0f, 20.0f);
        ImGui::TextColored (missionx::color::color_vec4_yellow, "Estimate Time: ");
        ImGui::SameLine ();
        if (ImGui::InputTextWithHint ("###EstimateTime", "~45min, ~60min, ~90min etc..", inLegData.buff_arr[inLegData.iCurrentBuf], sizeof (inLegData.buff_arr[inLegData.iCurrentBuf]), ImGuiInputTextFlags_None))
        {
          this->strct_conv_layer.xConvInfo.updateAttribute (inLegData.buff_arr[inLegData.iCurrentBuf], mxconst::get_ATTRIB_ESTIMATE_TIME ().c_str (), mxconst::get_ATTRIB_ESTIMATE_TIME ().c_str ()); // 1
        }
        ++inLegData.iCurrentBuf;

        ImGui::NewLine ();

        ImGui::TextColored (missionx::color::color_vec4_yellow, "Weather Settings: ");
        ImGui::SameLine ();
        ImGui::SetNextItemWidth (250.0f);
        if (ImGui::InputTextWithHint ("###WeatherSettings", "User Preferred Settings / Set to Overcast", inLegData.buff_arr[inLegData.iCurrentBuf], sizeof (inLegData.buff_arr[inLegData.iCurrentBuf]), ImGuiInputTextFlags_None))
        {
          this->strct_conv_layer.xConvInfo.updateAttribute (inLegData.buff_arr[inLegData.iCurrentBuf], mxconst::get_ATTRIB_WEATHER_SETTINGS ().c_str (), mxconst::get_ATTRIB_WEATHER_SETTINGS ().c_str ()); // 3
        }
        ++inLegData.iCurrentBuf;

        ImGui::NewLine ();
        ImGui::TextColored (missionx::color::color_vec4_yellow, "Other settings: ");
        ImVec2 vec2_dimentions = ImVec2 (win_size_vec2.x - 50.0f, 60.0f);

        if (ImGui::InputTextMultiline ("###OtherSettings", inLegData.buff_arr[inLegData.iCurrentBuf], sizeof (inLegData.buff_arr[inLegData.iCurrentBuf]), vec2_dimentions))
        {
          this->strct_conv_layer.xConvInfo.updateAttribute (inLegData.buff_arr[inLegData.iCurrentBuf], mxconst::get_ATTRIB_OTHER_SETTINGS ().c_str (), mxconst::get_ATTRIB_OTHER_SETTINGS ().c_str ()); // 4
        }
        ++inLegData.iCurrentBuf;

        ImGui::PopStyleColor (2);

        ImGui::NewLine ();
      } // < Mission Info >

      this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_SMALL ()); // v3.303.14
      inLegData.iCurrentBuf = 4; // we ned this since when the "collapse header" is closed, the code won't run so the numbering will start in 0 and not in the correct buffer
      if (ImGui::CollapsingHeader ("< briefer > and Starting location"))
      {
        this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_SMALL ()); // v3.303.14
        // inLegData.iCurrentBuf is used through the next few widgets
        ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_purple);
        ImGui::PushStyleColor (ImGuiCol_FrameBg, missionx::color::color_vec4_beige);

        ImGui::TextColored (missionx::color::color_vec4_yellow, "Start Heading: ");
        ImGui::SameLine ();
        if (ImGui::InputTextWithHint ("###BrieferStartHeading", "0..359", inLegData.buff_arr[inLegData.iCurrentBuf], sizeof (inLegData.buff_arr[inLegData.iCurrentBuf]), ImGuiInputTextFlags_CharsDecimal))
        {
          inLegData.xLeg.updateAttribute (inLegData.buff_arr[inLegData.iCurrentBuf], mxconst::get_ATTRIB_HEADING_PSI ().c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str ()); // 3
        }
        ImGui::SameLine (0.0f, 5.0f);

        const auto lmbda_getAndSetHeading = [&inLegData] (XPLMDataRef inRef)
        {
          auto        heading_f = XPLMGetDataf (inRef);
          std::string heading_s = mxUtils::formatNumber<int> (static_cast<int> (heading_f), 0);
          inLegData.setBuff (inLegData.iCurrentBuf, heading_s);

          inLegData.xLeg.updateAttribute (inLegData.buff_arr[inLegData.iCurrentBuf], mxconst::get_ATTRIB_HEADING_PSI ().c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str ());
        };

        ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_yellow);
        if (ImGui::Button ("P"))
        {
          lmbda_getAndSetHeading (missionx::drefConst.dref_heading_true_psi_f);
        }
        this->mx_add_tooltip (missionx::color::color_vec4_white, "Get Plane Heading.");
        ImGui::SameLine (0.0f, 5.0f);
        if (ImGui::Button ("C"))
        {
          const XPLMDataRef dref = XPLMFindDataRef ("sim/graphics/view/view_heading"); // camera heading

          lmbda_getAndSetHeading (dref);
        }
        this->mx_add_tooltip (missionx::color::color_vec4_white, "Get Camera Heading.");
        ImGui::PopStyleColor ();
        // end of same inLegData.iCurrentBuf

        ++inLegData.iCurrentBuf;

        ImGui::NewLine ();

        ImGui::TextColored (missionx::color::color_vec4_yellow, "Mission Description (%i chars): ", missionx::LOG_BUFF_SIZE);
        ImGui::TextColored (missionx::color::color_vec4_beige, "%zu", std::string (inLegData.buff_arr[inLegData.iCurrentBuf]).length ());
        const ImVec2 vec2_dimentions = ImVec2 (win_size_vec2.x - 50.0f, 60.0f);
        if (ImGui::InputTextMultiline ("###BrieferMissionDesc", inLegData.buff_arr[inLegData.iCurrentBuf], sizeof (inLegData.buff_arr[inLegData.iCurrentBuf]), vec2_dimentions)) // 5
        {
          Utils::xml_add_cdata (inLegData.xLeg, inLegData.buff_arr[inLegData.iCurrentBuf]);
        }

        ImGui::PopStyleColor (2);

        ImGui::NewLine ();
      } // < Mission Info >

      inLegData.iCurrentBuf = 6;
    }
    ImGui::PopStyleColor (3);

    this->mxUiResetAllFontsToDefault ();
  }

}

// ----------------------------

void ui_conv_screen::subDraw_popup_user_lat_lon(mx_trig_strct_& inout_trig)
{
  static ImVec2 vec2Pos;
  auto          win_size_vec2 = ImGui::GetWindowSize ();
  const ImVec2  center (ImGui::GetIO ().DisplaySize.x * 0.5f, ImGui::GetIO ().DisplaySize.y * 0.5f);
  ImGui::SetNextWindowPos (center, ImGuiCond_Appearing, ImVec2 (0.5f, 0.5f));
  ImGui::SetNextWindowSize (ImVec2 (win_size_vec2.x / 2.0f, 100.0f));

  {
    //// draw 2 text items as lat and long
    ImGui::TextColored (missionx::color::color_vec4_yellow, "Enter Latitude / Longitude");
    ImGui::TextColored (missionx::color::color_vec4_yellow, "Lat/Lon ");
    ImGui::SameLine ();
    ImGui::InputFloat ("##userCustomLat", &vec2Pos.x);
    ImGui::SameLine (0.0f, 2.0f);
    ImGui::TextColored (missionx::color::color_vec4_yellow, "/");
    ImGui::SameLine ();
    ImGui::InputFloat ("##userCustomLon", &vec2Pos.y);

    ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
    ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgrey);
    ImGui::PushStyleColor (ImGuiCol_ButtonActive, missionx::color::color_vec4_grey);
    ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
    if (ImGui::Button ("Apply##ApplyUserLatLon"))
    {
      Point p (vec2Pos.x, vec2Pos.y);
      inout_trig.setBuff (inout_trig.iCurrentBuf, p.get_point_lat_lon_as_string ());
      ImGui::CloseCurrentPopup ();
    }
    ImGui::PopStyleColor (4);

    ImGui::SameLine (ImGui::GetWindowWidth () - 80.0f);
    if (ImGui::Button ("Cancel##CancelUserLatLon"))
      ImGui::CloseCurrentPopup ();
  }

}

// ----------------------------

void ui_conv_screen::subDraw_popup_outcome(mx_trig_strct_& inout_trig, IXMLNode& inMessageTemplates)
{
    IXMLNode xOutcome = Utils::xml_get_or_create_node_ptr (inout_trig.node_ptr, mxconst::get_ELEMENT_OUTCOME ());

  assert (xOutcome.isEmpty () == false && "<outcome> element can't be empty.");

  const auto lmbda_get_outcome_list_of_attributes_from_node = [] (IXMLNode &in_xOutcome)
  {
    std::vector<const char *> vecOutcomeAttributes;
    for (int i1 = 0; i1 < in_xOutcome.nAttribute (); ++i1)
    {
      vecOutcomeAttributes.emplace_back (in_xOutcome.getAttributeName (i1));
    }

    return vecOutcomeAttributes;
  };

  static int  attrib_picked_i = -1;
  std::string attrib_label_cc;
  const auto  vecAttributeList = lmbda_get_outcome_list_of_attributes_from_node (xOutcome); // we fill vector from <outcome> element attribute names. When we update its attribute the memory address changes, therefore we must refresh the vector every time. We can solve that if we will create the attributes ahead of time and not dynamically based on the XML header.

  // List of options
  ImGui::SetNextItemWidth (180.0f);
  ImGui::Combo ("##comboListOfOutcomeAttribs", &attrib_picked_i, vecAttributeList.data (), static_cast<int> (vecAttributeList.size ()));
  // close button
  ImGui::SameLine (ImGui::GetWindowWidth () - 100.0f);
  if (ImGui::Button ("Close##closeOutcomePopup"))
    ImGui::CloseCurrentPopup ();

  if (attrib_picked_i > -1 && attrib_picked_i < (static_cast<int> (vecAttributeList.size ()) - 1))
    attrib_label_cc = std::string (vecAttributeList[attrib_picked_i]);

  if (attrib_label_cc.empty () == false)
  {
    // first input text, represent keyName or value (if index < 2 than it represent keyName)
    ImGui::InputText (attrib_label_cc.c_str (), inout_trig.buffArray[inout_trig.iCurrentBuf], sizeof (inout_trig.buffArray[inout_trig.iCurrentBuf]));
    if (attrib_picked_i < 2)
      this->mx_add_tooltip (missionx::color::color_vec4_beige, "Enter an alias to the message"); // TODO maybe create the keyName internaly so simmer will only focus on the text
    else if (attrib_picked_i < 4)
      this->mx_add_tooltip (missionx::color::color_vec4_beige, "Enter an alias to the script you will create later / or the  script exists"); // TODO maybe create the keyName internaly so simmer will only focus on the text
    else
      this->mx_add_tooltip (missionx::color::color_vec4_beige, "Enter text that represent the option you picked (command or dataref list divided by comma (\",\")");

    // second input text only if outcome is message
    if (attrib_picked_i < 2) // create message for options 0,1 in combo
    {
      ++inout_trig.iCurrentBuf;
      ImGui::SetNextItemWidth (300.0f);
      ImGui::InputText ("Enter Message Text##outcomeText", inout_trig.buffArray[inout_trig.iCurrentBuf], sizeof (inout_trig.buffArray[inout_trig.iCurrentBuf]));
      this->mx_add_tooltip (missionx::color::color_vec4_beige, "Enter message text"); // TODO maybe create the keyName internaly so simmer will only focus on the text
    }

    ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
    ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgrey);
    ImGui::PushStyleColor (ImGuiCol_ButtonActive, missionx::color::color_vec4_grey);
    ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
    if (ImGui::Button ("Apply##ApplyOutcomeChanges"))
    {
      std::string attrib_val1_s = (attrib_picked_i < 2) ? inout_trig.getBuff (inout_trig.iCurrentBuf - 1) : inout_trig.getBuff (inout_trig.iCurrentBuf); // if user picked options 0,1 in combo then we have 2 input text
      if (attrib_picked_i < 2) // trig_fire or trig_left
      {
        std::string attrib_val2_s = inout_trig.getBuff (inout_trig.iCurrentBuf); // message text

        if (attrib_val1_s.empty ()) // if empty then reset <outcome> attrib
          xOutcome.updateAttribute ("", attrib_label_cc.c_str (), attrib_label_cc.c_str ());
        else
        {
          xOutcome.updateAttribute (attrib_val1_s.c_str (), attrib_label_cc.c_str (), attrib_label_cc.c_str ()); // this will allow to assign message keyName without text, in cases where messages was created already.
          if (!attrib_val2_s.empty ())
          {
            IXMLNode xMessage = Utils::xml_get_node_from_node_tree_by_attrib_name_and_value_IXMLNode (inMessageTemplates, mxconst::get_ELEMENT_MESSAGE (), mxconst::get_ATTRIB_NAME (), attrib_val1_s, false);

            if (xMessage.isEmpty ()) // not found
              xMessage = Utils::xml_create_message (attrib_val1_s, attrib_val2_s);

            if (xMessage.isEmpty () == false)
              inMessageTemplates.addChild (xMessage);
          }
        }
      }
      else
        xOutcome.updateAttribute (attrib_val1_s.c_str (), attrib_label_cc.c_str (), attrib_label_cc.c_str ()); // this will allow to assign the text or reset it if empty for <script> and <commands>
    }
    ImGui::PopStyleColor (4);
  }


  ImGui::PushStyleColor (ImGuiCol_Separator, missionx::color::color_vec4_white);
  ImGui::Separator ();
  ImGui::PopStyleColor ();

  if (!xOutcome.isEmpty ())
  {
    IXMLRenderer xmlRenderer;
    std::string  outcom_s = xmlRenderer.getString (xOutcome);
    char         buf[missionx::LOG_BUFF_SIZE]{ 0 };
//#ifdef IBM
//    memcpy_s (buf, sizeof (buf), outcom_s.c_str (), (sizeof (buf) > outcom_s.length ()) ? outcom_s.length () : sizeof (buf));
//#else
//    memcpy (buf, outcom_s.c_str (), (sizeof (buf) > outcom_s.length ()) ? outcom_s.length () : sizeof (buf));
//#endif
    mxUtils::copy_string_to_buffer(outcom_s, buf[0], sizeof(buf));


    ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
    ImGui::PushStyleColor (ImGuiCol_ChildBg, missionx::color::color_vec4_antiquewhite);
    ImGui::BeginChild ("##outcomeXML_text", ImVec2 (ImGui::GetContentRegionAvail ().x, 60.0f));
    {
      ImGui::TextWrapped ("%s", xmlRenderer.getString (xOutcome));
    }
    ImGui::EndChild ();
    ImGui::PopStyleColor (2);

    ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_white);
    ImGui::InputTextMultiline ("##trigOutcomeMultiLine", buf, sizeof (buf), ImVec2 (ImGui::GetContentRegionAvail ().x, 30.0f), ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor ();
    this->mx_add_tooltip (missionx::color::color_vec4_yellow, "You can copy the <outcome> text.");
  }
}

// ----------------------------

void ui_conv_screen::subDraw_fpln_table(IXMLNode& inMainNode, std::map<int, missionx::mx_local_fpln_strct>& in_map_tableOfParsedFpln)
{
  auto win_size_vec2 = ImGui::GetWindowSize();

  // Alternating rows slightly grayish
  ImGui::PushStyleColor(ImGuiCol_TableRowBg, IM_COL32(0x20, 0x20, 0x20, 0xff));
  ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, IM_COL32(0x30, 0x30, 0x30, 0xff));
  constexpr int col_i      = 10;
  int           COLUMN_NUM = (this->strct_conv_layer.flag_foundBriefer_index0) ? col_i : col_i + 1;
  {
    ImgWindow::HelpMarker("The table holds the list of waypoints from the imported flight plan.\n\"Leg\": Mark as a waypoints you must reach.\n\t\tIndex 0 is your 'start' location.\n\"GD\": Flag flight leg to be on ground.\n\"BR\": Convert Index 1 waypoint to briefer, only if index 0 is unavailable.\n\"Details\": Opens a popup.\n\"IG\": Ignore this "
                                          "waypoint. Won't be part of the GPS.");
    if (ImGui::BeginTable("Table_fpln_lnm", COLUMN_NUM, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit))
    {
      ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible

      // Set up the columns of the table - TBD
      ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_yellow);
      {
        ImGui::TableSetupColumn("Indx", ImGuiTableColumnFlags_None, 20.0f); // Index, row number
        ImGui::TableSetupColumn("Ident", ImGuiTableColumnFlags_None, 100.0f); // to ICAO + (keyName)
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_None, 75.0f);

        ImGui::TableSetupColumn("Leg", ImGuiTableColumnFlags_None, 30.0f); // Is this a flight leg ?
        ImGui::TableSetupColumn("GD", ImGuiTableColumnFlags_None, 30.0f); // on ground ?

        if (this->strct_conv_layer.flag_foundBriefer_index0 == false)
          ImGui::TableSetupColumn("BR", ImGuiTableColumnFlags_None, 30.0f); // Convert to briefer so we will have our departure data if we have no briefer waypoint

        ImGui::TableSetupColumn("Lat / Lon", ImGuiTableColumnFlags_None, 150.0f);
        ImGui::TableSetupColumn("Distance nm", ImGuiTableColumnFlags_None, 85.0f); // distance between waypoints

        ImGui::TableSetupColumn("Settings", ImGuiTableColumnFlags_None, 70.0f); // will open modal window that will display lat/lon + notes
        ImGui::TableSetupColumn("Marker", ImGuiTableColumnFlags_None, 30.0f); // v3.0.303.2 Display Marker at leg location
        ImGui::TableSetupColumn("IG", ImGuiTableColumnFlags_None, 30.0f); // ignore the waypoint during mission build
        ImGui::TableHeadersRow();
      }
      ImGui::PopStyleColor();


      //// Add ROWS to the table
      for (auto& [indx, legData] : in_map_tableOfParsedFpln)
      {
        int  i1                  = 0;
        bool flagDisableRowColor = false;
        if (legData.flag_ignore_leg)
        {
          flagDisableRowColor = true;
          ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_ScrollbarGrabActive)); // dark gray
        }


        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(i1); // 0
        ImGui::TextUnformatted(mxUtils::formatNumber<int>(indx).c_str());

        ++i1;
        ImGui::TableSetColumnIndex(i1); // 1 keyName (ident)

        if (flagDisableRowColor == false)
        {
          ImGui::PushStyleColor(ImGuiCol_Text, missionx::color::color_vec4_bisque);
        }
        if (legData.name.empty())
          ImGui::Text("%s", legData.ident.c_str());
        else
          ImGui::Text("%s", (std::string(legData.name) + ((legData.ident.compare(legData.name) == 0) ? "" : "(" + legData.ident + ")")).c_str());

        if (flagDisableRowColor == false)
          ImGui::PopStyleColor();

        ++i1;
        ImGui::TableSetColumnIndex(i1); // 2 type
        ImGui::Text("%s", legData.type.c_str());

        ++i1;
        ImGui::TableSetColumnIndex(i1); // 3 is leg checkbox
        if (legData.indx > 0 && !legData.flag_convertToBriefer) // briefer won't be a leg
        {
          char buf[64];
          snprintf(buf, sizeof (buf) - 1, "###isLeg%i", legData.indx);
          if (ImGui::Checkbox(buf, &legData.flag_isLeg))
            this->strct_conv_layer.flag_refresh_table_from_file = true;

          this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Is this waypoint a location you must reach ?\nIf not then it will be a waypoint in your GPS (optional).\nTip: In most cases first two rows are probably briefer and starting locations.");
        }

        ++i1;
        ImGui::TableSetColumnIndex(i1); // 4 on Ground / Airborne
        if (legData.indx > 0 && !legData.flag_convertToBriefer) // skip briefer
        {
          char buf[64];
          snprintf(buf, sizeof (buf) - 1, "###onGroundOrAirborne%i", legData.indx);
          if (ImGui::Checkbox(buf, &legData.target_trig_strct.flag_on_ground))
            legData.target_trig_strct.elev_rule_s = (legData.target_trig_strct.flag_on_ground) ? "true" : "false";
          this->mx_add_tooltip(missionx::color::color_vec4_yellow, "On Ground");
        }

        if (this->strct_conv_layer.flag_foundBriefer_index0 == false) // convert to briefer Hide Show Column
        {
          ++i1;
          ImGui::TableSetColumnIndex(i1); // convert to briefer - depends on >strct_conv_layer.flag_foundBriefer_index0
          if (legData.indx == 1) // skip briefer
          {
            char buf[64];
            snprintf(buf, sizeof (buf) - 1, "###ConvertToBriefer%i", legData.indx);
            if (ImGui::Checkbox(buf, &legData.flag_convertToBriefer))
              this->strct_conv_layer.flag_refresh_table_from_file = true;

            this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Convert Leg to Briefer");
          }
        }


        ++i1;
        ImGui::TableSetColumnIndex(i1); // lat/lon (alt)
        {
          char buf[64];
          char bufLatLon[30];

          snprintf(buf, sizeof (buf) - 1, "###lnmLatLon%i", legData.indx);
          snprintf(bufLatLon, sizeof (bufLatLon) - 1, "%.6f/%.6f", static_cast<float>(legData.p.getLat()), static_cast<float>(legData.p.getLon()));

          ImGui::SetNextItemWidth(180.0f);
          if (legData.indx % 2 == 0)
            ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0x20, 0x20, 0x20, 0xff));
          else
            ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0x30, 0x30, 0x30, 0xff));

          ImGui::InputText(buf, bufLatLon, sizeof (bufLatLon), ImGuiInputTextFlags_ReadOnly);
          ImGui::PopStyleColor();
        }

        ++i1;
        ImGui::TableSetColumnIndex(i1); // distance
        {
          ImGui::TextColored(missionx::color::color_vec4_bisque, "%.2f (%.2f)", legData.distToPrevWaypoint, legData.cumulativeDist);
        }

        ++i1;
        ImGui::TableSetColumnIndex(i1); // popup - details
        if (flagDisableRowColor == false)
        {
          if (legData.indx == 0 || legData.flag_isLeg || legData.flag_convertToBriefer)
          {
            char buf[64];
            if (legData.indx == 0 || (legData.indx == 1 && legData.flag_convertToBriefer)) // briefer row
              snprintf(buf, sizeof (buf) - 1, "Briefer###LegSetting%i", legData.indx);
            else
              snprintf(buf, sizeof (buf) - 1, "Flight Leg###LegSetting%i", legData.indx);


            if (ImGui::Button(buf))
            {
              this->strct_conv_layer.way_row_picked_i = legData.indx;

              if (legData.indx == 0 || (legData.indx == 1 && legData.flag_convertToBriefer)) // briefer popup
                ImGui::OpenPopup(POPUP_BRIEFER_SETTINGS.c_str());
              else
              {
                this->strct_conv_layer.flag_refreshTriggerListFrom_xNode = true;
                ImGui::OpenPopup(POPUP_FLIGHT_LEG_SETTINGS.c_str());
              }
            } // button
          } // end if to display button
        } // ignore


        ++i1; // v3.0.303.2
        ImGui::TableSetColumnIndex(i1); // 3D marker
        if (legData.indx && legData.flag_isLeg && !legData.flag_convertToBriefer && !legData.flag_ignore_leg) // Show option only for picked legs that are not briefer or ignored
        {
          char buf[64];
          snprintf(buf, sizeof (buf) - 1, "###add3DMarker%i", legData.indx);

          ImGui::Checkbox(buf, &legData.flag_add_marker);
          this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Place 3D marker at leg location.");
        }


        ++i1;
        ImGui::TableSetColumnIndex(i1); // Checkbox: Ignore waypoint line when build mission file
        if (legData.indx && legData.flag_convertToBriefer == false) // briefer won't be a leg
        {
          char buf[64];
          snprintf(buf, sizeof (buf) - 1, "###ignoreWaypoint%i", legData.indx);

          ImGui::Checkbox(buf, &legData.flag_ignore_leg);
          this->mx_add_tooltip(missionx::color::color_vec4_yellow, "Ignore this waypoint when building the mission file?");
        }
        //// End of table columns


        ///// FLIGHT LEG DETAIL POPUP
        ImGui::PushStyleColor(ImGuiCol_PopupBg, missionx::color::color_vec4_blue);
        {
          const auto popupHeight_f = ImGui::GetIO().DisplaySize.y * 0.85f;

          const ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f - 30.0f);
          ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
          ImGui::SetNextWindowSize(ImVec2(win_size_vec2.x - 20.0f, popupHeight_f));

          if (ImGui::BeginPopupModal(POPUP_FLIGHT_LEG_SETTINGS.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
          {
            draw_conv_popup_flight_leg_detail(legData);
            ImGui::EndPopup();
          }

          ///// FLIGHT LEG DETAIL POPUP
          ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
          ImGui::SetNextWindowSize(ImVec2(win_size_vec2.x - 20.0f, popupHeight_f));

          if (ImGui::BeginPopupModal(POPUP_BRIEFER_SETTINGS.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
          {
            draw_conv_popup_briefer(legData);
            ImGui::EndPopup();
          }
        } // end popup block code handling
        ImGui::PopStyleColor();


        if (flagDisableRowColor) // if we ignored line then reset style
          ImGui::PopStyleColor();
      } // end loop over table rows

      if (this->strct_conv_layer.flag_refresh_table_from_file)
      {
        auto table_ptr = ImGui::GetCurrentTable();
        ImGui::TableSetColumnWidthAutoAll(table_ptr);
        this->strct_conv_layer.flag_refresh_table_from_file = false;
      }
      // End of table
      ImGui::EndTable();
    } // END ImGui::BeginTable
  }
  ImGui::PopStyleColor(2);
}

// ----------------------------


bool ui_conv_screen::validate_conversion_table(IXMLNode& inMainNode, std::map<int, missionx::mx_local_fpln_strct> in_map_tableOfParsedFpln)
{
  // check if we have at least one flight leg, if not then the last one will be the active flight plan
  bool flag_found_active_flight_leg{ false };
  for (auto &[indx, legData] : in_map_tableOfParsedFpln)
  {
    if (indx == 0) // ignore briefer
      continue;

    if (legData.flag_ignore_leg)
      continue;

    if (legData.flag_isLeg)
      flag_found_active_flight_leg = true;
  }
  if (flag_found_active_flight_leg == false)
  {
    this->set_bottom_message_line1 ("Could not find any active flight leg. Flag at least one waypoint as a \"Leg\"", 8);
    return false;
  }

  return true;
}

// ----------------------------

void ui_conv_screen::subDraw_ui_xTrigger_main(missionx::mx_local_fpln_strct& inLegData, bool& in_out_needRefresh_b, int inLegIndex, std::map<int, missionx::mx_trig_strct_>& inMapOfGlobalTriggers, std::vector<std::string>& inVecGlobalTriggers_names)
{
  static mx_trig_strct_ *trig_ptr = nullptr;

  assert (inLegData.xTriggers.isEmpty () == false && "[subDraw_ui_xTrigger_main] Leg triggers can't be empty");

  // parse xTriggers
  if (in_out_needRefresh_b)
  {
    trig_ptr                             = nullptr; // static pointer reset
    int index                            = 0;
    this->strct_conv_layer.trig_picked_i = -1; // no pick
    in_out_needRefresh_b                 = false;
    inMapOfGlobalTriggers.clear ();
    inVecGlobalTriggers_names.clear ();
    for (int i1 = 0; i1 < inLegData.xTriggers.nChildNode (mxconst::get_ELEMENT_TRIGGER ().c_str ()); ++i1)
    {
      missionx::mx_trig_strct_ trig;
      auto                     nodeTrig = inLegData.xTriggers.getChildNode (mxconst::get_ELEMENT_TRIGGER ().c_str (), i1);
      trig.trig_name_s                  = Utils::readAttrib (nodeTrig, mxconst::get_ATTRIB_NAME (), "");
      trig.trig_type_s                  = Utils::readAttrib (nodeTrig, mxconst::get_ATTRIB_TYPE (), "");
      trig.trig_onGround_s              = Utils::readAttrib (nodeTrig, mxconst::get_ATTRIB_PLANE_ON_GROUND (), "");

      if (trig.trig_name_s.empty ())
      {
        Log::logMsg ("Found trigger without name. skipping");
        continue;
      }
      else if (Utils::isElementExistsInVec (inVecGlobalTriggers_names, trig.trig_name_s))
      {
        Log::logMsg ("Trigger by the name: " + trig.trig_name_s + " is already exists. Skipping...");
        continue;
      }

      if (mxconst::get_TRIG_TYPE_RAD ().compare (trig.trig_type_s) == 0)
        trig.trig_type_indx = 0;
      else if (mxconst::get_TRIG_TYPE_SCRIPT ().compare (trig.trig_type_s) == 0)
        trig.trig_type_indx = 1;
      else if (mxconst::get_TRIG_TYPE_POLY ().compare (trig.trig_type_s) == 0)
        trig.trig_type_indx = 2;
      else if (mxconst::get_TRIG_TYPE_CAMERA ().compare (trig.trig_type_s) == 0) // v3.0.303.7 fix unsupported camera type trigger
        trig.trig_type_indx = 3;
      else
      {
        Log::logMsg ("Reading tirgger: " + trig.trig_name_s + " has unsupporte trigger type, by this UI screen. It might be supported by the plugin though. Check designer guide. Skipping...");
        continue;
      }


      trig.flag_first_point_is_center_cbox = Utils::readBoolAttrib (nodeTrig, mxconst::get_ATTRIB_FIRST_POINT_IS_CENTER_B (), false); // v3.0.303.4 // boolean


      trig.indx     = index;
      trig.node_ptr = nodeTrig;
      inVecGlobalTriggers_names.emplace_back (trig.trig_name_s);
      Utils::addElementToMap (inMapOfGlobalTriggers, index, trig);
      ++index;

    } // end loop over all triggers

  } // end re-read xNode information for <trigger>

  //////////// Side by Side

  static std::string suggested_name;
  // Child 1: no border, enable horizontal scrollbar
  {
    ////// Trigger information
    if (this->strct_conv_layer.trig_ui_mode == mxTrig_ui_mode_enm::naTrigger)
      ImGui::TextColored (missionx::color::color_vec4_yellow, "[opt] Add custom Events/Triggers");
    else
      ImGui::TextColored (missionx::color::color_vec4_yellow, "Trigger Information: ");

    ImGui::BeginChild ("left_ChildListTriggers", ImVec2 (ImGui::GetContentRegionAvail ().x * 0.22f, 100.0f), ImGuiChildFlags_None, ImGuiWindowFlags_None); // v24.06.1 replaced "GetWindowContentRegionMax()" with "GetContentRegionAvail()"
    {
      //// LIST
      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgrey);
      ImGui::PushStyleColor (ImGuiCol_ButtonActive, missionx::color::color_vec4_grey);
      ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
      {
        //// Show Add / Cancel addition of new trigger
        if (this->strct_conv_layer.trig_ui_mode > mxTrig_ui_mode_enm::naTrigger)
        {
          if (ImGui::Button (" Cancel Operation "))
          {
            if (trig_ptr)
            {
              if (trig_ptr->node_ptr.isEmpty () == false && trig_ptr->copyOfNode_ptr.isEmpty () == false)
              {
                if (this->strct_conv_layer.trig_ui_mode == mxTrig_ui_mode_enm::newTrigger)
                {
                  trig_ptr->node_ptr.deleteNodeContent ();
                  trig_ptr->init ();
                }
                else if (this->strct_conv_layer.trig_ui_mode == mxTrig_ui_mode_enm::editTrigger)
                {
                  auto pNode = trig_ptr->node_ptr.getParentNode ();
                  if (!pNode.isEmpty ())
                  {
                    trig_ptr->node_ptr.deleteNodeContent ();
                    trig_ptr->node_ptr = trig_ptr->copyOfNode_ptr.deepCopy ();
                    pNode.addChild (trig_ptr->node_ptr, trig_ptr->indx);
                  }

                } // end cancel operation
              }
              in_out_needRefresh_b = true; // refresh after cancel
            }

            this->strct_conv_layer.trig_ui_mode = mxTrig_ui_mode_enm::naTrigger;
          }
        }
        else if (this->strct_conv_layer.trig_ui_mode == mxTrig_ui_mode_enm::naTrigger)
        {
          if (ImGui::Button (" +++ Add Trigger +++"))
          {
            this->strct_conv_layer.trig_ui_mode = mxTrig_ui_mode_enm::newTrigger;
            this->strct_conv_layer.trigger.init ();
            this->strct_conv_layer.trigger.node_ptr = Utils::xml_get_node_from_XSD_map_as_a_copy (mxconst::get_ELEMENT_TRIGGER ());
            this->strct_conv_layer.trigger.indx     = this->strct_conv_layer.trig_seq;
            // v3.0.303.7
            auto              xLocElevData = Utils::xml_get_or_create_node_ptr (this->strct_conv_layer.trigger.node_ptr, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA ());
            auto              xRadius      = Utils::xml_get_or_create_node_ptr (xLocElevData, mxconst::get_ELEMENT_RADIUS ());
            int               val_i        = Utils::readNodeNumericAttrib<int> (xRadius, mxconst::get_ATTRIB_LENGTH_MT (), missionx::MIN_RAD_UI_VALUE_MT);
            const std::string val_s        = mxUtils::formatNumber<int> (val_i);
            xRadius.updateAttribute (val_s.c_str (), mxconst::get_ATTRIB_LENGTH_MT ().c_str (), mxconst::get_ATTRIB_LENGTH_MT ().c_str ());
            // end v3.0.303.7

            trig_ptr                 = &this->strct_conv_layer.trigger;
            trig_ptr->copyOfNode_ptr = trig_ptr->node_ptr.deepCopy (); // store a copy to revert to
            ++this->strct_conv_layer.trig_seq;

            suggested_name = "trig_" + mxUtils::formatNumber<int> (inLegIndex) + "_" + mxUtils::formatNumber<int> (this->strct_conv_layer.trig_seq) + "_" + Utils::get_hash_string (Utils::get_time_as_string ());

            (*trig_ptr).node_ptr.updateAttribute (suggested_name.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
          }
        }
      } // end style colors
      ImGui::PopStyleColor (4);

      // Display list of triggers
      bool vDisableCombo = false;
      if (this->strct_conv_layer.trig_ui_mode != mxTrig_ui_mode_enm::naTrigger) // newTrigger / editTrigger
      {
        ImGui::BeginDisabled ();
        vDisableCombo = true;
      }

      ImGui::SetNextItemWidth (ImGui::GetContentRegionAvail ().x);
      if (ImGui::BeginListBox ("##ListOfTriggerNames"))
      {
        for (auto &[indx, data] : inMapOfGlobalTriggers)
        {
          const bool is_selected = (this->strct_conv_layer.trig_picked_i == indx);
          if (ImGui::Selectable (data.trig_name_s.c_str (), is_selected))
          {

            this->strct_conv_layer.trig_picked_i = indx;
            trig_ptr                             = &this->strct_conv_layer.mapOfGlobalTriggers[indx];
            trig_ptr->copyOfNode_ptr             = trig_ptr->node_ptr.deepCopy (); // store a copy to revert to

            this->strct_conv_layer.trig_ui_mode = mxTrig_ui_mode_enm::editTrigger;
            suggested_name                      = trig_ptr->trig_name_s;
          }
          // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
          if (is_selected)
            ImGui::SetItemDefaultFocus ();
        }
        ImGui::EndListBox ();
      } // end ListBox

      if (vDisableCombo)
        ImGui::EndDisabled ();
    }
    ImGui::EndChild ();
  }

  // Child 2:
  if (trig_ptr != nullptr && this->strct_conv_layer.trig_ui_mode > mxTrig_ui_mode_enm::naTrigger)
  {
    subDraw_ui_xTrigger_detail ((*trig_ptr), in_out_needRefresh_b, suggested_name, inLegData);
  }

}

// ----------------------------


void
ui_conv_screen::subDraw_ui_xTrigger_detail (mx_trig_strct_ &inTrig_ptr, bool &in_out_needRefresh_b, std::string &suggested_name, missionx::mx_local_fpln_strct &inLegData)
{
    bool                   flag_create_trigger = false; // used with the button "create/update" trigger
  const std::string_view btnStoreLabel_sv    = (this->strct_conv_layer.trig_ui_mode == mxTrig_ui_mode_enm::newTrigger) ? "Create Trigger##CreateOrUpdateTriggerButton" : "Update Trigger##CreateOrUpdateTriggerButton";

  const auto lmbda_update_trig_type = [&] ()
  {
    inTrig_ptr.trig_type_s = std::string (this->strct_conv_layer.vecTrigType_list.at (inTrig_ptr.trig_type_indx)); // debug
    inTrig_ptr.node_ptr.updateAttribute (this->strct_conv_layer.vecTrigType_list_trans.at (inTrig_ptr.trig_type_indx), mxconst::get_ATTRIB_TYPE ().c_str (), mxconst::get_ATTRIB_TYPE ().c_str ());

    inTrig_ptr.node_ptr.updateAttribute (mxUtils::formatNumber<int> (inTrig_ptr.trig_type_indx).c_str (), mxconst::get_CONV_ATTRIB_trig_ui_type_combo_indx ().c_str (), mxconst::get_CONV_ATTRIB_trig_ui_type_combo_indx ().c_str ());
  };

  inTrig_ptr.iCurrentBuf = 0;

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
  ImGui::SameLine ();

  ImGui::BeginChild ("right_ChildTriggerDetails", ImVec2 (ImGui::GetContentRegionAvail ().x - 10.0f, 300.0f), ImGuiChildFlags_None, window_flags); // v24.06.1 replaced: "GetContentRegionMax()" with "GetContentRegionAvail()"
  {

    // draw trigger element
    if (this->strct_conv_layer.trig_ui_mode == mxTrig_ui_mode_enm::editTrigger)
    {
      ImGui::PushStyleColor (ImGuiCol_FrameBg, missionx::color::color_vec4_maroon);

      IXMLRenderer      xmlRenderer;
      const std::string print_s = xmlRenderer.getString (inTrig_ptr.node_ptr);
      char              print_buff[2048]; // { 0 };
//#ifdef IBM
//      memcpy_s (print_buff, sizeof (print_buff), print_s.c_str (), (print_s.length () > sizeof (print_buff)) ? sizeof (print_buff) : print_s.length ());
//#else
//      memcpy (print_buff, print_s.c_str (), (print_s.length () > sizeof (print_buff)) ? sizeof (print_buff) : print_s.length ());
//#endif // IBM

      mxUtils::copy_string_to_buffer(print_s, print_buff[0], sizeof(print_buff));

      // readonly multi line input. We display 10 rows
      ImGui::InputTextMultiline ("###triggerXMLoutput", print_buff, sizeof (print_buff), ImVec2 (ImGui::GetContentRegionAvail ().x, ImGui::GetTextLineHeight () * 12.0f), ImGuiInputTextFlags_ReadOnly);

      ImGui::PopStyleColor ();

      // CREATE Trigger Button
      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgray);
      ImGui::PushStyleColor (ImGuiCol_ButtonActive, missionx::color::color_vec4_grey);
      ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
      if (ImGui::Button (btnStoreLabel_sv.data ()))
        flag_create_trigger = true;
      ImGui::PopStyleColor (4); // create trigger button


      ImGui::PushStyleColor (ImGuiCol_Separator, missionx::color::color_vec4_white);
      ImGui::Separator ();
      ImGui::PopStyleColor ();
    }


    ImGui::TextColored (missionx::color::color_vec4_yellow, "Type:");
    ImGui::SameLine ();
    ImGui::SetNextItemWidth (85.0f);
    if (ImGui::Combo ("##TriggerTypeCombo", &inTrig_ptr.trig_type_indx, this->strct_conv_layer.vecTrigType_list.data (), static_cast<int> (this->strct_conv_layer.vecTrigType_list.size ())))
    {
      lmbda_update_trig_type ();

      // v3.0.303.7 initialize RAD/Camera based trigger
      if (inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::rad) || inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::camera))
      {
        auto              xLocElevData = Utils::xml_get_or_create_node_ptr (inTrig_ptr.node_ptr, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA ());
        auto              xRadius      = Utils::xml_get_or_create_node_ptr (xLocElevData, mxconst::get_ELEMENT_RADIUS ());
        int               val_i        = Utils::readNodeNumericAttrib<int> (xRadius, mxconst::get_ATTRIB_LENGTH_MT (), missionx::MIN_RAD_UI_VALUE_MT);
        const std::string val_s        = mxUtils::formatNumber<int> (val_i);
        xRadius.updateAttribute (val_s.c_str (), mxconst::get_ATTRIB_LENGTH_MT ().c_str (), mxconst::get_ATTRIB_LENGTH_MT ().c_str ());
      }

      // clear buff[0] data
      inTrig_ptr.resetBuff (inTrig_ptr.iCurrentBuf);
    }

    // display radius widget if trigger type is rad
    if (inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::rad) || inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::camera))
    {
      auto xLocElevData = Utils::xml_get_or_create_node_ptr (inTrig_ptr.node_ptr, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA ());
      auto xRadius      = Utils::xml_get_or_create_node_ptr (xLocElevData, mxconst::get_ELEMENT_RADIUS ());
      ImGui::SameLine ();
      subDraw_ui_xRadius (xRadius, 15);
    }

    // Constraint // no buff
    if (inTrig_ptr.trig_type_indx != static_cast<int> (missionx::mx_trig_type_enum::script) && inTrig_ptr.trig_type_indx != static_cast<int> (missionx::mx_trig_type_enum::camera)) // if not camera then no elevation info is needed, will always be ignored
    {
      ImGui::TextColored (missionx::color::color_vec4_yellow, "Elev ft:");
      ImGui::SameLine ();
      ImGui::SetNextItemWidth (100.0f);
      if (ImGui::Combo ("###ConstraintPosition", &inTrig_ptr.trig_plane_pos_combo_indx, this->strct_conv_layer.vecTrigOnGround_list.data (), static_cast<int> (this->strct_conv_layer.vecTrigOnGround_list.size ())))
      {
        inTrig_ptr.trig_onGround_s = std::string (this->strct_conv_layer.vecTrigOnGround_list.at (inTrig_ptr.trig_plane_pos_combo_indx)); // debug
        Utils::xml_search_and_set_attribute_in_IXMLNode (inTrig_ptr.node_ptr, mxconst::get_ATTRIB_PLANE_ON_GROUND (), this->strct_conv_layer.vecTrigOnGround_list_trans.at (inTrig_ptr.trig_plane_pos_combo_indx), mxconst::get_ELEMENT_CONDITIONS ());
        Utils::xml_search_and_set_attribute_in_IXMLNode (inTrig_ptr.node_ptr, mxconst::get_CONV_ATTRIB_trig_ui_plane_pos_combo_indx (), mxUtils::formatNumber<int> (inTrig_ptr.trig_plane_pos_combo_indx), inTrig_ptr.node_ptr.getName ());
      }
      if (inTrig_ptr.trig_plane_pos_combo_indx == 2) // 2 = airborne
      {
        ImGui::SameLine (0.0f, 10.0f);
        auto xLocElevData = Utils::xml_get_or_create_node_ptr (inTrig_ptr.node_ptr, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA ());
        auto xElevVolume  = Utils::xml_get_or_create_node_ptr (xLocElevData, mxconst::get_ELEMENT_ELEVATION_VOLUME ());
        subDraw_ui_xTrigger_elev (inTrig_ptr, xElevVolume);
      }
    }

    // lat/lon  // use buff
    if (inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::rad) || inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::camera))
    {
      if (this->strct_conv_layer.trig_ui_mode == mxTrig_ui_mode_enm::newTrigger)
        ImGui::TextColored (missionx::color::color_vec4_yellow, "Enter center of event:");
      else
        ImGui::TextColored (missionx::color::color_vec4_yellow, "Update center of event: "); // mxTrig_ui_mode_enm::editTrigger

      ImGui::InputText ("###trigCenterPosition", inTrig_ptr.buffArray[inTrig_ptr.iCurrentBuf], sizeof (inTrig_ptr.buffArray[inTrig_ptr.iCurrentBuf]), ImGuiInputTextFlags_ReadOnly); // buffArray  // we will store the points only when clicking apply


      //// position // use buff based on rad
      ImGui::SameLine ();
      if (ImGui::Button ("P##pTrig"))
      {
        inTrig_ptr.setBuff (inTrig_ptr.iCurrentBuf, missionx::data_manager::getPoint_as_stringFromPlaneCamera ("sim/flightmodel/position/latitude", "sim/flightmodel/position/longitude"));
      }
      this->mx_add_tooltip (missionx::color::color_vec4_darkorange, "plane position");

      ImGui::SameLine ();
      if (ImGui::Button ("C##cTrig"))
      {
        inTrig_ptr.setBuff (inTrig_ptr.iCurrentBuf, missionx::data_manager::getPoint_as_stringFromPlaneCamera ("sim/graphics/view/view_x", "sim/graphics/view/view_y", 'c')); // camera pos
      }
      ImGui::SameLine (0.0f, 5.0f);
      this->mx_add_tooltip (missionx::color::color_vec4_darkorange, "camera position");

      ImGui::SameLine ();
      if (ImGui::Button ("U##uTrig"))
      {
        ImGui::OpenPopup (POPUP_USER_LAT_LON.c_str ());
      }
      ImGui::SameLine (0.0f, 5.0f);
      this->mx_add_tooltip (missionx::color::color_vec4_darkorange, "User defined position");

      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgrey);
      ImGui::PushStyleColor (ImGuiCol_ButtonActive, missionx::color::color_vec4_grey);
      ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
      if (ImGui::Button ("Apply Position##applyToLocAndElev"))
      {
        auto loc_and_elev_ptr = inTrig_ptr.node_ptr.getChildNode (mxconst::get_ELEMENT_LOC_AND_ELEV_DATA ().c_str ());
        Utils::xml_delete_all_subnodes (loc_and_elev_ptr, mxconst::get_ELEMENT_POINT ());

        std::string val_buff_s = inTrig_ptr.getBuff (inTrig_ptr.iCurrentBuf);
        auto        childNode  = Utils::xml_create_node_from_string (val_buff_s);

        if (childNode.isEmpty ())
          this->set_bottom_message_line1 ("Point is not valid: " + inTrig_ptr.getBuff (inTrig_ptr.iCurrentBuf), 10);
        else
        {
          loc_and_elev_ptr.addChild (childNode);
          this->set_bottom_message_line1 ("", 1);
        }
      }
      ImGui::PopStyleColor (4);

      ImGui::PushStyleColor (ImGuiCol_ChildBg, missionx::color::color_vec4_black);
      if (ImGui::BeginPopupModal (POPUP_USER_LAT_LON.c_str (), NULL, ImGuiWindowFlags_AlwaysAutoResize))
      {
        subDraw_popup_user_lat_lon (inTrig_ptr);
        ImGui::EndPopup ();
      }
      ImGui::PopStyleColor ();

      ++inTrig_ptr.iCurrentBuf; // if we display positioning we should increment the buff array position too

      // End positioning of trigger
    }
    else if (inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::poly)) // v3.0.301 B3
    {
      auto xLocAndElev = Utils::xml_get_or_create_node_ptr (inTrig_ptr.node_ptr, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA ());

      ImGui::PushStyleColor (ImGuiCol_Separator, missionx::color::color_vec4_white);
      ImGui::Separator ();
      ImGui::PopStyleColor ();

      subDraw_ui_xPolyBox (xLocAndElev, inTrig_ptr);
      ++inTrig_ptr.iCurrentBuf;
    }
    else if (inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::script)) // v3.0.301 B3
    {
      ImGui::PushStyleColor (ImGuiCol_Separator, missionx::color::color_vec4_white);
      ImGui::Separator ();
      ImGui::PopStyleColor ();


      subDraw_ui_xScriptlet (inLegData.xFlightPlan, this->strct_conv_layer.trig_ui_mode, &inTrig_ptr, "Enter the trigger's condition <scriptlet> code to fire the trigger:", &inLegData, inTrig_ptr.buffArray[inTrig_ptr.iCurrentBuf]);
      ++inTrig_ptr.iCurrentBuf;
    }



    ImGui::PushStyleColor (ImGuiCol_Separator, missionx::color::color_vec4_white);
    ImGui::Separator ();
    ImGui::PopStyleColor ();
    // Outcome button + popup // use buff

    // calculate position
    ImGui::NewLine ();
    ImGui::SameLine (0.0, ImGui::GetContentRegionAvail ().x * 0.33f);

    ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
    ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgrey);
    ImGui::PushStyleColor (ImGuiCol_ButtonActive, missionx::color::color_vec4_grey);
    ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
    if (ImGui::Button ("Trigger Outcome Settings##btnOutcomeTrigPopup"))
    {
      ImGui::OpenPopup (POPUP_TRIG_OUTCOME.c_str ());
    }
    ImGui::PopStyleColor (4);

    { // POPUP Outcome
      const auto   win_size_vec2 = ImGui::GetWindowSize ();
      const ImVec2 center (ImGui::GetIO ().DisplaySize.x * 0.5f, ImGui::GetIO ().DisplaySize.y * 0.5f);
      ImGui::SetNextWindowPos (center, ImGuiCond_Appearing, ImVec2 (0.5f, 0.5f));
      ImGui::SetNextWindowSize (ImVec2 (win_size_vec2.x * 0.85f, 220.0f));
      if (ImGui::BeginPopupModal (POPUP_TRIG_OUTCOME.c_str (), NULL, ImGuiWindowFlags_AlwaysAutoResize))
      {
        subDraw_popup_outcome (inTrig_ptr, inLegData.xMessageTmpl);
        ImGui::EndPopup ();
        ++inTrig_ptr.iCurrentBuf;
      }
    }


    // draw trigger element
    if (this->strct_conv_layer.trig_ui_mode != mxTrig_ui_mode_enm::editTrigger)
    {

      ImGui::PushStyleColor (ImGuiCol_Separator, missionx::color::color_vec4_white);
      ImGui::Separator ();
      ImGui::PopStyleColor ();

      // CREATE Trigger Button
      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgray);
      ImGui::PushStyleColor (ImGuiCol_ButtonActive, missionx::color::color_vec4_grey);
      ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
      if (ImGui::Button (btnStoreLabel_sv.data ()))
        flag_create_trigger = true;
      ImGui::PopStyleColor (4); // create trigger button



      IXMLRenderer      xmlRenderer;
      const std::string print_s = xmlRenderer.getString (inTrig_ptr.node_ptr);
      char              print_buff[2048]; // { 0 };
//#ifdef IBM
//      memcpy_s (print_buff, sizeof (print_buff), print_s.c_str (), (print_s.length () > sizeof (print_buff)) ? sizeof (print_buff) : print_s.length ());
//#else
//      memcpy (print_buff, print_s.c_str (), (print_s.length () > sizeof (print_buff)) ? sizeof (print_buff) : print_s.length ());
//#endif // IBM

      mxUtils::copy_string_to_buffer(print_s, print_buff[0], sizeof(print_buff));


      // readonly multi line input. We display 10 rows
      ImGui::InputTextMultiline ("###triggerXMLoutput", print_buff, sizeof (print_buff), ImVec2 (ImGui::GetContentRegionAvail ().x, ImGui::GetTextLineHeight () * 12.0f), ImGuiInputTextFlags_ReadOnly);
    }


  } // end Right Child
  ImGui::EndChild ();



  // Create/Update the trigger node
  if (flag_create_trigger)
  {
    bool flag_trig_is_valid = true;
    // do validation
    if (inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::rad) || inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::camera))
    {

      // v3.0.301 B3
      // remove the cond_script attribute from <conditions>, this should only be used with "script based trigger"
      auto xConditions = Utils::xml_get_or_create_node_ptr (inTrig_ptr.node_ptr, mxconst::get_ELEMENT_CONDITIONS ());
      xConditions.updateAttribute ("", mxconst::get_ATTRIB_COND_SCRIPT ().c_str (), mxconst::get_ATTRIB_COND_SCRIPT ().c_str ());

      auto   xLocElevData = Utils::xml_get_or_create_node_ptr (inTrig_ptr.node_ptr, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA ());
      auto   xPoint       = Utils::xml_get_or_create_node_ptr (xLocElevData, mxconst::get_ELEMENT_POINT ());
      double lat          = Utils::readNumericAttrib (xPoint, mxconst::get_ATTRIB_LAT (), 0.0f);
      double lon          = Utils::readNumericAttrib (xPoint, mxconst::get_ATTRIB_LONG (), 0.0f);
      if (lat == 0.0 || lon == 0.0)
      {
        flag_trig_is_valid = false;
        this->set_bottom_message_line1 ("Trigger is not valid. Position data is not valid", missionx::DEFAULT_MESSAGE_TIME_I);
      }

      // v3.0.304.4 validate radius value
      [[maybe_unused]] bool flag_found = false;
      if (Utils::xml_get_attribute_value_drill (xLocElevData, mxconst::get_ATTRIB_LENGTH_MT (), flag_found, mxconst::get_ELEMENT_RADIUS ()).empty ())
      {
        flag_trig_is_valid = false;
        this->set_bottom_message_line1 ("Trigger is not valid. Set Radius", missionx::DEFAULT_MESSAGE_TIME_I);
      }

      // v3.0.303.7 if trig type = camera then reset on_ground attribute to empty (which means ignore elevation), we might consider ---10 instead
      if (inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::camera))
        Utils::xml_search_and_set_attribute_in_IXMLNode (inTrig_ptr.node_ptr, mxconst::get_ATTRIB_PLANE_ON_GROUND (), "", mxconst::get_ELEMENT_CONDITIONS ());
    }
    else if (inTrig_ptr.trig_type_indx == static_cast<int> (missionx::mx_trig_type_enum::poly))
    {
      auto xLocAndElev = Utils::xml_get_or_create_node_ptr (inTrig_ptr.node_ptr, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA ());
      assert (xLocAndElev.isEmpty () == false && "<Trigger> must have <loc_and_elev_data> element.");

      if (xLocAndElev.nChildNode (mxconst::get_ELEMENT_POINT ().c_str ()) > 0)
      {
        flag_trig_is_valid = true;
      }
      else
      {
        this->set_bottom_message_line1 ("Trigger is not valid. Not enough <point> elements.", missionx::DEFAULT_MESSAGE_TIME_I);
        flag_trig_is_valid = false;
      }
    }
    else // v3.0.303.7 remove <loc_and_elev_data> since it is a script based
    {
      Utils::xml_delete_all_subnodes (inTrig_ptr.node_ptr, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA ());
    }


    if (flag_trig_is_valid)
    {
      lmbda_update_trig_type ();

      in_out_needRefresh_b = true;

      if (this->strct_conv_layer.trig_ui_mode == mxTrig_ui_mode_enm::newTrigger)
      {

        inLegData.xTriggers.addChild (inTrig_ptr.node_ptr.deepCopy ());
        if (inLegData.xLeg.isEmpty () == false)
        {
          auto xLink = inLegData.xLeg.addChild (mxconst::get_ELEMENT_LINK_TO_TRIGGER ().c_str ());
          xLink.updateAttribute (suggested_name.c_str (), mxconst::get_ATTRIB_NAME ().c_str (), mxconst::get_ATTRIB_NAME ().c_str ());

          this->set_bottom_message_line1 ("Added Trigger: " + suggested_name, 5);
        }
      }
      else if (this->strct_conv_layer.trig_ui_mode == mxTrig_ui_mode_enm::editTrigger)
      {
        this->set_bottom_message_line1 ("Updated Trigger: " + suggested_name, 5);
      }
      else
        this->set_bottom_message_line1 ("", 1);


      this->strct_conv_layer.trig_ui_mode = mxTrig_ui_mode_enm::naTrigger; // reset the trigger creation ui status
    } // finish save changes
  }
  // END CREATE TRIGGER NODE
}

// ----------------------------

void ui_conv_screen::subDraw_ui_xRadius(IXMLNode& node, int pad_x_i)
{
  int val_i = Utils::readNodeNumericAttrib<int> (node, mxconst::get_ATTRIB_LENGTH_MT (), missionx::MIN_RAD_UI_VALUE_MT);

  ImGui::SetNextItemWidth (ImGui::GetContentRegionAvail ().x - pad_x_i);

  ImGui::SetNextItemWidth (120.0f);
  if (ImGui::InputInt ("##TrigRadius", &val_i, 100))
  {
    if (val_i < missionx::MIN_RAD_UI_VALUE_MT)
      val_i = missionx::MIN_RAD_UI_VALUE_MT; // validate value
    if (val_i > missionx::MAX_RAD_UI_VALUE_MT)
      val_i = missionx::MAX_RAD_UI_VALUE_MT; // validate value

    const std::string val_s = mxUtils::formatNumber<int> (val_i);
    node.updateAttribute (val_s.c_str (), mxconst::get_ATTRIB_LENGTH_MT ().c_str (), mxconst::get_ATTRIB_LENGTH_MT ().c_str ());
  }

  this->mx_add_tooltip (missionx::color::color_vec4_darkorange, mxUtils::formatNumber<float> ((static_cast<float> (val_i) * missionx::meter2nm), 2) + " nm");

  ImGui::SameLine (0.0f, 3.0f);
  ImGui::TextColored (missionx::color::color_vec4_cyan, "meters");
}

// ----------------------------

void ui_conv_screen::subDraw_ui_xPolyBox(IXMLNode& pNode, mx_trig_strct_& inTrig_ptr)
{
    assert (pNode.isEmpty () == false && "[polyBox] Parent Node could not be empty");
  // p1 = trig_ptr.pos
  // p2-------------p3
  // |              |
  // p1-------------p4

  //        "*" = pCenter = center of box
  // p2-------------p3
  // |       *      |
  // p1-------------p4

  // store first Point as the starting calculation point to display to the user - if exists
  // Display starting lat/lon   // display heading of boxed trigger
  // Display Meters %heading    // display Meters %heading - 90.0
  // Apply button - will delete all existing <points> and then construct new ones

  static float heading_f{ 0.0f };
  static int   iMetersVector1{ 100 };
  static int   iMetersVector2{ 100 };
  static int   iVectorLengthBT_StoredInTrigger{ 0 };
  static int   iVectorLengthLR_StoredInTrigger{ 0 };
  IXMLNode     buttomLeft_or_center_xPoint_ptr = Utils::xml_get_or_create_node_ptr (pNode, mxconst::get_ELEMENT_POINT ()); // get or create first elemnt <point>, can represent the bottomLeft or center of the trigers triangle area

  inTrig_ptr.pos.setLat (Utils::readNumericAttrib (buttomLeft_or_center_xPoint_ptr, mxconst::get_ATTRIB_LAT (), 0.0));
  inTrig_ptr.pos.setLon (Utils::readNumericAttrib (buttomLeft_or_center_xPoint_ptr, mxconst::get_ATTRIB_LONG (), 0.0));
  inTrig_ptr.pos.setElevationFt (Utils::readNumericAttrib (buttomLeft_or_center_xPoint_ptr, mxconst::get_ATTRIB_ELEV_FT (), 0.0));
  inTrig_ptr.setBuff (inTrig_ptr.iCurrentBuf, inTrig_ptr.pos.get_point_lat_lon_as_string ());

  iVectorLengthBT_StoredInTrigger = Utils::readNodeNumericAttrib<int> (inTrig_ptr.node_ptr, mxconst::get_ATTRIB_VECTOR_BT_LENGTH_MT (), 0);
  iVectorLengthLR_StoredInTrigger = Utils::readNodeNumericAttrib<int> (inTrig_ptr.node_ptr, mxconst::get_ATTRIB_VECTOR_LR_LENGTH_MT (), 0);

  ImGui::Checkbox ("Plane in center of box", &inTrig_ptr.flag_first_point_is_center_cbox);



  // Position of Plane
  ImGui::TextColored (missionx::color::color_vec4_yellow, "Calculate the trigger's boundaries");
  if (inTrig_ptr.flag_first_point_is_center_cbox) // v3.0.301 B4
    ImGui::TextColored (missionx::color::color_vec4_yellow, "Center of trigger area:");
  else
    ImGui::TextColored (missionx::color::color_vec4_yellow, "Bottom Left Position: ");

  ImGui::SameLine ();
  ImGui::InputText ("###trigBoxPolyBottomLeftPos", inTrig_ptr.buffArray[inTrig_ptr.iCurrentBuf], sizeof (inTrig_ptr.buffArray[inTrig_ptr.iCurrentBuf]), ImGuiInputTextFlags_ReadOnly);
  ImGui::SameLine ();
  if (ImGui::Button ("C##cBoxTrig"))
  {
    inTrig_ptr.pos = missionx::data_manager::getPlane_or_Camera_position_as_Point ('c'); // c = camera
    inTrig_ptr.setBuff (inTrig_ptr.iCurrentBuf, inTrig_ptr.pos.get_point_lat_lon_as_string ()); // camera pos
    if (buttomLeft_or_center_xPoint_ptr.isEmpty () == false)
    {
      buttomLeft_or_center_xPoint_ptr.updateAttribute (inTrig_ptr.pos.getLat_s ().c_str (), mxconst::get_ATTRIB_LAT ().c_str (), mxconst::get_ATTRIB_LAT ().c_str ());
      buttomLeft_or_center_xPoint_ptr.updateAttribute (inTrig_ptr.pos.getLon_s ().c_str (), mxconst::get_ATTRIB_LONG ().c_str (), mxconst::get_ATTRIB_LONG ().c_str ());
      buttomLeft_or_center_xPoint_ptr.updateAttribute (inTrig_ptr.pos.getElevFt_s ().c_str (), mxconst::get_ATTRIB_ELEV_FT ().c_str (), mxconst::get_ATTRIB_ELEV_FT ().c_str ());
    }
    if (inTrig_ptr.flag_first_point_is_center_cbox)
    {
      inTrig_ptr.node_ptr.updateAttribute (inTrig_ptr.pos.getLat_s ().c_str (), mxconst::get_ATTRIB_LAT ().c_str (), mxconst::get_ATTRIB_LAT ().c_str ());
      inTrig_ptr.node_ptr.updateAttribute (inTrig_ptr.pos.getLon_s ().c_str (), mxconst::get_ATTRIB_LONG ().c_str (), mxconst::get_ATTRIB_LONG ().c_str ());
    }
  }
  this->mx_add_tooltip (missionx::color::color_vec4_white, "Get Camera position.");

  // heading

  ImGui::TextColored (missionx::color::color_vec4_orange, "First Vector Heading:");
  this->mx_add_tooltip (missionx::color::color_vec4_yellow, "The plugin will calculate the box as Heading and Heading + 90 deg vectors");
  ImGui::SameLine ();
  ImGui::SetNextItemWidth (110.0f);
  if (ImGui::InputFloat ("###trigBoxVectorHeading", &heading_f))
  {
    if (heading_f < 0.0f)
      heading_f = 359.0f;
    if (heading_f > 359.0f)
      heading_f = 0.0f;
  }

  ImGui::SameLine ();
  if (ImGui::Button ("C##trigBoxCameraHading"))
  {
    const XPLMDataRef dref = XPLMFindDataRef ("sim/graphics/view/view_heading"); // camera heading
    heading_f              = XPLMGetDataf (dref);
    inTrig_ptr.setBuff (inTrig_ptr.iCurrentBuf, mxUtils::formatNumber<float> (heading_f, 2));
  }
  this->mx_add_tooltip (missionx::color::color_vec4_white, "Get Camera Heading.");
  ImGui::SameLine (0.0f, 10.0f);
  ImGui::TextColored (missionx::color::color_vec4_white, "Second vector heading = +90deg = %.2fdeg", (heading_f + 90.0f > 359.0f) ? heading_f + 90.0f - 360.0f : heading_f + 90.0f);

  // Vector Length
  if (inTrig_ptr.flag_first_point_is_center_cbox) // v3.0.301 B4
    ImGui::TextColored (missionx::color::color_vec4_antiquewhite, "Final length will be twice the vector's entered numbers.");

  ImGui::TextColored (missionx::color::color_vec4_orange, "First Vector Length (mt):");
  ImGui::SameLine ();
  ImGui::SetNextItemWidth (80.0f);
  if (ImGui::InputInt ("###trigBoxVectorLength1", &iMetersVector1, 25, ImGuiInputTextFlags_ReadOnly))
  {
    if (iMetersVector1 < 5)
      iMetersVector1 = 5;
    if (iMetersVector1 > 90000)
      iMetersVector1 = 90000;
  }
  if (inTrig_ptr.flag_first_point_is_center_cbox) // v3.0.301 B4
    this->mx_add_tooltip (missionx::color::color_vec4_darkorange, "Length = 2 * " + mxUtils::formatNumber<float> ((static_cast<float> (iMetersVector1) * missionx::meter2nm), 2) + " nm");
  else
    this->mx_add_tooltip (missionx::color::color_vec4_darkorange, mxUtils::formatNumber<float> ((static_cast<float> (iMetersVector1) * missionx::meter2nm), 2) + " nm");

  ImGui::SameLine (0.0f, 10.0f);

  ImGui::TextColored (missionx::color::color_vec4_orange, "Second Vector Length (mt):");
  ImGui::SameLine ();
  ImGui::SetNextItemWidth (80.0f);
  if (ImGui::InputInt ("###trigBoxVectorLength2", &iMetersVector2, 25, ImGuiInputTextFlags_ReadOnly))
  {
    if (iMetersVector2 < 5)
      iMetersVector2 = 5;
    if (iMetersVector2 > 90000)
      iMetersVector2 = 90000;
  }
  this->mx_add_tooltip (missionx::color::color_vec4_darkorange, mxUtils::formatNumber<float> ((static_cast<float> (iMetersVector2) * missionx::meter2nm), 2) + " nm");

  ImGui::SameLine (0.0f, 10.0f);
  if (ImGui::Button ("C##distanceRelativeToPlane"))
  {
    if (inTrig_ptr.pos.getLat () == 0.0 || inTrig_ptr.pos.getLon () == 0.0)
    {
      this->set_bottom_message_line1 ("Your starting position is not valid.", 10);
    }
    else
    {
      Point plane    = missionx::data_manager::getPlane_or_Camera_position_as_Point ('c'); // p = plane
      iMetersVector2 = static_cast<int> (plane.calcDistanceBetween2Points (inTrig_ptr.pos, missionx::mx_units_of_measure::meter));
    }
  }
  this->mx_add_tooltip (missionx::color::color_vec4_darkorange, "Calculate Second vector distance relative to camera location (camera.pos - start.pos)\nUse camera in plane for easier positioning using the map.");

  // display original vector lengths
  if (iVectorLengthBT_StoredInTrigger)
  {
    ImGui::TextColored (missionx::color::color_vec4_cyan, "Current Trigger stored vector Lengths are: %i and %i respectively", iVectorLengthBT_StoredInTrigger, iVectorLengthLR_StoredInTrigger);
  }

  // APPLY
  ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
  ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgrey);
  ImGui::PushStyleColor (ImGuiCol_ButtonActive, missionx::color::color_vec4_grey);
  ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
  if (ImGui::Button ("Calculate Trigger Boundaries##trigBoxApply"))
  {
    if (inTrig_ptr.pos.getLat () == 0.0 || inTrig_ptr.pos.getLat () == 0.0)
    {
      this->set_bottom_message_line1 ("Your starting position is not valid.", 10);
    }
    else
    {
      // p1 = trig_ptr.pos = bottom left
      // p2-------------p3
      // |              |
      // p1-------------p4
      //       "*" = pCenter = center of box
      // p2-------------p3
      // |       *      |
      // p1-------------p4
      std::string DUMMY_SKELATON_ELEMENT{};
      // Point p1; // p2, p3, p4;

      if (inTrig_ptr.flag_first_point_is_center_cbox)
      {
        // store special info on the triggers main element
        inTrig_ptr.node_ptr.updateAttribute ("true", mxconst::get_ATTRIB_FIRST_POINT_IS_CENTER_B ().c_str (), mxconst::get_ATTRIB_FIRST_POINT_IS_CENTER_B ().c_str ());
        inTrig_ptr.node_ptr.updateAttribute (inTrig_ptr.pos.getLat_s ().c_str (), mxconst::get_ATTRIB_LAT ().c_str (), mxconst::get_ATTRIB_LAT ().c_str ());
        inTrig_ptr.node_ptr.updateAttribute (inTrig_ptr.pos.getLon_s ().c_str (), mxconst::get_ATTRIB_LONG ().c_str (), mxconst::get_ATTRIB_LONG ().c_str ());

        DUMMY_SKELATON_ELEMENT = "<DUMMY> " + inTrig_ptr.pos.get_point_lat_lon_as_string () + " </DUMMY>";
      }
      else
      {

        //// store special info on the triggers main element
        inTrig_ptr.node_ptr.updateAttribute ("", mxconst::get_ATTRIB_FIRST_POINT_IS_CENTER_B ().c_str (), mxconst::get_ATTRIB_FIRST_POINT_IS_CENTER_B ().c_str ());
        inTrig_ptr.node_ptr.updateAttribute ("", mxconst::get_ATTRIB_LAT ().c_str (), mxconst::get_ATTRIB_LAT ().c_str ());
        inTrig_ptr.node_ptr.updateAttribute ("", mxconst::get_ATTRIB_LONG ().c_str (), mxconst::get_ATTRIB_LONG ().c_str ());


        DUMMY_SKELATON_ELEMENT = "<DUMMY> " + inTrig_ptr.pos.get_point_lat_lon_as_string () + " </DUMMY>";
      }

      inTrig_ptr.node_ptr.updateAttribute (mxUtils::formatNumber<int> (iMetersVector1).c_str (), mxconst::get_ATTRIB_VECTOR_BT_LENGTH_MT ().c_str (), mxconst::get_ATTRIB_VECTOR_BT_LENGTH_MT ().c_str ());
      inTrig_ptr.node_ptr.updateAttribute (mxUtils::formatNumber<int> (iMetersVector2).c_str (), mxconst::get_ATTRIB_VECTOR_LR_LENGTH_MT ().c_str (), mxconst::get_ATTRIB_VECTOR_LR_LENGTH_MT ().c_str ());

      IXMLNode xRectangle = Utils::xml_get_or_create_node_ptr (pNode, mxconst::get_ELEMENT_RECTANGLE ());
      assert (xRectangle.isEmpty () == false && (std::string (__func__).append (": ").append (pNode.getName ()).append (" - Failed to create node: ").c_str ()));

      xRectangle.updateAttribute (mxUtils::formatNumber<float> (heading_f, 2).c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str (), mxconst::get_ATTRIB_HEADING_PSI ().c_str ());
      xRectangle.updateAttribute ((mxUtils::formatNumber<int> (iMetersVector1) + "|" + mxUtils::formatNumber<int> (iMetersVector2)).c_str (), mxconst::get_ATTRIB_DIMENSIONS ().c_str (), mxconst::get_ATTRIB_DIMENSIONS ().c_str ());
      xRectangle.updateAttribute ((inTrig_ptr.flag_first_point_is_center_cbox) ? mxconst::get_MX_TRUE ().c_str () : mxconst::get_MX_FALSE ().c_str (), mxconst::get_ATTRIB_FIRST_POINT_IS_CENTER_B ().c_str (), mxconst::get_ATTRIB_FIRST_POINT_IS_CENTER_B ().c_str ());



#ifndef RELEASE
      Log::logMsg ("Rectangular main point: " + inTrig_ptr.pos.get_point_lat_lon_as_string ());
#endif // !RELEASE



      Utils::xml_delete_all_subnodes (pNode, mxconst::get_ELEMENT_POINT ()); // delete all <point> sub nodes

      // this part is only to simplify and beautify the trigger output
      IXMLResults   parse_result_strct;
      IXMLDomParser iDomTemplate;
      auto          xDummy = iDomTemplate.parseString (DUMMY_SKELATON_ELEMENT.c_str (), mxconst::get_DUMMY_ROOT_DOC ().c_str (), &parse_result_strct).deepCopy (); // parse xml into ITCXMLNode

      if (xDummy.nChildNode (mxconst::get_ELEMENT_POINT ().c_str ()) > 0)
      {
        inTrig_ptr.setBuff (inTrig_ptr.iCurrentBuf, inTrig_ptr.pos.get_point_lat_lon_as_string ()); // camera pos, reformat point to have lat/lon/elev information
        pNode.addChild (xDummy.getChildNode (mxconst::get_ELEMENT_POINT ().c_str (), 0).deepCopy ()); // this is the starting position of the rectangular shape

        if (inTrig_ptr.flag_first_point_is_center_cbox)
          this->set_bottom_message_line1 ("Calculated triggers boundaries for center rectangular point.", 10);
        else
          this->set_bottom_message_line1 ("Calculated triggers boundaries for bottomLeft rectangular point.", 10);
      }
      else
        this->set_bottom_message_line1 ("Failed calculation, not enough points, try to rerun the trigger calculation button.", 10);
    }
  }
  ImGui::PopStyleColor (4);
}

// ----------------------------

void ui_conv_screen::subDraw_ui_xTrigger_elev(mx_trig_strct_& inTrig_ptr, IXMLNode& node, bool inResetPick)
{
  static const std::vector<const char *> vecElevOptions           = { "min/max elev", "lower than..", "above than..", "max elev above ground", "min elev above ground" };
  static const std::vector<const char *> vecElevOptions_sign_trns = { "|", "--", "++", "---", "+++" };
  static int                             elev_ft_arr[2]           = { 0, 0 };
  static std::string                     elev_s{ "" };

  if (inResetPick)
  {
    elev_s.clear ();
  }

  ImGui::SetNextItemWidth (140.0f);
  if (ImGui::Combo ("##dynElevPick", &inTrig_ptr.trig_ui_elev_type_combo_indx, vecElevOptions.data (), static_cast<int> (vecElevOptions.size ())))
  {
    elev_s.clear ();
    inTrig_ptr.node_ptr.updateAttribute (mxUtils::formatNumber<int> (inTrig_ptr.trig_ui_elev_type_combo_indx).c_str (), mxconst::get_CONV_ATTRIB_trig_ui_elev_type_combo_indx ().c_str (), mxconst::get_CONV_ATTRIB_trig_ui_elev_type_combo_indx ().c_str ());
  }

  if (inTrig_ptr.trig_ui_elev_type_combo_indx > -1)
    ImGui::SameLine ();

  switch (inTrig_ptr.trig_ui_elev_type_combo_indx)
  {
    case 0: // min/max
    {
      ImGui::SetNextItemWidth (160.0f);
      if (ImGui::InputInt2 ("ft.##TrigElevFt", elev_ft_arr))
      {
      }
      // validation
      if ((elev_ft_arr[1] - elev_ft_arr[0]) < 500)
      {
        elev_ft_arr[1] = elev_ft_arr[0] + 500; // we make sure that the min/max elevation is no less than 500ft
      }
      elev_s = mxUtils::formatNumber<int> (elev_ft_arr[0]) + "|" + mxUtils::formatNumber<int> (elev_ft_arr[1]);
    }
    break;
    case 1:
    case 2:
    case 3:
    case 4:
    {
      ImGui::SetNextItemWidth (120.0f);
      ImGui::InputInt ("##TrigElevFt", &elev_ft_arr[0], 100);
      if (elev_ft_arr[0] < 0) // make sure value is not negative so we won't have: "++-200" or "---200" when we wanted "--200"
        elev_ft_arr[0] = 0;

      elev_s = vecElevOptions_sign_trns.at (inTrig_ptr.trig_ui_elev_type_combo_indx) + mxUtils::formatNumber<int> (elev_ft_arr[0]);
    }
    break;
    default:
      break;
  } // end switch


  node.updateAttribute (elev_s.c_str (), mxconst::get_ATTRIB_ELEV_LOWER_UPPER_FT ().c_str (), mxconst::get_ATTRIB_ELEV_LOWER_UPPER_FT ().c_str ());
}

// ----------------------------

void ui_conv_screen::subDraw_ui_xScriptlet(IXMLNode& pNode, mxTrig_ui_mode_enm& inMode, mx_trig_strct_* inTrig_ptr, const std::string inScriptInputLabel, missionx::mx_local_fpln_strct* inLegData, char* inOutBuff)
{
  static int  script_index = 0;
  static char buff[missionx::LOG_BUFF_SIZE]{ 0 };

  const auto lmbda_get_buff = [&] ()
  {
    if (inTrig_ptr == nullptr)
    {
      if (inOutBuff == nullptr)
        return buff;
      else
        return inOutBuff;
    }

    return inTrig_ptr->buffArray[inTrig_ptr->iCurrentBuf];
  };

  auto working_buff = lmbda_get_buff ();

  int leg_index_i = (inLegData == nullptr) ? 0 : inLegData->indx;

  // extract xCondition if available
  auto        xConditions_ptr  = (inTrig_ptr == nullptr) ? IXMLNode::emptyIXMLNode : Utils::xml_get_or_create_node_ptr (inTrig_ptr->node_ptr, mxconst::get_ELEMENT_CONDITIONS ());
  std::string scriptlet_name_s = Utils::readAttrib (xConditions_ptr, mxconst::get_ATTRIB_COND_SCRIPT (), "");
  if (scriptlet_name_s.empty ())
  {
    scriptlet_name_s = "leg_" + mxUtils::formatNumber<int> (leg_index_i) + "_scriptlet_" + mxUtils::formatNumber<int> (script_index);

    xConditions_ptr.updateAttribute (scriptlet_name_s.c_str (), mxconst::get_ATTRIB_COND_SCRIPT ().c_str (), mxconst::get_ATTRIB_COND_SCRIPT ().c_str ());

    ++script_index;
  }


  IXMLNode xScriptlet_ptr = (scriptlet_name_s.empty ()) ? IXMLNode::emptyIXMLNode : Utils::xml_get_node_pointer_from_node_tree_by_attrib_name_and_value_IXMLNode (pNode, mxconst::get_ELEMENT_SCRIPTLET (), mxconst::get_ATTRIB_NAME (), scriptlet_name_s);
  if (xScriptlet_ptr.isEmpty ())
    xScriptlet_ptr = Utils::xml_get_or_create_node_ptr (pNode, mxconst::get_ELEMENT_SCRIPTLET (), mxconst::get_ATTRIB_NAME (), scriptlet_name_s);

  assert (xScriptlet_ptr.isEmpty () == false && " scriptlet cNode can't be empty");

  // std::string script_cdata_s = (xScriptlet_ptr.isEmpty ()) ? "" : Utils::xml_read_cdata_node (xScriptlet_ptr, "");
  std::string script_cdata_s = (xScriptlet_ptr.isEmpty ()) ? "" : Utils::xml_get_text_or_cdata_text (xScriptlet_ptr, "");
  if (inTrig_ptr)
    inTrig_ptr->setBuff (inTrig_ptr->iCurrentBuf, script_cdata_s);



  ImGui::TextColored (missionx::color::color_vec4_yellow, "%s", ((inScriptInputLabel.empty ()) ? "Enter <scriptlet> Code:" : inScriptInputLabel.c_str ()));
  this->mx_add_tooltip (missionx::color::color_vec4_white, "The script must handle when to fire the trigger.\nIt can also manage messages and other supported commands.\nIt is up to you to decide the complexity of the code.\nBest practice: use \"Trigger Outcome\" to handle messages and post fire staff.\n\nCheck \"Designer Guide\" for more explanations.");

  if (ImGui::InputTextMultiline ("##scriptletUi", working_buff, missionx::LOG_BUFF_SIZE, ImVec2 (ImGui::GetContentRegionAvail ().x - 10.0f, 60.0f), ImGuiInputTextFlags_AllowTabInput))
  {
    std::string buff_s = std::string (working_buff);

    assert (scriptlet_name_s.empty () == false && "scriptlet name attribute can't be empty");

    Utils::xml_add_cdata (xScriptlet_ptr, buff_s);

  } // update origin node and buff after each change


  // display characters left to write
  ImGui::TextColored (missionx::color::color_vec4_floralwhite, "%zu", missionx::LOG_BUFF_SIZE - std::string_view (working_buff).length ());

}

// ----------------------------

void ui_conv_screen::draw_conv_main_fpln_to_mission_window()
{
    auto win_size_vec2 = ImGui::GetContentRegionAvail ();

  ImGui::SetWindowFontScale (missionx::strct_setup_layer.fPreferredFontScale);

  // First Time code
  if (this->strct_conv_layer.flag_first_time)
  {
    this->strct_conv_layer.conv_sub_ui = mx_conv_sub_ui::conv_pick_fpln; // enum value

    this->strct_conv_layer.file_picked_i    = -1;
    this->strct_conv_layer.way_row_picked_i = -1;
    this->strct_conv_layer.vecFileList_char.clear ();
    this->strct_conv_layer.mapFileList.clear ();
    this->strct_conv_layer.set_conv_map_files (this->read_fpln_files ()); // set the mapFileList and the vecFileList_char

    this->strct_conv_layer.flag_first_time = false;
  }


  constexpr static std::string_view welcome_str_vu             = R"(Welcome to flight plan conversion screen. This screen should be used only in 2D mode and not in VR.
In this screen you will pick a flight plan based on LittleNavMap (".lnmpln") from X-Plane FPLN folder ("Output/FMS plans").
)";
  constexpr static std::string_view design_mission_fpln_str_vu = R"(Design the mission Flight Plan based on the parsed LittleNavMap file.
1. Your briefer waypoint is your starting location (usually line 0. If it is absent, you can convert line 1 to be a briefer "BR")
2. Pick waypoints that are mandatory to pass, you must have at least one. Fill the information of that flight leg using the respective button in the "details column".
3. Once you [Generate] the mission, you can load it from "Load Mission" screen (Pick the Random image - the first one, in that screen).
)";

  constexpr static std::string_view triggers_fpln_str_vu = R"(The "Trigger or Event screen" allows you to define different ways to interact with the simmer.
You can define: "Radius, Polygonal, Script or even Camera" based areas of effect, depends on your needs.
Each trigger must have a unique name in all the mission file.
)";

  constexpr static std::string_view features_not_implemented_str_vu = R"(The list of features that were not implemented:
> start/end screen.
> script editor.
> tasks - there is only the mandatory task.
> choices

There are other options that are best handle manually inside an editor and not in the Mission-X Conversion window.
(Read the designer guide))";


  // Display Welcome text //
  ImGui::BeginGroup ();
  {
    this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ()); // v3.303.14
    ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_aquamarine);
    ImGui::TextWrapped ("%s", (this->strct_conv_layer.conv_sub_ui == mx_conv_sub_ui::conv_pick_fpln) ? welcome_str_vu.data () : (this->strct_conv_layer.conv_sub_ui == mx_conv_sub_ui::conv_design_fpln) ? design_mission_fpln_str_vu.data () : triggers_fpln_str_vu.data ());
    ImGui::PopStyleColor ();
    this->mxUiReleaseLastFont (); // v3.303.14
  }
  ImGui::Separator ();

  ImGui::EndGroup ();



  // Display list of fpln or design mission //
  switch (this->strct_conv_layer.conv_sub_ui)
  {
    case (mx_conv_sub_ui::conv_pick_fpln):
    {
      ImGui::BeginGroup ();
      {
        ImgWindow::HelpMarker (features_not_implemented_str_vu.data ()); // v3.0.301 B4 added unsupported features example

        this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ()); // v3.303.14

        ImGui::TextColored (missionx::color::color_vec4_yellow, "Pick a file from the list:");

        ImGui::SameLine (win_size_vec2.x * 0.75f);
        ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
        ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgray);
        ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
        if (ImGui::Button (" Load Saved State File "))
        {
          this->strct_conv_layer.flag_load_conversion_file = true; // will cause the file information to be refreshed in the next run
          this->strct_conv_layer.file_picked_i             = -1; // reset pick state so we will only see the "converter" file keyName
          std::string conv_file                            = Utils::getMissionxCustomSceneryFolderPath_WithSep (true) + "/random/briefer/" + mxconst::get_CONVERTER_FILE ();
        }
        ImGui::PopStyleColor (3);

        this->mxUiReleaseLastFont (); // v3.303.14

        this->mx_add_tooltip (missionx::color::color_vec4_yellowgreen, "Only loads last conversion 'saved state' file - if exists.");


        this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_SMALL ()); // v3.303.14
        if (ImGui::BeginListBox ("##ListOfFlightPlanFiles"))
        {
          for (int n = 0; n < static_cast<int> (this->strct_conv_layer.vecFileList_char.size ()); n++)
          {
            const bool is_selected = (this->strct_conv_layer.file_picked_i == n);
            if (ImGui::Selectable (this->strct_conv_layer.vecFileList_char.at (n), is_selected)) // , 0, ImVec2(350.0f, 150.0f)
              this->strct_conv_layer.file_picked_i = n;

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
              ImGui::SetItemDefaultFocus ();
          }
          ImGui::EndListBox ();
        }
        this->mxUiReleaseLastFont (); // v3.303.14

        // bottom buttons
        this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ()); // v3.303.14
        if (ImGui::Button (" Refresh File List "))
        {
          this->strct_conv_layer.flag_first_time = true; // will cause the file information to be refreshed in the next run
        }
        if (this->strct_conv_layer.file_picked_i > -1 || this->strct_conv_layer.flag_load_conversion_file)
        {
          ImGui::SameLine (0.0f, 20.0f);
          if (ImGui::Button ("  Read Flight Plan File and continue to step 2...  ") || this->strct_conv_layer.flag_load_conversion_file)
          {

            std::string file;
            std::string filePath;

            // v3.305.1 reset xSavedGlobalSettingsNode Node
            this->strct_conv_layer.xSavedGlobalSettingsNode = IXMLNode ();

            if (this->strct_conv_layer.flag_load_conversion_file)
            {
              file     = mxconst::get_CONVERTER_FILE ();
              filePath = Utils::getMissionxCustomSceneryFolderPath_WithSep (true) + "/random/briefer/" + file;
            }
            else if (this->strct_conv_layer.file_picked_i > -1)
            {
              file     = this->strct_conv_layer.vecFileList_char.at (this->strct_conv_layer.file_picked_i);
              filePath = this->strct_conv_layer.mapFileList[file];
            }
            else
              break;

            missionx::data_manager::map_tableOfParsedFpln.clear ();

            if (this->strct_conv_layer.flag_load_conversion_file)
            {
              // call read_and_parse_conversion_file
              missionx::data_manager::map_tableOfParsedFpln = read_and_parse_saved_state (filePath); // will update this->strct_conv_layer.xXPlaneDataRef_global and this->strct_conv_layer.xSavedGlobalSettingsNode
            }
            else
              missionx::data_manager::map_tableOfParsedFpln = missionx::data_manager::read_and_parse_littleNavMap_fpln (filePath);


            if (missionx::data_manager::map_tableOfParsedFpln.size () == static_cast<size_t> (0))
            {
              this->set_bottom_message_line1 ("No information was loaded.", 8);
            }
            else // prepare a new in memory XML
            {
              this->strct_conv_layer.xConvMainNode = IXMLNode::createXMLTopNode ("xml", TRUE);
              this->strct_conv_layer.xConvMainNode.addAttribute (mxconst::get_ATTRIB_VERSION ().c_str (), "1.0");
              this->strct_conv_layer.xConvMainNode.addAttribute ("encoding", "ASCII"); // "ISO-8859-1");
              this->strct_conv_layer.xConvMainNode.addClear ("\n\tFile has been created by Mission-X plug-in.\n\tAny modification might break or invalidate the file.\n\t", "<!--", "-->");

              IXMLDomParser iDomTemplate;
              IXMLResults   parse_result_strct;
              this->strct_conv_layer.xConvDummy = iDomTemplate.parseString (std::string (this->strct_conv_layer.DUMMY_SKELATON_ELEMENT.data ()).c_str (), mxconst::get_DUMMY_ROOT_DOC ().c_str (), &parse_result_strct).deepCopy (); // parse xml into ITCXMLNode
              this->strct_conv_layer.xConvMainNode.addChild (this->strct_conv_layer.xConvDummy);

              // Add mission info
              if (!this->strct_conv_layer.xConvInfo.isEmpty ())
                this->strct_conv_layer.xConvInfo.deleteNodeContent ();

              this->strct_conv_layer.xConvInfo = this->strct_conv_layer.xConvDummy.addChild (mxconst::get_ELEMENT_MISSION_INFO ().c_str ());
              Utils::xml_add_comment (this->strct_conv_layer.xConvDummy, " ----------------- ");

              // main <xpdata>
              if (this->strct_conv_layer.flag_load_conversion_file)
                this->strct_conv_layer.xConvDummy.addChild (this->strct_conv_layer.xXPlaneDataRef_global);
              else
                this->strct_conv_layer.xXPlaneDataRef_global = this->strct_conv_layer.xConvDummy.addChild (mxconst::get_ELEMENT_XPDATA ().c_str ());

              Utils::xml_add_comment (this->strct_conv_layer.xConvDummy, " ----------------- ");


              // main <triggers>
              if (!this->strct_conv_layer.xTriggers_global.isEmpty ())
                this->strct_conv_layer.xTriggers_global.deleteNodeContent ();

              this->strct_conv_layer.xTriggers_global = this->strct_conv_layer.xConvDummy.addChild (mxconst::get_ELEMENT_TRIGGERS ().c_str ());
              Utils::xml_add_comment (this->strct_conv_layer.xConvDummy, " ----------------- ");



              // flag if we have index 0 or not (briefer)
              this->strct_conv_layer.flag_foundBriefer_index0     = Utils::isElementExists (data_manager::map_tableOfParsedFpln, 0); // do we have index 0 (key = 0) in the map ?
              this->strct_conv_layer.flag_refresh_table_from_file = true;

              // make sure we have xLeg IXMLNode for each row
              bool   isNotFirstTime{ false };
              double last_cumulative = 0.0;
              Point  pPrev;
              for (auto &[indx, legData] : data_manager::map_tableOfParsedFpln)
              {
                if (legData.xLeg.isEmpty () || this->strct_conv_layer.flag_load_conversion_file)
                {
                  legData.xFlightPlan = this->strct_conv_layer.xConvDummy.addChild (mxconst::get_ELEMENT_FLIGHT_PLAN ().c_str ());

                  // Add <leg>
                  if (this->strct_conv_layer.flag_load_conversion_file)
                  {
                    legData.xFlightPlan.addChild (legData.xLeg); // use the <leg> from "saved state" file.

                    // v3.0.303.7 move all loaded <scriptlet> inside <scripts> to flightPlan
                    int iScriptlets = legData.xLoadedScripts.nChildNode (mxconst::get_ELEMENT_SCRIPTLET ().c_str ());
                    for (int i1 = 0; i1 < iScriptlets; ++i1)
                      legData.xFlightPlan.addChild (legData.xLoadedScripts.getChildNode (mxconst::get_ELEMENT_SCRIPTLET ().c_str (), i1)); // move <scriptlets> from <script> to <flight_plan> element
                  }
                  else
                  {
                    legData.xLeg       = legData.xFlightPlan.addChild (mxconst::get_ELEMENT_LEG ().c_str ());
                    legData.attribName = mxconst::get_ELEMENT_LEG () + Utils::formatNumber<int> (legData.indx);
                    legData.xLeg.updateAttribute (legData.attribName.c_str (), mxconst::get_ATTRIB_NAME ().c_str ());
                  }

                  // <objectives>
                  legData.xObjectives = legData.xFlightPlan.addChild (mxconst::get_ELEMENT_OBJECTIVES ().c_str ());

                  // Add <triggers>
                  if (legData.xTriggers.isEmpty ()) // v3.0.303.4
                    legData.xTriggers = legData.xFlightPlan.addChild (mxconst::get_ELEMENT_TRIGGERS ().c_str ());
                  else
                    legData.xFlightPlan.addChild (legData.xTriggers);

                  // Add <message_templates>
                  if (legData.xMessageTmpl.isEmpty ()) // v3.0.303.4
                    legData.xMessageTmpl = legData.xFlightPlan.addChild (mxconst::get_ELEMENT_MESSAGE_TEMPLATES ().c_str ());
                  else
                    legData.xFlightPlan.addChild (legData.xMessageTmpl);
                }
                // calculate distances
                if (isNotFirstTime)
                {
                  legData.distToPrevWaypoint = Point::calcDistanceBetween2Points (pPrev, legData.p);
                  legData.cumulativeDist     = last_cumulative + legData.distToPrevWaypoint;
                }
                pPrev           = legData.p;
                last_cumulative = legData.cumulativeDist;
                isNotFirstTime  = true;
              }

              this->strct_conv_layer.conv_sub_ui = mx_conv_sub_ui::conv_design_fpln;
            } // end else if map table holds values
          } // end pressed "read lnvmap flight plan"
        }
        this->mxUiReleaseLastFont (); // v3.303.14
      }
      ImGui::EndGroup ();
      // v3.0.301 B4

      // features not supported

    } // conv_pick_fpln
    break;



    case (mx_conv_sub_ui::conv_design_fpln):
    {
      static bool  bRerunRandomDateTime{ false };
      const ImVec2 child_size_vec2 = ImVec2 (win_size_vec2.x - 10.0f, 250.0f);

      ImGui::BeginGroup ();
      ImGui::BeginChild ("TableOfLNM_Waypoints", child_size_vec2, ImGuiChildFlags_Borders);
      {
        this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ()); // v3.303.14
        ImGui::TextColored (missionx::color::color_vec4_bisque, "Picked File: ");
        ImGui::SameLine (0.0f, 2.0f);

        if (this->strct_conv_layer.file_picked_i > -1)
        {
          ImGui::TextColored (missionx::color::color_vec4_green, "%s", this->strct_conv_layer.vecFileList_char.at (this->strct_conv_layer.file_picked_i));
        }
        else
        {
          ImGui::TextColored (missionx::color::color_vec4_green, "%s", mxconst::get_CONVERTER_FILE ().c_str ());
        }
        this->mxUiReleaseLastFont ();


        /////// BUTTONS //////
        this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ()); // v3.303.14

        ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
        ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgray);
        ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
        if (ImGui::Button ("Cancel"))
        {
          this->strct_conv_layer.flag_load_conversion_file = false; // v3.0.303.4  reset "reset conversion state"
          this->strct_conv_layer.conv_sub_ui               = mx_conv_sub_ui::conv_pick_fpln;
        }
        ImGui::PopStyleColor (3);

        ImGui::SameLine (win_size_vec2.x - 230.0f); // draw button from right

        ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
        ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgreen);
        ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);
        if (ImGui::Button (">> Generate <<"))
        {
          if (bRerunRandomDateTime) // v3.303.10
            this->execAction (missionx::mx_window_actions::ACTION_GENERATE_RANDOM_DATE_TIME);

          if (this->strct_conv_layer.xSavedGlobalSettingsNode.isEmpty ())
          {
            this->strct_conv_layer.flag_use_loaded_globalSetting_from_conversion_file = false; // v3.305.1
            this->set_bottom_message_line1 ("Please wait while generating the mission from the Flight Plan.", 10);
            this->execAction (missionx::mx_window_actions::ACTION_GENERATE_MISSION_FROM_LNM_FPLN);
          }
          else // open popup
          {
            ImGui::OpenPopup (POPUP_PICK_GLOBAL_SETTING_NODE.c_str ());
          }
        }
        ImGui::PopStyleColor (3);

        ImGui::SameLine (); // draw button from right
        ImGui::Checkbox ("Store State##conversionScreen", &this->strct_conv_layer.flag_store_state);

        this->mxUiReleaseLastFont ();

        // ------------ Little Nav Map Table -----------------------

        subDraw_fpln_table (this->strct_conv_layer.xConvMainNode, missionx::data_manager::map_tableOfParsedFpln);

      } // end table group

      // display POPUP
      this->draw_conv_popup_which_global_settings_to_save (POPUP_PICK_GLOBAL_SETTING_NODE);


      ImGui::EndChild ();
      ImGui::EndGroup ();


      // ------------ Bottom Table Buttons -----------------------

      this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ()); // v3.303.14

      ImGui::PushStyleColor (ImGuiCol_Text, missionx::color::color_vec4_black);
      ImGui::PushStyleColor (ImGuiCol_Button, missionx::color::color_vec4_lightgray);
      ImGui::PushStyleColor (ImGuiCol_ButtonHovered, missionx::color::color_vec4_azure);

      if (ImGui::Button ("Dataref Editor"))
      {
        ImGui::OpenPopup (POPUP_DATAREF_SETTINGS.c_str ());
      }

      // v3.305.1 [Global Settings] editor button
      if (!(this->strct_conv_layer.xSavedGlobalSettingsNode.isEmpty ()))
      {
        ImGui::SameLine (0.0f, 10.0f);
        if (ImGui::Button ("GlobalSettings Editor"))
        {
          ImGui::OpenPopup (POPUP_GLOBAL_SETTINGS.c_str ());
        }
      }


      ImGui::PopStyleColor (3);

      this->mxUiReleaseLastFont ();

      // v3.303.14 add the advance window that hold: date and weather settings
      ImGui::SameLine (0.0f, 50.0f);
      bRerunRandomDateTime = this->add_ui_checkbox_rerun_random_date_and_time ();
      ImGui::SameLine ();
      this->add_ui_advance_settings_random_date_time_weather_and_weight_button (missionx::adv_settings_strct.iClockDayOfYearPicked, missionx::adv_settings_strct.iClockHourPicked, missionx::adv_settings_strct.iClockMinutesPicked, mxconst::get_TEXT_TYPE_TITLE_REG ());

      ImGui::SameLine (0.0f, 25.0f);
      this->add_designer_mode_checkbox ();


      // display POPUP
      const auto popupHeight_f = ImGui::GetIO ().DisplaySize.y * 0.75f;

      const ImVec2 center (ImGui::GetIO ().DisplaySize.x * 0.5f, ImGui::GetIO ().DisplaySize.y * 0.5f);
      ImGui::SetNextWindowPos (center, ImGuiCond_Appearing, ImVec2 (0.5f, 0.5f));
      ImGui::SetNextWindowSize (ImVec2 (win_size_vec2.x - 20.0f, popupHeight_f));
      ImGui::PushStyleColor (ImGuiCol_PopupBg, missionx::color::color_vec4_blue);
      {
        if (ImGui::BeginPopupModal (POPUP_DATAREF_SETTINGS.c_str (), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
          draw_conv_popup_datarefs (this->strct_conv_layer.xXPlaneDataRef_global);
          ImGui::EndPopup ();
        }

        // v3.305.1 GlobalSettings Popup
        ImGui::SetNextWindowSize (ImVec2 (win_size_vec2.x - 20.0f, popupHeight_f));
        if (ImGui::BeginPopupModal (POPUP_GLOBAL_SETTINGS.c_str (), NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
          draw_conv_popup_globalSettings (this->strct_conv_layer.xSavedGlobalSettingsNode);
          ImGui::EndPopup ();
        }
      }
      ImGui::PopStyleColor ();
    }
    break;

    default:
    {
    }
    break;
  } // end switch between layers

  ImGui::SetWindowFontScale (missionx::strct_setup_layer.fPreferredFontScale);
}

// ----------------------------

std::map<int, missionx::mx_local_fpln_strct> ui_conv_screen::read_and_parse_saved_state(const std::string inPathAndFile)
{
  std::string                                  errMsg;
  std::map<int, missionx::mx_local_fpln_strct> fpln;


  // Read Little Nav Map flight plan
  Log::logMsg ("[Load] Reading Stored Conversion Screen Saved State File: " + inPathAndFile); // debug
  IXMLDomParser iDom;
  ITCXMLNode    xMainNode = iDom.openFileHelper (inPathAndFile.c_str (), mxconst::get_CONVERSION_ROOT_DOC ().c_str (), &errMsg);

  if (errMsg.empty () == false) // error
  {
    this->strct_conv_layer.flag_load_conversion_file = false;
    Log::logMsg (errMsg);
  }
  else
  {
    int legCounter       = xMainNode.nChildNode (mxconst::get_ELEMENT_FPLN ().c_str ());
    int triggerCounter_i = 0;

    for (int i1 = 0; i1 < legCounter; ++i1)
    {
      missionx::mx_local_fpln_strct leg;
      auto                          xLeg = xMainNode.getChildNode (mxconst::get_ELEMENT_FPLN ().c_str (), i1).deepCopy ();

      xLeg.updateName (mxconst::get_ELEMENT_LEG ().c_str ());
      leg.xLeg = xLeg.deepCopy ();


      if (xLeg.isEmpty () == false)
      {
        leg.indx       = Utils::readNodeNumericAttrib<int> (xLeg, mxconst::get_ATTRIB_INDEX (), i1);
        leg.name       = Utils::readAttrib (xLeg, mxconst::get_ELEMENT_LNM_Name (), std::string ("NAME").append (mxUtils::formatNumber<int> (i1)));
        leg.ident      = Utils::readAttrib (xLeg, mxconst::get_ELEMENT_LNM_Ident (), leg.name);
        leg.attribName = Utils::readAttrib (xLeg, mxconst::get_ATTRIB_NAME (), std::string ("NAME").append (mxUtils::formatNumber<int> (i1)));
        leg.type       = Utils::readAttrib (xLeg, mxconst::get_ELEMENT_LNM_Type (), "");

        leg.flag_isLeg            = Utils::readBoolAttrib (xLeg, mxconst::get_CONV_ATTRIB_isLeg (), false);
        leg.flag_isLast           = Utils::readBoolAttrib (xLeg, mxconst::get_CONV_ATTRIB_isLast (), false);
        leg.flag_ignore_leg       = Utils::readBoolAttrib (xLeg, mxconst::get_CONV_ATTRIB_ignore_leg (), false);
        leg.flag_convertToBriefer = Utils::readBoolAttrib (xLeg, mxconst::get_CONV_ATTRIB_convertToBriefer (), false);
        leg.flag_add_marker       = Utils::readBoolAttrib (xLeg, mxconst::get_CONV_ATTRIB_add_marker (), false);

        leg.marker_type_i                       = Utils::readNodeNumericAttrib<int> (xLeg, mxconst::get_CONV_ATTRIB_markerType (), 0); // 0 = default marker in combo
        leg.radius_to_display_3D_marker_in_nm_f = Utils::readNodeNumericAttrib<float> (xLeg, mxconst::get_CONV_ATTRIB_radius_to_display_marker (), 10.0f); // 10 = default display radius

        // Target Point
        auto xPoint = xLeg.getChildNode (mxconst::get_ELEMENT_POINT ().c_str ());
        if (!xPoint.isEmpty ())
        {
          leg.p.node = xPoint.deepCopy ();
          leg.p.parse_node ();
        }

        // Target elevation
        auto xTargetTrig = xLeg.getChildNode (mxconst::get_ELEMENT_TRIGGER ().c_str ());
        if (!xTargetTrig.isEmpty ())
        {

          leg.target_trig_strct.elev_combo_picked_i  = Utils::readNodeNumericAttrib<int> (xTargetTrig, mxconst::get_CONV_ATTRIB_elev_combo_picked_i (), 0);
          leg.target_trig_strct.slider_elev_value_i  = Utils::readNodeNumericAttrib<int> (xTargetTrig, mxconst::get_CONV_ATTRIB_slider_elev_value_i (), 0);
          leg.target_trig_strct.radius_of_trigger_mt = Utils::readNodeNumericAttrib<int> (xTargetTrig, mxconst::get_ATTRIB_LENGTH_MT (), 100); // trigger area, if not set than use 2000 meters

          leg.target_trig_strct.elev_min         = Utils::readNodeNumericAttrib<int> (xTargetTrig, mxconst::get_ATTRIB_ELEV_MIN_FT (), 0);
          leg.target_trig_strct.elev_max         = Utils::readNodeNumericAttrib<int> (xTargetTrig, mxconst::get_ATTRIB_ELEV_MAX_FT (), 0);
          leg.target_trig_strct.elev_rule_s      = Utils::readAttrib (xTargetTrig, mxconst::get_CONV_ATTRIB_elev_rule_s (), "");
          leg.target_trig_strct.elev_lower_upper = Utils::readAttrib (xTargetTrig, mxconst::get_ATTRIB_ELEV_MIN_MAX_FT (), "");
          leg.target_trig_strct.flag_on_ground   = Utils::readBoolAttrib (xTargetTrig, mxconst::get_CONV_ATTRIB_on_ground (), false);
        }

        // read <BUFFERS>
        const auto buf_i = xLeg.getChildNode (mxconst::get_ELEMENT_BUFFERS ().c_str ()).nChildNode (mxconst::get_ELEMENT_BUFF ().c_str ());

        assert (buf_i <= leg.MAX_ARRAY && "Convert state has more buffers than allowed.");

        for (int i2 = 0; i2 < buf_i; ++i2)
        {
          // std::string buf_s = Utils::xml_read_cdata_node (xLeg.getChildNode (mxconst::get_ELEMENT_BUFFERS ().c_str ()).getChildNode (mxconst::get_ELEMENT_BUFF ().c_str (), i2), "");
          std::string buf_s = Utils::xml_get_text_or_cdata_text (xLeg.getChildNode (mxconst::get_ELEMENT_BUFFERS ().c_str ()).getChildNode (mxconst::get_ELEMENT_BUFF ().c_str (), i2), "");
          leg.setBuff (i2, buf_s);
        }

        // read <triggers>
        leg.xTriggers = xLeg.getChildNode (mxconst::get_ELEMENT_TRIGGERS ().c_str ()).deepCopy ();
        triggerCounter_i += leg.xTriggers.nChildNode (mxconst::get_ELEMENT_TRIGGER ().c_str ()); // count triggers

        // read <message_templates>
        leg.xMessageTmpl = xLeg.getChildNode (mxconst::get_ELEMENT_MESSAGE_TEMPLATES ().c_str ()).deepCopy ();

        // v3.0.303.7 Add Scriptlets
        leg.xLoadedScripts = Utils::xml_get_or_create_node (xLeg, mxconst::get_ELEMENT_SCRIPTS ());

        assert (leg.xLoadedScripts.isEmpty () == false && " Scripts node is empty()");

        for (int i3 = 0; i3 < xLeg.nChildNode (mxconst::get_ELEMENT_SCRIPTLET ().c_str ()); ++i3)
        {
          leg.xLoadedScripts.addChild (xLeg.getChildNode (mxconst::get_ELEMENT_SCRIPTLET ().c_str (), i3).deepCopy ());
          #ifndef RELEASE
          Utils::xml_print_node (leg.xLoadedScripts);
          #endif // !RELEASE
        }


        // delete <leg>
        Utils::xml_delete_all_subnodes (leg.xLeg, "", true); // delete all child nodes including cdata This should allow a clean save format after loading the saved state

        Utils::addElementToMap (fpln, leg.indx, leg);
      } // if leg is valid

    } // end loop

    // set the trigger sequence:
    this->strct_conv_layer.trig_seq = triggerCounter_i + 1; // v3.0.304.4


    // Dataref information
    IXMLNode xXpdata = xMainNode.getChildNode (mxconst::get_ELEMENT_XPDATA ().c_str ()).deepCopy ();
    if (!xXpdata.isEmpty ())
    {
      std::string xpdata_4096_s;
      this->strct_conv_layer.xXPlaneDataRef_global = xXpdata.deepCopy ();
      for (int i1 = 0; i1 < xXpdata.nChildNode (mxconst::get_ELEMENT_DATAREF ().c_str ()); ++i1)
      {
        IXMLRenderer render;
        xpdata_4096_s += render.getString (xXpdata.getChildNode (mxconst::get_ELEMENT_DATAREF ().c_str (), i1));
      }

//#ifdef IBM
//      memcpy_s (this->strct_conv_layer.buff_dataref, sizeof (this->strct_conv_layer.buff_dataref), xpdata_4096_s.c_str (), sizeof (this->strct_conv_layer.buff_dataref) - 1);
//#else
//      memcpy (this->strct_conv_layer.buff_dataref, xpdata_4096_s.c_str (), sizeof (this->strct_conv_layer.buff_dataref) - 1);
//#endif
      mxUtils::copy_string_to_buffer(xpdata_4096_s, this->strct_conv_layer.buff_dataref[0], sizeof(this->strct_conv_layer.buff_dataref));


      this->strct_conv_layer.xXPlaneDataRef_global = xXpdata.deepCopy ();
    }

    // v3.305.1 store <global_settings> sub nodes in buffer
    auto xGlobalSetting = xMainNode.getChildNode (mxconst::get_GLOBAL_SETTINGS ().c_str ()).deepCopy ();
    if (!xGlobalSetting.isEmpty ())
    {
      std::string data_4096_s;
      for (int i1 = 0; i1 < xGlobalSetting.nChildNode (); ++i1) // read all sub elements
      {
        IXMLRenderer render;
        data_4096_s += render.getString (xGlobalSetting.getChildNode (i1));
      }

//#ifdef IBM
//      memcpy_s (this->strct_conv_layer.buff_globalSettings, sizeof (this->strct_conv_layer.buff_globalSettings), data_4096_s.c_str (), sizeof (this->strct_conv_layer.buff_globalSettings) - 1);
//#else
//      memcpy (this->strct_conv_layer.buff_globalSettings, data_4096_s.c_str (), sizeof (this->strct_conv_layer.buff_globalSettings) - 1);
//#endif
      mxUtils::copy_string_to_buffer(data_4096_s, this->strct_conv_layer.buff_globalSettings[0], sizeof(this->strct_conv_layer.buff_globalSettings));

      this->strct_conv_layer.xSavedGlobalSettingsNode = xGlobalSetting.deepCopy ();
    }

  } // end else if


  return fpln;
}

// ----------------------------

void ui_conv_screen::draw_conv_popup_which_global_settings_to_save(std::string_view inPopupWindowName)
{
  bool flagSave = false;

  const ImVec2 center (ImGui::GetIO ().DisplaySize.x * 0.5f, ImGui::GetIO ().DisplaySize.y * 0.5f);
  ImGui::SetNextWindowPos (center, ImGuiCond_Appearing, ImVec2 (0.5f, 0.5f));
  ImGui::SetNextWindowSize (ImVec2 (550.0f, 150.0f));


  ImGui::PushStyleColor (ImGuiCol_ChildBg, missionx::color::color_vec4_black);
  {
    if (ImGui::BeginPopupModal (inPopupWindowName.data (), NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
      int iStyle = 0;
      ImGui::PushStyleColor (ImGuiCol_::ImGuiCol_ButtonHovered, missionx::color::color_vec4_green);
      iStyle++;
      ImGui::PushStyleColor (ImGuiCol_::ImGuiCol_Button, missionx::color::color_vec4_dodgerblue);
      iStyle++;
      ImGui::PushStyleColor (ImGuiCol_::ImGuiCol_Text, missionx::color::color_vec4_white);
      iStyle++;
      ImGui::PushStyleColor (ImGuiCol_::ImGuiCol_TextSelectedBg, missionx::color::color_vec4_black);
      iStyle++;

      this->mxUiSetFont (mxconst::get_TEXT_TYPE_MSG_POPUP ());
      ImGui::TextColored (missionx::color::color_vec4_yellow, "Pick which Global Settings you would like to save:\n---------------------------------------------------------\n1. The one you loaded from the converter save file, \tor\n2. Generate a new one based on the Advanced Settings.\n");

      // ImGui::NewLine();
      ImGui::Separator ();

      this->mxUiSetFont (mxconst::get_TEXT_TYPE_TITLE_REG ());
      if (ImGui::Button ("Use existing Global Settings", ImVec2 (210, 0)))
      {
        flagSave                                                                  = true;
        this->strct_conv_layer.flag_use_loaded_globalSetting_from_conversion_file = true;
      }
      this->mx_add_tooltip (missionx::color::color_vec4_white, "The plugin will use the <global_settings> loaded from the \"converter.sav\" file.");

      ImGui::SameLine (0.0f, 20.0f);
      ImGui::SetItemDefaultFocus ();
      if (ImGui::Button ("Generate a new Global Settings", ImVec2 (210, 0)))
      {
        flagSave                                                                  = true;
        this->strct_conv_layer.flag_use_loaded_globalSetting_from_conversion_file = false;
      }
      this->mx_add_tooltip (missionx::color::color_vec4_white, "The plugin will use the Advanced Settings information to generate a new <global_settings> element.\nThis includes weather/time and weight");

      ImGui::SameLine (0.0f, 20.0f);
      ImGui::SetItemDefaultFocus ();
      if (ImGui::Button ("Cancel", ImVec2 (70, 0)))
      {
        flagSave = false;
        ImGui::CloseCurrentPopup ();
      }

      ImGui::PopStyleColor (iStyle); // pop out before EndPopup

      this->mxUiReleaseLastFont (2);

      if (flagSave)
      {
        ImGui::CloseCurrentPopup ();
        this->set_bottom_message_line1 ("Please wait while generating the mission from the Flight Plan.", 10);
        this->execAction (missionx::mx_window_actions::ACTION_GENERATE_MISSION_FROM_LNM_FPLN);
      }

      ImGui::EndPopup ();
    } // end quit popup
  }
  ImGui::PopStyleColor ();
}

// ----------------------------
// ----------------------------
// ----------------------------

} // end missionx namespace
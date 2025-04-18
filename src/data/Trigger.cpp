#include "Trigger.h"
#include "../core/dataref_manager.h"


missionx::Trigger::Trigger()
{

  isEnabled         = true;
  isLinked          = false;
  bAllConditionsAreMet = false;
  bScriptCondMet    = false;
  linkedTo.clear();
  trigState    = mx_trigger_state_enum::never_triggered;
  trigElevType = mx_trigger_elev_type_enum::not_defined;

  cueType = missionx::mx_cue_types::cue_trigger;

  err.clear();
}


//missionx::Trigger::~Trigger() {}

bool
missionx::Trigger::parse_node()
{
  // read mandatory attributes and
  // do validation and initialization of basic trigger information. This replaces the read_trigger() logic rules.

  const std::string trigName = Utils::readAttrib(this->node, mxconst::get_ATTRIB_NAME(), "");
  const std::string trigType = Utils::readAttrib(this->node, mxconst::get_ATTRIB_TYPE(), "");

  if (trigName.empty())
  {
    Log::logMsg("Trigger \"" + trigName + "\", Has no name. Fix trigger settings. skipping...");
    return false; // skip trigger
  }

  // check trigger type is valid
  if ((mxconst::get_TRIG_TYPE_RAD () != trigType) && (mxconst::get_TRIG_TYPE_CAMERA () != trigType) && (mxconst::get_TRIG_TYPE_POLY () != trigType) && (mxconst::get_TRIG_TYPE_SCRIPT() != trigType) && (mxconst::get_TRIG_TYPE_POLY () != trigType))
  {
    Log::logMsg("Trigger \"" + trigName + "\", Has a none valid type: " + mxconst::get_QM() + trigType + mxconst::get_QM() + ". Fix trigger settings. skipping...");
    return false; // skip trigger
  }
  else
  {
    bool flag_b = true;
    // trigger type is valid

    this->name      = trigName;
    this->isEnabled = this->getBoolValue(mxconst::get_ATTRIB_ENABLED(), true); // v3.305.3 fix bug where we did not read the enabled attribute during node parsing.
    

    /// Get pointer to trigger elements from "node"
    this->xConditions     = Utils::xml_get_node_from_node_tree_IXMLNode(this->node, mxconst::get_ELEMENT_CONDITIONS(), false);
    this->xOutcome        = Utils::xml_get_node_from_node_tree_IXMLNode(this->node, mxconst::get_ELEMENT_OUTCOME(), false);
    this->xLocAndElev_ptr = Utils::xml_get_node_from_node_tree_IXMLNode(this->node, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA(), false);

    this->xElevVol        = Utils::xml_get_node_from_node_tree_IXMLNode(this->xLocAndElev_ptr, mxconst::get_ELEMENT_ELEVATION_VOLUME(), false);
    this->xRadius         = Utils::xml_get_node_from_node_tree_IXMLNode(this->xLocAndElev_ptr, mxconst::get_ELEMENT_RADIUS(), false);
    this->xRect           = Utils::xml_get_node_from_node_tree_IXMLNode(this->xLocAndElev_ptr, mxconst::get_ELEMENT_RECTANGLE(), false); // v3.0.253.5
    this->xReferencePoint = Utils::xml_get_node_from_node_tree_IXMLNode(this->xLocAndElev_ptr, mxconst::get_ELEMENT_REFERENCE_POINT(), false);

    this->xSetDatarefs         = Utils::xml_get_node_from_node_tree_IXMLNode(this->xOutcome, mxconst::get_ELEMENT_SET_DATAREFS(), false);     // v3.303.12 subnode of <outcome>
    this->xSetDatarefs_on_exit = Utils::xml_get_node_from_node_tree_IXMLNode(this->xOutcome, mxconst::get_ELEMENT_SET_DATAREFS_ON_EXIT(), false); // v3.303.12  subnode of <outcome>

    const std::string trigPostScript = Utils::xml_get_attribute_value_drill(this->node, mxconst::get_ATTRIB_POST_SCRIPT(), flag_b, mxconst::get_ELEMENT_TRIGGER());
    const std::string trigCondScript = Utils::xml_get_attribute_value_drill(this->node, mxconst::get_ATTRIB_COND_SCRIPT(), flag_b, mxconst::get_ELEMENT_CONDITIONS());


    //////////// Handle Trigger TYPES ///////////////////////////////////

    // Start with TRIGGER based SCRIPT
    if (mxconst::get_TRIG_TYPE_SCRIPT() == trigType)
    {
      if (trigPostScript.empty() && trigCondScript.empty()) // "post_script" or "cond_script" should be enough to cover the trigger script based logic
      {
        Log::logMsg("Trigger \"" + trigName + "\", is SCRIPT based, but 'cond_script' nor 'post_script' are defined. Please fix and try again. skipping trigger...");
        return false;
      }

      this->trigElevType = missionx::mx_trigger_elev_type_enum::script; // v3.305.1c
    }
    else if (mxconst::get_TRIG_TYPE_RAD() == trigType || mxconst::get_TRIG_TYPE_CAMERA() == trigType) // if radius
    {
      if (this->xRadius.isEmpty())
      {
        Log::logMsg("Trigger \"" + trigName + "\", has missing 'radius' element. Please check. Please fix and try again. skipping trigger...");
        return false;
      }
      else
      {
        // since we store data inside the XML node, we do not need to store in mapProperties
        const auto val_d = Utils::readNumericAttrib(xRadius, mxconst::get_ATTRIB_LENGTH_MT(), 50.0);
        if (val_d == 50.0)
        {
          const auto val_s = Utils::formatNumber<double>(val_d, 2);
          xRadius.updateAttribute(val_s.c_str(), mxconst::get_ATTRIB_LENGTH_MT().c_str(), mxconst::get_ATTRIB_LENGTH_MT().c_str()); // we update the XML if default value was returned. We will need this for the save file
        }
        this->setNumberProperty(mxconst::get_ATTRIB_LENGTH_MT(), val_d); // should fix the radius issue
      }
    }

    bool        isPlaneOnGround    = false;
    std::string strIsPlaneOnGround = Utils::stringToLower(Utils::readAttrib(this->xConditions, mxconst::get_ATTRIB_PLANE_ON_GROUND(), EMPTY_STRING)); // empty means catch everything

    if (!strIsPlaneOnGround.empty())
      Utils::isStringBool(strIsPlaneOnGround, isPlaneOnGround);

    //////////// Trigger POINTS ///////////////////////////////////

    // Read Points for POLY, SLOPE, RAD, CAMERA and SLOPE. Slopes have more combination though
    if ((mxconst::get_TRIG_TYPE_POLY() == trigType || mxconst::get_TRIG_TYPE_POLY() == trigType || mxconst::get_TRIG_TYPE_RAD() == trigType || mxconst::get_TRIG_TYPE_CAMERA() == trigType)) // if trigger is not script based
    {

      // Read points
      int nChilds2 = this->xLocAndElev_ptr.nChildNode(mxconst::get_ELEMENT_POINT().c_str());

      const auto lmbda_need_only_1_point = [&](const std::string& inTrigType) {
        // v3.0.253.5 added poly + rect element test since it is a special case of auto area calculation like radius
        if (mxconst::get_TRIG_TYPE_POLY () == inTrigType)
        {
          if (!this->xRect.isEmpty())
            return true;
        }


        return ((mxconst::get_TRIG_TYPE_POLY() == trigType) || (mxconst::get_TRIG_TYPE_RAD() == trigType) || (mxconst::get_TRIG_TYPE_CAMERA() == trigType)) ? true : false;
      };

      const bool flag_stopAfterFirstValidPoint = lmbda_need_only_1_point(trigType); // THIS IS FOR rad, camera AND Calculated slope

      // read all points main loop
      this->deqPoints.clear(); // this to make sure we do not have old point from the read_mission_file() class
      for (int i2 = 0; i2 < nChilds2; i2++)
      {
        Point    p;
        IXMLNode xPoint = this->xLocAndElev_ptr.getChildNode(mxconst::get_ELEMENT_POINT().c_str(), i2);
        if (!xPoint.isEmpty())
        {
          p.node = xPoint.deepCopy();
          if (p.parse_node())
          {

            if (strIsPlaneOnGround.empty() || isPlaneOnGround) // if strIsPlaneOnGround then we should test against ground level, elevation not important
              p.setElevationFt(0.0);                           // cue will fetch ground elevation from

            // skip if point is not valid
            if (p.getLat() == 0.0 || p.getLon() == 0.0) // not valid
              continue;

            this->addPoint(p); // store point in triggers vector (this->deqPoints)

            if (flag_stopAfterFirstValidPoint) // stop in case of rad/slope (slop will calculate points differently, needs only one)
              break;
          }
        }
      } // end loop over all points

      ////// v3.0.253.5 construct points for poly + rect trigger type.
      if (flag_stopAfterFirstValidPoint && mxconst::get_TRIG_TYPE_POLY () == trigType && !this->xRect.isEmpty())
      {
        if (this->deqPoints.empty())
        {
          Log::logMsgErr("[trigger rect] Not enough points for auto rectangular trigger creation. Skipping trigger: " + trigName);
          return false;
        }

        // extract rectangle information
        const std::vector<float> vecDimentions_mt = Utils::splitStringToNumbers<float> (Utils::readAttrib (this->xRect, mxconst::get_ATTRIB_DIMENSIONS(), mxconst::get_DEFAULT_RECT_DIMENTIONS()), "|");
        const auto               heading_f        = Utils::readNodeNumericAttrib<float>(this->xRect, mxconst::get_ATTRIB_HEADING_PSI(), 0.0f);

        if (vecDimentions_mt.size() > 1)
        {
          missionx::strct_box rect_area;

          // v3.0.301 B4 adding point as center // ATTRIB_FIRST_POINT_IS_CENTER_B
          if (Utils::readBoolAttrib(xRect, mxconst::get_ATTRIB_FIRST_POINT_IS_CENTER_B(), false))
          { // calculate bottom left.
            //       "*" = pCenter = center of box
            // p2-------------p3
            // |       *      |
            // p1-------------p4
            Point pCenter(this->deqPoints.front());
            Point p1_4 = Point::calcPointBasedOnDistanceAndBearing_2DPlane(pCenter, heading_f + 180.0f, (double)vecDimentions_mt.at(0), missionx::mx_units_of_measure::meter);
            Point p2_3 = Point::calcPointBasedOnDistanceAndBearing_2DPlane(pCenter, heading_f, (double)vecDimentions_mt.at(0), missionx::mx_units_of_measure::meter);

            rect_area.topLeft  = Point::calcPointBasedOnDistanceAndBearing_2DPlane(p2_3, heading_f - 90.0f, (double)vecDimentions_mt.at(1), missionx::mx_units_of_measure::meter);
            rect_area.topRight = Point::calcPointBasedOnDistanceAndBearing_2DPlane(p2_3, heading_f + 90.0f, (double)vecDimentions_mt.at(1), missionx::mx_units_of_measure::meter);
            
            rect_area.bottomLeft  = Point::calcPointBasedOnDistanceAndBearing_2DPlane(p1_4, heading_f - 90.0f, (double)vecDimentions_mt.at(1), missionx::mx_units_of_measure::meter);
            rect_area.bottomRight = Point::calcPointBasedOnDistanceAndBearing_2DPlane(p1_4, heading_f + 90.0f, (double)vecDimentions_mt.at(1), missionx::mx_units_of_measure::meter);            
          }
          else 
          {
            rect_area.bottomLeft = this->deqPoints.front();
            rect_area.calcBoxBasedOn_bottomLeft_edgeDistances_and_heading((double)(vecDimentions_mt.at(0) * (double)missionx::meter2nm), (double)(vecDimentions_mt.at(1) * (double)missionx::meter2nm), heading_f);
          }

          this->deqPoints.clear();
          this->deqPoints.push_back(rect_area.bottomLeft);
          this->deqPoints.push_back(rect_area.topLeft);
          this->deqPoints.push_back(rect_area.topRight);
          this->deqPoints.push_back(rect_area.bottomRight);

          // delete all points in trigger and store only the bottomLeft one.
          if (this->xLocAndElev_ptr.nChildNode(mxconst::get_ELEMENT_POINT().c_str()) > 1)
          {
            while (this->xLocAndElev_ptr.nChildNode(mxconst::get_ELEMENT_POINT().c_str()) > 0)
              this->xLocAndElev_ptr.getChildNode(mxconst::get_ELEMENT_POINT().c_str()).deleteNodeContent();

            rect_area.bottomLeft.storeDataToPointNode();
            this->xLocAndElev_ptr.addChild(rect_area.bottomLeft.node.deepCopy());
          }
        }
        else
        {
          Log::logMsgErr("[trigger rect] Fail to read valid auto rectangule trigger creation dimention. Skipping trigger: " + trigName);
          return false;
        }
      }
      ////// v3.0.253.5 end creating auto rectangle trigger //////


      // check if  <elevation_volume> exists
      if (strIsPlaneOnGround.empty()) // if designer set plane on ground and the attribute is empty then ignore elevation setting complitely.
      {
        this->trigElevType = missionx::mx_trigger_elev_type_enum::not_defined;    // ignore elevation settings.
        this->setNodeProperty<int>(mxconst::get_PROP_TRIG_ELEV_TYPE(), (int)this->trigElevType); // deprecated: we should not use this property in the long term, instead use the XML header.
      }
      else
      {
        if (!this->xElevVol.isEmpty() && !isPlaneOnGround) // Read element only if element "elevation volume" exists and flag "onGround" is set to no.
        {
          // elev_lower_upper will replace elev_min_max in future build
          std::string elev_lower_upper = Utils::readAttrib(xElevVol, mxconst::get_ATTRIB_ELEV_LOWER_UPPER_FT(), EMPTY_STRING);

          if (elev_lower_upper.empty())
          {
            this->trigElevType = missionx::mx_trigger_elev_type_enum::not_defined; // we do not care about elevation tests. Elevation tests will always return true.
            this->setNodeProperty<int>(mxconst::get_PROP_TRIG_ELEV_TYPE(), (int)this->trigElevType); // set as property too
          }
          else
          {
            if (!this->parseElevationVolume(elev_lower_upper, this->err)) // parse elevation volume
            {
              Log::logMsg(this->err);

              // set radius elevation to ZERO
              if ((mxconst::get_TRIG_TYPE_RAD().compare(trigType) == 0 || mxconst::get_TRIG_TYPE_CAMERA().compare(trigType) == 0) && this->deqPoints.size() > 0) // if radius
              {
                Log::logMsgErr("[fallback] Trigger: " +mxconst::get_QM() + trigName +mxconst::get_QM() + ". Modified Radius trigger elevation to ground due to failure in parsing <elevation_volume>. This means, plane should just be in area and on ground.");
                isPlaneOnGround    = true;
                strIsPlaneOnGround = mxconst::get_MX_TRUE();                                        // true
                this->setStringProperty(mxconst::get_ATTRIB_PLANE_ON_GROUND(), strIsPlaneOnGround); // save value as string since it can be empty
                this->node.updateAttribute(strIsPlaneOnGround.c_str(), mxconst::get_ATTRIB_PLANE_ON_GROUND().c_str(), mxconst::get_ATTRIB_PLANE_ON_GROUND().c_str());

                this->deqPoints.at(0).setElevationFt(0.0);                                                                                                                   // set on ground
                this->xLocAndElev_ptr.getChildNode(mxconst::get_ELEMENT_POINT().c_str()).updateAttribute("0.0", mxconst::get_ATTRIB_ELEV_FT().c_str(), mxconst::get_ATTRIB_ELEV_FT().c_str()); // set XML
                this->node.updateAttribute("0.0", mxconst::get_ATTRIB_ELEV_FT().c_str(), mxconst::get_ATTRIB_ELEV_FT().c_str());                                                         // set XML
              }
              // Skip rest of settings for SLOPE without <calc_slope> settings. It is mandatory for it.
              // a POLY trigger can ignore these settings, and just assume enter or leave events.
              if ((mxconst::get_TRIG_TYPE_POLY().compare(trigType) == 0) && !(this->node.isAttributeSet(mxconst::get_PROP_HAS_CALC_SLOPE().c_str()))) // if it is a poly or slope without calc_slope element then skip, since we must have elevation information
              {
                Log::logMsgErr("Trigger \"" + trigName + "\", must have elevation volume settings. Please fix. Skipping trigger...");
                return false;
              }
            }
          }
        }                                                // xElevVol
        else if (xElevVol.isEmpty() && !isPlaneOnGround) // fix a bug when trigger was defined but not elevation volume. This will fallback to "plane in air" option
        {
          this->trigElevType = missionx::mx_trigger_elev_type_enum::in_air;
          this->setNodeProperty<int>(mxconst::get_PROP_TRIG_ELEV_TYPE(), (int)this->trigElevType);   // this will override original settings
        }
        else
        {
          // set elevation to ZERO
          if ((mxconst::get_TRIG_TYPE_RAD().compare(trigType) == 0 || mxconst::get_TRIG_TYPE_CAMERA().compare(trigType) == 0 || mxconst::get_TRIG_TYPE_POLY().compare(trigType) == 0) && this->deqPoints.size() > 0) // if radius
          {
            this->deqPoints.at(0).setElevationFt(0.0);                                                                                                                   // set on ground
            this->xLocAndElev_ptr.getChildNode(mxconst::get_ELEMENT_POINT().c_str()).updateAttribute("0.0", mxconst::get_ATTRIB_ELEV_FT().c_str(), mxconst::get_ATTRIB_ELEV_FT().c_str()); // set XML
            this->node.updateAttribute("0.0", mxconst::get_ATTRIB_ELEV_FT().c_str(), mxconst::get_ATTRIB_ELEV_FT().c_str());                                                         // set XML
          }
        }

      } // end if condition plane_on_ground is set or not



      // read center point if present
      if (!this->xReferencePoint.isEmpty())
      {
        Point p;
        p.setLat(Utils::readNumericAttrib(xReferencePoint, mxconst::get_ATTRIB_LAT(), 0.0));
        p.setLon(Utils::readNumericAttrib(xReferencePoint, mxconst::get_ATTRIB_LONG(), 0.0));

        this->pCenter = p;
      }

    } // end if trigger poly/slope


    this->calcCenterOfArea();
    this->applyPropertiesToLocal();
    this->prepareCueMetaData();
  }


  return true;
}

void
missionx::Trigger::xml_subNodeDiscovery()
{
  if (!this->node.isEmpty())
  {
    this->xConditions     = this->node.getChildNode(mxconst::get_ELEMENT_CONDITIONS().c_str());
    this->xOutcome        = this->node.getChildNode(mxconst::get_ELEMENT_OUTCOME().c_str());
    this->xLocAndElev_ptr = this->node.getChildNode(mxconst::get_ELEMENT_LOC_AND_ELEV_DATA().c_str());

    this->xElevVol        = this->xLocAndElev_ptr.getChildNode(mxconst::get_ELEMENT_ELEVATION_VOLUME().c_str());
    this->xRadius         = this->xLocAndElev_ptr.getChildNode(mxconst::get_ELEMENT_RADIUS().c_str());
    this->xReferencePoint = this->xLocAndElev_ptr.getChildNode(mxconst::get_ELEMENT_REFERENCE_POINT().c_str());
  }
}


void
missionx::Trigger::re_arm()
{
  // v3.0.219.1

  this->setNodeProperty<bool>(mxconst::get_ATTRIB_ENABLED(), true); 
  this->setNodeProperty<bool>(mxconst::get_PROP_All_COND_MET_B(), false); 
  this->setNodeProperty<bool>(mxconst::get_PROP_SCRIPT_COND_MET_B(), false); 
  this->setNodeProperty<int>(mxconst::get_ATTRIB_ENABLED(), (int)missionx::mx_trigger_state_enum::wait_to_be_triggered_again); 

  this->bEnteringPhysicalAreaMessageFiredOnce = false; // v3.305.1b
  this->bPlaneIsInTriggerArea                 = false; // v3.305.1c

  applyPropertiesToLocal();
}

bool
missionx::Trigger::parseElevationVolume(std::string inMinMax, std::string& outErr)
{
  this->setStringProperty(mxconst::get_ATTRIB_ELEV_MIN_MAX_FT(), inMinMax);
  this->setStringProperty(mxconst::get_ATTRIB_ELEV_LOWER_UPPER_FT(), inMinMax); // v3.0.205.3 will replace ATTRIB_ELEV_MIN_MAX_FT in future


#if defined DEBUG || defined _DEBUG
  std::string trigName = getName(); // v3.0.219.2
#endif
  std::vector<std::string> vecSplitElev = mxUtils::split_v2(inMinMax, "|");
  if (vecSplitElev.empty())
  {
    outErr = "Trigger - " + getName() + " has malform 'elevation_volume' settings ";
    return false;
  }
  else
  {
    int count = (int)vecSplitElev.size();

    // check if first variable starts with "++" or "--"
    if (count == 0)
    {
      return false; // skip elevation settings
    }

    std::string strElevationWithoutPrefix = "";                   // v3.0.219.2 holds elevation without the ++/--/+++/---
    std::string numStr                    = vecSplitElev.front(); // first elev value
    double      trigMin, trigMax;
    trigMin = trigMax = 0.0;

    bool   flag_calculateRelativeToGround     = false;
    double terrainLevel_ft                    = 0.0;
    double suggestedElevationRelativeToGround = 0.0;


    // check special case with +++/--- or ++/--
    if ((numStr.find("+++") == 0) || (numStr.find("---") == 0)) // v3.0.219.2 if string starts with +++/---
    {
      strElevationWithoutPrefix = numStr.substr(3);
      XPLMProbeResult probeResult;
      this->calcCenterOfArea();
      terrainLevel_ft = UtilsGraph::getTerrainElevInMeter_FromPoint(this->pCenter, probeResult) * meter2feet;

      if (probeResult == xplm_ProbeHitTerrain)
      {
        flag_calculateRelativeToGround     = true;
        suggestedElevationRelativeToGround = Utils::stringToNumber<double>(strElevationWithoutPrefix);
        suggestedElevationRelativeToGround += terrainLevel_ft; // add terrain elevation
      }

    }                                                              // end special case where we use +++ or ---
    else if ((numStr.find("++") == 0) || (numStr.find("--") == 0)) // v3.0.219.2 if string starts with ++/--
    {
      strElevationWithoutPrefix = numStr.substr(2);
    }


    if ((numStr.find("++") == 0 || (numStr.find("+++") == 0 && flag_calculateRelativeToGround)) && count == 1 /* no lower|upper settings*/)
    {
      numStr = strElevationWithoutPrefix; //v3.0.219.2 if we probed ground we ned to retrive from 3rd character.
      if (flag_calculateRelativeToGround)                                            // v3.0.219.2
        numStr = Utils::formatNumber<double>(suggestedElevationRelativeToGround, 2); // change original numStr value to keep original code logic intact


      double elevation_d = Utils::stringToNumber<double>(numStr);

      this->setNodeProperty<double>(mxconst::get_ATTRIB_ELEV_FT(), elevation_d); // v3.0.241.1 update both properties and xml

      elevation_d += 100000.0; // define highest value = elevation_d(min) + 100000ft
      this->setNodeProperty<double>(mxconst::get_ATTRIB_ELEV_MAX_FT(), elevation_d); // v3.0.241.1 update both properties and xml

      std::string isPlaneOnGound = Utils::readAttrib(this->xConditions, mxconst::get_ATTRIB_PLANE_ON_GROUND(), "");

      this->trigElevType = missionx::mx_trigger_elev_type_enum::above;
      return true; // skip rest of code
    }
    else if ((numStr.find("--") == 0 || (numStr.find("---") == 0 && flag_calculateRelativeToGround)) && count == 1 /* no lower|upper settings*/)
    {
      numStr = strElevationWithoutPrefix;                                            //v3.0.219.2 if we probed ground we ned to retrive from 3rd character.
      if (flag_calculateRelativeToGround)                                            // v3.0.219.2
        numStr = Utils::formatNumber<double>(suggestedElevationRelativeToGround, 2); // change original numStr value to keep original code logic intact


      double dNumStr = Utils::stringToNumber<double>(numStr);
      double min     = missionx::LOWEST_GROUND_ELEV_FOR_TRIGGERS; // constant
      // make sure ATTRIB_ELEV_MAX_FT always larger than ATTRIB_ELEV_FT
      if (dNumStr < min)
      {
        double tmp = min;
        min        = dNumStr;
        dNumStr    = tmp;
      }

      this->setNodeProperty<double>(mxconst::get_ATTRIB_ELEV_FT(), min); 
      this->setNodeProperty<double>(mxconst::get_ATTRIB_ELEV_MAX_FT(), dNumStr); 


      this->trigElevType = missionx::mx_trigger_elev_type_enum::below;
      return true; // skip rest of code
    }
    else if (!flag_calculateRelativeToGround && Utils::is_number(numStr) /* keep original logic*/)
      trigMin = Utils::stringToNumber<double>(numStr);    // will test min against max to see if they are valid
    else if (flag_calculateRelativeToGround && count > 1) // if we had +++/--- sign and we have lower|upper values
      trigMin = suggestedElevationRelativeToGround;       // lower elevation should add ground level
    else
    {
      outErr = "Fail to convert: " + numStr + ", to number. Skipping trigger elevation settings.";
      return false;
    }

    // check if on ground
    if (count == 1 && trigMin == 0.0)
    {
      this->trigElevType = mx_trigger_elev_type_enum::on_ground;
      if (!this->xConditions.isEmpty())
        Utils::xml_set_attribute_in_node_asString(this->xConditions, mxconst::get_ATTRIB_PLANE_ON_GROUND(), mxconst::get_MX_YES(), this->xConditions.getName()); // v3.0.241.1 update both properties and xml

      this->setNodeProperty<int>(mxconst::get_PROP_TRIG_ELEV_TYPE(), (int)this->trigElevType);  // this will override original settings
    }
    else if (count > 1) // read max if present
    {
      numStr.clear();
      numStr = vecSplitElev.at(1);
      if (Utils::is_number(numStr))
      {
        trigMax = Utils::stringToNumber<double>(numStr); // will test min against max to see if they are valid
        if (flag_calculateRelativeToGround)              // v3.0.219.2 add terrain level to upper elevation
          trigMax += terrainLevel_ft;
      }
      else
      {
        outErr = "Fail to convert: " + numStr + ", to number. Skipping trigger elevation settings.";
        return false;
      }

      // check min < max and switch if not
      if (trigMax < trigMin)
      {
        double tmp = trigMin;
        trigMin    = trigMax;
        trigMax    = tmp;
      }

      // set properties
      this->setNodeProperty<double>(mxconst::get_ATTRIB_ELEV_FT(), trigMin); // v3.0.241.1 update both properties and xml
      this->setNodeProperty<double>(mxconst::get_ATTRIB_ELEV_MAX_FT(), trigMax); // v3.0.241.1 update both properties and xml

      this->trigElevType = mx_trigger_elev_type_enum::min_max; // set type
      this->setNodeProperty<int>(mxconst::get_PROP_TRIG_ELEV_TYPE(), (int)this->trigElevType); // v3.0.241.1 update both properties and xml

    } // if max is present
    else
    {
      outErr = "Trigger: " + this->getName() + " has an issue with attribute 'elevation_volume' does not have MAX value. Please fix settings. Skipping trigger elevation settings.";
      return false;
    } // end regular min/max validation

  } // end vecSplitElev vector is not empty


  return true;
}

// -------------------------------------

std::string
missionx::Trigger::to_string()
{
  std::string format;
  format.clear();
  format = "Trigger " +mxconst::get_QM() + this->getName() +mxconst::get_QM() + ((this->isLinked) ? " - is linked to task: " + this->linkedTo : " - is standalone.") + mxconst::get_UNIX_EOL() + "properties information:" + mxconst::get_UNIX_EOL();
  format += "Has special elevation settings: " + translateTrigElevType(this->trigElevType) + ".\n";
  format += Utils::xml_get_node_content_as_text(this->node); 
  format += Points::to_string();
  //format += "Trigger Center: " + pCenter.to_string_xy(); // v3.303.14

  format += "\n<---------->";

  return format;
}

// -------------------------------------

const std::string Trigger::get_string_debug_as_header()
{
  return this->getName() + ((this->isLinked) ? " - is linked to task: " + this->linkedTo : " - is standalone.") + " State: " + this->translateTrigState(this->trigState) + "(" + ((this->getBoolValue(mxconst::get_ATTRIB_ENABLED(), true)) ? "enabled" : "disabled") + ")";
}

// -------------------------------------

const std::string
missionx::Trigger::to_string_ui_info()
{
  // format = "Trigger" + ((this->isLinked) ? " - is linked to task: " + this->linkedTo : " - is standalone."); // v3.305.3 deprecated since we display it at the tree node level

  std::string format;
  format.clear();

  //format = "State: " + Trigger::translateTrigState(this->trigState) + ",Elevation settings: " + translateTrigElevType(this->trigElevType) + ", Rearm: " + this->getStringAttributeValue(mxconst::get_ATTRIB_RE_ARM(), "false") + "\n\n"; // v3.305.3  

  format = "Condition XML:\n";
  format += Utils::xml_get_node_content_as_text( this->xConditions) + "\n";
  format += "Outcome XML:\n";
  format += Utils::xml_get_node_content_as_text( this->xOutcome) + "\n";
  format += "Elevation XML:\n";
  format += Utils::xml_get_node_content_as_text( this->xLocAndElev_ptr) + "\n";

  return format;
}

// -------------------------------------

const std::string
missionx::Trigger::to_string_ui_info_gist()
{
  const std::string sPostScript = this->getStringAttributeValue(mxconst::get_ATTRIB_POST_SCRIPT(), "");
  return "State: " + Trigger::translateTrigState(this->trigState) + ", Elevation settings: " + translateTrigElevType(this->trigElevType) + ", Rearm: " + this->getStringAttributeValue(mxconst::get_ATTRIB_RE_ARM(), mxconst::get_MX_FALSE()) + ((sPostScript.empty()) ? "" : ", PostScript: " + sPostScript); // v3.305.3
}

// -------------------------------------

const std::string
missionx::Trigger::to_string_ui_info_conditions()
{
  const std::string trigType = Utils::readAttrib(this->node, mxconst::get_ATTRIB_TYPE(), "");
  if (trigType.compare(mxconst::get_TRIG_TYPE_SCRIPT()) == 0 )
    return   "ACM: " + this->getStringAttributeValue(mxconst::get_PROP_All_COND_MET_B(), mxconst::get_MX_FALSE()) + ", SCM: " + ((this->bScriptCondMet) ? mxconst::get_MX_TRUE() : mxconst::get_MX_FALSE()) ;
  
  return   "ACM: " + this->getStringAttributeValue(mxconst::get_PROP_All_COND_MET_B(), mxconst::get_MX_FALSE()) + ", In Area: " + ((this->bPlaneIsInTriggerArea) ? mxconst::get_MX_TRUE() : mxconst::get_MX_FALSE()) + ", SCM: " + ((this->bScriptCondMet) ? mxconst::get_MX_TRUE() : mxconst::get_MX_FALSE()) + ", TE: " + (Timer::wasEnded(this->timer) ? mxconst::get_MX_TRUE() : mxconst::get_MX_FALSE());
  
}

// -------------------------------------

void
missionx::Trigger::prepareCueMetaData() // v3.0.213.7
{
  this->cue.node_ptr             = this->node;
  this->cue.cueType              = this->cueType;
  this->cue.deqPoints_ptr        = &this->deqPoints;
  if (this->cue.cueType == missionx::mx_cue_types::cue_trigger)
    this->cue.originName = "Trigger: " + getName();
  else if (cue.cueType == missionx::mx_cue_types::cue_inventory)
    this->cue.originName = "Inventory: " + getName();
  else
    this->cue.originName = getName();

  // RAD radius
  if (this->getTriggerType().compare(mxconst::get_TRIG_TYPE_RAD ()) == 0 || this->getTriggerType().compare(mxconst::get_TRIG_TYPE_CAMERA ()) == 0)
  {
    float val_f = (float)Utils::readNumericAttrib(this->xRadius, mxconst::get_ATTRIB_LENGTH_MT(), 0.0);
    this->cue.setRadiusAsMeter(val_f); // v3.0.202a init Cue settings
    if (val_f > 0.0f)
      this->cue.hasRadius = true;
    else
      Log::logMsgErr("[trigger:prepareCueMetadata] The radius value of trigger: " + this->getName() + "might not be set. Trigger will be invalidated. Please fix trigger settings.");
  }
}

void
missionx::Trigger::setCumulative_flag(bool inVal)
{

  this->setNodeProperty<bool>(mxconst::get_ATTRIB_CUMULATIVE_TIMER_FLAG(), inVal); 
  this->timer.setCumulative_flag(inVal);
}

void
missionx::Trigger::setAllConditionsAreMet_flag(bool inVal)
{
  this->bAllConditionsAreMet = inVal;
  this->setNodeProperty<bool>(mxconst::get_PROP_All_COND_MET_B(), inVal); 
}

void
missionx::Trigger::setTrigState(missionx::mx_trigger_state_enum inState)
{
  this->trigState = inState;
  this->setNodeProperty<int>(mxconst::get_PROP_STATE_ENUM(), (int)inState); 
}

// Translate the "_trigger_elev" enum to readable string
std::string
missionx::Trigger::translateTrigElevType(mx_trigger_elev_type_enum inType)
{
  switch (inType)
  {
    case mx_trigger_elev_type_enum::above:
      return "above";
      break;
    case mx_trigger_elev_type_enum::below:
      return "below";
      break;
    case mx_trigger_elev_type_enum::min_max:
      return "min/max";
      break;
    case mx_trigger_elev_type_enum::on_ground:
      return "on ground";
      break;
    default:
      break;
  }

  // mx_trigger_elev_type_enum::not_defined
  return "no";
}

/*
never_triggered,
entered, // = triggered, is one time pointState that fires "script_name_when_enter" code.
inside_trigger_zone, // plane in trigger area or logic function.
left, // = plane left area. One time event, it fires: "script_name_when_left". The plug-in will alter the pointState to "was_not_triggered"
wait_to_be_triggered_again, // = triggered at least one time
never_trigger_again // =
*/
std::string
missionx::Trigger::translateTrigState(mx_trigger_state_enum inState)
{
  switch (inState)
  {
    case mx_trigger_state_enum::trig_fired:
      return "entered";
      break;
    case mx_trigger_state_enum::inside_trigger_zone:
      return "inside_trigger_zone";
      break;
    case mx_trigger_state_enum::left:
      return "left";
      break;
    case mx_trigger_state_enum::wait_to_be_triggered_again:
      return "wait_to_be_triggered_again";
      break;
    case mx_trigger_state_enum::never_trigger_again:
      return "never_trigger_again";
      break;
    default:
      break;
  }
  return "never_triggered";
}

void
missionx::Trigger::progressTriggerStates()
{

  if (this->trigState == missionx::mx_trigger_state_enum::trig_fired)
  {
    this->setTrigState(missionx::mx_trigger_state_enum::inside_trigger_zone); // v3.0.241.1 also set xml node
  }
  else if (this->trigState == missionx::mx_trigger_state_enum::left)
  {
    this->setTrigState(missionx::mx_trigger_state_enum::wait_to_be_triggered_again); // v3.0.241.1 also set xml node
  }
}


bool
missionx::Trigger::isInPhysicalArea(Point& pObject)
{
#ifdef TIMER_FUNC
  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), false);
#endif // TIMER_FUNC

  static bool bResult;
  bResult = false;
  const std::string trigType = Utils::readAttrib(this->node, mxconst::get_ATTRIB_TYPE(), ""); // v3.0.241.1

  if ((mxconst::get_TRIG_TYPE_POLY().compare(trigType) == 0) || (mxconst::get_TRIG_TYPE_POLY().compare(trigType) == 0))
  {
    bResult = this->isPointInPolyArea(pObject);
  }
  else if (mxconst::get_TRIG_TYPE_RAD().compare(trigType) == 0 || mxconst::get_TRIG_TYPE_CAMERA().compare(trigType) == 0) // check in Radius
  {
    double radius_nm = Utils::readNumericAttrib(this->xRadius, mxconst::get_ATTRIB_LENGTH_MT(), 50.0);
    radius_nm        = radius_nm * missionx::meter2nm;

    if (pCenter.pointState == mx_point_state::point_undefined) // v3.0.213.7
      this->calcCenterOfArea();

    bResult = isPointInRadiusArea(this->pCenter, (float)radius_nm, pObject); // v3.0.207.1
  }
  else if (mxconst::get_TRIG_TYPE_SCRIPT().compare(trigType) == 0) // trigger based on script has no physical area
  {
    bResult = true;
  } // end area type checks

  return bResult;
}
// isInPhysicalArea

bool
missionx::Trigger::isPlaneInSlope() // check if elevation is in slope angle
{
  bool bResult = false;

  return bResult;
}

bool
missionx::Trigger::isInElevationArea(Point& pObject)
{
#ifdef TIMER_FUNC
  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), false);
#endif // TIMER_FUNC


  const std::string trigType = Utils::readAttrib(this->node, mxconst::get_ATTRIB_TYPE(), ""); // v3.0.241.1
  
  if ( ( (int)(mxconst::get_TRIG_TYPE_SCRIPT().compare(trigType) == 0 )  // always in elevation since all tests need to be done in script.
       + (int)( mxconst::get_TRIG_TYPE_CAMERA().compare(trigType) == 0 ) // always in elevation since all tests need to be done in script.
       + (int)(this->trigElevType == missionx::mx_trigger_elev_type_enum::not_defined) 
       + (int)(this->trigElevType == missionx::mx_trigger_elev_type_enum::on_ground)
       ) > 0
    )
  {
    return true; // always in elevation since all tests need to be done in script.
  }



  // check faxil
//#ifndef RELEASE
////  const float faxil = XPLMGetDataf(missionx::drefConst.dref_faxil_gear_f); // 0 = in air. Anything else = on ground. // "sim/flightmodel/forces/faxil_gear"
//#endif // !RELEASE


  // check if only in air
  if (this->trigElevType == missionx::mx_trigger_elev_type_enum::in_air) // v3.0.213.4 added "just in air" cases, no need for elev volume
  {
    return (XPLMGetDataf(missionx::drefConst.dref_faxil_gear_f) == 0.0f);
  }
  else // Check volume information
  {

    // check slope
    if (mxconst::get_TRIG_TYPE_POLY().compare(trigType) == 0) // TODO test slope calculation
    {
      // call special function
      Point slopeProjectilePoint = this->deqPoints.front();
      Point planePoint           = missionx::dataref_manager::getPlanePointLocationThreadSafe();

      if (this->node.isAttributeSet(mxconst::get_PROP_HAS_CALC_SLOPE().c_str())) // special case for slope
      {
        int angle_of_projectile_3d = Utils::readNodeNumericAttrib<int>(this->node, mxconst::get_ATTRIB_SLOPE_ANGLE_3D(), 3); // default is angle of 3 degrees

        double distanceBetweenPlaneAndSlopeProjectile_nm                   = Utils::calcDistanceBetween2Points_nm(slopeProjectilePoint.getLat(), slopeProjectilePoint.getLon(), planePoint.getLat(), planePoint.getLon());
        double distanceInFeet                                              = distanceBetweenPlaneAndSlopeProjectile_nm * nm2meter * meter2feet;
        double expectedSlopeElevAtPlaneLocation_fromProjectilePoint_inFeet = Utils::calcElevBetween2Points_withGivenAngle_InFeet((float)distanceInFeet, (float)angle_of_projectile_3d);

        if (expectedSlopeElevAtPlaneLocation_fromProjectilePoint_inFeet >= planePoint.getElevationInFeet())
          return true; // in Elevation
      }
    }
    else
    {
      // check elevation volume
      const double elev_ft_min = Utils::readNodeNumericAttrib<double>(this->node, mxconst::get_ATTRIB_ELEV_FT(), 0.0);
      const double elev_ft_max = Utils::readNodeNumericAttrib<double>(this->node, mxconst::get_ATTRIB_ELEV_MAX_FT(), 0.0);
      double       oElev       = pObject.getElevationInFeet();

      // check above
      if (this->trigElevType == mx_trigger_elev_type_enum::above)
        return (oElev >= elev_ft_min);
      else if (this->trigElevType == mx_trigger_elev_type_enum::below)
        return (oElev <= elev_ft_max); // lower than max elevation // v3.0.213.4 fixed bug where condition was oposite (higher than max, instead lower max).
      else                               // min/max
        return ((oElev >= elev_ft_min) && (oElev <= elev_ft_max) );
    }

  } // end else if not "in_air" condition but instead test in envelop/volume

  return false;
}



bool
missionx::Trigger::isOnGround(Point& pObject)
{
#ifdef TIMER_FUNC
  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), false);
#endif // TIMER_FUNC


  const std::string trigType = Utils::readAttrib(this->node, mxconst::get_ATTRIB_TYPE(), ""); // v3.0.241.1

  if ( ((int)(mxconst::get_TRIG_TYPE_SCRIPT().compare(trigType) == 0)
      + (int)(mxconst::get_TRIG_TYPE_CAMERA().compare(trigType) == 0) 
      + (int)(this->trigElevType == missionx::mx_trigger_elev_type_enum::not_defined)
       ) > 0
     )
  {
    return true; // always in elevation since all tests need to be done in script.
  }

//  #ifndef RELEASE // debug
////  const float faxil = XPLMGetDataf(missionx::drefConst.dref_faxil_gear_f); // 0 = in air. Anything else = on ground.
//  #endif

  return (XPLMGetDataf(missionx::drefConst.dref_faxil_gear_f) != 0.0f); // 0.0 == in air. <>0.0f on ground

} // isOnGround



void
missionx::Trigger::storeCoreAttribAsProperties()
{
  int enumVal = 0;

  this->setNodeProperty<bool>(mxconst::get_ATTRIB_ENABLED(), this->isEnabled); // v3.0.241.1 update both property and XML node if we want

  this->setNodeProperty<bool>(mxconst::get_PROP_IS_LINKED(), this->isLinked); // v3.0.241.1 update both property and XML node if we want

  this->setNodeStringProperty(mxconst::get_PROP_LINKED_TO(), this->linkedTo); // v3.0.241.1 update both property and XML node if we want

  this->setNodeProperty<bool>(mxconst::get_PROP_All_COND_MET_B(), this->bAllConditionsAreMet); // v3.0.241.1 update both property and XML node if we want

  this->setNodeProperty<bool>(mxconst::get_PROP_SCRIPT_COND_MET_B(), this->bScriptCondMet); // v3.0.241.1 update both property and XML node if we want

  enumVal = (int)this->trigState;
  this->setNodeProperty<int>(mxconst::get_PROP_STATE_ENUM(), enumVal); // v3.0.241.1 update both property and XML node if we want

  enumVal = (int)this->trigElevType;
  this->setNodeProperty<int>(mxconst::get_PROP_TRIG_ELEV_TYPE(), enumVal); // v3.0.241.1 update both property and XML node if we want
}


void
missionx::Trigger::saveCheckpoint(IXMLNode& inParent)
{
  storeCoreAttribAsProperties();

  // v3.0.241.1
  IXMLNode xTrigger = inParent.addChild(this->node.deepCopy()); // v3.0.241.1
}


void
missionx::Trigger::applyPropertiesToLocal()
{
  assert(!this->node.isEmpty()); // abort if node is not set

  std::string err;
  err.clear();

  this->isEnabled            = Utils::readBoolAttrib(this->node, mxconst::get_ATTRIB_ENABLED(), true);          // v3.0.241.1
  this->bAllConditionsAreMet = Utils::readBoolAttrib(this->node, mxconst::get_PROP_All_COND_MET_B(), false);    // v3.0.241.1
  this->bScriptCondMet       = Utils::readBoolAttrib(this->node, mxconst::get_PROP_SCRIPT_COND_MET_B(), false); // v3.0.241.1
  this->linkedTo             = Utils::readAttrib(this->node, mxconst::get_PROP_LINKED_TO(), "");                // v3.0.241.1
  const bool flag_cumulative = Utils::readBoolAttrib(this->node, mxconst::get_ATTRIB_CUMULATIVE_TIMER_FLAG(), false);

  this->setNodeProperty<bool>(mxconst::get_PROP_All_COND_MET_B(), this->bAllConditionsAreMet); // v3.0.241.1 update both property and XML node if we want
  this->setNodeProperty<bool>(mxconst::get_PROP_SCRIPT_COND_MET_B(), this->bScriptCondMet); // v3.0.241.1 update both property and XML node if we want


  this->setNodeProperty<bool>(mxconst::get_ATTRIB_ENABLED(), this->isEnabled); // v3.0.241.1 update both property and XML node if we want
  this->setNodeProperty<bool>(mxconst::get_PROP_IS_LINKED(), this->isLinked);  // v3.0.241.1 update both property and XML node if we want



  // read enum info
  int val = 0;
  this->setNodeProperty<int>(mxconst::get_PROP_STATE_ENUM(), val); // v3.0.241.1 update both property and XML node if we want

  this->setNodeProperty<int>(mxconst::get_PROP_STATE_ENUM(), val); 
  this->trigState = (mx_trigger_state_enum)val;


  ///// Handle TYPE of ELEVATION. Is it onGround, inAir or None  /////
  bool        isOnGround         = false;                                                                                                          // v3.0.217.7
  std::string strIsPlaneOnGround = (this->xConditions.isEmpty()) ? "" : Utils::readAttrib(this->xConditions, mxconst::get_ATTRIB_PLANE_ON_GROUND(), ""); // v3.0.241.1 read directly from XML // v3.0.217.7 replaces the boolean property into string

  // v3.0.217.7 parse string value into bool
  if (!strIsPlaneOnGround.empty())
    Utils::isStringBool(strIsPlaneOnGround, isOnGround);

  // v3.0.221.11
  this->setCumulative_flag(flag_cumulative); // v3.0.241.1 reminder: also sets the XML node


  if (strIsPlaneOnGround.empty()) // v3.0.217.7
  {
    this->trigElevType = mx_trigger_elev_type_enum::not_defined;
  }
  else if (isOnGround)
  {
    this->trigElevType = mx_trigger_elev_type_enum::on_ground; // will fire if plane in physical area and on ground
  }
  else // if not on ground, read volume info
  {
    val             = Utils::readNodeNumericAttrib<int>(this->node, mxconst::get_PROP_TRIG_ELEV_TYPE(), (int)mx_trigger_elev_type_enum::on_ground); // v3.303.11 set default to "on_ground"
    int elevTypeInt = (int)this->trigElevType;

    if (val != (int)this->trigElevType)
    {
      if (val < (int)this->trigElevType)
        this->setNodeProperty<int>(mxconst::get_PROP_TRIG_ELEV_TYPE(), elevTypeInt);
      else
        this->trigElevType = (mx_trigger_elev_type_enum)val;
    }
  }

  this->setNodeProperty<int>(mxconst::get_PROP_TRIG_ELEV_TYPE(), (int)this->trigElevType); // v3.0.241.1 update both property and XML node if we want
}


// --------------------------------

void
missionx::Trigger::eval_physical_conditions_are_met(missionx::Point& p)
{
#ifdef TIMER_FUNC
  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), "Eval Physical cond are met", false);
#endif // TIMER_FUNC


  {
#ifdef TIMER_FUNC
    missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), "Calc is in physical area", false);
#endif // TIMER_FUNC
    this->flag_inPhysicalArea          = this->isInPhysicalArea(p);
    this->flag_inRestrictElevationArea = this->isInElevationArea(p);
    this->flag_isOnGround              = this->isOnGround(p);
  }

  {
#ifdef TIMER_FUNC
    missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), "Write physical info to xml node.", false);
#endif // TIMER_FUNC

    this->setNodeProperty<bool>(mxconst::get_PROP_PLANE_IN_PHYSICAL_AREA(), this->flag_inPhysicalArea);        // v3.303.14 store if plane in physical area - this does not mean it is inside the elvation volume
    this->setNodeProperty<bool>(mxconst::get_PROP_PLANE_IN_ELEV_VOLUME(), this->flag_inRestrictElevationArea); // v3.303.14 store if plane in elvation volume
    this->setNodeProperty<bool>(mxconst::get_PROP_PLANE_ON_GROUND(), this->flag_isOnGround);                   // v3.305.1c store if plane is on ground
  }

  //return this->flag_inPhysicalArea * flag_inRestrictElevationArea * this->flag_isOnGround;
}


// --------------------------------


bool
missionx::Trigger::eval_all_conditions_are_met_exclude_timer()
{
//#ifdef TIMER_FUNC
//  missionx::TimerFunc timerFunc(std::string(__FILE__), std::string(__func__), "eval all cond met except timer", false);
//#endif                                                                                  // TIMER_FUNC

  const std::string trigType = this->getStringAttributeValue(mxconst::get_ATTRIB_TYPE(), ""); // poly, rad, slope, script
  if (mxconst::get_TRIG_TYPE_SCRIPT().compare(trigType) == 0)                               // trigger based on script can also have physical tests, like on Ground/air etc..
    return this->bScriptCondMet && this->flag_inPhysicalArea && this->flag_inRestrictElevationArea && this->flag_isOnGround;

  if (this->trigElevType == missionx::mx_trigger_elev_type_enum::on_ground) // on ground
    return this->flag_inPhysicalArea && this->flag_isOnGround && this->bScriptCondMet;

  if (this->trigElevType > missionx::mx_trigger_elev_type_enum::on_ground) // in air
    return this->flag_inPhysicalArea && this->flag_inRestrictElevationArea && this->bScriptCondMet; // trig.bScriptCondMet

  if (this->trigElevType == missionx::mx_trigger_elev_type_enum::not_defined) // plane is physical area only
    return this->flag_inPhysicalArea && this->bScriptCondMet;

  Log::logMsgErr("Could not evaluate if all trigger conditions are met: " + this->getName());
  return false;
}


// --------------------------------


missionx::mxProperties
missionx::Trigger::getInfoToSeed()
{
  missionx::mxProperties seedProperties;
  seedProperties.clear();

  // the key will reflect the name to seed
  // (mxState),(mxType=trigger/script/undefined), (mxTaskActionName=trigger/script name), (mxTaskIsComplete=true|false), (mx_enabled=true|false),
  // (mx_script_conditions_met_b=true|false), (mxTaskHasBeenEvaluate=true|false), (mx_always_evaluate=true|false), (mx_mandatory=true|false)
  seedProperties.setStringProperty(mxconst::get_MX_() + mxconst::get_EXT_mxState(), Trigger::translateTrigState((missionx::mx_trigger_state_enum)Utils::readNodeNumericAttrib<int>(this->node, mxconst::get_PROP_STATE_ENUM(), (int)missionx::mx_trigger_state_enum::never_triggered)));                    // cast twice, to int and then to enum
  seedProperties.setStringProperty(mxconst::get_MX_() + mxconst::get_PROP_TRIG_ELEV_TYPE(), Trigger::translateTrigElevType((missionx::mx_trigger_elev_type_enum)Utils::readNodeNumericAttrib<int>(this->node, mxconst::get_PROP_TRIG_ELEV_TYPE(), (int)missionx::mx_trigger_elev_type_enum::not_defined))); // cast twice, to int and then to enum
  seedProperties.setStringProperty(mxconst::get_MX_() + mxconst::get_ATTRIB_ELEV_LOWER_UPPER_FT(), Utils::readAttrib(this->xElevVol, mxconst::get_ATTRIB_ELEV_LOWER_UPPER_FT(), ""));
  seedProperties.setStringProperty(mxconst::get_MX_() + mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_FIRED(), Utils::readAttrib(this->xOutcome, mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_FIRED(), ""));
  seedProperties.setStringProperty(mxconst::get_MX_() + mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_LEFT(), Utils::readAttrib(this->xOutcome, mxconst::get_ATTRIB_MESSAGE_NAME_WHEN_LEFT(), ""));
  seedProperties.setStringProperty(mxconst::get_MX_() + mxconst::get_ATTRIB_POST_SCRIPT(), Utils::readAttrib(this->node, mxconst::get_ATTRIB_POST_SCRIPT(), ""));

  seedProperties.setBoolProperty(mxconst::get_MX_() + mxconst::get_PROP_All_COND_MET_B(), Utils::readAttrib(this->node, mxconst::get_PROP_All_COND_MET_B(), mxconst::get_MX_FALSE()));
  seedProperties.setBoolProperty(mxconst::get_MX_() + mxconst::get_PROP_SCRIPT_COND_MET_B(), Utils::readAttrib(this->node, mxconst::get_PROP_SCRIPT_COND_MET_B(), mxconst::get_MX_FALSE()));
  seedProperties.setBoolProperty(mxconst::get_MX_() + mxconst::get_ATTRIB_ENABLED(), Utils::readAttrib(this->node, mxconst::get_ATTRIB_ENABLED(), mxconst::get_MX_FALSE()));
  seedProperties.setBoolProperty(mxconst::get_MX_() + mxconst::get_PROP_IS_LINKED(), Utils::readAttrib(this->node, mxconst::get_PROP_IS_LINKED(), mxconst::get_MX_FALSE()));

  seedProperties.setBoolProperty(mxconst::get_MX_() + mxconst::get_PROP_PLANE_IN_PHYSICAL_AREA(), Utils::readAttrib(this->node, mxconst::get_PROP_PLANE_IN_PHYSICAL_AREA(), mxconst::get_MX_FALSE())); // v3.303.14 store if plane in physical area - this does not mean it is inside the elvation volume
  seedProperties.setBoolProperty(mxconst::get_MX_() + mxconst::get_PROP_PLANE_IN_ELEV_VOLUME(), Utils::readAttrib(this->node, mxconst::get_PROP_PLANE_IN_ELEV_VOLUME(), mxconst::get_MX_FALSE()));     // v3.303.14 store if plane in elvation volume

  return seedProperties;
}


void
missionx::Trigger::calcCenterOfArea()
{
  this->xLocAndElev_ptr = Utils::xml_get_or_create_node(this->node, mxconst::get_ELEMENT_LOC_AND_ELEV_DATA());          // v3.0.241.1
  this->xReferencePoint = Utils::xml_get_or_create_node(this->xLocAndElev_ptr, mxconst::get_ELEMENT_REFERENCE_POINT()); // v3.0.241.1

  if (pCenter.pointState == missionx::mx_point_state::point_undefined)
  {
    std::string trigType = Utils::readAttrib(this->node, mxconst::get_ATTRIB_TYPE(), "");                              // get type of trigger
    if ((mxconst::get_TRIG_TYPE_RAD().compare(trigType) == 0 || mxconst::get_TRIG_TYPE_CAMERA().compare(trigType) == 0) && !deqPoints.empty()) // RAD
    {
      pCenter.node = deqPoints.front().node.deepCopy();
      pCenter.parse_node();
    }
    else if (mxconst::get_TRIG_TYPE_SCRIPT().compare(trigType) == 0)
    {
      // designer should define a center point if he/she wants
    }
    else if ((mxconst::get_TRIG_TYPE_POLY().compare(trigType) == 0) || (mxconst::get_TRIG_TYPE_POLY().compare(trigType) == 0))
    {
      pCenter = Trigger::calculateCenterOfShape(this->deqPoints);
    }
  }

  // v3.0.221.9 added target_pos property to use in shared_data cases
  if (!(pCenter.pointState == missionx::mx_point_state::point_undefined))
  {
    std::string trigger_pos = Utils::formatNumber<double>(pCenter.getLat(), 8) + "|" + Utils::formatNumber<double>(pCenter.getLon(), 8) + "|" + Utils::formatNumber<double>(pCenter.getElevationInFeet(), 2);
    if (trigger_pos.length() > (size_t)2)
    {
      this->setStringProperty(mxconst::get_ATTRIB_TARGET_POS(), trigger_pos);

      if (this->xLocAndElev_ptr.isEmpty())
        this->xLocAndElev_ptr = this->node.getChildNode(mxconst::get_ELEMENT_LOC_AND_ELEV_DATA().c_str()); // hopfuly will fix repeated location&elevation duplicaition

      // Update the XML elements with the center values
      this->node.updateAttribute(trigger_pos.c_str(), mxconst::get_ATTRIB_TARGET_POS().c_str(), mxconst::get_ATTRIB_TARGET_POS().c_str()); // v3.0.241.1 for debug, but we really need to write it in reference point
      if (this->xReferencePoint.isEmpty())
      {
        if (this->xLocAndElev_ptr.isEmpty())
          this->xLocAndElev_ptr = this->node.addChild(mxconst::get_ELEMENT_LOC_AND_ELEV_DATA().c_str());

        if (!xLocAndElev_ptr.isEmpty())
          this->xReferencePoint = xLocAndElev_ptr.addChild(mxconst::get_ELEMENT_REFERENCE_POINT().c_str());
      }

      if (!this->xReferencePoint.isEmpty())
      {
        const std::string lat_s    = Utils::formatNumber<double>(pCenter.getLat(), 8);
        const std::string lon_s    = Utils::formatNumber<double>(pCenter.getLon(), 8);
        const std::string elevft_s = Utils::formatNumber<double>(pCenter.getElevationInFeet(), 2);

        xReferencePoint.updateAttribute(lat_s.c_str(), mxconst::get_ATTRIB_LAT().c_str(), mxconst::get_ATTRIB_LAT().c_str());
        xReferencePoint.updateAttribute(lon_s.c_str(), mxconst::get_ATTRIB_LONG().c_str(), mxconst::get_ATTRIB_LONG().c_str());
        xReferencePoint.updateAttribute(elevft_s.c_str(), mxconst::get_ATTRIB_ELEV_FT().c_str(), mxconst::get_ATTRIB_ELEV_FT().c_str());
      }
    }
  }

  Log::logMsg("trig: " + this->getName() + ", Center: " + pCenter.to_string_xy());

}

void
missionx::Trigger::clear_condition_properties()
{
  if (this->xConditions.isEmpty())
  {
    this->xConditions = this->node.addChild(mxconst::get_ELEMENT_CONDITIONS().c_str());
  }
  Utils::xml_search_and_set_attribute_in_IXMLNode(this->xConditions, mxconst::get_ATTRIB_PLANE_ON_GROUND(), "", xConditions.getName());
  Utils::xml_search_and_set_attribute_in_IXMLNode(this->xConditions, mxconst::get_ATTRIB_COND_SCRIPT(), "", xConditions.getName());
  //Utils::xml_clear_all_node_attribute_values(this->xConditions); // v3.305.3
  Utils::xml_delete_all_node_attribute(this->xConditions);       // v3.305.3
}

void
missionx::Trigger::clear_outcome_properties()
{
  if (this->xOutcome.isEmpty())
  {
    this->xOutcome = this->node.addChild(mxconst::get_ELEMENT_OUTCOME().c_str());
  }
  this->setNodeStringProperty(mxconst::get_ATTRIB_POST_SCRIPT(), "");
  //Utils::xml_clear_all_node_attribute_values(this->xOutcome); // v3.305.3
  Utils::xml_delete_all_node_attribute(this->xOutcome);       // v3.305.3

  #ifndef RELEASE
  Utils::xml_print_node(xOutcome);
  #endif // !RELEASE
}

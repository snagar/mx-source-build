#include "../dataref_manager.h"
#ifdef IBM
#define _USE_MATH_DEFINES
#include <cmath>
#endif
#include "obj3d.h"
#include <assert.h>
#include <fcntl.h>

// #define DEBUG_MOVE
namespace missionx
{
XPLMDataRef obj3d::fps_dref;
}

// -----------------------------------------------------------------

void
missionx::obj3d::init_path_cycle()
{
  mvStat.noOfPointsInPath    = static_cast<int>(deqPoints.size());
  mvStat.currentPointNo      = 0;
  mvStat.hasReachedLastPoint = false;
  mvStat.timeOnVector      = 0.0f;

  if (mvStat.noOfPointsInPath > 0)
  {
    this->init_start_position();
  }
  else
  {
    #ifndef RELEASE
    Log::logMsg( fmt::format("!!! [{}] ERROR, No points in <path> of 3D Object >>>> {} <<<< !!!", __func__, this->getName() ) );
    #endif
  }
}

// -----------------------------------------------------------------

void
missionx::obj3d::init_start_position()
{
  // This function should be called only once to determine the kind of action that needs to be taken - static vs moving
  // Warning: we should never call initPathCycle() function from this function

  assert(mvStat.noOfPointsInPath > 0 && fmt::format("[{}] No points on path", __func__).c_str() );

  // Only handle first time
  #ifndef RELEASE
  Log::logMsg(fmt::format("[{}] start for 3D Object: >>>> {} <<<<, Counter: {}", __func__, this->getName(), mvStat.currentPointNo) );
  #endif
  mvStat.lastZuluStartDraw = 0.0f;
  mvStat.timeOnVector      = 0.0f;

  // mvStat.hasReachedPointTo_need_to_continue_or_cycle = Utils::readBoolAttrib(this->node, mxconst::get_ATTRIB_CYCLE(), false);

  this->itPathEnd = this->deqPoints.end();
  if (this->g_instance_ref) // is object visible
  {

    this->itPath = this->deqPoints.begin(); // should point to the starting point
    mvStat.pointFrom.clone((*this->itPath));

    if (mvStat.noOfPointsInPath == 1)
    {
      mvStat.isMoving                        = false; // flag object as not moving since only one point
      mvStat.isInRecursiveState              = false;
      mvStat.hasReachedLastPoint             = true;
      mvStat.pointFrom.timeToWaitOnPoint_sec = 0.0f;

    } // check next point not overflow
    else if (mvStat.noOfPointsInPath > 1)
    {
      // v26.03.1 copied from "calculate_position_on_path()"
      mvStat.isFirstTime = false;
      if (this->itPath->getSpeedKmh() > 0.0)
        mvStat.pointFrom.setSpeedInKmh(this->itPath->getSpeedKmh()); // v3.0.253.7 fix speed not inherited correctly due to the fact we clone from "displayCoordinate"


      ++mvStat.currentPointNo;
      ++this->itPath; // next point for next iteration.

      mvStat.hasReachedPointTo = false;
      mvStat.isMoving          = true; // v3.0.207.4 flag object as moving
    } // end if more than 1 point in path

    mvStat.isFirstTime = false;

    mvStat.pointTo.clone((*this->itPath)); // If we are a static 3D Object, then same as first Point if not then same as the second Point
    mvStat.pointFrom.calcSimLocalData();
    mvStat.pointTo.calcSimLocalData();

    mvStat.prevPoint.clone(mvStat.pointFrom);

    this->displayCoordinate.clone(mvStat.pointFrom); // where to place 3D Object
    this->displayCoordinate.parse_node();

    // Calculate new statistic info ( S = V * T )
    calcNewCourseBetweenTwoPointsOnVector(); // We call it for static or moving objects

    // validate we have not reached end
    if (this->itPathEnd == this->itPath)
      mvStat.hasReachedLastPoint = true;
    else
    {
      // set wait timer
      this->setNextWaitTimer();
    }


  } // end if rendering instance
}

// -----------------------------------------------------------------

void
missionx::obj3d::set_next_point()
{
  if (mvStat.noOfPointsInPath > 1)
  {
    ++this->itPath;

    // has reached last point ?
    if (itPathEnd == itPath)
    {
      mvStat.hasReachedLastPoint = true;

      // Do not change status to complete if it is a MOVING_TARGET type
      if (this->getIsPathNeedToCycle() && mvStat.noOfPointsInPath > 1)
      {
        mvStat.isInRecursiveState = true;
        // init_path_cycle(true); // This is a recursive, might cause an issue.
        init_path_cycle(); // This is a recursive, might cause an issue.
        mvStat.isInRecursiveState = false;
      }
      else if ( !mvStat.isInRecursiveState) // if started at least 1 iteration
      {
        mvStat.hasReachedLastPoint = true;
        mvStat.isMoving            = false; // v3.0.207.4 flag object as not moving since only one point
      }

      this->displayCoordinate.clone(mvStat.pointFrom); // v3.0.207.2 // where to place 3D Object
    }
    else // if we have not reached the end of the path then copy location information
    {
      mvStat.pointFrom.clone(this->displayCoordinate); // v3.0.207.3 for smooth transition.

      if (this->itPath->getSpeedKmh() > 0.0f)
        mvStat.pointFrom.setSpeedInKmh(this->itPath->getSpeedKmh()); // v3.0.253.7 fix speed not inherited correctly due to the fact we clone from "displayCoordinate"

      mvStat.pointFrom.calcSimLocalData();

      mvStat.pointTo.clone((*itPath)); // Copy the next point definitions to "pointTo", this includes the speed
      mvStat.pointTo.calcSimLocalData();

      // Calculate new statistic info ( S = V * T )
      calcNewCourseBetweenTwoPointsOnVector(); // We call it twice in this function, once for the first time and now when there is point transition
      mvStat.hasReachedPointTo = false;
      mvStat.isMoving          = true; // flag object as moving

      this->setNextWaitTimer();
    }
  }
}

// -----------------------------------------------------------------

void
missionx::obj3d::setNextWaitTimer()
{
  // set wait timer
  mvStat.waitTimer.reset();
  // if (mvStat.pointFrom.timeToWaitOnPoint_sec == 0.0f)
  //   mvStat.waitTimer.reset();
  // else
  // {
  //   mvStat.waitTimer.reset();
  //   missionx::Timer::start(mvStat.waitTimer, mvStat.pointFrom.timeToWaitOnPoint_sec, "Obj3d_Wait_Timer_" + this->getName()); // Setting Wait Time Values
  // }

  if (mvStat.pointFrom.timeToWaitOnPoint_sec > 0.0f)
    missionx::Timer::start(mvStat.waitTimer, mvStat.pointFrom.timeToWaitOnPoint_sec, "Obj3d_Wait_Timer_" + this->getName()); // Setting Wait Time Values

  // init timer if and only if timer is running. The mvStat.timer help calculating position on vector
  if (mvStat.timer.isRunning() && !mvStat.waitTimer.isRunning())
  {
    missionx::Timer::start(mvStat.timer, 0, "Obj3d_timer_" + this->getName());

    mvStat.lastZuluStartDraw = 0.0f; // mvStat.timer->getZuluStartTime();
    mvStat.timeOnVector      = 0;
  }

  mvStat.prevPoint.setLat(mvStat.pointFrom.getLat());
  mvStat.prevPoint.setLon(mvStat.pointFrom.getLon());
}

missionx::obj3d::obj3d()
{
  this->obj3dType                             = missionx::obj3d::obj3d_type::static_obj;
  this->cueType                               = missionx::mx_cue_types::cue_obj; // v.0.303.6
  this->displayDefaultObjectFileOverAlternate = true;
  this->isInDisplayList                       = false;

  this->setBoolProperty(mxconst::get_PROP_SCRIPT_COND_MET_B(), true);
  this->file_and_path.clear();

  this->g_object_ref   = nullptr;
  this->g_instance_ref = nullptr;

  dr.structSize = sizeof(dr);

  this->deqPoints.clear(); // v3.0.213.7

  fps_dref = XPLMFindDataRef("sim/time/framerate_period"); // v3.0.223.5 framerate_period = 1/fps

  isScriptCondMet = false; // v3.0.209.2
}

// -----------------------------------------------------------------

missionx::obj3d::~obj3d() = default;

// -----------------------------------------------------------------

bool
missionx::obj3d::parse_node()
{
  assert(!this->node.isEmpty()); // v3.0.241.1

  std::string name, file_name, alternate_obj_file;
  name.clear();
  file_name.clear();
  alternate_obj_file.clear();

  name               = Utils::readAttrib(this->node, mxconst::get_ATTRIB_NAME(), "");
  file_name          = Utils::readAttrib(this->node, mxconst::get_ATTRIB_FILE_NAME(), "");
  alternate_obj_file = Utils::readAttrib(this->node, mxconst::get_ATTRIB_ALTERNATE_OBJ_FILE(), "");

  if (name.empty() || file_name.empty())
  {
    Log::logMsgErr("[parse_node 3d obj] Found 3d element without name or file name. Skipping...");
    return false;
  }

  this->name = name;
  this->setStringProperty(mxconst::get_ATTRIB_FILE_NAME(), file_name);
  this->setStringProperty(mxconst::get_ATTRIB_ALTERNATE_OBJ_FILE(), alternate_obj_file);

  // new v3.0.241.1 decide if it is a moving 3D object or static one
  if (const int xPathPointNodes = this->node.getChildNode(mxconst::get_ELEMENT_PATH().c_str()).nChildNode(mxconst::get_ELEMENT_POINT().c_str())
    ; xPathPointNodes >= 1)
  {
    this->setNodeProperty<int> (mxconst::get_ATTRIB_OBJ3D_TYPE(), static_cast<int> (obj3d::obj3d_type::moving_obj));
    this->obj3dType = obj3d::obj3d_type::moving_obj;
  }
  else
  {
    this->setNodeProperty<int> (mxconst::get_ATTRIB_OBJ3D_TYPE(), static_cast<int> (obj3d::obj3d_type::static_obj));
    this->obj3dType = obj3d::obj3d_type::static_obj;
  }

  const std::string instance_name = Utils::readAttrib(this->node, mxconst::get_ATTRIB_INSTANCE_NAME(), "");
  if (!instance_name.empty())
  {
    // Read conditions element

    // v3.0.241.10 b3 <displayObject> pointer // v3.0.303.2 fix by picking the correct sub <display_object> with attrib "instance_name" and not the first one. We can have multiple <display_object> for one <obj3d>
    this->xDisplayObject_ptr = this->node.getChildNodeWithAttribute(mxconst::get_ELEMENT_DISPLAY_OBJECT().c_str(), mxconst::get_ATTRIB_INSTANCE_NAME().c_str(), instance_name.c_str());

    this->xConditions = Utils::xml_get_or_create_node_ptr (this->node, mxconst::get_ELEMENT_CONDITIONS());

    const std::string distance_to_display_s = Utils::readAttrib (this->xConditions, mxconst::get_ATTRIB_DISTANCE_TO_DISPLAY_NM(), "10"); // 10nm default distance to display 3D object
    const std::string keep_until_leg_s = Utils::readAttrib (this->xConditions, mxconst::get_ATTRIB_KEEP_UNTIL_LEG(), mxconst::get_ATTRIB_KEEP_UNTIL_GOAL(), "", true); // compatible with leg
    const std::string cond_script_s = Utils::readAttrib (this->xConditions, mxconst::get_ATTRIB_COND_SCRIPT(), "");

    std::set<std::string> exceptionAttributeSet = { mxconst::get_ATTRIB_NAME(), mxconst::get_ATTRIB_FILE_NAME() };
    Utils::xml_copy_node_attributes_excluding_black_list(this->xConditions, this->node, &exceptionAttributeSet);

    // writing information from xConditions to xObj element
    const auto distance_to_display_d = mxUtils::stringToNumber<double>(distance_to_display_s, distance_to_display_s.length()); // v3.303.11 fix length bug, was always 1 now it is the length of the string so text value "10" should have length of 2 and not 1
    this->setNodeProperty<double>(mxconst::get_ATTRIB_DISTANCE_TO_DISPLAY_NM(), distance_to_display_d);
    this->setNodeStringProperty(mxconst::get_ATTRIB_KEEP_UNTIL_LEG(), keep_until_leg_s);  // store in <obj3d > element
    this->setNodeStringProperty(mxconst::get_ATTRIB_COND_SCRIPT(), cond_script_s);
    // end Condition element

    // moved path read before location, since in moving object we do not really need it
    // read 3D PATH /////
    this->xPath = this->node.getChildNode(mxconst::get_ELEMENT_PATH().c_str());
    if (!xPath.isEmpty())
    {
      // read cycle attribute
      const bool flag_cycle = Utils::readBoolAttrib(xPath, mxconst::get_ATTRIB_CYCLE(), false);
      this->setBoolProperty (mxconst::get_ATTRIB_CYCLE(), flag_cycle);

      const int numPoint_in_Elements = xPath.nChildNode(mxconst::get_ELEMENT_POINT().c_str());
      for (int i1 = 0; i1 < numPoint_in_Elements; i1++)
      {
        IXMLNode xPoint = this->xPath.getChildNode(mxconst::get_ELEMENT_POINT().c_str(), i1);
        if (!xPoint.isEmpty())
        {
          missionx::Point p;
          p.node = xPoint.deepCopy();
          if (p.parse_node())
            this->deqPoints.push_back(p);
          else
            Log::logMsgErr( fmt::format ("[{}] Point: ({:.6f}/{:.6f}), is not valid. Skipping to next coordination...", __func__, p.lat, p.lon) );
        }
      } // end loop over points
    }   // end xPath

    // decide if static or moving object /////
    if (this->deqPoints.empty()) // replaced path with deqPoints //v3.0.202 is this a static(default) or moving object. This test is not conclusive, in future we will check for scripts too.
    {
      this->setNodeProperty<int>(mxconst::get_ATTRIB_OBJ3D_TYPE(), static_cast<int> (obj3d::obj3d_type::static_obj));
      this->obj3dType = obj3d::obj3d_type::static_obj;
    }
    else
    {
      this->setNodeProperty<int>(mxconst::get_ATTRIB_OBJ3D_TYPE(), static_cast<int> (obj3d::obj3d_type::moving_obj));
      this->obj3dType = obj3d::obj3d_type::moving_obj;

      Utils::xml_copy_node_attributes(this->xLocation, this->deqPoints.front().node);
    }


    // read location element for static 3D Objects + validate ///////
    // missionx::mx_location_3d_objects info; // v26.06.1 defined at the class header as protected
    // v3.0.241.10b3 copy attributes from display object to Obj3D

    missionx::mx_location_3d_objects strct_initial_location_info;
    if (!this->xDisplayObject_ptr.isEmpty())
    {
      strct_initial_location_info = Point::readLocationElement(xDisplayObject_ptr); // Read location information from the specific <display_object>

      #ifndef RELEASE
      Log::logMsgNone( Utils::xml_get_node_content_as_text(xDisplayObject_ptr));
      #endif
    }
    if (this->obj3dType == obj3d::obj3d_type::static_obj)
    {

      this->xLocation = this->node.getChildNode(mxconst::get_ELEMENT_LOCATION().c_str());
      if (!this->xLocation.isEmpty()) // v3.0.217.5 added if element is valid
      {
        if (Utils::is_number(strct_initial_location_info.lat) && Utils::is_number(strct_initial_location_info.lon))
        {
          this->setNodeStringProperty(mxconst::get_ATTRIB_LAT(), strct_initial_location_info.lat);
          this->setNodeStringProperty(mxconst::get_ATTRIB_LONG(), strct_initial_location_info.lon);
        }
        else
        {
          Log::logMsgErr(fmt::format("[{}] One of the coordination Lat/Lon might be malformed in 3D Object: '{}'. Skipping...",__func__, name) );
          return false;
        }
      } // end location is valid


      // Copy <display_object> attributes over to the Obj3D parent element
      std::set<std::string> local_excludeAttributeSet = { mxconst::get_ATTRIB_NAME(), mxconst::get_ATTRIB_FILE_NAME() };

      Utils::xml_copy_node_attributes_excluding_black_list(xDisplayObject_ptr, this->node, &local_excludeAttributeSet);


    } // end if 3D Object is static


    // read Tilt info
    this->xTilt = Utils::xml_get_or_create_node(this->node, mxconst::get_ELEMENT_TILT(), false); // v3.0.241.10 b3 fix bug where we write to <tilt> but it was not defined

    double pitch, roll;
    double heading = pitch = roll = 0.0; // mxconst::get_ZERO();

    if (!this->xTilt.isEmpty())
    {
      heading = Utils::readNodeNumericAttrib<double>(this->xTilt, mxconst::get_ATTRIB_HEADING_PSI(), 0.0);
      if (!strct_initial_location_info.heading.empty() && mxUtils::is_number(strct_initial_location_info.heading))
        heading = Utils::stringToNumber<double>(strct_initial_location_info.heading, 4);


      pitch = Utils::readNodeNumericAttrib<double>(this->xTilt, mxconst::get_ATTRIB_PITCH(), 0.0);
      if (!strct_initial_location_info.pitch.empty() && mxUtils::is_number(strct_initial_location_info.pitch))
        pitch = Utils::stringToNumber<double>(strct_initial_location_info.pitch, 4);

      roll = Utils::readNodeNumericAttrib<double>(this->xTilt, mxconst::get_ATTRIB_ROLL(), 0.0);
      if (!strct_initial_location_info.roll.empty() && mxUtils::is_number(strct_initial_location_info.roll))
        roll = Utils::stringToNumber<double>(strct_initial_location_info.roll, 4);
    }

    // write to parent node
    this->setNodeProperty<double>(mxconst::get_ATTRIB_HEADING_PSI(), heading);
    this->setNodeProperty<double>(mxconst::get_ATTRIB_PITCH(), pitch);
    this->setNodeProperty<double>(mxconst::get_ATTRIB_ROLL(), roll);
    // write to tilt node
    this->setNodeProperty<double>(xTilt, mxconst::get_ATTRIB_HEADING_PSI(), heading, xTilt.getName());
    this->setNodeProperty<double>(xTilt, mxconst::get_ATTRIB_PITCH(), pitch, xTilt.getName());
    this->setNodeProperty<double>(xTilt, mxconst::get_ATTRIB_ROLL(), roll, xTilt.getName());

    // add default value
    this->setNodeProperty<bool>(mxconst::get_PROP_DISPLAY_DEFAULT_OBJECT_FILE_OVER_ALTERNATE(), true); // display default object file or "alternate" obj file

    // v3.0.241.7 add missing  mxconst::get_ATTRIB_HIDE()
    const bool hide_b = this->getHideObject();
    this->setNodeProperty<bool>(mxconst::get_ATTRIB_HIDE(), hide_b);


    this->applyPropertiesToLocal();

    //#ifndef RELEASE
    //      {
    //        IXMLRenderer printXML;
    //        Log::logMsg("\nObject3D:" + this->getName() + "\n" + std::string(printXML.getString(this->node)) + "\n\n");
    //      }
    //#endif // !RELEASE


    /////// Instance data - from save point //////
    this->xInstance = this->node.getChildNode(mxconst::get_PROP_INSTANCE_DATA_ELEMENT().c_str());
    if (!xInstance.isEmpty())
    {
      Log::logDebugBO("This 3D Object element is an instanced one");

      this->setNodeStringProperty(mxconst::get_ATTRIB_INSTANCE_NAME(), Utils::readAttrib(xInstance, mxconst::get_ATTRIB_NAME(), ""));  // v3.0.303.2 added support to store in xml node
      this->setNodeProperty<int>(mxconst::get_PROP_CURRENT_POINT_NO(), Utils::readNodeNumericAttrib<int>(xInstance, mxconst::get_PROP_CURRENT_POINT_NO(), 0)); // v3.0.303.2 added support to store in xml node
      this->setNodeProperty<bool>(mxconst::get_PROP_LOADED_FROM_CHECKPOINT(), Utils::readBoolAttrib(xInstance, mxconst::get_PROP_LOADED_FROM_CHECKPOINT(), true)); // v3.0.303.2 added support to store in xml node
      this->setNodeProperty<bool>(mxconst::get_ATTRIB_DISPLAY_AT_POST_LEG_B(), Utils::readBoolAttrib(xInstance, mxconst::get_ATTRIB_DISPLAY_AT_POST_LEG_B(), false)); // v3.303.11 add the instance property "display_at_post_leg_b"


      this->displayCoordinate.parse_savepoint_format_to_point(Utils::readAttrib(xInstance, mxconst::get_PROP_CURRENT_LOCATION(), ""));
      this->mvStat.pointFrom.parse_savepoint_format_to_point(Utils::readAttrib(xInstance, mxconst::get_PROP_POINT_FROM(), ""));
      this->mvStat.pointTo.parse_savepoint_format_to_point(Utils::readAttrib(xInstance, mxconst::get_PROP_POINT_TO(), ""));
      this->mvStat.currentPointNo = Utils::readNodeNumericAttrib<int>(xInstance, mxconst::get_PROP_CURRENT_POINT_NO(), 0);

      this->mvStat.prevPoint = this->mvStat.pointFrom;
    }
  }

   // end if <object> element is valid

  return true;
}

// -----------------------------------------------------------------

void
missionx::obj3d::prepareCueMetaData()
{
  this->cue.node_ptr             = this->node;
  this->cue.cueType              = this->cueType;
  this->cue.deqPoints_ptr        = &this->deqPoints;

  this->cue.originName = "Obj3D: " + this->getName();

  this->cue.setRadiusAsMeter(10.0f); // set constant radius size to 5 meters
  this->cue.hasRadius = true; // we always use radius for instanced objects, we just want to distinguish them in the 3D world

  this->cue.color.setToPeach_orange();
  this->cue.canBeRendered = true;
}

// -----------------------------------------------------------------

void
missionx::obj3d::load_cb(const char* real_path, void* ref)
{
  auto* dest = static_cast<XPLMObjectRef*>(ref);
  if (*dest == nullptr)
  {
    *dest = XPLMLoadObject(real_path);
  }
}

// -----------------------------------------------------------------

void obj3d::create_instance(const std::string& in_current_leg_name)
{
  if (this->g_object_ref)
  {
    if (this->g_instance_ref) // v3.0.207.2 Try to solve instance vanish after providing a new location. In this code, we will create new instance every frame.
      XPLMDestroyInstance(this->g_instance_ref);

    this->g_instance_ref = XPLMCreateInstance(this->g_object_ref, nullptr);
    this->setNodeStringProperty(mxconst::get_ATTRIB_INSTANCE_CREATED_IN_LEG(), in_current_leg_name); // v26.03.1
  }

  #ifndef RELEASE
  if (this->g_instance_ref) // debug
    Log::logMsg(fmt::format("Instance created: {} \n", this->getInstanceName()) );
  else
    Log::logMsg("Instance: " + this->getName() + " Fail to create... \n");
  #endif
}

// -----------------------------------------------------------------

void
missionx::obj3d::cb_calc_pos_of_the_moving_object(const float &inElapsedSinceLastCall, const float &inElapsedTimeSinceLastFlightLoop, const int &inCounter)
{
  // The following function will calculate position using time between inElapsedSinceLastCall

  #ifdef DEBUG_MOVE
  Log::logMsg( fmt::format ("[{}] {} => inElapsedSinceLastCall: {:.5f}, inElapsedTimeSinceLastFlightLoop: {:.5f}, inCounter: {}", __func__, this->getInstanceName(), inElapsedSinceLastCall, inElapsedTimeSinceLastFlightLoop, inCounter ));
  #endif


  // do we need to calculate anything ?
  if (this->mvStat.hasReachedLastPoint)
  {
    #ifdef DEBUG_MOVE
    Log::logMsg( fmt::format ("[{}]--> dist_lat: {:.7f}, dist_lon: {:.7f}), mvStat.hasReachedLastPoint: {}.", __func__, this->displayCoordinate.getLat(), this->displayCoordinate.getLon(), mvStat.hasReachedLastPoint ) );
    #endif

    return;
  }

  // Are we there ?
  mvStat.hasReachedPointTo = (fabs(this->displayCoordinate.getLat() - mvStat.pointFrom.getLat()) >= fabs(mvStat.pointTo.getLat() - mvStat.pointFrom.getLat())
                              ||
                              fabs(this->displayCoordinate.getLon() - mvStat.pointFrom.getLon()) >= fabs(mvStat.pointTo.getLon() - mvStat.pointFrom.getLon()));


  if (this->mvStat.hasReachedPointTo && this->mvStat.noOfPointsInPath > 1)
  {
    #ifndef RELEASE
    Log::logMsg(fmt::format("\n[{}] >>>>>> START debug Reached Point To <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<", __func__ ) );
    // Log::logMsg(fmt::format("[{}] >>>>>> displayCoordinate: {},{} <<  pointFrom: {},{} << pointTo: {},{} <<<<<<", __func__, displayCoordinate.getLat(), displayCoordinate.getLon(), mvStat.pointFrom.getLat(), mvStat.pointFrom.getLon(), mvStat.pointTo.getLat(), mvStat.pointTo.getLon(), mvStat.pointTo.getLon() ));
    Log::logMsg(fmt::format("[{}] >>>>>> displayCoordinate: {},{} <<  pointFrom: {},{} << pointTo: {},{} <<<<<<", __func__, displayCoordinate.lat, displayCoordinate.lon, mvStat.pointFrom.lat, mvStat.pointFrom.lon, mvStat.pointTo.lat, mvStat.pointTo.lon, mvStat.pointTo.lon ));
    Log::logMsg(fmt::format("[{}] >>>>>> Lat displayCoordinate - pointFrom: {} >= pointTo - pointFrom: {} ? {}", __func__, fabs(this->displayCoordinate.getLat() - mvStat.pointFrom.getLat()), fabs(displayCoordinate.getLat() - mvStat.pointFrom.getLat() ), fabs(this->displayCoordinate.getLat() - mvStat.pointFrom.getLat()) >= fabs(mvStat.pointTo.getLat() - mvStat.pointFrom.getLat()) ) );
    Log::logMsg(fmt::format("[{}] >>>>>> Lon displayCoordinate - pointFrom: {} >= pointTo - pointFrom: {} ? {}", __func__, fabs(this->displayCoordinate.getLon() - mvStat.pointFrom.getLon()), fabs(displayCoordinate.getLon() - mvStat.pointFrom.getLon() ), fabs(this->displayCoordinate.getLon() - mvStat.pointFrom.getLon()) >= fabs(mvStat.pointTo.getLon() - mvStat.pointFrom.getLon()) ) );
    Log::logMsg(fmt::format("[{}] >>>>>> End debug Reached Point To <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n", __func__ ) );
    #endif


    ++this->itPath;
    this->mvStat.currentPointNo++;
    mvStat.timeOnVector = 0.0f;

    // Check if we reached the end of the path, and if we need to cycle
    // if (this->mvStat.currentPointNo >= this->mvStat.noOfPointsInPath)
    // if we reached the end of the iterator
    if (this->itPath == this->itPathEnd)
    {
      this->mvStat.hasReachedLastPoint = true;
      this->itPath = this->deqPoints.begin(); // make sure to point to first <point> in <path>

      #ifndef RELEASE
      Log::logMsg(fmt::format("[{}]==> Reached Cycle Decision.\n", __func__));
      #endif

      if (this->getIsPathNeedToCycle())
      {
        #ifndef RELEASE
        Log::logMsg(fmt::format("[{}]==> Will Cycle !\n", __func__));
        #endif
        this->mvStat.currentPointNo = 0;
        this->mvStat.hasReachedLastPoint = false;
        // this->mvStat.hasReachedPointTo_need_to_continue_or_cycle = true;
      }
      else
      {
        this->mvStat.timer.stop();
        this->mvStat.currentPointNo = 0;
        // this->mvStat.hasReachedPointTo_need_to_continue_or_cycle = false;
      }
    }

    // ---------------------------
    // Assign new points from path
    // ---------------------------
    if ( !this->mvStat.hasReachedLastPoint)
    {
      #ifndef RELEASE
      Log::logMsg(fmt::format("[{}]==> Reached mvStat.currentPointNo: {}\n", __func__, mvStat.currentPointNo));
      #endif

      // clone the data from the next point, but retain the current lat/lon
      this->displayCoordinate.setSpeedInKmh(mvStat.pointTo.getSpeedKmh());
      this->displayCoordinate.setAdjustHeading(mvStat.pointTo.getAdjustHeading());
      // store key heading attributes
      const auto adjust_heading_on_point_arrival = mvStat.pointTo.getAdjustHeading();
      const auto point_to_heading_s = Utils::readAttrib(mvStat.pointTo.node, mxconst::get_ATTRIB_HEADING_PSI(), "");
      bool b_pointTo_does_not_have_heading_psi_attribute = true;

      #ifndef RELEASE
      if (mvStat.currentPointNo == 0)
        auto dummy_breakpoint = 0;
      auto debug_display_coordinate_heading = this->displayCoordinate.getHeading();
      Log::logMsg(fmt::format("[{}]==> displayCoordinate heading={}, mvStat.pointTo: adjustHeading={}, heading_psi={}\n", __func__, debug_display_coordinate_heading, mvStat.pointTo.getAdjustHeading(), mvStat.pointTo.getHeading() ));
      #endif
      if (!point_to_heading_s.empty() && point_to_heading_s != "0" && mxUtils::is_number(point_to_heading_s) )
      {
          this->displayCoordinate.setHeading(mxUtils::stringToNumber<double>(point_to_heading_s));
          this->displayCoordinate.setAdjustHeading(0.0); // reset adjust heading
          b_pointTo_does_not_have_heading_psi_attribute = false;
      }

      // Initialize next point
      // Double check the iterator is not out of bounds.
      if (this->itPath == this->itPathEnd)
        this->itPath = this->deqPoints.begin();

      mvStat.pointTo.clone((*this->itPath));

      this->mvStat.pointFrom.clone(this->displayCoordinate);
      #ifndef RELEASE
      Log::logMsg(fmt::format("[{}] 1 >>>>>> displayCoordinate: {},{}:{} <<  pointFrom: {},{}:{} << pointTo: {},{}:{} <<<<<<", __func__, displayCoordinate.lat, displayCoordinate.lon, displayCoordinate.heading, mvStat.pointFrom.lat, mvStat.pointFrom.lon, mvStat.pointFrom.heading, mvStat.pointTo.lat, mvStat.pointTo.lon, mvStat.pointTo.heading ));
      #endif

      this->displayCoordinate.parse_node();
      this->mvStat.pointFrom.setSpeedInKmh(this->displayCoordinate.getSpeedKmh());

      this->mvStat.pointFrom.clone(this->displayCoordinate);
      #ifndef RELEASE
      Log::logMsg(fmt::format("[{}] 2 >>>>>> displayCoordinate: {},{}:{} <<  pointFrom: {},{}:{} << pointTo: {},{}:{} <<<<<<", __func__, displayCoordinate.lat, displayCoordinate.lon, displayCoordinate.heading, mvStat.pointFrom.lat, mvStat.pointFrom.lon, mvStat.pointFrom.heading, mvStat.pointTo.lat, mvStat.pointTo.lon, mvStat.pointTo.heading ));
      #endif

      this->mvStat.currentTimeElapsed_sinceStart = 0.0f;
      this->calcNewCourseBetweenTwoPointsOnVector (); // init: "mvStat.secondsToReachTarget" and "mvStat.distanceFromTo_meters"
      this->setNextWaitTimer();

      // Modify heading only if its value is not Zero ("0"). //v26.06.1 Moved code. Immediate transition.
      if (b_pointTo_does_not_have_heading_psi_attribute)
      {
        if (adjust_heading_on_point_arrival < 0.0f)
          this->displayCoordinate.setHeading(this->displayCoordinate.getHeading() - std::fabs(adjust_heading_on_point_arrival) );
        else
          this->displayCoordinate.setHeading(this->displayCoordinate.getHeading() + adjust_heading_on_point_arrival);
      }

      this->displayCoordinate.setPitch(this->mvStat.pointTo.getPitch());
      this->displayCoordinate.setRoll(this->mvStat.pointTo.getRoll());

      this->mvStat.pointFrom.clone(this->displayCoordinate);
      #ifndef RELEASE
      Log::logMsg(fmt::format("[{}] 3 >>>>>> displayCoordinate: {},{}:{} <<  pointFrom: {},{}:{} << pointTo: {},{}:{} <<<<<<", __func__, displayCoordinate.lat, displayCoordinate.lon, displayCoordinate.heading, mvStat.pointFrom.lat, mvStat.pointFrom.lon, mvStat.pointFrom.heading, mvStat.pointTo.lat, mvStat.pointTo.lon, mvStat.pointTo.heading ));
      #endif


      #ifndef RELEASE
      Log::logMsg( fmt::format("\t\t\t===> New Point: {}\n\n\n", mvStat.currentPointNo) );
      #endif
    }
  } // end handling if we reached point on path

  // ---------------
  // Eval waitTimer
  // ---------------
  missionx::Timer::wasXplaneTimerEnded(mvStat.waitTimer);

  // Wait timer on path change. Evaluate each call, since we also need to start the movement timer
  if (!this->mvStat.waitTimer.isRunning() && mvStat.timer.getState() == missionx::mx_timer_state::timer_not_set) // check if timer started
  {
    missionx::Timer::start(mvStat.timer, 0, "obj3d_timer_" + this->getName()); // SecondsToRun=0 => "run continuously"
    mvStat.lastZuluStartDraw = 0.0f;
  }

  #ifdef DEBUG_MOVE
  // Log::logMsg( fmt::format ("[{}]--> dist_lat: {:.7f}, dist_lon: {:.7f}), total_dist_deg: {:.7f}, mvStat.hasReachedPointTo: {}. WaitTimer: {:.2f}, Timer: {:.2f}", __func__, dist_lat, dist_lon, total_dist_deg, mvStat.hasReachedPointTo, mvStat.waitTimer.getSecondsPassed_for_TotalXP(), mvStat.timer.getSecondsPassed_for_TotalXP()) );
  #endif


  // -----------------------------------------------
  // Calculate and assign the new relative position
  // -----------------------------------------------
  const auto b_we_have_not_reached_last_point_on_path = !this->mvStat.hasReachedLastPoint;
  if (!this->mvStat.waitTimer.isRunning() && b_we_have_not_reached_last_point_on_path )
  {
    mvStat.shouldWeRenderObject = true;
    initFpsInfo();


    this->calcPosOfMovingObject(inElapsedSinceLastCall);
    this->displayCoordinate.calcSimLocalData();
    this->positionInstancedObject();

    #ifdef DEBUG_MOVE
    // auto debug_time_passed = mvStat.timer.getCumulativeXplaneTimeInSec(); // v26.03.1
    // Log::logMsg( fmt::format ("[{}] {}, speed: {:.2f}mts, time_passed: {:.4f},  displayCoordinate: ({:.7f}/{:.7f} : el: {:.2f}ft, pi: {:.2f}deg, roll: {:.2f}deg)"
    //            , __func__, this->getInstanceName(), this->displayCoordinate.getSpeedMts(), debug_time_passed
    //            , this->displayCoordinate.lat, this->displayCoordinate.lon
    //            , this->displayCoordinate.getElevationInFeet()
    //            , this->displayCoordinate.getPitch(), this->displayCoordinate.getRoll()) );
    #endif
  }

}

// -----------------------------------------------------------------
// void
// missionx::obj3d::cb_calc_pos_of_the_moving_object2(const float &inElapsedSinceLastCall, const float &inElapsedTimeSinceLastFlightLoop, const int &inCounter)
// {
//   // The following function will calculate position using time between inElapsedSinceLastCall
//
//   #ifdef DEBUG_MOVE
//   Log::logMsg( fmt::format ("[{}] {} => inElapsedSinceLastCall: {:.5f}, inElapsedTimeSinceLastFlightLoop: {:.5f}, inCounter: {}", __func__, this->getInstanceName(), inElapsedSinceLastCall, inElapsedTimeSinceLastFlightLoop, inCounter ));
//   #endif
//
//
//   // do we need to calculate anything ?
//   if (this->mvStat.hasReachedPointTo_need_to_continue_or_cycle)
//   {
//     #ifdef DEBUG_MOVE
//     Log::logMsg( fmt::format ("[{}]--> dist_lat: {:.7f}, dist_lon: {:.7f}), mvStat.hasReachedLastPoint: {}.", __func__, this->displayCoordinate.getLat(), this->displayCoordinate.getLon(), mvStat.hasReachedLastPoint ) );
//     #endif
//
//     return;
//   }
//
//   // Are we there ?
//   mvStat.hasReachedPointTo = (fabs(this->displayCoordinate.getLat() - mvStat.pointFrom.getLat()) >= fabs(mvStat.pointTo.getLat() - mvStat.pointFrom.getLat())
//                               ||
//                               fabs(this->displayCoordinate.getLon() - mvStat.pointFrom.getLon()) >= fabs(mvStat.pointTo.getLon() - mvStat.pointFrom.getLon()));
//
//
//   if (this->mvStat.hasReachedPointTo && this->mvStat.noOfPointsInPath > 1)
//   {
//     #ifndef RELEASE
//     Log::logMsg(fmt::format("\n[{}] >>>>>> START debug Reached Point To <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<", __func__ ) );
//     // Log::logMsg(fmt::format("[{}] >>>>>> displayCoordinate: {},{} <<  pointFrom: {},{} << pointTo: {},{} <<<<<<", __func__, displayCoordinate.getLat(), displayCoordinate.getLon(), mvStat.pointFrom.getLat(), mvStat.pointFrom.getLon(), mvStat.pointTo.getLat(), mvStat.pointTo.getLon(), mvStat.pointTo.getLon() ));
//     Log::logMsg(fmt::format("[{}] >>>>>> displayCoordinate: {},{} <<  pointFrom: {},{} << pointTo: {},{} <<<<<<", __func__, displayCoordinate.lat, displayCoordinate.lon, mvStat.pointFrom.lat, mvStat.pointFrom.lon, mvStat.pointTo.lat, mvStat.pointTo.lon, mvStat.pointTo.lon ));
//     Log::logMsg(fmt::format("[{}] >>>>>> Lat displayCoordinate - pointFrom: {} >= pointTo - pointFrom: {} ? {}", __func__, fabs(this->displayCoordinate.getLat() - mvStat.pointFrom.getLat()), fabs(displayCoordinate.getLat() - mvStat.pointFrom.getLat() ), fabs(this->displayCoordinate.getLat() - mvStat.pointFrom.getLat()) >= fabs(mvStat.pointTo.getLat() - mvStat.pointFrom.getLat()) ) );
//     Log::logMsg(fmt::format("[{}] >>>>>> Lon displayCoordinate - pointFrom: {} >= pointTo - pointFrom: {} ? {}", __func__, fabs(this->displayCoordinate.getLon() - mvStat.pointFrom.getLon()), fabs(displayCoordinate.getLon() - mvStat.pointFrom.getLon() ), fabs(this->displayCoordinate.getLon() - mvStat.pointFrom.getLon()) >= fabs(mvStat.pointTo.getLon() - mvStat.pointFrom.getLon()) ) );
//     Log::logMsg(fmt::format("[{}] >>>>>> End debug Reached Point To <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n", __func__ ) );
//     #endif
//
//
//
//     this->mvStat.currentPointNo++;
//     mvStat.timeOnVector = 0.0f;
//
//     // Check if we reached the end of the path, and if we need to cycle
//     if (this->mvStat.currentPointNo >= this->mvStat.noOfPointsInPath)
//     {
//       this->mvStat.hasReachedLastPoint = true;
//
//       if (this->getIsPathNeedToCycle())
//       {
//         this->mvStat.currentPointNo = 0;
//         this->mvStat.hasReachedLastPoint = false;
//         this->mvStat.hasReachedPointTo_need_to_continue_or_cycle = false;
//       }
//       else
//       {
//         this->mvStat.timer.stop();
//         this->mvStat.currentPointNo = 0;
//         this->mvStat.hasReachedPointTo_need_to_continue_or_cycle = true;
//
//       }
//     }
//
//     // ---------------------------
//     // Assign new points from path
//     // ---------------------------
//     if (/*this->mvStat.currentPointNo > 0 && */ this->mvStat.currentPointNo < this->mvStat.noOfPointsInPath && !this->mvStat.hasReachedPointTo_need_to_continue_or_cycle)
//     {
//       // v26.06.1 Record error before CTD
//       if (this->mvStat.currentPointNo + 1 > this->mvStat.noOfPointsInPath )
//         Log::logMsg(fmt::format("[{}] mvStat.currentPointNo +1 {} should not exceed mvStat.noOfPointsInPath: {}", __func__, mvStat.currentPointNo,mvStat.noOfPointsInPath) );
//
//       #ifndef RELEASE
//       assert ( (this->mvStat.currentPointNo + 1 <= this->mvStat.noOfPointsInPath ) && fmt::format("[{}] mvStat.currentPointNo +1 {} should not exceed mvStat.noOfPointsInPath: {}", __func__, mvStat.currentPointNo,mvStat.noOfPointsInPath).c_str() );
//       Log::logMsg(fmt::format("[{}]==> Reached mvStat.currentPointNo: {}\n", __func__, mvStat.currentPointNo));
//       #endif
//       // clone the data from the next point, but retain the current lat/lon
//       this->displayCoordinate.setSpeedInKmh(mvStat.pointTo.getSpeedKmh());
//       this->displayCoordinate.setAdjustHeading(mvStat.pointTo.getAdjustHeading());
//       const auto point_to_heading_s = Utils::readAttrib(mvStat.pointTo.node, mxconst::get_ATTRIB_HEADING_PSI(), "");
//       bool b_pointTo_does_not_have_heading_psi_attribute = true;
//       #ifndef RELEASE
//       auto debug_display_coordinate_heading = this->displayCoordinate.getHeading();
//       Log::logMsg(fmt::format("[{}]==> debug_display_coordinate_heading: {}\n", __func__, debug_display_coordinate_heading));
//       #endif
//       if (!point_to_heading_s.empty() && mxUtils::is_number(point_to_heading_s) && point_to_heading_s != "0")
//       {
//           this->displayCoordinate.setHeading(mxUtils::stringToNumber<double>(point_to_heading_s));
//           this->displayCoordinate.setAdjustHeading(0.0); // reset adjust heading
//           b_pointTo_does_not_have_heading_psi_attribute = false;
//       }
//
//       this->mvStat.pointTo.clone(this->deqPoints[this->mvStat.currentPointNo + 1]); // deque points start in Zero "0...(N-1)"
//
//       this->mvStat.pointFrom.clone(this->displayCoordinate);
//       #ifndef RELEASE
//       Log::logMsg(fmt::format("[{}] 1 >>>>>> displayCoordinate: {},{} <<  pointFrom: {},{} << pointTo: {},{} <<<<<<", __func__, displayCoordinate.lat, displayCoordinate.lon, mvStat.pointFrom.lat, mvStat.pointFrom.lon, mvStat.pointTo.lat, mvStat.pointTo.lon, mvStat.pointTo.lon ));
//       #endif
//
//       this->displayCoordinate.parse_node();
//       this->mvStat.pointFrom.setSpeedInKmh(this->displayCoordinate.getSpeedKmh());
//
//       this->mvStat.pointFrom.clone(this->displayCoordinate);
//       #ifndef RELEASE
//       Log::logMsg(fmt::format("[{}] 2 >>>>>> displayCoordinate: {},{} <<  pointFrom: {},{} << pointTo: {},{} <<<<<<", __func__, displayCoordinate.lat, displayCoordinate.lon, mvStat.pointFrom.lat, mvStat.pointFrom.lon, mvStat.pointTo.lat, mvStat.pointTo.lon, mvStat.pointTo.lon ));
//       #endif
//
//       this->mvStat.currentTimeElapsed_sinceStart = 0.0f;
//       this->calcNewCourseBetweenTwoPointsOnVector (); // init: "mvStat.secondsToReachTarget" and "mvStat.distanceFromTo_meters"
//
//       this->setNextWaitTimer();
//
//       // Modify heading only if its value is not Zero ("0"). //v26.06.1 Moved code. Immediate transition.
//       if (b_pointTo_does_not_have_heading_psi_attribute)
//       {
//         if (this->mvStat.pointTo.adjust_heading > 0)
//             this->displayCoordinate.setHeading(static_cast<double>(this->mvStat.pointTo.adjust_heading + this->mvStat.pointFrom.getHeading())); // v3.0.207.5 calculate relative to the adjust heading value
//         else if (this->mvStat.pointTo.adjust_heading < 0)
//             this->displayCoordinate.setHeading(this->mvStat.pointFrom.getHeading() - std::fabs(this->mvStat.pointTo.adjust_heading )); // v3.0.207.5 calculate relative to the adjust heading value
//       }
//
//       // this->displayCoordinate.setPitch((this->mvStat.pointTo.getPitch() - this->mvStat.pointFrom.getPitch()) + this->mvStat.pointFrom.getPitch());
//       // this->displayCoordinate.setRoll((this->mvStat.pointTo.getRoll() - this->mvStat.pointFrom.getRoll()) + this->mvStat.pointFrom.getRoll());
//       this->displayCoordinate.setPitch(this->mvStat.pointTo.getPitch());
//       this->displayCoordinate.setRoll(this->mvStat.pointTo.getRoll());
//
//       this->mvStat.pointFrom.clone(this->displayCoordinate);
//       #ifndef RELEASE
//       Log::logMsg(fmt::format("[{}] 3 >>>>>> displayCoordinate: {},{} <<  pointFrom: {},{} << pointTo: {},{} <<<<<<", __func__, displayCoordinate.lat, displayCoordinate.lon, mvStat.pointFrom.lat, mvStat.pointFrom.lon, mvStat.pointTo.lat, mvStat.pointTo.lon, mvStat.pointTo.lon ));
//       #endif
//
//
//       #ifndef RELEASE
//       Log::logMsg( fmt::format("\t\t\t===> New Point: {}\n\n\n", mvStat.currentPointNo) );
//       #endif
//     }
//   } // end handling if we reached point on path
//
//   // ---------------
//   // Eval waitTimer
//   // ---------------
//   missionx::Timer::wasXplaneTimerEnded(mvStat.waitTimer);
//
//   // Wait timer on path change. Evaluate each call, since we also need to start the movement timer
//   if (!this->mvStat.waitTimer.isRunning() && mvStat.timer.getState() == missionx::mx_timer_state::timer_not_set) // check if timer started
//   {
//     missionx::Timer::start(mvStat.timer, 0, "obj3d_timer_" + this->getName()); // SecondsToRun=0 => "run continuously"
//     mvStat.lastZuluStartDraw = 0.0f;
//   }
//
//   #ifdef DEBUG_MOVE
//   // Log::logMsg( fmt::format ("[{}]--> dist_lat: {:.7f}, dist_lon: {:.7f}), total_dist_deg: {:.7f}, mvStat.hasReachedPointTo: {}. WaitTimer: {:.2f}, Timer: {:.2f}", __func__, dist_lat, dist_lon, total_dist_deg, mvStat.hasReachedPointTo, mvStat.waitTimer.getSecondsPassed_for_TotalXP(), mvStat.timer.getSecondsPassed_for_TotalXP()) );
//   #endif
//
//
//   // -----------------------------------------------
//   // Calculate and assign the new relative position
//   // -----------------------------------------------
//   const auto b_we_have_not_reached_last_point_on_path = !this->mvStat.hasReachedLastPoint;
//   if (!this->mvStat.waitTimer.isRunning() && b_we_have_not_reached_last_point_on_path )
//   {
//     mvStat.shouldWeRenderObject = true;
//     initFpsInfo();
//
//
//     this->calcPosOfMovingObject(inElapsedSinceLastCall);
//     this->displayCoordinate.calcSimLocalData();
//     this->positionInstancedObject();
//
//     #ifdef DEBUG_MOVE
//     // auto debug_time_passed = mvStat.timer.getCumulativeXplaneTimeInSec(); // v26.03.1
//     // Log::logMsg( fmt::format ("[{}] {}, speed: {:.2f}mts, time_passed: {:.4f},  displayCoordinate: ({:.7f}/{:.7f} : el: {:.2f}ft, pi: {:.2f}deg, roll: {:.2f}deg)"
//     //            , __func__, this->getInstanceName(), this->displayCoordinate.getSpeedMts(), debug_time_passed
//     //            , this->displayCoordinate.lat, this->displayCoordinate.lon
//     //            , this->displayCoordinate.getElevationInFeet()
//     //            , this->displayCoordinate.getPitch(), this->displayCoordinate.getRoll()) );
//     #endif
//   }
//
// }

// -----------------------------------------------------------------

void
missionx::obj3d::initFpsInfo()
{
  // mvStat.fps = missionx::dataref_manager::get_raw_fps_f();
  mvStat.fps = XPLMGetDataf(fps_dref);
  if (mvStat.fps == 0.0f) // prevent divide by zero
    mvStat.fps = 1.0f;

  mvStat.fps = 1.0f / mvStat.fps;

}

// -----------------------------------------------------------------

void
missionx::obj3d::calcPosOfMovingObject(const float & inElapsedSinceLastCall)
{
  static double elevInMeter = 0.0;
  missionx::Timer::wasXplaneTimerEnded(mvStat.timer); // v3.0.223.5 - changed function to use X-Plane timer and not the regular timer, since the regular one does not work well with draw callback only good for day time considerations.

  if (inElapsedSinceLastCall > 0.0f && this->mvStat.fps > 0.0f)
  {
    mvStat.currentTimeElapsed_sinceStart += inElapsedSinceLastCall;
    this->mvStat.timeOnVector            = (mvStat.currentTimeElapsed_sinceStart) / this->mvStat.secondsToReachTarget; // secondsToReachTarget was timeToReachTarget
  }

  // #ifndef DEBUG_MOVE
  // Log::logMsg( fmt::format ("[{}]--> inElapsedSinceLastCall: {:.3f}, currentTimeElapsed_sinceStart: {:.3f} / secondsToReachTarget: {:2f} = timeOnVector: {:.3f}"
  //                                  , __func__, inElapsedSinceLastCall, mvStat.currentTimeElapsed_sinceStart
  //                                  , mvStat.secondsToReachTarget, mvStat.timeOnVector)
  //                                  );
  // #endif


  this->mvStat.time_was_advanced_by_draw_function = true;
  // Calculate Position on vector and set "displayCoordinate"
  this->setCoordinateOnVector(this->mvStat.pointFrom, this->mvStat.pointTo, mvStat.timeOnVector);
  this->displayCoordinate.calcSimLocalData();

  elevInMeter = this->getElevInMeter();

  if (elevInMeter == 0 || elevInMeter < mvStat.groundElevation)
    this->calculate_real_elevation_to_DisplayCoordination();
}


// -----------------------------------------------------------------

void
missionx::obj3d::setCoordinateOnVector(missionx::Point& pointFrom, missionx::Point& pointTo, const float &in_time_on_vector)
{
  const auto newLat = (pointTo.getLat() - pointFrom.getLat()) * in_time_on_vector + pointFrom.getLat();
  const auto newLon = (pointTo.getLon() - pointFrom.getLon()) * in_time_on_vector + pointFrom.getLon();
  const auto newElev = (pointTo.getElevationInMeters() - pointFrom.getElevationInMeters()) * in_time_on_vector + pointFrom.getElevationInMeters();

  // #ifndef RELEASE
  // std::string name = this->getName();

  // Log::logMsg(fmt::format("[{}] setCoordinateOnVector: {}", __func__, this->getName()) );
  // Log::logMsg(fmt::format("[{}] pointTo.getLat({}) - pointFrom.getLat({})) * time({}) + pointFrom.getLat({})", __func__, pointTo.getLat(), pointFrom.getLat(), in_time_on_vector, pointFrom.getLat()));
  // Log::logMsg(fmt::format("[{}] pointTo.getLon({}) - pointFrom.getLon({})) * time({}) + pointFrom.getLon({})", __func__, pointTo.getLon(), pointFrom.getLon(), in_time_on_vector, pointFrom.getLon()));
  // Log::logMsg(fmt::format("[{}] newLat: {} newLon: {}, newElev: {}\n", __func__, newLat, newLon, newElev));
  // Log::logMsg(fmt::format("[{}] newLat: {} newLon: {}, newElev: {}\n", __func__, newLat, newLon, newElev));

  // #endif


  this->displayCoordinate.setLat(newLat);
  this->displayCoordinate.setLon(newLon);
  this->displayCoordinate.setElevationMt(newElev);


  // v25.06.1 deprecate and move after point transition
  // // Modify heading only if its value is not Zero ("0").
  // if (pointTo.adjust_heading > 0)
  //   this->displayCoordinate.setHeading((static_cast<double>(pointTo.adjust_heading) * static_cast<double>(in_time_on_vector)) + pointFrom.getHeading()); // v3.0.207.5 calculate relative to the adjust heading value
  // else if (pointTo.adjust_heading < 0)
  //   this->displayCoordinate.setHeading(pointFrom.getHeading() - std::fabs(pointTo.adjust_heading * in_time_on_vector)); // v3.0.207.5 calculate relative to the adjust heading value

  // this->displayCoordinate.setPitch((pointTo.getPitch() - pointFrom.getPitch()) * in_time_on_vector + pointFrom.getPitch());
  // this->displayCoordinate.setRoll((pointTo.getRoll() - pointFrom.getRoll()) * in_time_on_vector + pointFrom.getRoll());
}


void
missionx::obj3d::calcNewCourseBetweenTwoPointsOnVector()
{
  std::string err;
  // double      distance_meters = 0.0f;
  err.clear();

  mvStat.distanceFromTo_meters = this->mvStat.pointFrom.calcDistanceBetween2Points(this->mvStat.pointTo, missionx::mx_units_of_measure::meter, &err);
  if (!err.empty())
    mvStat.distanceFromTo_meters = 0.0;

  mvStat.secondsToReachTarget = (static_cast<float>(mvStat.distanceFromTo_meters) / ((mvStat.pointFrom.getSpeedMts() == 0.0f) ? 0.000000000001f : mvStat.pointFrom.getSpeedMts())); // seconds to reach destination

  #ifndef RELEASE
    const auto debug_speed_mts = (mvStat.pointFrom.getSpeedMts() == 0.0f) ? 0.000000000001f : mvStat.pointFrom.getSpeedMts(); // seconds to reach destination
    Log::logMsg(fmt::format("[{}] new speed_mts: {:.2f}, distance_meters: {:.2f}, secondsToReachTarget: {:.2f}", __func__, debug_speed_mts, mvStat.distanceFromTo_meters, mvStat.secondsToReachTarget) );
  #endif
}

void
missionx::obj3d::calcNewCourseBetweenTwoPointsOnVector2()
{
  std::string err;
  double      distance = 0.0f;
  err.clear();

  distance = this->mvStat.pointFrom.calcDistanceBetween2Points(this->mvStat.pointTo, missionx::mx_units_of_measure::meter, &err);
  if (!err.empty())
    distance = 0.0;

  // mvStat.distanceFromTo_meters = distance * (double)(static_cast<double>(missionx::nm2meter) * static_cast<double>(missionx::meter2feet)); //  (float)Utils::nmToFeet(&distance);
  mvStat.distanceFromTo_meters = distance; //  (float)Utils::nmToFeet(&distance);

  mvStat.secondsToReachTarget = (static_cast<float>(mvStat.distanceFromTo_meters) / ((mvStat.pointFrom.getSpeedFts() == 0.0f) ? 0.000000000001f : mvStat.pointFrom.getSpeedFts())); // seconds to reach destination

#ifdef DEBUG_MOVE
  // sprintf(LOG_BUFF, "[Moving3D calc] new pointFrom.lat: %f, new pointFrom.lon: %f", mvStat.pointFrom.getLat(), mvStat.pointFrom.getLon());
  // Utils::logMsg(LOG_BUFF);
  // sprintf(LOG_BUFF, "[Moving3D calc] new pointTo.lat: %f  , new pointTo.lon: %f", mvStat.pointTo.getLat(), mvStat.pointTo.getLon());
  // Utils::logMsg(LOG_BUFF);
#endif
}


missionx::Point&
missionx::obj3d::getCurrentCoordination()
{
  return this->displayCoordinate;
}


double
missionx::obj3d::getLat()
{
  return this->displayCoordinate.getLat();
}


double
missionx::obj3d::getLong()
{
  return this->displayCoordinate.getLon();
}


double
missionx::obj3d::getElevInFeet()
{
  return this->displayCoordinate.getElevationInFeet();
}


double
missionx::obj3d::getElevInMeter()
{
  return this->displayCoordinate.getElevationInMeters();
}

std::string
missionx::obj3d::getPropKeepUntilLeg()
{
    return Utils::readAttrib(this->node, mxconst::get_ATTRIB_KEEP_UNTIL_LEG(), "");
}

std::string obj3d::get_instance_created_in_leg() const
{
  return Utils::readAttrib(this->node, mxconst::get_ATTRIB_INSTANCE_CREATED_IN_LEG(), "");
}

std::string
missionx::obj3d::getPropLinkTask()
{
  const std::string result = Utils::readAttrib(this->node, mxconst::get_ATTRIB_LINK_TASK(), "");

  return result;
}

std::string
missionx::obj3d::getPropLinkToObjectiveName()
{
  std::string result = Utils::readAttrib(this->node, mxconst::get_PROP_LINK_OBJECTIVE_NAME(), "");

  return result;
}

bool
missionx::obj3d::getIsPathNeedToCycle()
{
  std::string err;
  // const bool  result = Utils::readBoolAttrib(this->node, mxconst::get_ATTRIB_CYCLE(), false);
  // return result;
  // TODO: make sure that when we save, the <path> part is retained.
  return   Utils::readBoolAttrib(this->xPath, mxconst::get_ATTRIB_CYCLE(), false);

}

bool
missionx::obj3d::getHideObject()
{
  //return result;
  return Utils::readBoolAttrib(this->node, mxconst::get_ATTRIB_HIDE(), false);
}

void
missionx::obj3d::calculate_real_elevation_to_DisplayCoordination()
{
  std::string err;

  double above_ground_ft_prop = 0.0;
  if (Utils::readAttrib(this->node, mxconst::get_ATTRIB_ELEV_ABOVE_GROUND_FT(), "").empty() == false) // if attribute not empty
  {
    above_ground_ft_prop = Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_ELEV_ABOVE_GROUND_FT(), 0.0);
  }
  this->displayCoordinate.setElevationAboveGroundFt(above_ground_ft_prop); // v3.0.251.1 important to add this piece of information so calculation will be correct

  // v3.0.251.1 calculate elevation based on lat/long location and add the above ground info to set the 3D Object location correctly
  this->displayCoordinate.calc_elevation_include_above_ground_info_and_sync_to_WorldToLocalData();

  this->setNodeProperty<double>(mxconst::get_ATTRIB_ELEV_FT(), this->displayCoordinate.getElevationInFeet()); // v3.0.241.1 update the main instance element <obj3d > with the elevation
  this->displayCoordinate.elevWasProbed = true;

#ifdef DEBUG_MOVE
  Log::logMsg(fmt::format("[{}] Terrain Probed, elevation [{}mt | {}ft]", __func__, this->displayCoordinate.getElevationInMeters(), this->displayCoordinate.getElevationInFeet() ) );

#endif
}


void
missionx::obj3d::positionInstancedObject()
{

  this->displayCoordinate.calc_elevation_include_above_ground_info_and_sync_to_WorldToLocalData(); // v3.0.251.1 calculate real elevation = terrain + above ground

  // DEBUG
  // #define DISPLAY_3D_INSTANCE
  #ifdef DISPLAY_3D_INSTANCE
    Log::logMsg(fmt::format("[{}] Info {}: \n{}\n", __func__, this->getInstanceName(), this->displayCoordinate.to_string_with_locals ()) );
  #endif // DISPLAY_3D_INSTANCE


  // prepare XPLMDrawInfo_t
  this->dr.structSize = sizeof(dr);
  this->dr.x          = static_cast<float>(this->displayCoordinate.local_x);
  this->dr.y          = static_cast<float>(this->displayCoordinate.local_y);
  this->dr.z          = static_cast<float>(this->displayCoordinate.local_z);
  this->dr.pitch      = static_cast<float>(this->displayCoordinate.getPitch());
  this->dr.heading    = static_cast<float>(this->displayCoordinate.getHeading());
  this->dr.roll       = static_cast<float>(this->displayCoordinate.getRoll());


  // Show Instance
  if (this->g_instance_ref)
    XPLMInstanceSetPosition(this->g_instance_ref, &this->dr, nullptr);
}



std::string
missionx::obj3d::getInstanceName()
{
  return Utils::readAttrib(this->node, mxconst::get_ATTRIB_INSTANCE_NAME(), "NoInstanceNameFound!!!!");
}



bool
missionx::obj3d::isPlaneInDisplayDistance(missionx::Point& inPlanePoint)
{
  const double condDist = Utils::readNumericAttrib (this->node, mxconst::get_ATTRIB_DISTANCE_TO_DISPLAY_NM(), 0.0);
  const double dist     = Point::calcDistanceBetween2Points(inPlanePoint, this->displayCoordinate);

  return (condDist >= dist);
}



missionx::Point
missionx::obj3d::getStartLocationAttributes()
{
  static std::string err;
  err.clear();
  missionx::Point p;

  //// Rewrite the logic while we lean on the display_object node
  if (!Utils::readAttrib(this->node, mxconst::get_ATTRIB_INSTANCE_NAME(), "").empty() || !this->xLocation.isEmpty()) // v3.0.241.1 we know this is an instance
  {
    p.setLat(Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_LAT(), Utils::readNodeNumericAttrib<double>(xDisplayObject_ptr, mxconst::get_ATTRIB_LAT(), 0.0)));
    p.setLon(Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_LONG(), Utils::readNodeNumericAttrib<double>(xDisplayObject_ptr, mxconst::get_ATTRIB_LONG(), 0.0)));
    p.setElevationFt(Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_ELEV_FT(), Utils::readNodeNumericAttrib<double>(xDisplayObject_ptr, mxconst::get_ATTRIB_ELEV_FT(), 0.0)));
    p.setElevationAboveGroundFt(Utils::readNumericAttrib(this->node, mxconst::get_ATTRIB_ELEV_ABOVE_GROUND_FT(), Utils::readNodeNumericAttrib<double>(xDisplayObject_ptr, mxconst::get_ATTRIB_ELEV_ABOVE_GROUND_FT(), 0.0))); // v3.0.303.2
  }
  else if (this->displayCoordinate.pointState == missionx::mx_point_state::defined)
  {
    p = this->displayCoordinate;
  }

  // store <tilt> data. All the data should have been in the parent node = <obj3d>
  p.setHeading(Utils::readNodeNumericAttrib<float>(this->node, mxconst::get_ATTRIB_HEADING_PSI(), Utils::readNodeNumericAttrib<float>(xDisplayObject_ptr, mxconst::get_ATTRIB_HEADING_PSI(), 0.0f)));
  p.setPitch(Utils::readNodeNumericAttrib<float>(this->node, mxconst::get_ATTRIB_PITCH(), Utils::readNodeNumericAttrib<float>(xDisplayObject_ptr, mxconst::get_ATTRIB_PITCH(), 0.0f)));
  p.setRoll(Utils::readNodeNumericAttrib<float>(this->node, mxconst::get_ATTRIB_ROLL(), Utils::readNodeNumericAttrib<float>(xDisplayObject_ptr, mxconst::get_ATTRIB_ROLL(), 0.0f)));



  p.storeData();
  p.calcSimLocalData();

  return p;
}


void
missionx::obj3d::applyPropertiesToLocal()
{
  static std::string pValue;
  static std::string err;
  pValue.clear();
  err.clear();

  // obje3d type
  if (this->node.isAttributeSet(mxconst::get_ATTRIB_OBJ3D_TYPE().c_str()))
  {
    const obj3d::obj3d_type mType = static_cast<obj3d::obj3d_type>(this->getAttribNumericValue<int>(mxconst::get_ATTRIB_OBJ3D_TYPE(), (int)obj3d::obj3d_type::static_obj, err));
    this->obj3dType               = mType;
  }


  // displayDefaultObjectFileOverAlternate
  this->displayDefaultObjectFileOverAlternate = true;

  const std::string name = this->getName();

  // read special data for static obj3d type
  if (this->obj3dType == missionx::obj3d::obj3d_type::static_obj) // obj3d::moving_obj)
  {
    // coordination from properties
    this->startLocation = getStartLocationAttributes();
  }
  else // v3.0.202  // obj3d::moving_obj
  {
    // v3.0.207.2: start location point is taken from "attributes" if static, and from "path" if moving. For moving, if path is empty then we take from attributes.
    if (this->deqPoints.empty())                          // v3.0.213.7 // replaced (this->path.empty())
      this->startLocation = getStartLocationAttributes(); // moving object receive display attribute from other properties (TBD)
    else
      this->startLocation = this->deqPoints.at(0); // v3.0.213.7 // get start location from PATH container and not the attributes in the XML data file
  } // end handling type of 3D Object

  this->displayCoordinate.clone(startLocation); // set current coordinate to LAT/LONG/ELEV + Heading/Pitch/Roll

  #ifndef RELEASE
  {
    IXMLRenderer xPrinter;
    Log::logMsg("Display Instance information:\n" + std::string(xPrinter.getString(this->node))); // display Instance Obj3D node
    Log::logMsg("Display Point node:\n" + std::string(xPrinter.getString(this->displayCoordinate.node)) + "\n");
  }
  #endif // !RELEASE


  // cond_script_s attribute outcome
  if (this->node.isAttributeSet(mxconst::get_PROP_SCRIPT_COND_MET_B().c_str())) // v3.0.241.1 first check if node has the attribute, then fallback to original code
    this->isScriptCondMet = Utils::readBoolAttrib(this->node, mxconst::get_PROP_SCRIPT_COND_MET_B(), false);
  else
    this->isScriptCondMet = true;
}


void
missionx::obj3d::storeCoreAttribAsProperties()
{
  // store plane location
  this->setNodeStringProperty(mxconst::get_PROP_CURRENT_LOCATION(), this->displayCoordinate.format_point_to_savepoint()); // v3.0.241.1
}


void
missionx::obj3d::saveCheckpoint(IXMLNode& inParent)
{
  this->storeCoreAttribAsProperties();

  IXMLNode xChild = this->node.deepCopy();

  // save instance data
  if (this->g_instance_ref)
  {
    // 1. save path
    // 2. save moving data (pointFrom, pointTo, currentPointNo, instance_name)

    Utils::xml_delete_all_subnodes(xChild, mxconst::get_PROP_INSTANCE_DATA_ELEMENT()); // v3.303.11 remove any "instance_data" element that was present to the save. This can happen when loading from save point and then saving again.

    IXMLNode xInstanceElement = xChild.addChild(mxconst::get_PROP_INSTANCE_DATA_ELEMENT().c_str());


    if (!xInstanceElement.isEmpty())
    {
      xInstanceElement.addAttribute(mxconst::get_ATTRIB_NAME().c_str(), this->getInstanceName().c_str());
      this->setNodeProperty<bool>(xInstanceElement, mxconst::get_ATTRIB_DISPLAY_AT_POST_LEG_B(), Utils::readBoolAttrib(xChild, mxconst::get_ATTRIB_DISPLAY_AT_POST_LEG_B(), false), xInstanceElement.getName()); // v3.303.11
      this->setNodeProperty<int>(xInstanceElement, mxconst::get_PROP_CURRENT_POINT_NO(), this->mvStat.currentPointNo, xInstanceElement.getName());
      this->setNodeProperty<bool>(xInstanceElement, mxconst::get_PROP_LOADED_FROM_CHECKPOINT(), true, xInstanceElement.getName());

      // v26.03.1
      Utils::xml_set_attribute_in_node_asString (xInstanceElement, mxconst::get_ATTRIB_INSTANCE_CREATED_IN_LEG(), this->get_instance_created_in_leg());

      Utils::xml_set_attribute_in_node_asString(xInstanceElement, mxconst::get_PROP_POINT_FROM(), this->mvStat.pointFrom.format_point_to_savepoint(), xInstanceElement.getName());
      Utils::xml_set_attribute_in_node_asString(xInstanceElement, mxconst::get_PROP_POINT_TO(), this->mvStat.pointTo.format_point_to_savepoint(), xInstanceElement.getName());
      Utils::xml_set_attribute_in_node_asString(xInstanceElement, mxconst::get_PROP_CURRENT_LOCATION(), this->displayCoordinate.format_point_to_savepoint(), xInstanceElement.getName());
    }

  }

  inParent.addChild(xChild);
}



std::string
missionx::obj3d::to_string()
{
  std::string format;
  format.clear();

  format              = "3D Object: " + this->getName () + "\"" + mxconst::get_UNIX_EOL();
  const size_t length = format.length();
  format += std::string("").append(length, '=') + mxconst::get_UNIX_EOL();

  format += "Display Coordinates node info: \n" + Utils::xml_get_node_content_as_text(this->displayCoordinate.node);

#ifndef RELEASE
  format += "\n3D Object raw node info: \n" + Utils::xml_get_node_content_as_text(this->node);
#endif

  return format;
}

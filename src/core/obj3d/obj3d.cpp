#include "obj3d.h"
#include <assert.h>

namespace missionx
{
XPLMDataRef obj3d::fps_dref;
}

// -----------------------------------------------------------------

void
missionx::obj3d::initPathCycle_new(bool isRecursive)
{
  mvStat.noOfPointsInPath    = (int)deqPoints.size();
  mvStat.currentPointNo      = 0;
  mvStat.hasReachedLastPoint = false;

  if (mvStat.noOfPointsInPath > 0)
  {
    if (isRecursive)
      this->CyclePath();
    else
      this->firstTimeInCycle();
  }
  else
  {
#ifndef RELEASE
    Log::logMsg("!!! ERROR [MovingObject3D initPathCycle_new] No points in <path> of 3D Object: >>>> " + this->getName() + " <<<< !!!");
#endif
  }
}

// -----------------------------------------------------------------

void
missionx::obj3d::firstTimeInCycle()
{
  // This function should be called only once to determind the kind of action that needs to be taken - static vs moving
  // Warning: we should never call initPathCycle() function from this function

  assert(mvStat.noOfPointsInPath > 0 && "[obj3d firstTimeCycle] No points on path");

  // Only handle first time
#ifndef RELEASE
  Log::logMsg("[MovingObject3D firstTimeInCycle]start for 3D Object: >>>> " + this->getName() + " <<<<" + ", Counter: " + Utils::formatNumber<int>(mvStat.currentPointNo));
#endif
  mvStat.lastZuluStartDraw = 0.0f;
  mvStat.timeOnVector      = 0.0f;

  itPathEnd = this->deqPoints.end();
  if (this->g_instance_ref) // is object visible
  {

    this->itPath = this->deqPoints.begin(); // should point to the starting point
    mvStat.pointFrom.clone((*itPath));

    if (mvStat.noOfPointsInPath == 1)
    {
      mvStat.isMoving                        = false; // flag object as not moving since only one point
      mvStat.isInRecursiveState              = false;
      mvStat.hasReachedLastPoint             = true;
      mvStat.pointFrom.timeToWaitOnPoint_sec = 0.0f;

    } // check next point not overflow
    else if (mvStat.noOfPointsInPath > 1)
    {
      ++mvStat.currentPointNo;
      ++itPath; // next point for next iteration.

      mvStat.hasReachedPointTo = false;
      mvStat.isMoving          = true; // v3.0.207.4 flag object as moving
    } // end if more than 1 point in path

    mvStat.isFirstTime = false;

    mvStat.pointTo.clone((*itPath)); // If we are static then same as first Point if not then same as the second Point
    mvStat.pointFrom.calcSimLocalData();
    mvStat.pointTo.calcSimLocalData();

    mvStat.prevPoint.clone(mvStat.pointFrom);

    this->displayCoordinate.clone(mvStat.pointFrom); // where to place 3D Object

    // Calculate new statistic info ( S = V * T )
    calcNewCourseBetweenTwoPointsOnVector(); // We call it for static or moving objects

    // validate we have not reached end
    if (itPathEnd == itPath)
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
missionx::obj3d::CyclePath()
{
  // Warning: we should never call initPathCycle() function from this function

  if (mvStat.noOfPointsInPath > 1)
  {
    this->itPath = (++this->deqPoints.begin());      // v3.0.253.6 we should start from the second and not the first point in path
    mvStat.pointFrom.clone(this->displayCoordinate); // v3.0.207.3 for smooth transition.

    mvStat.isFirstTime = false;
    if (this->itPath->getSpeedKmh() > 0.0)
      mvStat.pointFrom.setSpeedInKmh(this->itPath->getSpeedKmh()); // v3.0.253.7 fix speed not inherited correctly due to the fact we clone from "displayCoordinate"
    mvStat.pointFrom.calcSimLocalData();

    // validate we have not reached end
    if (itPathEnd == itPath)
      mvStat.hasReachedLastPoint = true;


    mvStat.pointTo.clone((*itPath)); // Copy the next point definitions to "pointTo", this includes the speed

    // Calculate new statistic info ( S = V * T )
    calcNewCourseBetweenTwoPointsOnVector(); // We call it twice in this function, once for the first time and now when there is point transition
    mvStat.hasReachedPointTo = false;
    mvStat.isMoving          = true; // flag object as moving

    // set wait timer
    this->setNextWaitTimer();
  }
}

// -----------------------------------------------------------------

void
missionx::obj3d::nextPoint_new()
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
        initPathCycle_new(true); // This is a recursive, might cause an issue.
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
  if (mvStat.pointFrom.timeToWaitOnPoint_sec == 0.0f)
    mvStat.waitTimer.reset();
  else
  {
    mvStat.waitTimer.reset();
    missionx::Timer::start(mvStat.waitTimer, mvStat.pointFrom.timeToWaitOnPoint_sec, "Obj3d_Wait_Timer_" + this->getName()); // Setting Wait Time Values
  }


  // init timer ifoif timer is running.
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


missionx::obj3d::~obj3d() {}

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
  else
  {
    this->name = name;
    this->setStringProperty(mxconst::get_ATTRIB_FILE_NAME(), file_name);
    this->setStringProperty(mxconst::get_ATTRIB_ALTERNATE_OBJ_FILE(), alternate_obj_file);

    // new v3.0.241.1 decide if it is a moving 3D object or static one
    const int xPathPointNodes = this->node.getChildNode(mxconst::get_ELEMENT_PATH().c_str()).nChildNode(mxconst::get_ELEMENT_POINT().c_str());
    if (xPathPointNodes >= 1)
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
              Log::logMsgErr("[read 3d obj] Point: " + p.to_string_xy() + ", is not valid. Skipping to next coordination...");
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
      missionx::mx_location_3d_objects info;
      // v3.0.241.10b3 copy attributes from display object to Obj3D
      if (!this->xDisplayObject_ptr.isEmpty())
      {
        info = Point::readLocationElement(xDisplayObject_ptr); // Read starting <location> element

        #ifndef RELEASE
        Log::logMsgNone( Utils::xml_get_node_content_as_text(xDisplayObject_ptr));
        #endif
      }
      if (this->obj3dType == obj3d::obj3d_type::static_obj)
      {

        this->xLocation = this->node.getChildNode(mxconst::get_ELEMENT_LOCATION().c_str());
        if (!this->xLocation.isEmpty()) // v3.0.217.5 added if element is valid
        {
          if (Utils::is_number(info.lat) && Utils::is_number(info.lon))
          {
            this->setNodeStringProperty(mxconst::get_ATTRIB_LAT(), info.lat);
            this->setNodeStringProperty(mxconst::get_ATTRIB_LONG(), info.lon); 
          }
          else
          {
            Log::logMsgErr("[read 3d obj] One of the coordination Lat/Lon might be malformed in 3D Object: " + mxconst::get_QM() + name + mxconst::get_QM() + ". Skipping...");
            return false;
          }
        } // end location is valid


        // Copy <display_object> attributes over to the Obj3D parent element
        std::set<std::string> local_exceptionAttributeSet = { mxconst::get_ATTRIB_NAME(), mxconst::get_ATTRIB_FILE_NAME() };

        Utils::xml_copy_node_attributes_excluding_black_list(xDisplayObject_ptr, this->node, &local_exceptionAttributeSet);


      } // end if 3D Object is static


      // read Tilt info
      this->xTilt = Utils::xml_get_or_create_node(this->node, mxconst::get_ELEMENT_TILT(), false); // v3.0.241.10 b3 fix bug where we write to <tilt> but it was not defined

      double pitch, roll;
      double heading = pitch = roll = 0.0; // mxconst::get_ZERO();

      if (!this->xTilt.isEmpty())
      {
        heading = Utils::readNodeNumericAttrib<double>(this->xTilt, mxconst::get_ATTRIB_HEADING_PSI(), 0.0);
        if (!info.heading.empty() && mxUtils::is_number(info.heading))
          heading = Utils::stringToNumber<double>(info.heading, 2);


        pitch = Utils::readNodeNumericAttrib<double>(this->xTilt, mxconst::get_ATTRIB_PITCH(), 0.0);
        if (!info.pitch.empty() && mxUtils::is_number(info.pitch))
          pitch = Utils::stringToNumber<double>(info.pitch, 2);

        roll = Utils::readNodeNumericAttrib<double>(this->xTilt, mxconst::get_ATTRIB_ROLL(), 0.0);
        if (!info.roll.empty() && mxUtils::is_number(info.roll))
          roll = Utils::stringToNumber<double>(info.roll, 2);
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


  } // end if <object> element is valid

  return true;
}

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


void
missionx::obj3d::load_cb(const char* real_path, void* ref)
{
  XPLMObjectRef* dest = (XPLMObjectRef*)ref;
  if (*dest == NULL)
  {
    *dest = XPLMLoadObject(real_path);
  }
}

void
missionx::obj3d::create_instance()
{
  if (this->g_object_ref)
  {
    if (this->g_instance_ref) // v3.0.207.2 Try to solve instance vanish after providing a new location. In this code, we will create new instance every frame.
      XPLMDestroyInstance(this->g_instance_ref);

    this->g_instance_ref = XPLMCreateInstance(this->g_object_ref, NULL);
  }

#ifndef RELEASE
  if (this->g_instance_ref) // debug
    Log::logMsg("Instance created... \n");
  else 
    Log::logMsg("Instance: " + this->getName() +" Fail to create... \n");
#endif
}

void
missionx::obj3d::initFpsInfo()
{
  mvStat.fps = XPLMGetDataf(fps_dref);
  if (mvStat.fps == 0.0f) // prevent divide by zero
    mvStat.fps = 1.0f;

  mvStat.fps = 1.0f / mvStat.fps;

  mvStat.deltaTime = mvStat.currentTimeElapsed_sinceStart - mvStat.oldTimeElapsed;
}


void
missionx::obj3d::calcPosOfMovingObject()
{
  static double elevInMeter = 0.0;
  missionx::Timer::wasXplaneTimerEnded(mvStat.timer); // v3.0.223.5 - changed function to use X-Plane timer and not the regular timer, since the regular one does not work well with draw callback only good for day time considerations.

  this->mvStat.deltaTime = missionx::Timer::getDeltaBetween2TimeFragments(this->mvStat.timer);
  if (this->mvStat.deltaTime > 0.0f && this->mvStat.fps > 0.0f)
  {
    mvStat.currentTimeElapsed_sinceStart = mvStat.timer.getSecondsPassed_for_TotalXP();
    this->mvStat.timeOnVector            = ((1 / this->mvStat.fps) + mvStat.currentTimeElapsed_sinceStart) / this->mvStat.secondsToReachTarget; // secondsToReachTarget was timeToReachTarget
  }

  this->mvStat.time_was_advanced_by_draw_function = true;
  this->setCoordinateOnVector(this->mvStat.pointFrom, this->mvStat.pointTo, this->mvStat.timeOnVector);
  this->displayCoordinate.calcSimLocalData();

  elevInMeter = this->getElevInMeter();

  if (elevInMeter == 0 || elevInMeter < mvStat.groundElevation)
    this->calculate_real_elevation_to_DisplayCoordination();
}



void
missionx::obj3d::setCoordinateOnVector(missionx::Point& pointFrom, missionx::Point& pointTo, float time)
{

#ifndef RELEASE
  std::string name = this->getName();
#endif

  this->displayCoordinate.setLat((pointTo.getLat() - pointFrom.getLat()) * time + pointFrom.getLat());
  this->displayCoordinate.setLon((pointTo.getLon() - pointFrom.getLon()) * time + pointFrom.getLon());
  this->displayCoordinate.setElevationMt((pointTo.getElevationInMeter() - pointFrom.getElevationInMeter()) * time + pointFrom.getElevationInMeter());

  if (pointTo.adjust_heading == 0) // keep same heading
  {
  }
  else if (pointTo.adjust_heading > 0)
    this->displayCoordinate.setHeading(((double)pointTo.adjust_heading * (double)time) + pointFrom.getHeading()); // v3.0.207.5 calculate relative to the adjust heading value
  else
    this->displayCoordinate.setHeading(pointFrom.getHeading() - std::fabs(pointTo.adjust_heading * time)); // v3.0.207.5 calculate relative to the adjust heading value

  this->displayCoordinate.setPitch((pointTo.getPitch() - pointFrom.getPitch()) * time + pointFrom.getPitch());
  this->displayCoordinate.setRoll((pointTo.getRoll() - pointFrom.getRoll()) * time + pointFrom.getRoll());
}


void
missionx::obj3d::checkAreWeThereYet()
{
  mvStat.shouldWeRenderObject = false; // we always init with this value

  if (!mvStat.hasReachedLastPoint)
  {
    // fetch FPS info
    initFpsInfo(); // we fetch this every draw callback, so maybe we should deprecate this line

    if (this->isInDisplayList) // if we already display the 3D Object  // former isRendered (v2.x)
    {

      // first time timer initialization, we use it to calculate movement on vector
      if (!this->mvStat.waitTimer.isRunning()                                    //  getState() != missionx::mx_timer_state::timer_running
          && mvStat.timer.getState() == missionx::mx_timer_state::timer_not_set) // check if timer started
      {
        missionx::Timer::start(mvStat.timer, 0, "obj3d_timer_" + this->getName()); // run continuesly
        mvStat.lastZuluStartDraw = 0.0f;
      }

      // workaround in the else of this Condition caused by double call or off screen pauses like moving screen or weather widget setup etc...
      if (mvStat.time_was_advanced_by_draw_function)
      {
        mvStat.time_was_advanced_by_draw_function = false; // v2.1.0 a15 // reset until next draw will advance timer
        mvStat.shouldWeRenderObject               = true;  // flag for instance display decition

        if (!mvStat.hasReachedPointTo)
        {

          if (mvStat.currentTimeElapsed_sinceStart && !mvStat.isFirstTime)
          {

#ifndef RELEASE
            const auto delta_dispAndPrev_lat    = fabs(this->displayCoordinate.getLat() - mvStat.prevPoint.getLat());
            const auto delta_pointToandPrev_lat = fabs(mvStat.pointTo.getLat() - mvStat.prevPoint.getLat());
            const auto delta_dispAndPrev_lon    = fabs(this->displayCoordinate.getLon() - mvStat.prevPoint.getLon());
            const auto delta_pointToandPrev_lon = fabs(mvStat.pointTo.getLon() - mvStat.prevPoint.getLon());

            Log::logMsg("displayCoordinate: " + this->displayCoordinate.format_point_to_savepoint());
            Log::logMsg("prevPoint: " + this->mvStat.prevPoint.format_point_to_savepoint());
            Log::logMsg("toPoint: " + this->mvStat.pointTo.format_point_to_savepoint());
            Log::logMsg("Delta lat Disp-Prev: " + mxUtils::formatNumber<double>(delta_dispAndPrev_lat, 6) + ",  Delta lat To-Prev: " + mxUtils::formatNumber<double>(delta_pointToandPrev_lat, 6));
            Log::logMsg("Delta lon Disp-Prev: " + mxUtils::formatNumber<double>(delta_dispAndPrev_lon, 6) + ",  Delta lon To-Prev: " + mxUtils::formatNumber<double>(delta_pointToandPrev_lon, 6));
#endif // !RELEASE

            // calculate distance passed and if reached destination point
            mvStat.hasReachedPointTo = ((fabs(this->displayCoordinate.getLat() - mvStat.prevPoint.getLat()) < fabs(mvStat.pointTo.getLat() - mvStat.prevPoint.getLat())) &&
                                        (fabs(this->displayCoordinate.getLon() - mvStat.prevPoint.getLon()) < fabs(mvStat.pointTo.getLon() - mvStat.prevPoint.getLon()))
                                        )
                                         ? false
                                         : true;

#ifdef DEBUG_MOVE
            double val1, val2, val3, val4;
            val1 = fabs(mvStat.pointTo.getLat() - this->displayCoordinate.getLat());
            val2 = fabs(mvStat.pointTo.getLat() - mvStat.prevPoint.getLat());
            val3 = fabs(mvStat.pointTo.getLon() - this->displayCoordinate.getLon());
            val4 = fabs(mvStat.pointTo.getLon() - mvStat.prevPoint.getLon());

            Utils::logMsg("(pointTo.lat-display.lat)=" + Utils::formatNumber<double>(val1) + ", (pointTo.lat-prev.lat)=" + Utils::formatNumber<double>(val2) + ((val1 < val2) ? "[t]" : "[f]") + " | " +
                          "(pointTo.lon-display.lon)=" + Utils::formatNumber<double>(val3) + ", (pointTo.lon-prev.lon)=" + Utils::formatNumber<double>(val4) + ((val3 < val4) ? "[t]" : "[f]"));

            Utils::logMsg(std::string(" - hasReachedPointTo: ") + ((mvStat.hasReachedPointTo) ? "<< TRUE >>" : "FALSE") + "\n");

#endif


            mvStat.prevPoint.setLat(this->displayCoordinate.getLat());
            mvStat.prevPoint.setLon(this->displayCoordinate.getLon());
            mvStat.prevPoint.setElevationMt(this->displayCoordinate.getElevationInMeter());
            mvStat.prevPoint.timeToWaitOnPoint_sec = this->displayCoordinate.timeToWaitOnPoint_sec;


          } // any change in time ?

        } // end test if reached destination


        /// Handle Reached target point
        if (mvStat.hasReachedPointTo)
        {
#ifdef DEBUG_MOVE
          Utils::logMsg("\nMoving to nextPoint()\n"); // debug
#endif
          mvStat.prevPoint.clone(this->displayCoordinate); // duplicated code
          // move to next point
          this->nextPoint_new(); // v3.0.253.7

          mvStat.currentTimeElapsed_sinceStart = 0.0f;
          mvStat.timeOnVector                  = 0.0f; // v2.1.0 a15

          this->mvStat.flag_wait_for_next_flc = true;
        }

        mvStat.isFirstTime = false;
      }
      else
      {
        mvStat.shouldWeRenderObject = true; // workaround - should we RENDER ?
      }
    } // if any time elapsed from lat iteration
    else
    {
      mvStat.shouldWeRenderObject = false;

    } // end if is in display list (rendered)
  }
}

void
missionx::obj3d::calcNewCourseBetweenTwoPointsOnVector()
{
  std::string err;
  double      distance = 0.0f;
  err.clear();

  distance = this->mvStat.pointFrom.calcDistanceBetween2Points(this->mvStat.pointTo, missionx::mx_units_of_measure::nm, &err);
  if (!err.empty())
    distance = 0.0;

  mvStat.distanceFromTo_ft = distance * (double)(((double)missionx::nm2meter) * ((double)missionx::meter2feet)); //  (float)Utils::nmToFeet(&distance);

  mvStat.secondsToReachTarget = ((float)mvStat.distanceFromTo_ft / ((mvStat.pointFrom.getSpeedFts() == 0.0f) ? 0.000000000001f : mvStat.pointFrom.getSpeedFts())); // seconds to reach destination

#ifdef DEBUG_MOVE
  sprintf(LOG_BUFF, "[Moving3D calc] new pointFrom.lat: %f, new pointFrom.lon: %f", mvStat.pointFrom.getLat(), mvStat.pointFrom.getLon());
  Utils::logMsg(LOG_BUFF);
  sprintf(LOG_BUFF, "[Moving3D calc] new pointTo.lat: %f  , new pointTo.lon: %f", mvStat.pointTo.getLat(), mvStat.pointTo.getLon());
  Utils::logMsg(LOG_BUFF);
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
  return this->displayCoordinate.getElevationInMeter();
}

std::string
missionx::obj3d::getPropKeepUntilLeg()
{
    return Utils::readAttrib(this->node, mxconst::get_ATTRIB_KEEP_UNTIL_LEG(), "");
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
  const bool  result = Utils::readBoolAttrib(this->node, mxconst::get_ATTRIB_CYCLE(), false);

  return result;
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
  Utils::logMsg("Terrain Probed, elevation [" + mxUtils::formatNumber<double>(this->displayCoordinate.getElevationInMeter()) + "mt | " + mxUtils::formatNumber<double>(this->displayCoordinate.getElevationInFeet()) + "ft]" +
                mxconst::get_UNIX_EOL());
#endif
}


void
missionx::obj3d::positionInstancedObject()
{

  const float dummy = 0.0f;
#ifdef DISPLAY_3D_INSTANCE
  std::string instName = this->getInstanceName(); // debug
  {
    IXMLRenderer xPrinter;
    Log::logMsg("\nDisplay Point information: " + displayCoordinate.to_string());
    Log::logMsg("Display Point node: " + std::string(xPrinter.getString(this->displayCoordinate.node)) + "\n\n");
  }
#endif // !RELEASE

  this->displayCoordinate.calc_elevation_include_above_ground_info_and_sync_to_WorldToLocalData(); // v3.0.251.1 calculate real elevation = terrain + above ground

  // prepare XPLMDrawInfo_t
  this->dr.structSize = sizeof(dr);
  this->dr.x          = (float)this->displayCoordinate.local_x;
  this->dr.y          = (float)this->displayCoordinate.local_y;
  this->dr.z          = (float)this->displayCoordinate.local_z;
  this->dr.pitch      = (float)this->displayCoordinate.getPitch();
  this->dr.heading    = (float)this->displayCoordinate.getHeading();
  this->dr.roll       = (float)this->displayCoordinate.getRoll();


  // Show Instance
  if (this->g_instance_ref)
  {
    XPLMInstanceSetPosition(this->g_instance_ref, &this->dr, &dummy);
  }
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
    const obj3d::obj3d_type mType = (obj3d::obj3d_type)(this->getAttribNumericValue<int>(mxconst::get_ATTRIB_OBJ3D_TYPE(), (int)obj3d::obj3d_type::static_obj, err));
    this->obj3dType         = mType;
  }


  // displayDefaultObjectFileOverAlternate
  this->displayDefaultObjectFileOverAlternate = true;

  const std::string name = this->getName();

  // read special data for static obj3d type
  if (this->obj3dType == missionx::obj3d::obj3d_type::static_obj) // obj3d::moving_obj)
  {
    // coordinations from properties
    this->startLocation = getStartLocationAttributes();
  }
  else // v3.0.202  // obj3d::moving_obj
  {
    // v3.0.207.2: start location point is taken from "attributes" if static, and from "path" if moving. For moving, if path is empty then we take from attributes.
    if (this->deqPoints.empty())                          // v3.0.213.7 // replaced (this->path.empty())
      this->startLocation = getStartLocationAttributes(); // moving object receive display attribute from other properties (TBD)
    else
    {
      this->startLocation = this->deqPoints.at(0); // v3.0.213.7 // replaced this->path.at(0); // get start location from PATH container and not the attributes in the XML data file
    }
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
  else if (this->node.isAttributeSet(mxconst::get_PROP_SCRIPT_COND_MET_B().c_str()))
    this->isScriptCondMet = Utils::readBoolAttrib(this->node, mxconst::get_PROP_SCRIPT_COND_MET_B(), false);// v3.303.11  // returned value of PROP_SCRIPT_COND_MET_B
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

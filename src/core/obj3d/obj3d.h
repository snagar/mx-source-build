#ifndef OBJ3D_H_
#define OBJ3D_H_

#pragma once

//#include "../coordinate/UtilsGraph.hpp"
#include <XPLMInstance.h>
#include <XPLMScenery.h>
#include "../../data/mxProperties.hpp"
#include "../../io/IXMLParser.h"
#include "../Timer.hpp"
#include "../coordinate/Points.hpp"
//#include "../dataref_manager.h"
#include "../../ui/gl/CueInfo.h" // v3.0.202a

namespace missionx
{

class obj3d : public missionx::Points
{
  /***
  Obje 3D main properties:
  name = always holds the 3D Object template name
  instance_name = if displayed, then has a unique instance name property
  */

private:
  // instance related
  static XPLMDataRef fps_dref; //"sim/operation/misc/frame_rate_period"

  missionx::mxProperties instProperties; // maybe this should be part of the struct instead of here
  
  CueInfo cue;

protected:
  mx_cue_types cueType;

public:
  using Points::Points;
  using Points::saveCheckpoint;

  typedef enum class _obj3d_type
    : uint8_t
  {
    static_obj = 0,
    moving_obj = 2
  } obj3d_type;

  obj3d();
  ~obj3d();

  // core
  XPLMObjectRef   g_object_ref;
  XPLMInstanceRef g_instance_ref;
  XPLMDrawInfo_t  dr;

  ///// NODE XML
  IXMLNode xConditions;
  IXMLNode xLocation;
  IXMLNode xTilt;
  IXMLNode xPath;
  IXMLNode xDisplayObject_ptr;
  IXMLNode xInstance;              // v3.0.241.1
  bool     parse_node();           // v3.0.241.1
  void     prepareCueMetaData();   // v3.0.303.6     {}; // v3.0.241.1 dummy function so read_mission_file::load_saved_elements() tempalte will work
  CueInfo  getCopyOfCueInfo() { return cue; }; // v3.0.303.6
  CueInfo& getCueInfo_ptr() { return cue; }; // v3.0.303.6

  obj3d_type obj3dType;
  bool       displayDefaultObjectFileOverAlternate; // display default object file or the "alternate_obj_file"
  bool       isScriptCondMet;                       // PROP_SCRIPT_COND_MET_B
  bool       isInDisplayList;
  bool       flagNeedToCalculateRelativePosition{ true }; // v3.303.11 used only when display_object has the "relative_pos_bearing_deg_distance_mt" attribute.

  std::string     file_and_path;     // holds absolute path to 3D Object file. This should not be stored in the savepoint file or at least ignore. It is set when added to the Display Instance List.
  missionx::Point displayCoordinate; // in which coordinate the 3D object is displayed


  ////// Moving Attributes //////////
  // Time * Velocity (speed) = Distance (Street)
  typedef struct _mvStat
  {
    bool isMoving =
      false; // v3.0.207.4 holds flag if object is in moving state. Static objects never moves. // !! If we control movment also from script we will need to calculate elapsed time when stopping movement and then resuming it. !!
    bool isFirstTime                        = true;  // true: first time we calculate movement
    bool shouldWeRenderObject               = false; // true: render object   false: skip rendering (replace DO & SKIP enums from missionx 2).
    bool hasReachedLastPoint                = false;
    bool hasReachedPointTo                  = false;
    bool time_was_advanced_by_draw_function = false;
    bool isInRecursiveState                 = false; // when nextPoint() function calls init, it also calls nextPoint() recursivly. We need to know that.

    int currentPointNo   = 0; // holds the current point on vector. We should deprecate it and use iterators instead
    int noOfPointsInPath = 0;

    float fps                           = 0.0f; // holds frame per second
    float deltaTime                     = 0.0f;
    float timeOnVector                  = 0.0f; // cumulative time on vector per movement part. Reset every point transition.
    float currentTimeElapsed_sinceStart = 0.0f;
    float oldTimeElapsed                = 0.0f;

    float moveSpeed         = 0.0f;
    float lastZuluStartDraw = 0.0f; // we might not need this variable


    // location information
    missionx::Point prevPoint; // Store the previous display point. Also can be addresses as prevPoint. Will assist in calculating distance moved and such.

    missionx::Point pointFrom;
    missionx::Point pointTo;
    double          distanceFromTo_ft    = 0.0f; // holds the distance in "feet" between 2 points
    float           secondsToReachTarget = 0.0f;

    XPLMProbeResult probeResult     = 0;   // we do not really use this parameter, but it is need when we probe ground
    double          groundElevation = 0.0; // v3.0.207.3 will store ground elevation every "flc()" of current object (instead of every "draw()" loop.

    missionx::Timer timer;     // v3.0.207.1 holds time passed from starting point.
    missionx::Timer waitTimer; // v3.0.207.1 holds time to wait at certain point location.

    bool flag_wait_for_next_flc{ true }; // v3.0.253.6 force plugin to wait until we reset this flag to false and only then evaluate position and movement. This help to solve racing issues with moving 3D Object
  } mxStat;

  _mvStat mvStat;

  missionx::Point startLocation; // v3.0.202 start location equals to the first location defined in <location> element.
  void            setCoordinateOnVector(missionx::Point& pointFrom, missionx::Point& pointTo, float time);

  // ---- Members related to movment of 3D Object
  std::deque<missionx::Point>::iterator itPath;
  std::deque<missionx::Point>::iterator itPathEnd;

  void initPathCycle_new(bool isRecursive = false); // v3.0.253.7
  void firstTimeInCycle();
  void CyclePath();
  void nextPoint_new(); // get next point in path, for moving objects
  void setNextWaitTimer();

  void checkAreWeThereYet(); // former moveSet()
  void calcNewCourseBetweenTwoPointsOnVector();

  ////// END Moving Attributes //////////


  // core members //

  // coordination members
  missionx::Point& getCurrentCoordination();
  double           getLat();
  double           getLong();
  double           getElevInFeet();
  double           getElevInMeter();


  // 3D Object related property members
  std::string getPropKeepUntilLeg();
  std::string getPropLinkTask();
  std::string getPropLinkToObjectiveName();
  bool        getIsPathNeedToCycle();
  bool        getHideObject();

  // terrain
  void calculate_real_elevation_to_DisplayCoordination();

  // Load 3D Object
  static void load_cb(const char* real_path, void* ref);
  // instance members
  void create_instance();         // create instance from g_object_ref (file reference)
  void initFpsInfo();             // // v3.0.207.1 get fps information
  void calcPosOfMovingObject();   // v3.0.207.1 // calculate the position of a moving 3D Object
  void positionInstancedObject(); // for moving object we need to calculate position every iteration

  std::string getInstanceName();
  bool        isPlaneInDisplayDistance(missionx::Point& inPlanePoint);

  missionx::Point getStartLocationAttributes(); // v3.0.200 // get location attribute properties as Point

  // savepoint
  void storeCoreAttribAsProperties();
  void applyPropertiesToLocal();
  void saveCheckpoint(IXMLNode& inParent);

  std::string to_string();
};


} // namespace
#endif

#include "dataref_param.h"
#include <fmt/core.h>

/**************

Reviewd: 24-nov-2012

Done:
1. Renamed from "Dataref" to "dataref_param"
2. No real change in code.

ToDo:

**************/


missionx::dataref_param::dataref_param(void)
{
  init();
}
// -----------------------------------

missionx::dataref_param::dataref_param (const std::string &inKey)
{
  init();
  this->setAndInitializeKey(inKey);
}


// -----------------------------------

//missionx::dataref_param::~dataref_param(void)
//{
//}

// -----------------------------------
void
missionx::dataref_param::init()
{
  key.clear();
  dataRefId                     = nullptr;
  dataRefType                   = 0;
  arraySize                     = 0; //
  arrayElementPicked            = 0; //
  arrayElementPickedTranslation = 0; //
  out_vecArrayFloatValues.clear(); // v3.303.12
  out_vecArrayIntValues.clear();   // v3.303.12

  flag_designerPickedOneElement            = false; //
  flag_designerPickedAllElement_inTheArray = false; //
  flag_individual_value_copy_inTheArray    = false; // v303
  
  target_IntArray   = nullptr;
  target_FloatArray = nullptr;
  target_CharArray  = nullptr;

  arrayKey.clear();
}

// -----------------------------------

void
missionx::dataref_param::resetArrays() //
{

  switch (dataRefType)
  {
    case (xplmType_IntArray):
    {
      if (this->target_IntArray != nullptr) // && (*target_IntArray >= 0))
      {
        delete[] target_IntArray;
        target_IntArray = nullptr;
      }
    }
    break;
    case (xplmType_FloatArray):
    {

      if (this->target_FloatArray != nullptr) //&& (*target_FloatArray >= 0.0f))
      {
        delete[] target_FloatArray;
        target_FloatArray = nullptr;
      }
    }
    break;
    case (xplmType_Data):
    {
      if (this->target_CharArray != nullptr) //&& (*target_FloatArray >= 0.0f))
      {
        delete[] target_CharArray;
        target_CharArray = nullptr;
      }
    }
    break;
    default:
      break;
  } // end switch
}


// -----------------------------------

bool
missionx::dataref_param::setDataArray(void* inArray, size_t inArraySize, size_t inOffset, size_t inCount)
{
  return false;
}

// -----------------------------------

int
missionx::dataref_param::getDrefArraySize(const XPLMDataRef& inDataRefId, const XPLMDataTypeID& inDataRefType)
{
  switch (inDataRefType)
  {

    case xplmType_FloatArray:
    {
        return XPLMGetDatavf(inDataRefId, NULL, 0, 0);
    }
    break;
    case xplmType_IntArray:
    {
        return XPLMGetDatavi(inDataRefId, nullptr, 0, 0);
    }
    break;
    case xplmType_Data: // Byte
    {
        return XPLMGetDatab(inDataRefId, nullptr, 0, 0);
    }
    break;

  } // end switch

  return 0;
}

// -----------------------------------

bool
missionx::dataref_param::initDataRefInfo()
{
  bool initOk = true;

  this->dataRefId = XPLMFindDataRef(key.c_str());
  if (!this->dataRefId) // v3.0.223.2 Try to handle keys that might be arrays like: sim/weapons/total_weapon_mass_now[0] which is really a dataref "sim/weapons/total_weapon_mass_now" with array of 25 cells
  {
    const auto pos1 = key.find_first_of('[');
    const auto pos2 = key.find_first_of(']', pos1);
    const auto charArrayCount = pos2 - pos1 - 1;

    this->arrayKey  = this->key.substr(0, key.find_first_of('[')); // v3.0.223.2
    this->dataRefId = XPLMFindDataRef(this->arrayKey.c_str());

    if (const auto offset_value_s = (pos1 != std::string::npos && pos2 != std::string::npos) ? key.substr(pos1 + 1, charArrayCount) : "";
        !offset_value_s.empty() && mxUtils::is_digits(offset_value_s))
    {
      this->arraySize = missionx::dataref_param::getDrefArraySize(this->dataRefId, XPLMGetDataRefTypes(dataRefId)); // try to find out the dataref array size

      this->offset_array_defined_by_user = mxUtils::stringToNumber<int>(offset_value_s);

      if (this->offset_array_defined_by_user <= this->arraySize)
      {
        this->arrayElementPicked            = this->offset_array_defined_by_user;
        this->arrayElementPickedTranslation = this->offset_array_defined_by_user; // since we start counting from 0
      }
      else
      {
        this->offset_array_defined_by_user = 0;
        initOk                             = false; // problem with array definition
      }
    }
  } // end special array case


  // if exists ( not NULL )
  if (dataRefId)
  {
    // store datatype
    this->dataRefType = XPLMGetDataRefTypes(dataRefId);

    if (this->getName().empty()) // v3.303.9.1 hopefully will solve cases where save checkpoint dataref won't have a name
      this->setName(key);

    // check if dataref is array and prepare memory accordingly
    switch (dataRefType)
    {

      case xplmType_Data: // Byte
      {
        this->arraySize = XPLMGetDatab(this->dataRefId, NULL, 0, 0);
        if (this->arraySize > 0)
        {
          target_CharArray = new char[arraySize];
          std::memset(target_CharArray, 0, sizeof(char) * arraySize); // initialize memory array
        }
        else
          target_CharArray = nullptr;
      }
      break;
      case xplmType_IntArray:
      {
        this->arraySize = XPLMGetDatavi(this->dataRefId, NULL, 0, 0);
        if (this->arraySize > 0)
        {
          target_IntArray = new int[arraySize];
          std::memset(target_IntArray, 0, sizeof(int) * arraySize); // initialize memory array
        }
        else
          target_IntArray = nullptr;
      }
      break;
      case xplmType_FloatArray:
      {
        this->arraySize = XPLMGetDatavf(this->dataRefId, NULL, 0, 0);

        if (this->arraySize > 0)
        {
          target_FloatArray = new float[arraySize];
          std::memset(target_FloatArray, 0, sizeof(float) * arraySize); // initialize memory array
        }
        else
          target_FloatArray = nullptr;
          //target_FloatArray = source_FloatArray = nullptr;
      }
      break;
      default:
      {
      }
      break;

    } // end switch

    if (this->offset_array_defined_by_user > 0 && this->offset_array_defined_by_user > this->arraySize) // v3.303.12 if user defined an offset value than this->offset_array_defined_by_user will be larger than zero. We then need to make sure it is not larger than the arraySize itself
      initOk = false;

  } // end if refID is valid
  else
  {
    initOk = false;

    errReason = "[UserDataRef] dataref:" + key + ", was not added. Check spelling.";
#ifdef DEBUG_LOGIC
    sprintf(LOG_BUFF, "\n[UserDataRef] dataref: %s, was not added. Check spelling.", key.c_str());
    XPLMDebugString(LOG_BUFF);
#endif
  }

  // From MxParam class
  this->flag_paramReadyToBeUsed = initOk;

  if (initOk)
    readDatarefValue_into_missionx_plugin(); // initialise the dataref_param class with the current value

  return initOk;
} // addUserDataRef


  // -----------------------------------

bool
missionx::dataref_param::parse_node()
{
  assert(!this->node.isEmpty());

  std::string name = Utils::readAttrib(this->node, mxconst::get_ATTRIB_NAME(), "");
  name             = Utils::trim(name);
  this->key        = Utils::readAttrib(this->node, mxconst::get_ATTRIB_DREF_KEY(), "");

  // validate data
  if (!(name.empty()) && !(this->key.empty())) // if both have values
  {
    this->setName(name);

    // init Dataref info
    if (this->initDataRefInfo())
    {
      // check if array and if designer defined specific cell in its name
      if (this->arraySize > 0) // check array dataref
      {
        if (!this->evaluateArray()) // this function can change "isParamReadyToBeUsed" to FALSE hence it won't be available.
        {
          Log::add_missionLoadError("[Logic]Dataref Array: " + name + ", is not valid ARRAY. Reason: " + this->errReason + "!!!");
          return false;
        }

      } // end if is array
    }
    else
    {
      Log::add_missionLoadError("[Logic]Init dataref data failed. Please check key or dataref name in Element. name: [" + name + "], Key: [" + this->key + "]");
      return false;
    } // if initStatic succeed
  }
  else // missing info
  {
    Log::add_missionLoadError("[Logic]Check dataref settings. Key: [" + this->key + "], name: [" + name + "]");
    return false;
  }

  return true;
}

// -----------------------------------

void
missionx::dataref_param::setAndInitializeKey(std::string inKey)
{
  this->key = inKey;
  initDataRefInfo();
}

// -----------------------------------

XPLMDataRef
missionx::dataref_param::getDataRefID()
{
  return this->dataRefId;
}

// -----------------------------------

XPLMDataTypeID
missionx::dataref_param::getDataRefType()
{
  return this->dataRefType;
}

// -----------------------------------

bool
missionx::dataref_param::evaluateArray()
{
  if (this->arraySize > 0)
  {
    int  arrayCell    = 0;
    bool isArrayValid = true; // always valid unless proved otherwise
    int  intValue     = 0;
    //std::string errStr;
    //errStr.clear();
    //std::string str = getName(errStr);
    std::string str = getName();
    if (str.empty())
      str = Utils::extractLastFromString(this->key, "/"); // name = key without the path


    size_t lastRightBracket = 0;
    size_t lastLeftBracket  = 0;

    if (str.empty()) // fix crash when calling "string.back()" when string is empty
      return false;

    if (str.back() == ']') // if there is a right bracket
    {
      lastRightBracket = str.find_last_of(']');

      // find last right bracket and pick string between the brackets
      lastLeftBracket = str.find_last_of('[');
      if (lastLeftBracket != std::string::npos) // if found left
      {
        std::string strNumber = str.substr((lastLeftBracket + 1), lastRightBracket - (lastLeftBracket + 1));
        // check if string is a number
        if (Utils::is_digits(strNumber))
        {
          intValue  = Utils::stringToNumber<int>(strNumber);
          arrayCell = intValue; // Utils::stringToNumber<int>(strNumber);
          if (arrayCell >= 0 && arrayCell <= this->arraySize)
          {
            isArrayValid                             = true;
            flag_designerPickedOneElement            = true;
            flag_designerPickedAllElement_inTheArray = false;
          }
          else
          { // array element is not in the valid range
            isArrayValid    = false;
            this->errReason = "[DREF ARRAY ERROR] dref: " + this->key + " is not in a valid range. Max array size is: " + Utils::formatNumber<int>(arraySize) + " starting at 0 .. " +
                              Utils::formatNumber<int>((arraySize - 1 <= 0) ? 0 : arraySize - 1) + "  . Skipping writing.";
          }
        }
        else
        { // not a number
          isArrayValid    = false;
          this->errReason = "[DREF ARRAY ERROR] dref: " + this->key + ", has an array index that is not a valid number.";
        }

      } // end leftBracket exists

    } // end right bracket exists
    else
    { // dataref key is FULL array

      isArrayValid                             = true;
      flag_designerPickedOneElement            = false;
      flag_designerPickedAllElement_inTheArray = true;

      arrayCell = 0;
    }

    if (isArrayValid)
    {
      if (flag_designerPickedOneElement)
      {
        this->arrayElementPicked = arrayCell;
        this->arrayElementPickedTranslation =
          ((arrayCell < 0) ? 0 : arrayCell /*arrayCell - 1*/); // make sure array translation is not negative number, It seems LR will ignore out of bound indexes. Example user tried to modify a cell larger than the array support.
      }
      else
      {
        this->arrayElementPicked            = arrayCell;
        this->arrayElementPickedTranslation = 0; // 0 = all attaya
      }
    }
    else
    {

      if (errReason.empty())
        this->errReason = "\n[eval dref] dref array: " + this->key + ", is not valid. Check its name/array bound. Skipping writing."; // mxconst::MX_ERR_WRONG_ARRAY_NUMBER;

      this->flag_paramReadyToBeUsed = false;
    }
  } // end if is array
  // end if array


  return flag_paramReadyToBeUsed;
}

// -----------------------------------

void
missionx::dataref_param::fill_IntArrayIntoVector(int* inOut_Array, std::vector<int>& outVecToFill, std::string& outStrArrayValue, const bool& inFlagManuallyPreparedVector)
{
  if (!(this->dataRefType == xplmType_IntArray))
  {
    Log::logMsgErr("Not Int Array Type. Skipping...");
    return;
  }
  outStrArrayValue.clear();


  if (inFlagManuallyPreparedVector)
  { // fill array float with the vector custom data
    auto counterSize = (this->arraySize <= outVecToFill.size()) ? this->arraySize : outVecToFill.size();
    for (auto i = (size_t)0; i < counterSize; ++i)
    {
      inOut_Array[i] = outVecToFill.at(i);
      outStrArrayValue.append(Utils::formatNumber<int>(inOut_Array[i]));
      if (i < (counterSize - 1))
        outStrArrayValue.append(",");
    }
  }
  else
  {
    outVecToFill.clear();
    for (int i = 0; i < this->arraySize; i++)
    {
      outVecToFill.push_back(inOut_Array[i]);
      outStrArrayValue.append(Utils::formatNumber<int>(inOut_Array[i]));
      if (i < (this->arraySize - 1))
        outStrArrayValue.append(",");
    }
  }
}

// -----------------------------------

void
missionx::dataref_param::fill_FloatArrayIntoVector(float* inOut_Array, std::vector<float>& outVecToFill, std::string& outStrArrayValue, const bool& inFlagManuallyPreparedTheOutVector_notTheTargetVector)
{
  if (!(this->dataRefType == xplmType_FloatArray))
  {
    Log::logMsgErr("Not Float Array Type. Skipping...");
    return;
  }
  // fill vector with values.

  outStrArrayValue.clear();

  if (inFlagManuallyPreparedTheOutVector_notTheTargetVector)
  { // fill array float with the vector custom data
    auto counterSize = (this->arraySize <= outVecToFill.size()) ? this->arraySize : outVecToFill.size();
    for (auto i = (size_t)0; i < counterSize; ++i)
    {
      inOut_Array[i] = outVecToFill.at(i);
      outStrArrayValue.append(Utils::formatNumber<float>(inOut_Array[i]));
      if (i < (counterSize - 1))
        outStrArrayValue.append(",");
    }  
  }
  else
  {
    outVecToFill.clear();
    for (int i = 0; i < this->arraySize; ++i)
    {
      outVecToFill.push_back(inOut_Array[i]);
      outStrArrayValue.append(Utils::formatNumber<float>(inOut_Array[i]));
      if (i < (this->arraySize - 1))
        outStrArrayValue.append(",");
    }
  }



}

// -----------------------------------

bool
missionx::dataref_param::flc_interpolation(bool& outDrefWasChanged)
{
  outDrefWasChanged = false; // v3.305.3
  if (this->strctInterData.currentCycleCounter_i < this->strctInterData.for_how_many_cycles_i)
  {
    if (missionx::Timer::wasEnded(this->strctInterData.timerToRun))
    {
      outDrefWasChanged ^= 1; // v3.305.3 change to true
      ++this->strctInterData.currentCycleCounter_i;
      int estimatedArraySize_i = Utils::countCharsInString(this->strctInterData.arrayValuesAsString, ',') + 1; // we add 1 since we believe the designer wrote a valid format, like: "1,2.5,0" and not ",,," or "1,2"

      // calculate the interpolation
      switch (this->dataRefType)
      {
        case xplmType_FloatArray:
        {
          // the cycle logic is in set_array_interpolation_values
          this->progress_array_interpolation_values<float>(this->strctInterData, this->strctInterData.vecLastInterValues_f);
          this->out_vecArrayFloatValues = this->strctInterData.vecLastInterValues_f;

#ifndef RELEASE
          std::string debugOutput{ "" };
          for (const auto& v : this->out_vecArrayFloatValues)
            debugOutput.append(mxUtils::formatNumber<float>(v, 4)).append( "," );

          Log::logMsg("Key: " + this->key + " [" + debugOutput + "]");
#endif // !RELEASE


          if (this->setTargetArray<xplmType_FloatArray, float>(estimatedArraySize_i, this->strctInterData.arrayValuesAsString, false, ","))
            dataref_param::set_dataref_values_into_xplane(*this);

        }
        break;
        case xplmType_IntArray:
        {
          // the cycle logic is in set_array_interpolation_values
          this->progress_array_interpolation_values<int>(this->strctInterData, this->strctInterData.vecLastInterValues_f);
          
#ifndef RELEASE
          std::string debugOutput{ "" };
          for (const auto& v : this->strctInterData.vecLastInterValues_f)
            debugOutput.append(mxUtils::formatNumber<int>((int)v)).append(",");

          Log::logMsg("Key: " + this->key + " [" + debugOutput + "]");
#endif // !RELEASE

          if (this->setTargetArray<xplmType_IntArray, int>(this->arraySize, this->strctInterData.arrayValuesAsString, false, ","))
            dataref_param::set_dataref_values_into_xplane(*this);
        }
        break;
        default:
        {

          // interpolate cycle
          if (this->strctInterData.flagIsValid)
          {
            this->strctInterData.last_value_d += this->strctInterData.delta_scalar;
            this->setValue(strctInterData.last_value_d);
            missionx::dataref_param::set_dataref_values_into_xplane(*this);
          }

          #ifndef RELEASE           
                    Log::logMsg(fmt::format("Key: {}\n\t\tlast_value_d={:.6f}, cycles: {}/{} , {}.", this->key, strctInterData.last_value_d, this->strctInterData.currentCycleCounter_i, this->strctInterData.for_how_many_cycles_i, (this->strctInterData.flagIsValid) ? "Valid" : "Not Valid"));
                    //Log::logMsg("Key: " + this->key + "\n\t\tlast_value_d= " + mxUtils::formatNumber<double>(strctInterData.last_value_d, 4) + ", cycle"));
          #endif // !RELEASE

        }
        break;
      } // end switch

      this->strctInterData.timerToRun.reset();
      if (this->strctInterData.seconds_to_run_i <= 0)
        this->strctInterData.timerToRun.setEnd();
      else
        missionx::Timer::start(this->strctInterData.timerToRun, (float)this->strctInterData.seconds_to_run_i);

      #ifndef RELEASE
        Log::logMsg("Interpolated dataref: " + this->name + " [" + this->getParamStringValue() + "]\n"); // debug
      #endif // !RELEASE


    } // end if timer ended
    else if(this->strctInterData.timerToRun.getState() == missionx::mx_timer_state::timer_not_set)
    {
        if (this->strctInterData.seconds_to_run_i <= 0)
          this->strctInterData.timerToRun.setEnd();
        else 
          missionx::Timer::start(this->strctInterData.timerToRun, (float)this->strctInterData.seconds_to_run_i);
    }
  }
    
  // decide if interpolation has reached its end
  if (this->strctInterData.currentCycleCounter_i >= this->strctInterData.for_how_many_cycles_i)
  {
    this->strctInterData.timerToRun.reset();
    return true;
  }

  return false;
}

// -----------------------------------

void
missionx::dataref_param::initInterpolation(const unsigned int inInterpolateEvery_n_Seconds, const unsigned int inCycles, const std::string& inTargetValue)
{

  this->strctInterData.for_how_many_cycles_i = inCycles;
  this->strctInterData.seconds_to_run_i      = inInterpolateEvery_n_Seconds;
  this->strctInterData.sTargetValues         = inTargetValue; // v3.305.3

  switch(this->dataRefType)
  {
    case xplmType_FloatArray:
    case xplmType_IntArray:
    {
      this->initArrayInterpolation(inInterpolateEvery_n_Seconds, inCycles, inTargetValue);

      // prepare debug delta string
      this->strctInterData.sDelta.clear();
      for (const auto& inVal : this->strctInterData.delta_array)
      {
        if (!this->strctInterData.sDelta.empty())
          this->strctInterData.sDelta.append(",");

        this->strctInterData.sDelta += mxUtils::formatNumber<float>(inVal, 3);        
      }


#ifndef RELEASE
      this->debugArrayInterpolationValuesAfterInitialization(inTargetValue);
#endif // !RELEASE

    }
    break;
    default:
    {
      double v_d = 0.0;

      std::vector<double> vecTargetValues = Utils::splitStringToNumbers<double>(inTargetValue, ",");
      if (vecTargetValues.size() > 0)
        v_d = vecTargetValues.at(0);

      this->strctInterData.flagIsValid = this->initScalarInterpolation(inInterpolateEvery_n_Seconds, inCycles, v_d);  

      this->strctInterData.sDelta = mxUtils::formatNumber<double>(this->strctInterData.delta_scalar, 3);
    }
    break;
  
  }
}

// -----------------------------------

bool
missionx::dataref_param::initScalarInterpolation(const unsigned int inInterpolateEvery_n_Seconds, const unsigned int inCycles, const double inTargetValue)
{
  if ( !this->strctInterData.timerToRun.isRunning() )
  {   

    this->readDatarefValue_into_missionx_plugin();
    this->strctInterData.starting_scalar_value_d = this->getValue<double>();
    this->strctInterData.sStartValues            = mxUtils::formatNumber<double>(this->strctInterData.starting_scalar_value_d, 6);

    if (fabs(inTargetValue - this->strctInterData.starting_scalar_value_d) <= missionx::NEARLY_ZERO)
      return false;

    this->strctInterData.delta_scalar = (inTargetValue - this->strctInterData.starting_scalar_value_d)/inCycles;
    if (this->strctInterData.delta_scalar == 0.0f)
      return false;

    this->strctInterData.last_value_d = this->strctInterData.starting_scalar_value_d;  //this->getValue<double>();
    this->strctInterData.target_value_d = inTargetValue;
    this->strctInterData.timerToRun.reset();
    missionx::Timer::start(this->strctInterData.timerToRun, (float)inInterpolateEvery_n_Seconds);
  }

  return true;
}

// -----------------------------------

void
missionx::dataref_param::initArrayInterpolation(const unsigned int inInterpolateEvery_n_Seconds, const unsigned int inCycles, const std::string& inTargetValuesAsString)
{
  if (!this->strctInterData.timerToRun.isRunning())
  {
    this->readDatarefValue_into_missionx_plugin();

    if (this->dataRefType == xplmType_FloatArray)
      this->strctInterData.vecLastInterValues_f = this->out_vecArrayFloatValues;
    else if (this->dataRefType == xplmType_IntArray)
      std::for_each(this->out_vecArrayIntValues.begin(), this->out_vecArrayIntValues.end(), [&](auto iVal) { this->strctInterData.vecLastInterValues_f.emplace_back((float)iVal); });
    else
    {
      Log::logMsg(std::string(__func__) + ", dataref array dataype is not supported for: " + this->getName());
      return; // skip the rest since the data is not supported
    }

    this->strctInterData.sStartValues = this->pStrArrayValue; // v3.305.3

    // call the initialization template using "vecLastInterValues_f" vector since it is being used for both "float" and "int" arrays.
    this->init_array_interpolation_operation<float>(this->strctInterData, this->strctInterData.vecLastInterValues_f, inTargetValuesAsString, inCycles);

    this->strctInterData.timerToRun.reset();
    missionx::Timer::start(this->strctInterData.timerToRun, (float)inInterpolateEvery_n_Seconds);

  }
}

// -----------------------------------

void
missionx::dataref_param::debugArrayInterpolationValuesAfterInitialization(const std::string& inTargetValuesAsString)
{
#ifndef RELEASE


  std::vector<float> inVecTargetValues = Utils::splitStringToNumbers<float>(inTargetValuesAsString, ",");

  for (int iLoop = 1; iLoop <= this->strctInterData.for_how_many_cycles_i; ++iLoop)
  {
    float loopCycle_f = (float)iLoop;
    int counter = 0;

    auto iterStartingValues    = this->strctInterData.vecLastInterValues_f.cbegin();
    auto iterEndStartingValues = this->strctInterData.vecLastInterValues_f.cend();

    auto iterDelta    = this->strctInterData.delta_array.cbegin();
    auto iterEndDelta = this->strctInterData.delta_array.cend();

    std::string delta_s;

    while (++counter && iterDelta != iterEndDelta && iterStartingValues != iterEndStartingValues)
    {

      [[maybe_unused]]
      const float deltaVal = std::round((*iterDelta) * loopCycle_f); // debug - calculate

      if (counter > 1)
      {
        if (this->dataRefType == xplmType_IntArray)
          delta_s += "," + mxUtils::formatNumber<float>(std::round(((*iterDelta) * loopCycle_f) + (*iterStartingValues))); // round value to int like number
        else
          delta_s += "," + mxUtils::formatNumber<float>((float)((*iterDelta) * loopCycle_f) + (float)(*iterStartingValues)); // float
      }
      else
      {
        if (this->dataRefType == xplmType_IntArray)
          delta_s += mxUtils::formatNumber<float>(std::round(((*iterDelta) * loopCycle_f) + (*iterStartingValues))); // round value to int like number
        else
          delta_s += mxUtils::formatNumber<float>((float)((*iterDelta) * loopCycle_f) + (float)(*iterStartingValues)); // float
      }

      ++iterDelta;
      ++iterStartingValues;
    }

    this->strctInterData.dq_cycle_values_as_string.emplace_back(delta_s);
  } // end for loop

#endif // !RELEASE
}

// -----------------------------------

void
missionx::dataref_param::readDatarefValue_into_missionx_plugin()
{
  this->pStrArrayValue.clear();

  if (this->flag_paramReadyToBeUsed)
  {
    // decide which kind of data need to fetch
    switch (dataRefType)
    {
      case xplmType_Int:
      {
        this->setValue((double)XPLMGetDatai(this->dataRefId));
      }
      break;
      case xplmType_Float:
      {
        this->setValue((double)XPLMGetDataf(this->dataRefId));
      }
      break;
      case (xplmType_Double):
      case (xplmType_Float | xplmType_Double):
      {
        this->setValue(XPLMGetDatad(this->dataRefId));
      }
      break;
      case (xplmType_IntArray):
      {
        if (flag_paramReadyToBeUsed && this->flag_designerPickedOneElement )
        {
          XPLMGetDatavi(this->dataRefId, target_IntArray, 0, this->arraySize);
          this->setValue((double)target_IntArray[this->arrayElementPickedTranslation]);
        }
        else if (flag_paramReadyToBeUsed && !this->flag_designerPickedOneElement ) // if we picked array and it is not one element
        {
          this->readArrayDatarefIntoLocalArray();                       // v3.303.13 added missing initialize step
        }
      }
      break;
      case (xplmType_FloatArray):
      {
        if (flag_paramReadyToBeUsed && this->flag_designerPickedOneElement )
        {
          XPLMGetDatavf(this->dataRefId, target_FloatArray, 0, this->arraySize);
          this->setValue((double)target_FloatArray[this->arrayElementPickedTranslation]);
        }
        else if (flag_paramReadyToBeUsed && !this->flag_designerPickedOneElement)
        {
          //XPLMGetDatavf(this->dataRefId, target_FloatArray, 0, this->arraySize); // v3.303.13 added missing initialize step

          this->readArrayDatarefIntoLocalArray();  // v3.303.13 added missing initialize step

        }
      }
      break;
      case (xplmType_Data): // v3.0.255.1
      {
        if (flag_paramReadyToBeUsed)
        {
          XPLMGetDatab(this->dataRefId, target_CharArray, 0, this->arraySize);
          this->setParamStringValue(std::string(target_CharArray));
          //this->setStringProperty(this->getName(), std::string(target_CharArray)); // moved into setParamStringValue()
        }
      }
      break;
      default:
        break;

    } // switch
  }
} // readDatarefValue

// -----------------------------------

void
missionx::dataref_param::readArrayDatarefIntoLocalArray()
{
  this->pStrArrayValue.clear();

  if (this->flag_paramReadyToBeUsed)
  {
    // decide which kind of data need to fetch
    switch (dataRefType)
    {
      case (xplmType_IntArray):
      {
        XPLMGetDatavi(this->dataRefId, this->target_IntArray, 0, this->arraySize);
        fill_IntArrayIntoVector(this->target_IntArray, this->out_vecArrayIntValues, this->pStrArrayValue);
        if (this->out_vecArrayIntValues.empty())
        {
          this->flag_paramReadyToBeUsed = false;
        }
      }
      break;
      case (xplmType_FloatArray):
      {
        XPLMGetDatavf(this->dataRefId, target_FloatArray, 0, this->arraySize);
        fill_FloatArrayIntoVector(this->target_FloatArray, this->out_vecArrayFloatValues, this->pStrArrayValue);
        if (out_vecArrayFloatValues.empty())
        {
          flag_paramReadyToBeUsed = false;
        }
        else
        {
          // initialize with first value from vector
          this->setValue((double)out_vecArrayFloatValues.at(0));
        }
      }
      break;
      case (xplmType_Data): // v3.0.255.1
      {
        XPLMGetDatab(this->dataRefId, this->target_CharArray, 0, this->arraySize);
        this->setStringProperty(this->getName(), std::string(this->target_CharArray));
      }
      break;
      default:
        break;

    }
  } // end if ready to be used

}

// -----------------------------------

void
missionx::dataref_param::storeCoreAttribAsProperties()
{
  this->setStringProperty(mxconst::get_ATTRIB_DREF_KEY(), this->key);
}

// -----------------------------------

void
missionx::dataref_param::applyPropertiesToCurrentDrefClass()
{
  const std::string key = Utils::readAttrib(this->node, mxconst::get_ATTRIB_DREF_KEY(), ""); // v3.0.241.1
  this->setAndInitializeKey(key);
}

// -----------------------------------

void
missionx::dataref_param::saveCheckpoint(IXMLNode& inParent)
{
  // todo: add value + type before

  inParent.addChild(this->node.deepCopy());


#ifdef MX_EXE
  Log::logMsgWarn("In console mode, there are no Datarefs. Only in plugin mode");
#endif //
}

// -----------------------------------

void
missionx::dataref_param::saveInterpolationCheckpoint(IXMLNode& inDrefNode)
{
  // the parent should be a <dataref>
  inDrefNode.addAttribute(mxconst::PROP_SECONDS_TO_RUN_I, mxUtils::formatNumber<int>(this->strctInterData.seconds_to_run_i).c_str());
  inDrefNode.addAttribute(mxconst::PROP_FOR_HOW_MANY_CYCLES_I, mxUtils::formatNumber<int>(this->strctInterData.for_how_many_cycles_i).c_str());
  inDrefNode.addAttribute(mxconst::PROP_CURRENTCYCLECOUNTER_I, mxUtils::formatNumber<int>(this->strctInterData.currentCycleCounter_i).c_str());

  inDrefNode.addAttribute(mxconst::PROP_SECONDS_PASSED_F, mxUtils::formatNumber<float>(this->strctInterData.timerToRun.getSecondsPassed(), 2).c_str());
  
  inDrefNode.addAttribute(mxconst::PROP_TARGET_VALUES_S, this->strctInterData.sTargetValues.c_str());
  inDrefNode.addAttribute(mxconst::PROP_START_VALUES_S, this->strctInterData.sStartValues.c_str());
  inDrefNode.addAttribute(mxconst::PROP_DELTA_VALUES_S, this->strctInterData.sDelta.c_str());

  switch (this->dataRefType)
  {
    case xplmType_IntArray:
    case xplmType_FloatArray:
    {
      std::string delta_array_s;
      int         counter = 0;

      for (const auto& v : this->strctInterData.delta_array)
      {
        if (counter == 0)
          delta_array_s += mxUtils::formatNumber<float>( v, 2);
        else 
          delta_array_s += "," + mxUtils::formatNumber<float>(v, 2);

        ++counter;
      }

      inDrefNode.addAttribute(mxconst::PROP_DELTA_ARRAY_F, delta_array_s.c_str());
      inDrefNode.addAttribute(mxconst::PROP_LAST_VALUE_ARRAY_D, this->strctInterData.arrayValuesAsString.c_str());

    }
    break;
    default:
    {
      inDrefNode.addAttribute(mxconst::PROP_DELTA_SCALAR, mxUtils::formatNumber<double>(this->strctInterData.delta_scalar, 6).c_str());
      inDrefNode.addAttribute(mxconst::PROP_LAST_VALUE_D, mxUtils::formatNumber<double>(this->strctInterData.last_value_d, 6).c_str());
      inDrefNode.addAttribute(mxconst::PROP_TARGET_VALUE_D, mxUtils::formatNumber<double>(this->strctInterData.target_value_d, 6).c_str());
      inDrefNode.addAttribute(mxconst::PROP_STARTING_SCALAR_VALUE_D, mxUtils::formatNumber<double>(this->strctInterData.starting_scalar_value_d, 6).c_str());
    }
    break;
  }

  // add timer data node
  if (this->strctInterData.timerToRun.node.isEmpty())
    this->strctInterData.timerToRun.node = inDrefNode.addChild( mxconst::get_ELEMENT_TIMER().c_str() );

  // add name to timer
  if (this->strctInterData.timerToRun.getName().empty() && !this->strctInterData.timerToRun.node.isEmpty())
    this->strctInterData.timerToRun.node.updateAttribute(this->getName().c_str(), mxconst::get_ATTRIB_NAME().c_str(), mxconst::get_ATTRIB_NAME().c_str());

  this->strctInterData.timerToRun.saveCheckpoint(inDrefNode);
  // force "timer_set" state so "START_MISSION()" will correctly start the timer.
  this->strctInterData.timerToRun.node.updateAttribute(mxUtils::formatNumber<int>((int)mx_timer_state::timer_is_set).c_str(), mxconst::get_ATTRIB_TIMER_STATE().c_str(), mxconst::get_ATTRIB_TIMER_STATE().c_str());

  int nCounter = inDrefNode.nChildNode(mxconst::get_ELEMENT_TIMER().c_str());
  
  // end saveInterpolationCheckpoint
}

// -----------------------------------

bool
missionx::dataref_param::initInterpolationDataFromSaveFile()
{
  this->strctInterData.seconds_to_run_i = this->getAttribNumericValue<int>(mxconst::PROP_SECONDS_TO_RUN_I, -1);
  this->strctInterData.for_how_many_cycles_i = this->getAttribNumericValue<int>(mxconst::PROP_FOR_HOW_MANY_CYCLES_I, -1);
  this->strctInterData.currentCycleCounter_i = this->getAttribNumericValue<int>(mxconst::PROP_CURRENTCYCLECOUNTER_I, -1);

  this->strctInterData.sTargetValues = this->getStringAttributeValue(mxconst::PROP_TARGET_VALUES_S, "");
  this->strctInterData.sStartValues  = this->getStringAttributeValue(mxconst::PROP_START_VALUES_S, "");
  this->strctInterData.sDelta        = this->getStringAttributeValue(mxconst::PROP_DELTA_VALUES_S, "");


      // validations
  if ((this->strctInterData.seconds_to_run_i < 0) + (this->strctInterData.for_how_many_cycles_i < 0) + (this->strctInterData.currentCycleCounter_i < 0)) // at least 1 variable is less than 0
    return false;


  switch (this->dataRefType)
  {
    case xplmType_IntArray:
    case xplmType_FloatArray:
    {
      std::string delta_array_s = this->getStringAttributeValue(mxconst::PROP_DELTA_ARRAY_F, "");
      auto        vecNumbers    = Utils::splitStringToNumbers<float>(delta_array_s, ",");

      int         estimatedArraySize_i = (int)vecNumbers.size();
      
      std::for_each(vecNumbers.begin(), vecNumbers.end(), [&](const auto& v) { this->strctInterData.delta_array.push_back(v); }); // copy all elements from vector A to B
      //for (const auto& v : vecNumbers) this->strctInterData.delta_array.push_back(v);


      if (this->dataRefType == xplmType_IntArray)
      {
        for (const auto& v : this->out_vecArrayIntValues)
          this->strctInterData.vecLastInterValues_f.push_back((float)v);
      }
      else
      {
        for (const auto& v : this->out_vecArrayFloatValues)
          this->strctInterData.vecLastInterValues_f.push_back(v);
      }


    }
    break;
    default:
    {
      std::string last_value_s = this->getStringAttributeValue(mxconst::PROP_LAST_VALUE_D, "");

      this->strctInterData.delta_scalar = this->getAttribNumericValue<double>(mxconst::PROP_DELTA_SCALAR, -1.0);
      this->strctInterData.last_value_d = (last_value_s.empty())? this->getValue<double>() : mxUtils::stringToNumber<double>(last_value_s, 6);

      this->strctInterData.target_value_d = this->getAttribNumericValue<double>(mxconst::PROP_TARGET_VALUE_D, -1.0);
      this->strctInterData.starting_scalar_value_d = this->getAttribNumericValue<double>(mxconst::PROP_STARTING_SCALAR_VALUE_D, -1.0);

      // validations
      if (this->strctInterData.delta_scalar == -1.0)
        return false;

    }
    break;
  }

  // We will initialize the timer during "START_MISSION()" since we want the OS clock to be as accurate as possible
  return true; // all is valid
}
 


// -----------------------------------

void
missionx::dataref_param::set_dataref_values_into_xplane(dataref_param& inDref, const bool& inFlagManuallyPreparedVector)
{
  // v3.303.12 deprecated variables, we will just use inDref.xxx directly.

  if (inDref.dataRefId) // if exists
  {

    switch (inDref.dataRefType)
    {
      case xplmType_Int:
      {
        XPLMSetDatai(inDref.dataRefId, inDref.getValue<int>());
      }
      break;
      case xplmType_Data: // v3.0.255.1
      {
        std::string       err;
        const std::string val = inDref.getParamStringValue();
        XPLMSetDatab(inDref.dataRefId, (void*)val.data(), 0, (int)val.length());
      }
      break;
      case xplmType_Float:
      {
        XPLMSetDataf(inDref.dataRefId, inDref.getValue<float>());
      }
      break;
      case (xplmType_Double): // v3.0.255.4.3 added to solve user dataref creation cases.
      case (xplmType_Float | xplmType_Double):
      {
        XPLMSetDatad(inDref.dataRefId, inDref.getValue<double>());
      }
      break;
      case (xplmType_IntArray): // can only return the value of specific array and not the whole array. arrayElementPicked must be defined
      {
        inDref.fill_IntArrayIntoVector(inDref.target_IntArray, inDref.out_vecArrayIntValues, inDref.pStrArrayValue, inFlagManuallyPreparedVector); // v3.303.13

        if (inDref.out_vecArrayIntValues.empty())
          return; // skip

        auto vecSize = inDref.out_vecArrayIntValues.size();
        if (!inDref.flag_individual_value_copy_inTheArray) // v3.303.12 no need if we are using the new implementation
        {

          for (auto i1 = (size_t)0; i1 < vecSize; ++i1)
          {
            inDref.out_vecArrayIntValues.at(i1) = inDref.getValue<int>();
          }
        }

        int* iValuesArray = &inDref.out_vecArrayIntValues[0];

        if (inDref.flag_individual_value_copy_inTheArray) // v3.303.12 added the new array implementation
        {
          XPLMSetDatavi(inDref.dataRefId, inDref.target_IntArray, inDref.offset_array_defined_by_user, inDref.offset_how_many_variables_to_read_from_the_array); 

        }
        else if (inDref.flag_designerPickedAllElement_inTheArray) // v3.0.221.10 assign value to all array based on one value
        {
          XPLMSetDatavi(inDref.dataRefId, iValuesArray, 0, (int)inDref.out_vecArrayIntValues.size());
        }
        else // assign value to specific value in array index
        {
          if ( inDref.arrayElementPickedTranslation <= vecSize)
          {
            XPLMSetDatavi(inDref.dataRefId, iValuesArray, inDref.arrayElementPickedTranslation, 1); // only 1
          }
          else
          {
#ifndef Release
            Log::logMsgErr("\n[write to dref] dref array: " + inDref.getName() + ", is out of bound. Max array size is: " + Utils::formatNumber<int>(inDref.arraySize) + " starting from 0.." +
                           Utils::formatNumber<int>((inDref.arraySize - 1 <= 0) ? 0 : inDref.arraySize - 1) + "  . Skipping writing.");
#endif // !Release
          }
        }
      }
      break;
      case (xplmType_FloatArray): // can only return the value of specific array and not the whole array. arrayElementPicked must be defined
      { 
        inDref.fill_FloatArrayIntoVector(inDref.target_FloatArray, inDref.out_vecArrayFloatValues, inDref.pStrArrayValue, inFlagManuallyPreparedVector); // v3.0.221.10 // v3.303.13 interpolation datarefs are using target_float/int arrays to hold their values and not the original arrays and vectors.

        if (inDref.out_vecArrayFloatValues.empty())
          return; // skip

        // We always prepare the whole array with same value and only then decide if we need to set 1 or all when calling XPLMSetDatavf
        auto vecSize = inDref.out_vecArrayFloatValues.size();
        if (!inDref.flag_individual_value_copy_inTheArray)
        {
          for (auto i1 = (size_t)0; i1 < vecSize; ++i1)
          {
            inDref.out_vecArrayFloatValues.at(i1) = inDref.getValue<float>() * inDref.flag_designerPickedOneElement + inDref.getValue<float>() * inDref.flag_designerPickedAllElement_inTheArray + inDref.target_FloatArray[i1] * inDref.flag_individual_value_copy_inTheArray;
          }
        }
        float* fValuesArray = &inDref.out_vecArrayFloatValues[0];

        // apply value/values to the dataref.
        if (inDref.flag_individual_value_copy_inTheArray) // v3.303.12 moved to its own logic since we use the new dref.offset_xxx parameters
        {
          XPLMSetDatavf(inDref.dataRefId, inDref.target_FloatArray, inDref.offset_array_defined_by_user, inDref.offset_how_many_variables_to_read_from_the_array);
        }
        else if (inDref.flag_designerPickedAllElement_inTheArray ) // v3.0.221.10 assign value to all array based on one value
        {
          XPLMSetDatavf(inDref.dataRefId, fValuesArray, 0, (int)inDref.out_vecArrayFloatValues.size());
        }
        else // assign value of specific array value
        {
          if ( inDref.arrayElementPickedTranslation <= vecSize)
          {
            XPLMSetDatavf(inDref.dataRefId, fValuesArray, inDref.arrayElementPickedTranslation, 1); // only 1
          }
          else
          {
#ifndef RELEASE
            Log::logMsgErr("\n[write to dref] dref array: " + inDref.getName() + ", is out of bound. Max array size is: " + Utils::formatNumber<int>(inDref.arraySize) + " starting from 0.." +
                           Utils::formatNumber<int>((inDref.arraySize - 1 <= 0) ? 0 : inDref.arraySize - 1) + "  . Skipping writing.");
#endif
          }
        }
      }
      break;
      default:
      {
        Log::logMsg("[dref param] Can't handle this Dataref Datatype!!! ");
      }
      break;
    } // end switch
  }   // end if
  else
  {
    if (inDref.arraySize > 0)
      Log::logDebugBO("\n[write to dref] dref: " + inDref.getName() + " is not valid. If it is an array, check its bounds. Max array size is: " + Utils::formatNumber<int>(inDref.arraySize) + " starting from 0.." +
                      Utils::formatNumber<int>((inDref.arraySize - 1 <= 0) ? 0 : inDref.arraySize - 1) + "  . Skipping...");
    else
      Log::logDebugBO("\n[write to dref] dref: " + inDref.getName() + " is not valid. Check its name. Skipping...");
  }
} // end set_dataref_value_into_xplane

// -----------------------------------

std::string
missionx::dataref_param::get_dataref_scalar_value_as_string()
{
  if (this->dataRefId) // if exists
  {

    switch (this->dataRefType)
    {
      case xplmType_Int:
      {
        return mxUtils::formatNumber<int>(this->getValue<int>());
      }
      break;
      case xplmType_Data: // v3.0.255.1
      {
        std::string       err;
        //const std::string val = this->getPropertyValue_asString(this->getName(), err);
        const std::string val = this->getParamStringValue();
        return val;
      }
      break;
      case xplmType_Float:
      {
        return mxUtils::formatNumber<float>(this->getValue<float>(), 8);
      }
      break;
      case (xplmType_Double): // v3.0.255.4.3 added to solve user dataref creation cases.
      case (xplmType_Float | xplmType_Double):
      {
        return mxUtils::formatNumber<double>(this->getValue<double>(), 8);
      }
      break;
      case xplmType_IntArray: // v3.303.13
      {
        size_t      counter = 1;
        std::string output_s;
        std::for_each(out_vecArrayIntValues.begin(),
                      out_vecArrayIntValues.end(),
                      [&](int n)
                      {
                        output_s += formatNumber<int>(n, 0) + ((counter == out_vecArrayIntValues.size()) ? "" : ",");
                        counter++;
                      });

        return output_s;
      }
      break;
      case xplmType_FloatArray: // v3.303.13
      {
        size_t      counter = 1;
        std::string output_s;
        std::for_each(out_vecArrayFloatValues.begin(),
                      out_vecArrayFloatValues.end(),
                      [&](float n)
                      {
                        output_s += formatNumber<float>(n, 8) + ((counter == out_vecArrayFloatValues.size()) ? "" : ",");
                        counter++;
                      });

        return output_s;
      }
      break;
      default:
        break;
    } // end switch
  }

  return "";
}

// -----------------------------------

std::string
missionx::dataref_param::get_dataref_array_values_as_string()
{
  return this->pStrArrayValue;
}


// -----------------------------------

std::string
missionx::dataref_param::to_string_ui_info()
{
  std::string format = this->key;


  if (this->dataRefId) // if exists
  {
    if (this->arraySize > 0)
    {
      this->readArrayDatarefIntoLocalArray();
      return format + ": " + this->get_dataref_array_values_as_string();
    }

    this->readDatarefValue_into_missionx_plugin();
    return format + ": " + this->get_dataref_scalar_value_as_string();
  }

  return format + ": n/a";
}

// -----------------------------------

std::string
missionx::dataref_param::to_string_ui_info_value_only()
{

  if (this->dataRefId) // if exists
  {
    if (this->arraySize > 0)
    {
      this->readArrayDatarefIntoLocalArray();
      return this->get_dataref_array_values_as_string();
    }

    this->readDatarefValue_into_missionx_plugin();
    return this->get_dataref_scalar_value_as_string();
  }

  return "n/a";
}


// -----------------------------------



/* Data of a type the current XPLM doesn't do.
xplmType_Unknown                         = 0

A single 4-byte integer, native endian.
,xplmType_Int                             = 1

A single 4-byte float, native endian.
,xplmType_Float                           = 2

A single 8-byte double, native endian.
,xplmType_Double                          = 4

An array of 4-byte floats, native endian.
,xplmType_FloatArray                      = 8

An array of 4-byte integers, native endian.
,xplmType_IntArray                        = 16

A variable block of data.
,xplmType_Data                            = 32
*/

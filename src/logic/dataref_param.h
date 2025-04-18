#ifndef MISSIONDATAREF_H_
#define MISSIONDATAREF_H_
#pragma once

/**************

ToDo:

**************/

#include "XPLMDataAccess.h"
#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
#include <deque>
#include <fmt/core.h>

#include "../core/Utils.h"
#include "../io/Log.hpp"
#include "MxParam.hpp"
#include "../core/Timer.hpp"

// using namespace std;
using namespace missionx;
// using namespace mxconst;


namespace missionx
{
#include "../core/xx_mission_constants.hpp"


class dataref_param : public MxParam
{
private:
  void init();
  //bool initDataRefInfo_array(void* inArray, size_t inArraySize);


typedef struct _dref_interpolation
  {

    bool   flagIsValid{ true };
    int    seconds_to_run_i{ 0 };
    int    for_how_many_cycles_i{ 0 };
    int    currentCycleCounter_i{ 0 }; // cycle counter    


    double             target_value_d{ 0.0 };
    double             last_value_d{ 0.0 };
    double             starting_scalar_value_d{ 0.0 };
    double             delta_scalar{ 0.0 };

    std::string sTargetValues; // v3.305.3
    std::string sStartValues; // v3.305.3
    std::string sDelta; // v3.305.3

#ifndef RELEASE
    std::vector<float>      vec_targetValues_f{};
    std::vector<float>      vecStartingValues_f{};
    std::deque<std::string> dq_cycle_values_as_string{}; // will hold the values for each delta
#endif // !RELEASE

    
    std::vector<float> vecLastInterValues_f{};
    std::vector<float> delta_array{}; // will hold the delta for each array field

    std::string arrayValuesAsString;

    missionx::Timer timerToRun;

    [[nodiscard]] std::string getDelta(const XPLMDataTypeID inDataRefType) const {
      return sDelta;
    }

  } mx_interpolation_strct;


public:
  dataref_param();
  explicit dataref_param (const std::string &inKey);
  //virtual ~dataref_param(void);

  //bool           flag_isInterpolatedDref{ false };
  std::string    key;
  XPLMDataRef    dataRefId;
  XPLMDataTypeID dataRefType;

  mx_interpolation_strct strctInterData; // v3.305.x


  // core member
  bool setDataArray(void* inArray, size_t inArraySize, size_t inOffset, size_t inCount); // copy the values from inArray into dref outXXXArray 
  static int  getDrefArraySize(const XPLMDataRef& inDataRefId, const XPLMDataTypeID& inDataRefType);
  bool initDataRefInfo();
  bool parse_node();           // v3.0.241.1
  void prepareCueMetaData(){} // v3.0.241.1 added so read_mission_file::load_saved_elements() template will compile
  bool flc_interpolation(bool & outDrefWasChanged); // v3.305.3 added out bool to flag if dref was changed to display the timing for debug // v3.303.13 returns "true" if done, or false if still needs to run

  // Read and assign dataref value, automatic resolving data type.  //You should call getValue() after this function.
  void readDatarefValue_into_missionx_plugin();
  void readArrayDatarefIntoLocalArray(); // v3.303.12

  // ARRAYS
  /*  evaluate against only 1 element */
  std::string arrayKey; // v3.0.223.2 will hold the name of the dataref without the brakets.

  bool flag_designerPickedOneElement;                    // v3.0.221.10 renamed from isElementPickedInArray // means that designer is only interested in 1 element value from the array
  /*  evaluate against ALL elements */                   /*  evaluate against only 1 element */
  bool         flag_designerPickedAllElement_inTheArray; // v3.0.221.10 renamed from isElementNotPickedInArray // means the designer wants to test against ALL values in the array
  bool         flag_individual_value_copy_inTheArray;    // v3.0.303 copy individual value and not same value for all
  int          arraySize{ -1 };                          // size of array
  int          offset_array_defined_by_user{ 0 };                     // v3.303.12 new implementation for user array. 0 = no offset. Using the format of key=value1,value2,value3.... If we have key[5] the the "5" is the offset. Should be 5-1 since we start counting from 0.
  int          offset_how_many_variables_to_read_from_the_array{ 0 }; // v3.303.12 new implementation for user array. how many values in the arrays from the start of the offset. Only valid if its value is larger than zero.
  unsigned int arrayElementPicked;                       // which part of the array to fetch
  unsigned int arrayElementPickedTranslation;            // Translate array number to C++ array. (usually: "arrayElementPicked-1")



  // v3.303.12
  int*               target_IntArray{ nullptr };   // Will hold write to x-plane dataref
  float*             target_FloatArray{ nullptr }; // Will hold write to x-plane dataref
  char*              target_CharArray{ nullptr };  // Will hold write to x-plane dataref

  // v3.303.12
  std::vector<float> out_vecArrayFloatValues; // will hold the values from the dataref array for float.
  std::vector<int>   out_vecArrayIntValues;   // will hold the values from the dataref array for integer.

  // members
  void           setAndInitializeKey(std::string inKey);
  XPLMDataRef    getDataRefID();
  XPLMDataTypeID getDataRefType();

  bool evaluateArray(); // check designer defined correct array settings and set correct parameters
  void resetArrays(); // deprecated in v3.303.13 - not in use, yet  // delete the array due to re-construct array parameter. Should be called when plane is changed or "stop/disable plugin" or "reset mission"
  void fill_IntArrayIntoVector(int* inOut_Array, std::vector<int>& outVecToFill, std::string& outStrArrayValue, const bool& inFlagManuallyPreparedVector = false);
  void fill_FloatArrayIntoVector(float* inOut_Array, std::vector<float>& outVecToFill, std::string& outStrArrayValue, const bool& inFlagManuallyPreparedTheOutVector_notTheTargetVector = false);

  // v3.303.13 interpolate

  void initInterpolation(const unsigned int inInterpolateEvery_n_Seconds, const unsigned int inCycles, const std::string& inTargetValue);   // 0 = no run
  bool initScalarInterpolation(const unsigned int inSeconds, const unsigned int inCycles, const double inTargetValue);                      // 0 = no run
  void initArrayInterpolation(const unsigned int inSeconds, const unsigned int inCycles, const std::string& inTargetValuesAsString);        // 0 = no run
  void debugArrayInterpolationValuesAfterInitialization(const std::string & inTargetValuesAsString);


  // checkpoint
  void storeCoreAttribAsProperties();
  void applyPropertiesToCurrentDrefClass();

  void saveCheckpoint(IXMLNode& inParent);
  void saveInterpolationCheckpoint(IXMLNode& inDrefNode); // v3.305.3
  bool initInterpolationDataFromSaveFile(); // v3.305.3

  // There are two ways to set custom values into dataref before sending to x-plane:
  // Option 1: use "data_manager::apply_dataref_based_on_key_value_strings()", it will basically call the template "setTarget" for array datatype and then it will call "set_dataref_value_into_xplane()" function.
  // Option 2: Manually fill in the "out_vecArrayFloatValues" or "out_vecArrayIntValues" and then call "set_dataref_value_into_xplane()" with the flag: "inFlagManuallyPreparedTheOutVector_notTheTargetVector=true", where the values in the vector will be copied over the array.
  static void set_dataref_values_into_xplane(dataref_param& inDref, const bool & inFlagManuallyPreparedVector = false);

  std::string get_dataref_scalar_value_as_string(); // v3.303.11 will only work for int/float/double values, won't work for arrays
  std::string get_dataref_array_values_as_string(); // v3.305.2

  std::string to_string_ui_info();
  std::string to_string_ui_info_value_only(); // v3.305.3

  //// v3.303.12 - not yet implemented or used
  //template<typename T>
  //int readDatarefValuesIntoThePlugin_tmplt(const XPLMDataRef& dataRefId, const XPLMDataTypeID& dataRefType, T * outArray, std::vector<T> & outVector, std::string & outArrayAsString )
  //{
  //  int numOfValues = 0;
  //  this->pStrArrayValue.clear();
  //
  //  switch (dataRefType)
  //  {
  //    case xplmType_Int:
  //    {
  //      this->setValue((double)XPLMGetDatai(dataRefId));        
  //    }
  //    break;
  //    case xplmType_Float:
  //    {
  //      this->setValue((double)XPLMGetDataf(dataRefId));
  //    }      
  //    break;
  //    case (xplmType_Double):
  //    case (xplmType_Float | xplmType_Double):
  //    {
  //      this->setValue(XPLMGetDatad(dataRefId));
  //    }
  //    break;
  //    case (xplmType_FloatArray):
  //    {
  //      if (this->flag_paramReadyToBeUsed )
  //      {
  //        XPLMGetDatavf(dataRefId, outArray, 0, this->arraySize); // fill local float array
  //        this->fill_FloatArrayIntoVector(outArray, outVector, outArrayAsString );
  //      }
  //    }
  //    break;
  //    case (xplmType_IntArray):
  //    {
  //      if (this->flag_paramReadyToBeUsed )
  //      {
  //        XPLMGetDatavi(dataRefId, outArray, 0, this->arraySize); // fill local int Array
  //
  //      }
  //    }
  //    break;
  //    case (xplmType_Data): 
  //    {
  //      if (flag_paramReadyToBeUsed)
  //      {
  //        XPLMGetDatab(this->dataRefId, outArray, 0, this->arraySize);
  //        this->setStringProperty(this->getName(), std::string(target_CharArray));
  //      }
  //    }
  //    break;
  //  }
  //
  //  return numOfValues;
  //}

  // setTargetArray "always" fills the array from the first cell.
  // later, when calling XPLMSetDatavi or XPLMSetDatavf we provide the offset for the target array (xplane) and how many variables to read from the source array: "target_IntArray" or "target_FloatArray"
  // setTargetArray template needs <[xplmType_IntArray|xplmType_FloatArray], [float|int]>
  // <T> dataref type, <N> C datatype to split the vector to (int or float)
  template<int T, typename N>
  int setTargetArray(const int inArraySize, std::string& inValuesAsStringDelimeted, const bool &inApplyToDataref, std::string inDelimiter = ",")
  {
    if (!(this->dataRefType == T))
    {
      Log::logMsgErr("Dataref: " + this->key + ", is not a valid Array... skipping");
      return false;
    }

    assert( (!(this->target_IntArray == nullptr && this->target_FloatArray == nullptr)) && std::string(__func__).append(", One of the array pointers must be initialized.").c_str() );

    if ((xplmType_FloatArray == T && this->target_FloatArray == nullptr) || (xplmType_IntArray == T && this->target_IntArray == nullptr))
    {
      Log::logMsgErr("Dataref: " + this->key + ", is not a valid Array, could not initialize its array object... skipping");
      return false;
    }

    this->offset_how_many_variables_to_read_from_the_array = 0;
    if (this->flag_paramReadyToBeUsed && this->arraySize > 0 && this->arraySize >= inArraySize)
    {
      std::vector<std::string> vecValues_s = mxUtils::split(inValuesAsStringDelimeted, ((inDelimiter.empty()) ? ',' : inDelimiter.front()), true);
      std::string              delim_s     = "" + inDelimiter;
      auto vecValues = Utils::splitStringToNumbers<N>(inValuesAsStringDelimeted, delim_s );
      int  x         = 0;   // We fill the source array from the first cell
      for (const auto& v : vecValues)
      {
        if (x < this->arraySize) // "x" always have to be smaller than arraySize since we start counting from 0 (zero)
        {
          switch (T)
          {
            case xplmType_FloatArray:
              this->target_FloatArray[x] = (float)v;
              ++this->offset_how_many_variables_to_read_from_the_array;
              break;
            case xplmType_IntArray:
              this->target_IntArray[x] = (int)v;
              ++this->offset_how_many_variables_to_read_from_the_array;
              break;
            default:
            {
              this->flag_paramReadyToBeUsed = false;
              Log::logMsgErr("Wrong type of array for dataref: " + this->key + "... skipping");
              return this->offset_how_many_variables_to_read_from_the_array;
            } // end default case          
          } // end switch
        }
        else
          break; // exit the for loop

        ++x;
      }
      //fill_IntArrayIntoVector(); // internal management

      // set intention of how should we write the dataref values into X-Plane once the "set_dataref_value_into_xplane" will be called.
      this->flag_individual_value_copy_inTheArray    = true;
      this->flag_designerPickedOneElement            = false;
      this->flag_designerPickedAllElement_inTheArray = false;
    }
    //else
    //  return this->offset_how_many_variables_to_read_from_the_array;

    if (inApplyToDataref)
      missionx::dataref_param::set_dataref_values_into_xplane((*this), false); // False = pick data from "target array" and not the "out vector"

    return this->offset_how_many_variables_to_read_from_the_array;
  } // end setTargetArray template


  template<typename T>
  void init_array_interpolation_operation(dataref_param::mx_interpolation_strct& inOut_inter_data, std::vector<float>& inVecCurrentValues, const std::string& in_drefTargetValuesAsString, const unsigned int inCycles)
  {
    std::vector<float> inVecTargetValues = Utils::splitStringToNumbers<float>(in_drefTargetValuesAsString, ","); // in release build, the target values vector is created from the original values string and all numbers are treated as floats

  
    auto iterTargetValues    = inVecTargetValues.cbegin();
    auto iterEndTargetValues = inVecTargetValues.cend();

    auto iterCurrentValues     = inVecCurrentValues.cbegin();
    auto iterEndCurrentValues = inVecCurrentValues.cend();
    
    while (iterTargetValues != iterEndTargetValues && iterCurrentValues != iterEndCurrentValues)
    {
      const float delta_d = ((*iterTargetValues) - (*iterCurrentValues)) / (inCycles);

      inOut_inter_data.delta_array.emplace_back((float)delta_d); // calculate the delta for each cycle

      // progress iterators
      ++iterTargetValues;
      ++iterCurrentValues;
    } // end while
  
  } // end template



  template<typename T>
  void progress_array_interpolation_values(dataref_param::mx_interpolation_strct& inOut_inter_data, std::vector<float>& inOut_VecCurrentValues/*, std::vector<T>& inVecTargetValues*/)
  {
    auto itDeltaArr    = inOut_inter_data.delta_array.cbegin(); // the delta values won't change
    auto itDeltaArrEnd = inOut_inter_data.delta_array.cend();   // the delta values won't change
  
    auto iterCurrentValues    = inOut_VecCurrentValues.begin();
    auto iterEndCurrentValues = inOut_VecCurrentValues.end();

  
    while (itDeltaArr != itDeltaArrEnd && iterCurrentValues != iterEndCurrentValues /*&& itTargetVal != itTargetValEnd*/)
    {

      if (std::is_integral_v<T>)      
        (*iterCurrentValues) += (*itDeltaArr); // scalar like char, int long      
      else
        (*iterCurrentValues) += (T)(*itDeltaArr); // add the delta

      // progress iterators
      ++itDeltaArr;
      ++iterCurrentValues;
    } // end while
  
    // store values as string
#ifndef RELEASE
    const auto vecSize = inOut_VecCurrentValues.size();

#endif // !RELEASE

    size_t     counter = (size_t)1;
    inOut_inter_data.arrayValuesAsString.clear();

    for (const auto v : inOut_VecCurrentValues)
    {
      if (std::is_integral_v<T>)
        inOut_inter_data.arrayValuesAsString += (counter < inOut_VecCurrentValues.size()) ? mxUtils::formatNumber<float>(std::round(v)) + "," : mxUtils::formatNumber<float>(std::round(v));
      else
        inOut_inter_data.arrayValuesAsString += (counter < inOut_VecCurrentValues.size()) ? mxUtils::formatNumber<float>(v) + "," : mxUtils::formatNumber<float>(v);      

      ++counter;
    }

  } // set_array_interpolation_values



  // XPLMDataTypeID = int value, casting of inValue will be defined by the type sent
  //  bool  setValue ( XPLMDataTypeID inType, V inValue);


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
};


}


#endif /* MISSIONDATAREF_H_ */

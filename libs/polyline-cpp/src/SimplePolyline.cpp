// Copyright 2016, Josh Baker.
// All rights reserved.
//
// Author: joshuabaker2@gmail.com (Josh Baker)
//
// The SimplePolyline Class

#include "SimplePolyline.h"

namespace missionx
{


  SimplePolyline::SimplePolyline(int precision) {
    _precision = precision;
    _factor = power(10, _precision);
  }

  SimplePolyline::SimplePolyline() { }

  /**
   * Encodes the given latitude, longitude coordinates.
   *
   * @param {float[2]} coordinates
   *  coordinates[0] = latitude
   *  coordinates[1] = longitude
   * @param {char*} output_str - The output c-string that is passed back by reference.
   */
  void SimplePolyline::encode(float coordinates[][2], int num_coords, char* output_str) {
    encodeSingleCoord(coordinates[0][0], output_str);
    encodeSingleCoord(coordinates[0][1], output_str);

    for (int i = 1; i < num_coords; i++) {
      encodeSingleCoord(coordinates[i][0] - coordinates[i - 1][0], output_str);
      encodeSingleCoord(coordinates[i][1] - coordinates[i - 1][1], output_str);
    }
  }


  /**
   * Decodes to a [[latitude, longitude]] coordinates array.
   *
   * This is adapted from the implementation in Project-OSRM.
   *
   * @param {String} str
   * @param {[float]} decodedCoords - return coordinates through here (passed by reference)
   * @returns {int} - The number of coordinates that were decoded.
   *
   * @see https://github.com/Project-OSRM/osrm-frontend/blob/master/WebContent/routing/OSRM.RoutingGeometry.js
   */
   //int SimplePolyline::decode(const char *str, float decodedCoords[][2]) {
   //  int32_t index = 0, shift = 0, result = 0;
   //  float latlng[2] = {0, 0};
   //  char byte;
   //  int i;
   //
   //  for (i = 0; index + 1 < strlen((char*) str); i++) {
   //    for (int j = 0; j < 2; j++) {
   //      shift = result = 0;
   //      do {
   //        byte = static_cast<char>(str[index++] - 63);
   //        result |= (int32_t) (byte & 0x1f) << (int32_t) shift;
   //        shift += 5;
   //      } while (byte >= 0x20);  // 0x20 is blank, numbers below are garbage
   //      latlng[j] += (((result & 1) != 0 ? ~(result >> 1) : (result >> 1))) / _factor;
   //      decodedCoords[i][j] = latlng[j];
   //    }
   //  }
   //  return i;
   //}
//
  //int SimplePolyline::decode(const char* str, std::list<mxVec2f>& decodedCoords)
  //{
  //  int32_t index = 0, shift = 0, result = 0;
  //  float latlng[2] = { 0.0f, 0.0f };
  //  char byte;
  //  int i;
//
  //  for (i = 0; index + 1 < strlen((char*)str); i++) {
  //    for (int j = 0; j < 2; j++) {
  //      shift = result = 0;
  //      do {
  //        byte = static_cast<char>(str[index++] - 63);
  //        result |= (int32_t)(byte & 0x1f) << (int32_t)shift;
  //        shift += 5;
  //      } while (byte >= 0x20);  // 0x20 is blank, numbers below are garbage
  //      latlng[j] += (((result & 1) != 0 ? ~(result >> 1) : (result >> 1))) / _factor;
  //    }
  //
  //    // store in simple struct
  //    mxVec2f vec2f{ latlng[0], latlng[1] };
  //    decodedCoords.push_back(vec2f);
  //  }
  //  return i;
  //}

  int SimplePolyline::decode(missionx::mx_ext_internet_fpln_strct& outFpln)
  {
    int32_t index = 0, shift = 0, result = 0;
    float latlng[2] = { 0.0f, 0.0f };
    char byte;
    int i;

    for (i = 0; index + 1 < (int32_t)outFpln.encode_polyline_s.length(); i++) {
      for (int j = 0; j < 2; j++) {
        shift = result = 0;
        do {
          byte = static_cast<char>(outFpln.encode_polyline_s[index++] - 63);
          result |= (int32_t)(byte & 0x1f) << (int32_t)shift;
          shift += 5;
        } while (byte >= 0x20);  // 0x20 is blank, numbers below are garbage
        latlng[j] += (((result & 1) != 0 ? ~(result >> 1) : (result >> 1))) / _factor;
      }

      // store in simple struct
      mxVec2f vec2f{ latlng[0], latlng[1] };
      outFpln.listNavPoints.push_back(vec2f);
      outFpln.formated_nav_points_s += ((i < (int)outFpln.encode_polyline_s.length()-1 ) ? "\n" : "") + missionx::mxUtils::formatNumber<float>(vec2f.lat, 8) + "," + missionx::mxUtils::formatNumber<float>(vec2f.lon, 8); // format lat/long and do we add space or not at the end
      
    }
    return i;
  }

  /**
   * Based off https://developers.google.com/maps/documentation/utilities/polylinealgorithm
   */
  void SimplePolyline::encodeSingleCoord(float coordinate, char* output_str) {
    int32_t intCoord = (int32_t)(coordinate * _factor);
    intCoord <<= (int32_t)1;

    if (intCoord < 0) {
      intCoord = ~intCoord;
    }

    int index;
    for (index = 0; output_str[index]; index++) {}

    while (intCoord >= 0x20) {
      output_str[index++] = static_cast<char> (static_cast<char> (0x20 | (int32_t)(intCoord & 0x1f)) + 63);
      intCoord >>= (int32_t)5;
    }
    output_str[index] = static_cast<char> (intCoord + 63);
  }

} // end namespace missionx



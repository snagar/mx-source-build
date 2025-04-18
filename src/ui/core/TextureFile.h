#ifndef TEXTUREFILE_H
#define TEXTUREFILE_H


/**************


**************/


#include "../../core/MxUtils.h"
#include "../../io/Log.hpp"

namespace missionx
{


class mxTextureFile
{
private:
public:
  typedef struct _mxIMAGEDATA
  {
    unsigned char* pData;
    int            Width;
    int            Height;
    int            Padding;
    short          Channels;

    _mxIMAGEDATA() //-V730
    {
      init();
    }

    void init()
    {
      Width    = 0;
      Height   = 0;
      Padding  = 0;
      Channels = 0;
    }

    int getW_i() { return Width; }

    float getW_f() { return (float)Width; }

    int   getH_i() { return Height; }
    float getH_f() { return (float)Height; }

  } IMAGEDATA;



  IMAGEDATA sImageData;

  std::string   fileName;
  std::string   filePath;
  int           TextureId;
  XPLMTextureID gTexture;


  mxTextureFile() { init(); }

  void init()
  {
    sImageData.init(); // init struct

    fileName.clear();
    filePath.clear();
    TextureId = 0;
    gTexture  = 0;
  }

  std::string getAbsoluteFileLocation() { return filePath + XPLMGetDirectorySeparator() + fileName; }

  void setTextureFile(std::string inFileName, std::string inFilePath)
  {
    this->fileName = inFileName;
    this->filePath = inFilePath;
  }

  int getWidth() { return sImageData.getW_i(); }
  int getHeight() { return sImageData.getH_i(); }
};

}



#endif // TEXTUREFILE_H

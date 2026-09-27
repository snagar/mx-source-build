#ifndef TEXTUREFILE_H
#define TEXTUREFILE_H


/**************


**************/


#include "../../core/MxUtils.h"
#include "../../io/Log.hpp"
#include <XPLMGraphics.h>

namespace missionx
{

class mxTextureFile
{
public:
  struct IMAGEDATA
  {
    unsigned char* pData{nullptr};
    int            Width;
    int            Height;
    int            Padding;
    short          Channels;

    IMAGEDATA() //-V730
    {
      init();
    }

    void init()
    {
      pData    = nullptr;
      Width    = 0;
      Height   = 0;
      Padding  = 0;
      Channels = 0;
    }

    [[nodiscard]] int   getW_i () const { return Width; }
    [[nodiscard]] int   getH_i () const { return Height; }
    [[nodiscard]] float getW_f () const { return static_cast<float> (Width); }
    [[nodiscard]] float getH_f () const { return static_cast<float> (Height); }

  };

  XPLMTextureID gTexture;

  // v25.08.1
  size_t      texture_hash_simple;

  std::string fileName;
  std::string filePath;

  IMAGEDATA sImageData;


  mxTextureFile()
  {
    gTexture  = 0;
    texture_hash_simple = 0;
    //texture_hash_sha256.clear();
    fileName.clear();
    filePath.clear();

    init();
  }

  void init()
  {
    sImageData.init(); // init struct

    fileName.clear();
    filePath.clear();
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


  void store_hash()
  {
    this->texture_hash_simple = this->getTextureHash ();
    //this->texture_hash_sha256 = this->getTextureSHA256 ();
  }

private:
  [[nodiscard]] std::size_t getTextureHash() const {
    if (!sImageData.pData || sImageData.Width <= 0 || sImageData.Height <= 0 || sImageData.Channels <= 0) {
      return 0; // invalid / empty data
    }

    std::hash<unsigned char> byte_hash;
    std::size_t h = 0;

    // Total number of bytes = Width * Height * Channels (+ optional padding)
    std::size_t dataSize = static_cast<std::size_t>(sImageData.Width)
                         * static_cast<std::size_t>(sImageData.Height)
                         * static_cast<std::size_t>(sImageData.Channels);

    for (std::size_t i = 0; i < dataSize; ++i) {
      h ^= byte_hash(sImageData.pData[i]) + 0x9e3779b9 + (h << 6) + (h >> 2); // boost::hash_combine trick
    }
    return h;
  }

};

}



#endif // TEXTUREFILE_H

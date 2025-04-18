#include "BitmapReader.h"
#include <filesystem>
namespace fs = std::filesystem;

/**************
**************/

missionx::BitmapReader::BitmapReader()
{

  //RED = NULL, GREEN = NULL, BLUE = NULL;

  //RED            = XPLMFindDataRef("sim/graphics/misc/cockpit_light_level_r");
  //GREEN          = XPLMFindDataRef("sim/graphics/misc/cockpit_light_level_g");
  //BLUE           = XPLMFindDataRef("sim/graphics/misc/cockpit_light_level_b");
  //COCKPIT_LIGHTS = XPLMFindDataRef("sim/cockpit/electrical/cockpit_lights");
  //LIGHTS_ON      = XPLMFindDataRef("sim/cockpit/electrical/cockpit_lights_on");

  //draw_phase    = -1;
  //current_phase = draw_phase;
}



bool
missionx::BitmapReader::loadGLTexture(mxTextureFile& inTextureFile, std::string &outErr, bool flipImage_b, bool is_sync_b)
{
  // int Status=FALSE;
  bool bTextureLoad = false;
  const fs::path    texturePath     = inTextureFile.getAbsoluteFileLocation();

  if (fs::is_regular_file(texturePath))
  {
    // const std::string TextureFileName = texturePath.string(); // v24.06.1 deprecated, used "texturePath" instead

    // STB Load Image
    if (loadImageStb(texturePath.string(), &inTextureFile.sImageData, flipImage_b, outErr))
    {
      // Status=TRUE;
      bTextureLoad = true;

       // Flip the Image. v3.0.253.8 we do not flip image on Y axes manually, we will use "stb" library own flag to do this for us
       #ifdef FLIP_IMAGE
          /*  create a copy of the image data  */
      
          unsigned char* img;
          img = (unsigned char*)malloc(inTextureFile.sImageData.Width*inTextureFile.sImageData.Height*inTextureFile.sImageData.Channels );
          memcpy( img, inTextureFile.sImageData.pData, inTextureFile.sImageData.Width*inTextureFile.sImageData.Height*inTextureFile.sImageData.Channels );
      
      
          //if (flipImage_b) // Nuklear might have its own code to flip image.
          if (true) // Nuklear might have its own code to flip image.
          {
            int i, j;
            for (j = 0; j * 2 < inTextureFile.sImageData.Height; ++j)
            {
              int index1 = j * (int)inTextureFile.sImageData.Width * inTextureFile.sImageData.Channels;
              int index2 = ((int)inTextureFile.sImageData.Height - 1 - j) * (int)inTextureFile.sImageData.Width * (int)inTextureFile.sImageData.Channels;
              for (i = (int)inTextureFile.sImageData.Width * (int)inTextureFile.sImageData.Channels; i > 0; --i)
              {
                static unsigned char temp;
                temp = img[index1];
                img[index1] = img[index2];
                img[index2] = temp;
                ++index1;
                ++index2;
              }
            }
          }
       #endif

      if (is_sync_b)
      {
        XPLMGenerateTextureNumbers(&inTextureFile.gTexture, 1);
        XPLMBindTexture2d(inTextureFile.gTexture, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // removed v3.0.251.1
        // glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // removed v3.0.251.1
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0); // added from imgui
                                                // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLint)inTextureFile.sImageData.Width, (GLint)inTextureFile.sImageData.Height, 0, ((inTextureFile.sImageData.Channels < 4) ? GL_RGB : GL_RGBA), GL_UNSIGNED_BYTE, img);
#ifdef FLIP_IMAGE
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLint)inTextureFile.sImageData.Width, (GLint)inTextureFile.sImageData.Height, 0, ((inTextureFile.sImageData.Channels < 4) ? GL_RGB : GL_RGBA), GL_UNSIGNED_BYTE, img);
#else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLint)inTextureFile.sImageData.Width, (GLint)inTextureFile.sImageData.Height, 0, ((inTextureFile.sImageData.Channels < 4) ? GL_RGB : GL_RGBA), GL_UNSIGNED_BYTE, inTextureFile.sImageData.pData);
#endif


        stbi_image_free(inTextureFile.sImageData.pData);
#ifdef FLIP_IMAGE
        stbi_image_free(img);
#endif

        inTextureFile.sImageData.pData = nullptr;
      }
    } // end if loadImageStb
  } // end if fs::path is valid
  // end if

  return bTextureLoad;
}
// end loadGLTexture



bool
missionx::BitmapReader::loadImageStb(std::string fileName, mxTextureFile::IMAGEDATA* ImageData, bool inFlipImage_b, std::string &outErr)
{
  int x, y, channels;

  // std::string outErr;
  outErr.clear();


  if (inFlipImage_b)
    stbi_set_flip_vertically_on_load(true);
  else
    stbi_set_flip_vertically_on_load(false);

  ImageData->pData = stbi_load(fileName.c_str(), &x, &y, &channels, 0, &outErr); // v3.0.243.1 newer version + compatibility with imgui3xp
  if (!outErr.empty())
    Log::logMsgThread(outErr);

  // convert to xplane struct
  if (ImageData->pData)
  {

    ImageData->Width    = x;
    ImageData->Height   = y;
    ImageData->Channels = (short)channels;

    return true;
  }

  return false;
}

//std::vector<uint8_t>
//missionx::BitmapReader::readFile(const char* path, std::string& errMsg) // -> std::vector<uint8_t>
//{
//  errMsg.clear();
//
//  std::ifstream file(path, std::ios::binary | std::ios::ate);
//  if (!file.is_open())
//  {
//    errMsg     = "Failed to open file " + std::string(path);
//    auto bytes = std::vector<uint8_t>(0);
//    return bytes;
//  }
//
//  auto size = file.tellg();
//  file.seekg(0, std::ios::beg);
//  auto bytes = std::vector<uint8_t>(size);
//  file.read(reinterpret_cast<char*>(&bytes[0]), size);
//  file.close();
//  return bytes;
//}

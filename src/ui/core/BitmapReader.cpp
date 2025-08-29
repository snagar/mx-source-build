#include "BitmapReader.h"
#include <filesystem>

#include <GL/gl.h>
#include <GL/glew.h>

namespace fs = std::filesystem;

/**************
**************/

missionx::BitmapReader::BitmapReader () { }


bool
missionx::BitmapReader::loadGLTexture(mxTextureFile& inTextureFile, std::string &outErr, bool flipImage_b, bool is_sync_b)
{
  // int Status=FALSE;
  bool bTextureLoad = false;

  if (const fs::path texturePath     = inTextureFile.getAbsoluteFileLocation()
    ; fs::is_regular_file(texturePath))
  {
    // STB Load Image
    if (loadImageStb(texturePath.string(), &inTextureFile.sImageData, flipImage_b, outErr))
    {
      // Status=TRUE;
      bTextureLoad = true;

      if (is_sync_b)
      {

        // ==> Start Old and working Code
        // v25.08.1 store hash. Caching tests should be done before generating GL texture information.
        inTextureFile.store_hash ();


        // Generate texture ID and bind it
        XPLMGenerateTextureNumbers (&inTextureFile.gTexture, 1);
        XPLMBindTexture2d (inTextureFile.gTexture, 0);

        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        //glPixelStorei (GL_UNPACK_ROW_LENGTH, 0); // from imgui

        // Upload image data using a sized internal format
        const GLenum format         = (inTextureFile.sImageData.Channels < 4) ? GL_RGB : GL_RGBA;
        const GLenum internalFormat = (inTextureFile.sImageData.Channels < 4) ? GL_RGB8 : GL_RGBA8;
        glTexImage2D (GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat), inTextureFile.sImageData.Width, inTextureFile.sImageData.Height, 0, format, GL_UNSIGNED_BYTE, inTextureFile.sImageData.pData);

        // Free the CPU-side image data
        stbi_image_free (inTextureFile.sImageData.pData);

        inTextureFile.sImageData.pData = nullptr;

        // <<===== End Old and working Code

        

        // // v25.08.1 store hash. Caching tests should be done before generating GL texture information.
        // inTextureFile.store_hash ();
        //
        // // Check if dimensions are a power of two
        // const bool isPowerOfTwo = ((inTextureFile.sImageData.Width & (inTextureFile.sImageData.Width - 1)) == 0) && ((inTextureFile.sImageData.Height & (inTextureFile.sImageData.Height - 1)) == 0);
        //
        // // Generate texture ID and bind it
        // XPLMGenerateTextureNumbers(&inTextureFile.gTexture, 1);
        // XPLMBindTexture2d(inTextureFile.gTexture, 0);
        //
        // // Set texture parameters
        // if (isPowerOfTwo)
        // {
        //   glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        //   // Setting max level to 2 as per your previous request
        //   glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 2); // Set max mipmap level  64x64, 32x32 and 16x16, no smaller
        // }
        // else
        // {
        //   glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        // }
        //
        // // These are common parameters that can be set regardless of mipmapping
        // glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        // glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        //
        //
        // // Upload image data using a sized internal format
        // const GLenum format         = (inTextureFile.sImageData.Channels < 4) ? GL_RGB : GL_RGBA;
        // const GLenum internalFormat = (inTextureFile.sImageData.Channels < 4) ? GL_RGB8 : GL_RGBA8;
        // glTexImage2D (GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat), inTextureFile.sImageData.Width, inTextureFile.sImageData.Height, 0, format, GL_UNSIGNED_BYTE, inTextureFile.sImageData.pData);
        //
        // // Only generate mipmaps if the dimensions are a power of two
        // if (isPowerOfTwo)
        // {
        //   // The glGenerateMipmap call should now work directly
        //   glGenerateMipmap (GL_TEXTURE_2D);  // <== CTD X-Plane. Crashes X-Plane.
        // }
        //
        // // Free the CPU-side image data
        // stbi_image_free (inTextureFile.sImageData.pData);
        //
        // inTextureFile.sImageData.pData = nullptr;
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

  outErr.clear();


  //if (inFlipImage_b)
  //  stbi_set_flip_vertically_on_load(true);
  //else
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


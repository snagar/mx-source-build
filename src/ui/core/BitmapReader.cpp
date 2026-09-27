#include "BitmapReader.h"
#include <filesystem>

#include "mx_img_window.h"

//namespace fs = std::filesystem;

/**************
**************/

missionx::BitmapReader::BitmapReader () { }


bool BitmapReader::load_texture_no_bind(mxTextureFile& inTextureFile, std::string& outErr, const bool flipImage_b)
{
  bool bTextureLoad = false;

  #ifndef RELEASE
  Log::logMsgThread(fmt::format( "[{}] Loading texture image: {}", __func__, inTextureFile.getAbsoluteFileLocation() ), format_type::none_cr); // don't add "\n"
  #endif

  const std::filesystem::path texturePath = inTextureFile.getAbsoluteFileLocation();
  if (std::filesystem::is_regular_file(texturePath))
  {
    // STB Load Image
    if (loadImageStb(texturePath.string(), &inTextureFile.sImageData, flipImage_b, outErr))
    {
      // Successfully loaded
      bTextureLoad = true;

      inTextureFile.store_hash();
    }
  }

  #ifndef RELEASE
  Log::logMsgThread(" - loaded.\n");
  #endif

  return bTextureLoad;
}


bool
missionx::BitmapReader::load_textute_and_bind(mxTextureFile& inTextureFile, std::string &outErr, bool flipImage_b, bool is_bind_texture_thread_safe)
{
  // int Status=FALSE;
  bool bTextureLoad = false;

  if (const std::filesystem::path texturePath = inTextureFile.getAbsoluteFileLocation(); std::filesystem::is_regular_file(texturePath))
  {
    // STB Load Image
    if (loadImageStb(texturePath.string(), &inTextureFile.sImageData, flipImage_b, outErr))
    {
      // Status=TRUE;
      bTextureLoad = true;

      // v25.08.1 store hash. Caching tests should be done before generating GL texture information.
      inTextureFile.store_hash();

      if (is_bind_texture_thread_safe)
      {
        bind_texture(inTextureFile);
      } // end if synch
    } // end if loadImageStb
  } // end if fs::path is valid
  // end if

  return bTextureLoad;
}
// end load_textute_and_bind



bool
missionx::BitmapReader::loadImageStb(std::string fileName, mxTextureFile::IMAGEDATA* ImageData, bool inFlipImage_b, std::string &outErr)
{
  int x, y, channels;

  outErr.clear();

  stbi_set_flip_vertically_on_load(false);
  #ifdef IMGWINDOW_USE_PANEL_GRAPHICS
  const int desired_channels = (ImgPanelGraphics::IsAvailable())? 4 : 0;
  #else
  constexpr int desired_channels = 0;
  #endif


  ImageData->pData = stbi_load(fileName.c_str(), &x, &y, &channels, desired_channels, &outErr); // v3.0.243.1 newer version + compatibility with imgui4xp
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



int BitmapReader::bind_texture(mxTextureFile& inTextureFile)
{
  if (!inTextureFile.sImageData.pData)
  {
    Log::logDebugBO(fmt::format("[{}] Cannot bind texture: {}. Texture data is empty.\n", __func__, inTextureFile.getAbsoluteFileLocation()), false, true);
    return 0;
  }

  const auto win = mx_img_window_weak_ptr.lock();
  // Log::logDebugBO(fmt::format("[{}] mx_inv_window_weak_ptr: .", __func__), false, true);
  if (!win)
  {
    Log::logDebugBO(fmt::format("[{}] Cannot bind texture: {}. Window Pointer expired or uninitialized.\n", __func__, inTextureFile.getAbsoluteFileLocation()), false, true);
    return 0;
  }

  // Delegate creation entirely through the weak_ptr
  // inTextureFile.gTexture = win->CreateTexture(inTextureFile.sImageData.pData, inTextureFile.sImageData.Width, inTextureFile.sImageData.Height, inTextureFile.sImageData.Channels);
  inTextureFile.gTexture = static_cast<XPLMTextureID>( win->CreateTexture(inTextureFile) );

  // Free CPU buffer
  stbi_image_free(inTextureFile.sImageData.pData);
  inTextureFile.sImageData.pData = nullptr;

  return (inTextureFile.gTexture != 0) ? 1 : 0;
}



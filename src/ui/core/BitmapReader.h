#ifndef BITMAPREADER_H
#define BITMAPREADER_H

/**************


**************/

// #include <array>
#include <memory>
#include "../../core/MxUtils.h"
#include "../../io/Log.hpp"

#include "TextureFile.h"
#include "mx_img_window.h" // v26.09.3 Pointer to the briefer ImgWindow. Used with Bind and safe delete textures using ImgPanelGraphics.

#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
//#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_SIMD // remove SSE2 implementation
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // in "libs/stb" folder

using namespace missionx;

namespace missionx
{

class BitmapReader
{
public:
  BitmapReader();

  // The function is thread safe, it only loads the texture but does not bind it.
  static bool load_texture_no_bind(mxTextureFile& inTextureFile, std::string &outErr, bool flipImage_b = false);  // v26.09.3
  // v3.0.140 added flip flag.
  // The "is_bind_texture_thread_safe" (former: is_thread_safe) makes sure that we are calling from the main callback loop, will skip texture binding if the value is false.
  static bool load_textute_and_bind(mxTextureFile& inTextureFile, std::string &outErr, bool flipImage_b = true, bool is_bind_texture_thread_safe = true);

  static bool loadImageStb(std::string fileName, mxTextureFile::IMAGEDATA* ImageData, bool inFlipImage_b, std::string &outErr);
  static int bind_texture(mxTextureFile &inout_texture);

  // v26.09.3
  inline static std::weak_ptr<mx_img_window> mx_img_window_weak_ptr;
  static void destroy_textures(std::ranges::input_range auto& inout_textures_map)
  {
    const auto win = mx_img_window_weak_ptr.lock();
    if (!win)
      return;

    for (auto& [file_path, texture_info] : inout_textures_map)
    {
      if (texture_info.gTexture)
      {
         win->SafeDeleteTexture(texture_info.gTexture); // Safe 3-frame deferred destruction

        //auto glTextureID = static_cast<GLuint>(static_cast<intptr_t>(texture_info.gTexture));
        //if (ImgPanelGraphics::IsAvailable())
        //  ImgPanelGraphics::DestroyTexture(&glTextureID);
        //else
        //  glDeleteTextures(1, &glTextureID);

        texture_info.gTexture = 0;
      }

      #ifndef RELEASE
      Log::logMsg(fmt::format("[{}] Deleted texture: {}.", __func__, file_path));
      #endif
    }
  }



};

} // namespace
#endif // BASE_BITMAP_H

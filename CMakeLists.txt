cmake_minimum_required(VERSION 3.20)

project(missionx LANGUAGES C CXX VERSION 25.09 DESCRIPTION "Mission-X X-Plane plugin v25.x")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_VERBOSE_MAKEFILE ON)

# Normalize build type string
string(TOLOWER ${CMAKE_BUILD_TYPE} BUILD_TYPE_LOWERCASE)

# Platform setup
if (LINUX)
  set(CMAKE_PREFIX_PATH "/usr/bin")
elif(APPLE)
  set(CMAKE_OSX_ARCHITECTURES arm64;x86_64)
  set(CMAKE_MACOSX_RPATH 1)
endif()



# Source groups
set(BASE_SRC src/core/coordinate/NavAidInfo.hpp
    src/core/coordinate/Point.hpp
    src/core/coordinate/Points.hpp
    src/core/coordinate/UtilsGraph.hpp
    src/core/embeded_script/base_script.hpp
    src/core/embeded_script/ext_script.cpp
    src/core/embeded_script/ext_script.h
    src/core/embeded_script/my_basic.c
    src/core/embeded_script/my_basic.h
    src/core/embeded_script/script_manager.cpp
    src/core/embeded_script/script_manager.h
    src/core/obj3d/obj3d.cpp
    src/core/obj3d/obj3d.h
    src/core/sound/Sound.cpp
    src/core/sound/Sound.h
    src/core/sound/SoundFragment.cpp
    src/core/sound/SoundFragment.h
    src/core/thread/base_thread.hpp
    src/core/thread/gather_stats.cpp
    src/core/thread/gather_stats.h
    src/core/vr/mxvr.cpp
    src/core/vr/mxvr.h
    src/core/base_c_includes.h
    src/core/base_xp_include.h
    src/core/data_manager.cpp
    src/core/data_manager.h
    src/core/dataref_const.cpp
    src/core/dataref_const.h
    src/core/dataref_manager.cpp
    src/core/dataref_manager.h
    src/core/Message.cpp
    src/core/Message.h
    src/core/mx_base_node.cpp
    src/core/mx_base_node.h
    src/core/mxproperty.hpp
    src/core/MxUtils.cpp
    src/core/MxUtils.h
    src/core/mx_return.hpp
    src/core/QueueMessageManager.cpp
    src/core/QueueMessageManager.h
    src/core/TimeFragment.hpp
    src/core/Timer.hpp
    src/core/Utils.cpp
    src/core/Utils.h
    src/core/xx_mission_constants.hpp
    src/data/Briefer.cpp
    src/data/Briefer.hpp
    src/data/Choice.cpp
    src/data/Choice.h
    src/data/EndMission.hpp
    src/data/Waypoint.cpp
    src/data/Waypoint.h
    src/data/mxProperties.hpp
    src/data/Objective.cpp
    src/data/Objective.h
    src/data/Task.cpp
    src/data/Task.h
    src/data/Trigger.cpp
    src/data/Trigger.h
    src/inv/Inventory.cpp
    src/inv/Inventory.h
    src/inv/station.h
    src/inv/station.cpp
    src/io/BrieferInfo.cpp
    src/io/BrieferInfo.h
    src/io/dbase.cpp
    src/io/dbase.h
    src/io/IXMLParser.cpp
    src/io/IXMLParser.h
    src/io/ListDir.cpp
    src/io/ListDir.h
    src/io/Log.cpp
    src/io/Log.hpp
    src/io/OptimizeAptDat.cpp
    src/io/OptimizeAptDat.h
    src/io/read_mission_file.cpp
    src/io/read_mission_file.h
    src/io/system_actions.cpp
    src/io/system_actions.h
    src/io/TemplateFileInfo.hpp
    src/io/writeLogThread.cpp
    src/io/writeLogThread.h
    src/logic/BindCommand.cpp
    src/logic/BindCommand.h
    src/logic/dataref_param.cpp
    src/logic/dataref_param.h
    src/logic/MxParam.hpp
    src/random/RandomEngine.cpp
    src/random/RandomEngine.h
    src/ui/core/BitmapReader.cpp
    src/ui/core/BitmapReader.h
    src/ui/core/MxShader.hpp
    src/ui/core/TextureFile.h
    src/ui/gl/CueInfo.cpp
    src/ui/gl/CueInfo.h
    src/ui/gl/WinImguiBriefer.h
    src/ui/gl/WinImguiBriefer.cpp
    src/ui/gl/WinImguiMxpad.h
    src/ui/gl/WinImguiMxpad.cpp
    src/ui/gl/WinImguiOptions.h
    src/ui/gl/WinImguiOptions.cpp
    src/ui/MxUICore.cpp
    src/ui/MxUICore.hpp
    src/lr_crash.h
    src/mission.cpp
    src/mission.h
    src/plugin.cpp
    src/plugin.h
    src/core/mxconst.h
    )

set(IMGUI_SQLITE_STB_POLY_LIB_SRC
    libs/imgui4xp/colors/mx_colors.hpp
    libs/imgui4xp/GL/glew.c
    libs/imgui4xp/GL/glew.h
    libs/imgui4xp/GL/glext.h
    libs/imgui4xp/imgui/implot/implot.cpp
    libs/imgui4xp/imgui/implot/implot.h
    libs/imgui4xp/imgui/implot/implot_internal.h
    libs/imgui4xp/imgui/implot/implot_items.cpp
    libs/imgui4xp/imgui/misc/cpp/imgui_stdlib.cpp
    libs/imgui4xp/imgui/misc/cpp/imgui_stdlib.h
    libs/imgui4xp/imgui/misc/freetype/imgui_freetype.cpp
    libs/imgui4xp/imgui/misc/freetype/imgui_freetype.h
    libs/imgui4xp/imgui/imconfig.h
    libs/imgui4xp/imgui/imgui.cpp ## modified to allow copy/paste to work in Windows and MacOS but not in Linux
    libs/imgui4xp/imgui/imgui.h
    libs/imgui4xp/imgui/imgui_draw.cpp
    libs/imgui4xp/imgui/imgui_internal.h
    libs/imgui4xp/imgui/imgui_tables.cpp
    libs/imgui4xp/imgui/imgui_widgets.cpp
    libs/imgui4xp/imgui/imstb_rectpack.h
    libs/imgui4xp/imgui/imstb_textedit.h
    libs/imgui4xp/imgui/imstb_truetype.h
    libs/imgui4xp/ImgWindow/ImgFontAtlas.cpp
    libs/imgui4xp/ImgWindow/ImgFontAtlas.h
    libs/imgui4xp/ImgWindow/ImgFreeTypeFontAtlas.cpp
    libs/imgui4xp/ImgWindow/ImgFreeTypeFontAtlas.h
    libs/imgui4xp/ImgWindow/ImgWindow.cpp
    libs/imgui4xp/ImgWindow/ImgWindow.h
    libs/imgui4xp/KHR/khrplatform.h
    libs/polyline-cpp/src/SimplePolyline.h
    libs/polyline-cpp/src/SimplePolyline.cpp
    libs/sqlite/sqlite/sqlite3.h
    libs/sqlite/sqlite/sqlite3.c
    libs/sqlite/sqlite/extension-functions.h
    libs/stb/stb_image.h
    libs/stb/stb_image.cpp
    libs/fmt/include/fmt/core.h
    libs/fmt/include/fmt/format.h
    libs/fmt/include/fmt/format-inl.h
    libs/fmt/src/format.cc
)

set(GLM_LIB_SRC
    libs/glm/glm/common.hpp
    libs/glm/glm/exponential.hpp
    libs/glm/glm/ext.hpp
    libs/glm/glm/fwd.hpp
    libs/glm/glm/geometric.hpp
    libs/glm/glm/glm.hpp
    libs/glm/glm/integer.hpp
    libs/glm/glm/mat2x2.hpp
    libs/glm/glm/mat2x3.hpp
    libs/glm/glm/mat2x4.hpp
    libs/glm/glm/mat3x2.hpp
    libs/glm/glm/mat3x3.hpp
    libs/glm/glm/mat3x4.hpp
    libs/glm/glm/mat4x2.hpp
    libs/glm/glm/mat4x3.hpp
    libs/glm/glm/mat4x4.hpp
    libs/glm/glm/matrix.hpp
    libs/glm/glm/packing.hpp
    libs/glm/glm/trigonometric.hpp
    libs/glm/glm/vec2.hpp
    libs/glm/glm/vec3.hpp
    libs/glm/glm/vec4.hpp
    libs/glm/glm/vector_relational.hpp
    libs/glm/glm/detail/glm.cpp
    )

set(FREETYPE_LIB_SRC
    libs/freetype2/include/ft2build.h
    libs/freetype2/include/freetype/config/ftoption.h
    libs/freetype2/src/base/ftinit.c
    libs/freetype2/src/base/ftdebug.c
    libs/freetype2/src/base/ftbase.c
    libs/freetype2/src/base/ftbbox.c
    libs/freetype2/src/base/ftglyph.c
    libs/freetype2/src/base/ftbdf.c
    libs/freetype2/src/base/ftbitmap.c
    libs/freetype2/src/base/ftcid.c
    libs/freetype2/src/base/ftfstype.c
    libs/freetype2/src/base/ftgasp.c
    libs/freetype2/src/base/ftgxval.c
    libs/freetype2/src/base/ftmm.c
    libs/freetype2/src/base/ftotval.c
    libs/freetype2/src/base/ftpatent.c
    libs/freetype2/src/base/ftpfr.c
    libs/freetype2/src/base/ftstroke.c
    libs/freetype2/src/base/ftsynth.c
    libs/freetype2/src/base/fttype1.c
    libs/freetype2/src/base/ftwinfnt.c
    libs/freetype2/src/bdf/bdf.c
    libs/freetype2/src/cff/cff.c
    libs/freetype2/src/cid/type1cid.c
    libs/freetype2/src/pcf/pcf.c
    libs/freetype2/src/pfr/pfr.c
    libs/freetype2/src/sfnt/sfnt.c
    libs/freetype2/src/svg/ftsvg.c
    libs/freetype2/src/truetype/truetype.c
    libs/freetype2/src/type1/type1.c
    libs/freetype2/src/type42/type42.c
    libs/freetype2/src/winfonts/winfnt.c
    libs/freetype2/src/raster/raster.c
    libs/freetype2/src/sdf/sdf.c
    libs/freetype2/src/smooth/smooth.c
    libs/freetype2/src/autofit/autofit.c
    libs/freetype2/src/cache/ftcache.c
    libs/freetype2/src/gzip/ftgzip.c
    libs/freetype2/src/lzw/ftlzw.c
    libs/freetype2/src/bzip2/ftbzip2.c
    libs/freetype2/src/gxvalid/gxvalid.c
    libs/freetype2/src/otvalid/otvalid.c
    libs/freetype2/src/psaux/psaux.c
    libs/freetype2/src/pshinter/pshinter.c
    libs/freetype2/src/psnames/psnames.c
)

if (WIN32)
  list(APPEND FREETYPE_LIB_SRC libs/freetype2/builds/windows/ftsystem.c)
else()
  list(APPEND FREETYPE_LIB_SRC libs/freetype2/src/base/ftsystem.c)
endif()
if(APPLE)
  list(APPEND FREETYPE_LIB_SRC libs/freetype2/src/base/ftmac.c)
endif()

# Git SHA generation
find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_SHA OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

if(NOT GIT_SHA)
  set(GIT_SHA "UNKNOWN")
endif()
file(WRITE "src/core/sha.hpp" "#ifndef SHA_HPP\n#define SHA_HPP\n#define GIT_SHA \"${GIT_SHA}\"\n#endif\n")
set(DYNAMIC_SHA src/core/sha.hpp)

# Target definition <== MUST come before target_include_directories because the modern CMake syntax
#                       need to define a target object first on which it will define the needed metadata information
#                       like the "target_include_directories" directive.
add_library(${PROJECT_NAME} SHARED
  ${BASE_SRC}
  ${IMGUI_SQLITE_STB_POLY_LIB_SRC}
  ${GLM_LIB_SRC}
  ${FREETYPE_LIB_SRC}
  ${DYNAMIC_SHA}
)


# Includes
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/SDK/CHeaders/XPLM
        ${CMAKE_SOURCE_DIR}/libs/SDK/CHeaders/Widgets
        ${CMAKE_SOURCE_DIR}/libs/SDK/CHeaders/Wrappers
        ${CMAKE_SOURCE_DIR}/libs/curl/include
        ${CMAKE_SOURCE_DIR}/libs/fmod/inc
        ${CMAKE_SOURCE_DIR}/libs/json/include
        ${CMAKE_SOURCE_DIR}/libs/imgui4xp
        ${CMAKE_SOURCE_DIR}/libs/imgui4xp/imgui
        ${CMAKE_SOURCE_DIR}/libs/imgui4xpKHR
        ${CMAKE_SOURCE_DIR}/libs/imgui4xp/imgWindow
        ${CMAKE_SOURCE_DIR}/libs/imgui4xp/colors
        ${CMAKE_SOURCE_DIR}/libs/sqlite/sqlite
        ${CMAKE_SOURCE_DIR}/libs/stb
        ${CMAKE_SOURCE_DIR}/libs/fmt/include
        ${CMAKE_SOURCE_DIR}/libs/polyline-cpp/src
        ${CMAKE_SOURCE_DIR}/src/ui/core
        ${CMAKE_SOURCE_DIR}/libs/freetype2/include
        ${CMAKE_SOURCE_DIR}/libs/brotil/include
        ${CMAKE_SOURCE_DIR}/libs/glm
)

if (LINUX)
  target_include_directories(${PROJECT_NAME} PRIVATE
          /usr/include/x86_64-linux-gnu
          /usr/include/x86_64-linux-gnu/curl
          /usr/include/x86_64-linux-gnu/openssl
  )
  elif (WIN32)
  target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/libs/OpenSSL/openssl-3xx/include)
endif()



# Compiler features
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" OUTPUT_NAME "${PROJECT_NAME}" SUFFIX ".xpl")
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

# Global compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
  XPLM200=1 XPLM210=1 XPLM300=1 XPLM301=1 XPLM302=1 XPLM303=1 XPLM_DEPRECATED
  IMGUI_INCLUDE_IMCONFIG_H IMGUI_IMPL_OPENGL_LOADER_GLEW ENABLE_MISSIONX_LOG MB_DOUBLE_FLOAT
  glm_shared_EXPORTS FT2_BUILD_LIBRARY _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS
  ENABLE_EXPERIMENTAL_DISABLE MISSIONX_LIBRARY
)

# OS-specific definitions
if (WIN32)
  target_compile_definitions(${PROJECT_NAME} PRIVATE IBM=1 _WIN32 _CONSOLE WIN32 _WINDOWS _WINDLL _MBCS _AFXDLL FMT_UNICODE=0)
elseif (APPLE)
  target_compile_definitions(${PROJECT_NAME} PRIVATE MAC=1 APL=1 GL_SILENCE_DEPRECATION)
elif (UNIX)
  target_compile_definitions(${PROJECT_NAME} PRIVATE LIN=1)
endif()

if (${BUILD_TYPE_LOWERCASE} STREQUAL "debug")
  target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
elseif (${BUILD_TYPE_LOWERCASE} STREQUAL "release")
  target_compile_definitions(${PROJECT_NAME} PRIVATE RELEASE)
endif()

# Compiler options
if (APPLE)
  target_compile_options(${PROJECT_NAME} PRIVATE -mmacosx-version-min=11.0)
endif()

if (MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -fexceptions -fvisibility=hidden -Wall -Wpedantic -Wextra -Wno-unused)
  target_compile_options(${PROJECT_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fpermissive>)
  if (${BUILD_TYPE_LOWERCASE} STREQUAL "debug")
    target_compile_options(${PROJECT_NAME} PRIVATE -O0 -g3)
  else()
    target_compile_options(${PROJECT_NAME} PRIVATE -O3 -g0)
  endif()
  if (NOT WIN32)
    target_compile_options(${PROJECT_NAME} PRIVATE -fPIC)
  endif()
endif()

# Dependencies
find_package(OpenGL REQUIRED)
find_library(FMOD_LIBRARY NAMES fmod fmod_vc.lib)
if (WIN32)
  find_library(FREETYPE2 NAMES freetype.lib)
endif()
if (NOT APPLE)
  if (WIN32)
    find_library(CURL_LIBRARY NAMES curl libcurl.lib)
    find_library(ZLIB_LIBRARY NAMES zlib.lib)
  else()
    find_library(OPENSSL_LIBRARY NAMES ssl crypto)
    find_library(CURL_LIBRARY NAMES curl)
  endif()
endif()
if (WIN32 OR APPLE)
  find_library(XPLM_LIBRARY NAMES XPLM XPLM_64.lib XPWidgets XPWidgets_64.lib)
endif()

# Linking by OS
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)

if (WIN32)
  if (MSVC)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${FMOD_LIBRARY} ${ZLIB_LIBRARY} ${CURL_LIBRARY} ${XPLM_LIBRARY})
  else()
    target_link_libraries(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++ opengl32 -static)
  endif()
elseif (APPLE)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    "-v -exported_symbols_list ${CMAKE_SOURCE_DIR}/cmake_build/${PROJECT_NAME}.sym_mac"
    "-Wl,-rpath,./Resources/plugins/missionx/libs/64"
    "-Wno-c++11-narrowing"
    "-framework ApplicationServices"
    "-framework Foundation"
    "-framework Security"
    ${FMOD_LIBRARY}
    ${XPLM_LIBRARY}
    curl
  )
elif (UNIX)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    "-Wl,--version-script=${CMAKE_SOURCE_DIR}/cmake_build/${PROJECT_NAME}.sym"
    "-Wl,-rpath,./Resources/dlls/64"
    "-Wl,-rpath,./Resources/dlls/64/gl"
    "-Wl,-rpath,/usr/local/lib64"
    "-Wl,-rpath,/usr/local/lib"
    "-Wl,-rpath,/usr/lib64"
    "-Wl,-rpath,/usr/lib"
    "-Wl,-rpath,/lib64"
    "-Wl,-rpath,/lib/x86_64-linux-gnu"
    "-Wl,-rpath,/usr/lib/x86_64-linux-gnu"
    "-Wl,-rpath,./Resources/plugins/missionx/libs/64"
    ${FMOD_LIBRARY}
    ${CURL_LIBRARY}
  )
endif()

# Debug info
message("PROJECT_NAME: ${PROJECT_NAME}")
message("CMAKE_LIBRARY_PATH: ${CMAKE_LIBRARY_PATH}")
message("OPENGL_LIBRARIES: ${OPENGL_LIBRARIES}")
message("FMOD_LIBRARY: ${FMOD_LIBRARY}")
message("CURL_LIBRARY: ${CURL_LIBRARY}")
message("CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")

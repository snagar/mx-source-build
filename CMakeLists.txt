cmake_minimum_required(VERSION 3.14)

### https://cmake.org/cmake/help/latest/manual/cmake-commands.7.html
### Example1: https://github.com/krux02/minimal_cmake_example/blob/master/CMakeLists.txt
### Example2: https://github.com/sparker256/imgui4xp/blob/master/src/CMakeLists.txt
### https://cmake.org/cmake/help/latest/command/list.html
### https://cmake.org/cmake/help/latest/command/add_library.html
### https://stackoverflow.com/questions/28597351/how-do-i-add-a-library-path-in-cmake
### https://stackoverflow.com/questions/61029105/cmake-add-library-cannot-create-target-cxx-because-another-target-with-the-s

####### MacOS Related #########
##  https://developer.apple.com/documentation/apple-silicon/porting-your-macos-apps-to-apple-silicon
## https://cmake.org/cmake/help/latest/prop_tgt/ENABLE_EXPORTS.html

project(missionx LANGUAGES C CXX VERSION 25.04 DESCRIPTION "Mission-X X-Plane plugin v25.x")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_VERBOSE_MAKEFILE ON)

# Run CMake with configuration changes?
# -DCMAKE_CXX_COMPILER:STRING=/usr/bin/x86_64-linux-gnu-g++-13
# -DCMAKE_C_COMPILER:STRING=/usr/bin/gcc-13

#cmake_minimum_required(VERSION 3.5)

if (UNIX)
  # Find the operating system name
  execute_process(COMMAND lsb_release -si OUTPUT_VARIABLE OS_NAME OUTPUT_STRIP_TRAILING_WHITESPACE)

  # Find the operating system version ID
  execute_process(COMMAND lsb_release -sr OUTPUT_VARIABLE OS_VERSION_ID OUTPUT_STRIP_TRAILING_WHITESPACE)
endif ()


# Return the date (yyyy-mm-dd)
macro(DATE RESULT)
  if(WIN32)
    execute_process(COMMAND "cmd" " /C date /T" OUTPUT_VARIABLE ${RESULT})
    string(REGEX REPLACE "(..)/(..)/(....).*" "\\3-\\2-\\1" ${RESULT} ${${RESULT}})
  elseif(UNIX)
    execute_process(COMMAND "date" "+%Y-%m-%d" OUTPUT_VARIABLE ${RESULT})
  else()
    message(SEND_ERROR "Unable to detect date")
    set(${RESULT} UNKNOWN)
  endif()
endmacro()

# Return the time (hh:mm:ss)
macro(TIME RESULT)
  if(WIN32)
    execute_process(COMMAND "cmd" " /C echo %TIME%" OUTPUT_VARIABLE ${RESULT})
    string(REGEX REPLACE "(..:..:..),(..)" "\\1" ${RESULT} ${${RESULT}})
  elseif(UNIX)
    execute_process(COMMAND "date" "+%H:%M:%S" OUTPUT_VARIABLE ${RESULT})
  else()
    message(SEND_ERROR "Unable to detect time")
    set(${RESULT} UNKNOWN)
  endif()
endmacro()



if (LINUX)
  set(CMAKE_PREFIX_PATH "/usr/bin")
elseif(APPLE)
  set(CMAKE_OSX_ARCHITECTURES arm64;x86_64) # https://stackoverflow.com/questions/65157483/macos-build-universal-binary-2-with-cmake
  set(CMAKE_MACOSX_RPATH 1)
endif()


string (TOLOWER ${CMAKE_BUILD_TYPE} BUILD_TYPE_LOWERCASE) ## store the build type always as lower case and use it through out the cmake file

include_directories("${CMAKE_SOURCE_DIR}/libs/SDK/CHeaders/XPLM")
include_directories("${CMAKE_SOURCE_DIR}/libs/SDK/CHeaders/Widgets")
include_directories("${CMAKE_SOURCE_DIR}/libs/SDK/CHeaders/Wrappers")
include_directories("${CMAKE_SOURCE_DIR}/libs/curl/include")
include_directories("${CMAKE_SOURCE_DIR}/libs/fmod/inc")
include_directories("${CMAKE_SOURCE_DIR}/libs/json/include") ## nlohmann json lib
include_directories("${CMAKE_SOURCE_DIR}/libs/imgui4xp")
include_directories("${CMAKE_SOURCE_DIR}/libs/imgui4xp/imgui")
include_directories("${CMAKE_SOURCE_DIR}/libs/imgui4xpKHR")
include_directories("${CMAKE_SOURCE_DIR}/libs/imgui4xp/imgWindow")
include_directories("${CMAKE_SOURCE_DIR}/libs/imgui4xp/colors")
include_directories("${CMAKE_SOURCE_DIR}/libs/sqlite/sqlite")
include_directories("${CMAKE_SOURCE_DIR}/libs/stb")
include_directories("${CMAKE_SOURCE_DIR}/libs/fmt/include")
include_directories("${CMAKE_SOURCE_DIR}/libs/polyline-cpp/src")
include_directories("${CMAKE_SOURCE_DIR}/src/ui/core")
include_directories("${CMAKE_SOURCE_DIR}/libs/freetype2/include")
include_directories("${CMAKE_SOURCE_DIR}/libs/brotil/include")
include_directories("${CMAKE_SOURCE_DIR}/libs/glm")

if (LINUX)
  # ubuntu 22.04
  include_directories("/usr/include/x86_64-linux-gnu/")
  include_directories("/usr/include/x86_64-linux-gnu/curl")
  include_directories("/usr/include/x86_64-linux-gnu/openssl")
#  include_directories("/usr/lib/x86_64-linux-gnu")

#  # CURL include
#  if (OS_NAME STREQUAL "Ubuntu" OR OS_NAME STREQUAL "Linuxmint")
#    if (OS_VERSION_ID MATCHES "^22\\." OR OS_VERSION_ID MATCHES "^21\\.")
#      message("INCLUDE: Ubuntu/Linuxmint version: ${OS_VERSION_ID}. Using curl 22.04 search path.")
#      include_directories("${CMAKE_SOURCE_DIR}/libs/curl-v22.04/include")
#    elseif (OS_VERSION_ID MATCHES "^24\\." OR OS_VERSION_ID MATCHES "^22\\.")
#      message("INCLUDE: Ubuntu/Linuxmint version: ${OS_VERSION_ID}. Using default curl search path.")
#      include_directories("${CMAKE_SOURCE_DIR}/libs/curl/include")
#    else ()
#      message(WARNING "INCLUDE: Unsupported Ubuntu version: ${OS_VERSION_ID}. Using default curl search path.")
#      include_directories("${CMAKE_SOURCE_DIR}/libs/curl/include") # Default path
#    endif ()
#  else ()
#    message(WARNING "INCLUDE: Unsupported OS: ${OS_NAME}. Using default curl search path.")
#    include_directories("${CMAKE_SOURCE_DIR}/libs/curl/include") # Default path for other OSes
#  endif ()


elseif (WIN32)
  include_directories("${CMAKE_SOURCE_DIR}/libs/OpenSSL/openssl-3xx/include")
endif()

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

set (GLM_LIB_SRC
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

set (FREETYPE_LIB_SRC
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

# Add freetype specific system file for native file handling
if (WIN32)
  message("WIN32 ftsystem.c used")
  list(APPEND FREETYPE_LIB_SRC "libs/freetype2/builds/windows/ftsystem.c")
  # elseif (LINUX)
  #   message("UNIX ftsystem.c used")
  #   list(APPEND FREETYPE_LIB_SRC "libs/freetype2/builds/unix/ftsystem.c")
else ()
  message("GENERIC ftsystem.c used")
  list(APPEND FREETYPE_LIB_SRC "libs/freetype2/src/base/ftsystem.c")
endif ()

if(APPLE)
  list(APPEND FREETYPE_LIB_SRC     libs/freetype2/src/base/ftmac.c)
endif()

## Mission-X source files
add_library(${PROJECT_NAME} SHARED
    ${BASE_SRC}
    ${IMGUI_SQLITE_STB_POLY_LIB_SRC}
    ${GLM_LIB_SRC}
    ${FREETYPE_LIB_SRC}
)

#################################
### Compiler Directives
#################################

#if (NOT WIN32)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
#endif()

if (APPLE)
  # We will aim to MacOS version 11.0 and up
  target_compile_options(${PROJECT_NAME} PUBLIC -mmacosx-version-min=11.0)
  target_link_libraries(${PROJECT_NAME} -mmacosx-version-min=11.0)
endif ()



# Enable all X-Plane SDK APIs up to the newest version.
add_definitions(-DXPLM200=1 -DXPLM210=1 -DXPLM300=1 -DXPLM301=1 -DXPLM302=1 -DXPLM303=1 -DXPLM_DEPRECATED)

# Define platform macros.
if (WIN32)
  add_definitions(-DIBM=1 -D_WIN32 -D_CONSOLE -DWIN32 -D_WINDOWS  -D_WINDLL -D_MBCS -D_AFXDLL -DFMT_UNICODE=0) ## IBM=1 the "FMT_UNICODE=0" is needed to solve FMT compilation from CMake, not relevant from MSVC.
elseif(APPLE)
  add_definitions(-DMAC=1 -DAPL=1 -DGL_SILENCE_DEPRECATION) ## APL=1 MAC=1 GL_SILENCE_DEPRECATION=remove mac warning regarding GL deprecation since 10.14
elseif (UNIX)
  add_definitions(-DLIN=1) ## LIN=1
endif()

if (NOT APPLE)
  #  add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT )  ## deprecated, using MacOS secure library
  add_definitions(-DGLEW_BUILD) ## hopefully will resolve warning C4273, based on the  libs/imgui4xp/GL/glew.h macro
endif()

## Enable other Mission-X flags
add_definitions(-DIMGUI_INCLUDE_IMCONFIG_H -DIMGUI_IMPL_OPENGL_LOADER_GLEW -DENABLE_MISSIONX_LOG -DMB_DOUBLE_FLOAT)
add_definitions(-Dglm_shared_EXPORTS -DFT2_BUILD_LIBRARY -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS)
add_definitions(-DENABLE_EXPERIMENTAL_DISABLE)


if ( ${BUILD_TYPE_LOWERCASE} STREQUAL "debug" )
  message("We are in debug - adding debug definition to compiler !!!!")
  # add_definitions(-DDEBUG -DCMAKE_INTDIR="debug")
  add_definitions(-DDEBUG)
elseif ( ${BUILD_TYPE_LOWERCASE} STREQUAL "release" )
  add_definitions(-DRELEASE)
endif()

# Visual Studio defines compiler switches via CMakeSettings.json
if (WIN32)
  add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
  add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif ()

if (NOT MSVC)

  # Force-enable exception support. This is most likely redundant, although for C
  # code the default is the opposite. Since we are mixing C++ and C libraries,
  # safer to set it on?
  #    add_compile_options(-fexceptions -fpermissive) # v3.305.3 add permissive flag
  add_compile_options(-fexceptions)
  # On UNIX systems this makes symbols non-exported by default. On Windows this
  # option is simply ignored, since symbol visibility works differently there.
  add_compile_options(-fvisibility=hidden)

  # from eclipse project

  add_compile_options(-c -fmessage-length=0 -Wunused-function -Wno-narrowing)

  ## Add -fpermissive only for CXX compiler and not C
  target_compile_options(${PROJECT_NAME}  PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fpermissive> )


  # Enable stricter warnings and then disable some we are not interested in.
  #    add_compile_options(-Wall -Wpedantic -Wshadow -Wextra)
  add_compile_options(-Wall -Wpedantic -Wextra)
  add_compile_options(-Wno-unused)

  # Always use position-independent code and highest optimization level (FPS!).
  if ( ${BUILD_TYPE_LOWERCASE} STREQUAL "debug" )
    message ("Adding debug optimization -O0")
    add_compile_options(-O0)
    add_compile_options(-g3)
  else()
    message ("Adding release optimization -O3")
    add_compile_options(-O3)
    add_compile_options(-g0)
  endif()

  if (NOT WIN32)
    add_compile_options(-fPIC)
  endif()
endif ()





#################################
### Linker Directives
#################################


# Filter out libraries that in "anaconda" or "git" repository. The idea is to force OS libraries.
#function(fn_filter_out_specific_folders validator_result_var item)
#  message("Validating: ${item} :" ${item} )
#
#  if(item MATCHES "git")
#    message("Found git in ${item} skipping")
#    set(${validator_result_var} FALSE PARENT_SCOPE)
#  endif()
#  if(item MATCHES "anaconda")
#    message("Found anaconda in ${item} skipping")
#    set(${validator_result_var} FALSE PARENT_SCOPE)
#  endif()
#  message("Did not Found anaconda nor git in ${item} skipping")
#endfunction()



## Link Third party libraries
### Specify library search locations.
if (WIN32)
  message("Windows Libraries Path")
  list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/libs/SDK/Libraries/Win ")
  list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/libs/curl/libs/x64/win")
  list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/libs/OpenSSL/openssl-3xx/lib")
  list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/libs/fmod/lib/x64")
elseif (APPLE)
  message("Apple Libraries Path")
  list(APPEND CMAKE_FRAMEWORK_PATH "${CMAKE_SOURCE_DIR}/libs/SDK/Libraries/Mac")
  list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/libs/fmod/lib/x64")
elseif (UNIX)
  message("\nLinux Libraries Path")
  message("\tFor error in finding libraries, make sure that there are no missing symbolic links to the actual library name.")
  message("\tExample: libcurl.so might be missing because the library name is: 'libcurl.so.4.8.0' and not 'libcurl.so'. Create symbolic link if needed.")
  message("\tapt install freeglut3-dev")
  message("\tln -s libcurl.so.4 libcurl.so")
  message("\tln -s libssl.so.3 libssl.so\n")


  list(APPEND CMAKE_LIBRARY_PATH "/lib/x86_64-linux-gnu") ## v24.06.1  Mint/Ubuntu ub 22.04, mint 21.x
  list(APPEND CMAKE_LIBRARY_PATH "/usr/lib/x86_64-linux-gnu") ## v24.06.1  Mint/Ubuntu ub 22.04, mint 21.x
  list(APPEND CMAKE_LIBRARY_PATH "/usr/local/lib64")
  list(APPEND CMAKE_LIBRARY_PATH "/usr/local/lib")
  list(APPEND CMAKE_LIBRARY_PATH "/usr/lib64")
  list(APPEND CMAKE_LIBRARY_PATH "/usr/lib")
  list(APPEND CMAKE_LIBRARY_PATH "/lib64") ## v24.06.1 Fedora
  list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/libs/fmod/lib/x64")

endif ()

## OpenGL libraries
#find_library(OPENGL_LIBRARY NAMES OpenGL GL GLU OpenGL OpenGL32.lib GLUE32.lib ) # apt install freeglut3-dev
find_package(OpenGL REQUIRED)
if(OPENGL_FOUND)
  include_directories(${OPENGL_INCLUDE_DIR})
  target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES})
else()
  message(FATAL_ERROR "OpenGL libraries not found!")
endif()


find_library(FMOD_LIBRARY NAMES fmod fmod_vc.lib)

if (WIN32)
  find_library(FREETYPE2 NAMES freetype.lib)
endif()


if (NOT APPLE)
  if (WIN32)
    message ("Windows - search for cURL libs")
    ## find_package needs "package.cmake" file to be present, this is why we use the "find_library" function instead
    find_library(OPENSSL_LIBRARY NAMES libssl.lib libcrypto.lib REQUIRED HINT ${CMAKE_SOURCE_DIR}/libs/OpenSSL/openssl-3xx/lib)  #
    find_library(CURL_LIBRARY NAMES libcurl.lib)  # cURL

  else()
    message ("Linux - search for cURL + OpenSSL libs")
    ## find_package needs "package.cmake" file to be present, this is why we use the "find_library" function instead
    find_library(OPENSSL_LIBRARY NAMES ssl crypto ) # HINT ${CMAKE_SOURCE_DIR}/libs/OpenSSL/openssl-3xx/lib)  #
    find_library(CURL_LIBRARY    NAMES curl    ) # cURL

    # Deprecated, works well without the filter
    #    find_library(OPENSSL_LIBRARY NAMES ssl    validator fn_filter_out_specific_folders ) # HINT ${CMAKE_SOURCE_DIR}/libs/OpenSSL/openssl-3xx/lib)  #
    #    find_library(OPENSSL_LIBRARY NAMES crypto validator fn_filter_out_specific_folders )
    #    find_library(CURL_LIBRARY    NAMES curl   validator fn_filter_out_specific_folders ) # cURL


  endif()
endif()

### Link X-Plane plugin system libraries. They are only provided for OS X and Windows.
if (WIN32 OR APPLE)
  find_library(XPLM_LIBRARY NAMES XPLM XPLM_64.lib XPWidgets XPWidgets_64.lib)
  target_link_libraries(${PROJECT_NAME} ${XPLM_LIBRARY})
endif ()


if (WIN32)
  # Unlike OS X and Linux we build standard libraries statically since X-Plane does not provide them.
  if (MSVC)
    # Visual Studio includes many standard libraries already, just need to make sure OpenGL is linked
    #  target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARY})
    target_link_libraries(${PROJECT_NAME} ${FMOD_LIBRARY})
    target_link_libraries(${PROJECT_NAME} ${CURL_LIBRARY})
    target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARY})
  else ()
    target_link_libraries(${PROJECT_NAME} -static-libgcc -static-libstdc++ opengl32)
    # MSYS2 for some reason always links against winpthread, so we have to work around it by statically linking the
    # entire plugin. This allows building the plugin natively on Windows.
    target_link_libraries(${PROJECT_NAME} -static)
  endif ()
elseif (APPLE)
  message("Apple Libraries Linking")
  # Restrict set of symbols exported from the plugin. This reduces changes of
  # conflict with other plugins, in particular ones with Lua interpreter
  # embedded.
  target_link_libraries(${PROJECT_NAME} "-v -exported_symbols_list ${CMAKE_SOURCE_DIR}/cmake_build/${PROJECT_NAME}.sym_mac")
  target_link_libraries(${PROJECT_NAME} -Wl,-rpath,./Resources/plugins/missionx/libs/64 -lcurl)  ## -lcurl MANDATORY for linking MacOS
  target_link_libraries(${PROJECT_NAME} "-Wno-c++11-narrowing")
  target_link_libraries(${PROJECT_NAME} "-framework ApplicationServices") ## needed for imgui + clipboard (IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS)
  target_link_libraries(${PROJECT_NAME} "-framework Foundation") ##
  target_link_libraries(${PROJECT_NAME} "-framework Security") ##

#  target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARY})
  target_link_libraries(${PROJECT_NAME} ${FMOD_LIBRARY})

  ## Deprecated since we are using MacOS internal shared "cURL" library
  #    set (CURL_LIBRARY "/Users/xplane/programming/git/missionx_v3_git/libs/curl/libs/x64/mac/libcurl.dylib") ## fix cmake fail to register the correct library file
  #    target_link_libraries(${PROJECT_NAME} ${CURL_LIBRARY})


elseif (UNIX)
  message("Linux Libraries Linking")
  # Specify additional runtime search paths for dynamically-linked libraries.
  # Restrict set of symbols exported from the plugin. This reduces changes of
  # conflict with other plugins, in particular ones with Lua interpreter
  # embedded.
  target_link_libraries(${PROJECT_NAME} -Wl,--version-script -Wl,${CMAKE_SOURCE_DIR}/cmake_build/${PROJECT_NAME}.sym )
  target_link_libraries(${PROJECT_NAME} -Wl,-rpath,./Resources/dlls/64)
  target_link_libraries(${PROJECT_NAME} -Wl,-rpath,./Resources/dlls/64/gl)
  target_link_libraries(${PROJECT_NAME} -Wl,-rpath,/usr/local/lib64)
  target_link_libraries(${PROJECT_NAME} -Wl,-rpath,/usr/local/lib)
  target_link_libraries(${PROJECT_NAME} -Wl,-rpath,/usr/lib64)
  target_link_libraries(${PROJECT_NAME} -Wl,-rpath,/usr/lib)
  target_link_libraries(${PROJECT_NAME} -Wl,-rpath,/lib64) ## v24.06.1 Fedora
  target_link_libraries(${PROJECT_NAME} -Wl,-rpath,/lib/x86_64-linux-gnu) ## v24.06.1  Mint/Ubuntu
  target_link_libraries(${PROJECT_NAME} -Wl,-rpath,/usr/lib/x86_64-linux-gnu) ## v24.06.1  ubuntu 24.10
  target_link_libraries(${PROJECT_NAME} -Wl,-rpath,./Resources/plugins/missionx/libs/64) # needed for XP11, no FMOD library
  # target_link_libraries(${PROJECT_NAME} -Wmultichar)  ## added to try and resolve IXMLParser library multichar warning, but it did not resolve it.

#  target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARY})
  target_link_libraries(${PROJECT_NAME} ${FMOD_LIBRARY})
  target_link_libraries(${PROJECT_NAME} ${CURL_LIBRARY}) # depends on your distro, you might need to add "libcurl.so" symlink to the real library name.
  target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARY})
endif ()


target_compile_definitions(${PROJECT_NAME} PRIVATE MISSIONX_LIBRARY)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".xpl")

############# DEBUG ##############
message("PROJECT_NAME: " ${PROJECT_NAME})
if (UNIX)
  message("OS_NAME:" ${OS_NAME})
  message("OS_VERSION_ID:" ${OS_VERSION_ID})
endif ()
#message("CMAKE_COMMAND: " ${CMAKE_COMMAND})
#message("CMAKE_BUILD_TYPE: " ${CMAKE_BUILD_TYPE}, "Local BUILD_TYPE_LOWERCASE: " ${BUILD_TYPE_LOWERCASE})
message("CMAKE_LIBRARY_PATH: " ${CMAKE_LIBRARY_PATH} )
#message("CMAKE_PREFIX_PATH: " ${CMAKE_PREFIX_PATH})
#message("CMAKE_CXX_STANDARD: " ${CMAKE_CXX_STANDARD})
#message("CMAKE_SOURCE_DIR: " ${CMAKE_SOURCE_DIR})
message("OPENGL_LIBRARIES:" ${OPENGL_LIBRARIES})
message("FMOD_LIBRARY:" ${FMOD_LIBRARY})
message("CURL_LIBRARY:" ${CURL_LIBRARY})
message("OPENSSL_LIBRARY:" ${OPENSSL_LIBRARY})
#message("CMAKE_OSX_ARCHITECTURES:" ${CMAKE_OSX_ARCHITECTURES})
#message("CMAKE_OSX_DEPLOYMENT_TARGET: " ${CMAKE_OSX_DEPLOYMENT_TARGET})
#message("CMAKE_CXX_FLAGS_DEBUG: " ${CMAKE_CXX_FLAGS_DEBUG})
#message("CMAKE_CC_FLAGS_DEBUG: " ${CMAKE_CC_FLAGS_DEBUG})
#message("CMAKE_CXX_FLAGS_RELEASE: " ${CMAKE_CXX_FLAGS_RELEASE})
#message("CMAKE_CC_FLAGS_RELEASE: " ${CMAKE_CC_FLAGS_RELEASE})
#message("CMAKE_EXECUTABLE_ENABLE_EXPORTS: " ${CMAKE_EXECUTABLE_ENABLE_EXPORTS})
#message("CMAKE_SHARED_LIBRARY_ENABLE_EXPORTS: " ${CMAKE_SHARED_LIBRARY_ENABLE_EXPORTS})
#message("C_COMPILER_ID: " ${C_COMPILER_ID})
#message("CXX_COMPILER_ID: " ${CXX_COMPILER_ID})

message("MissionX: CMAKE_CURRENT_BINARY_DIR: " ${CMAKE_CURRENT_BINARY_DIR})



## Start timing
#string(TIMESTAMP t)
#message(STATUS "local time: ${t}")
#file(WRITE "${CMAKE_BINARY_DIR}/build_time.txt" "Build started: at: ${t}\n")

#add_custom_command(
#        TARGET ${PROJECT_NAME} PRE_BUILD
#        COMMAND ${CMAKE_COMMAND} -E echo Build Started: at:\t%DATE% %TIME% > "${CMAKE_CURRENT_BINARY_DIR}/build_time.txt"
#)

# Add a post-build command to log the end time
#if (UNIX)
#  if (APPLE)
#    add_custom_command(
#        TARGET ${PROJECT_NAME}  POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy
#        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.xpl
#        ~/xp11/Resources/plugins/missionx/mac_x64/${PROJECT_NAME}.xpl
#    )
#  else() ## Linux
#    message ("Linux post build commands read.") ## debug
#
#    ## xp 12.x Beta
#    add_custom_command(
#        TARGET ${PROJECT_NAME}  POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy
#        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.xpl
#        ~/xp12b/Resources/plugins/missionx/lin_x64/${PROJECT_NAME}.xpl
#    )
#
#    ## xp 11
#    add_custom_command(
#        TARGET ${PROJECT_NAME}  POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy
#        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.xpl
#        ~/xp11/Resources/plugins/missionx/lin_x64/${PROJECT_NAME}.xpl
#    )
#
#    ## xp 12 release
#    add_custom_command(
#        TARGET ${PROJECT_NAME}  POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy
#        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.xpl
#        ~/xp12/Resources/plugins/missionx/lin_x64/${PROJECT_NAME}.xpl
#    )
#
#  endif()
#else () ## Win32
#  ## xp 12 release
#  add_custom_command(
#      TARGET ${PROJECT_NAME}  POST_BUILD
#      COMMAND ${CMAKE_COMMAND} -E copy
#      ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.xpl
#      C:/xp12/Resources/plugins/missionx/win_x64/${PROJECT_NAME}.xpl
#  )


  #    add_custom_command(
  #            TARGET ${PROJECT_NAME} POST_BUILD
  #            COMMAND ${CMAKE_COMMAND} -E echo Build finished at: %DATE% %TIME% >> "${CMAKE_CURRENT_BINARY_DIR}/build_time.txt"
  #    )

#endif()


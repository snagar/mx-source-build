#ifndef BASE_C_INCLUDE_H_
#define BASE_C_INCLUDE_H_
#pragma once

// **************
#include <algorithm>
#include <errno.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <vector>


#include <cstdlib>
#include <cstring>
#include <math.h>


#include <chrono>
#include <future>
#include <mutex>
#include <thread>

// OpenGL Support
#if defined(LIN) || defined(IBM)
#include <GL/glew.h>
#include <GL/glext.h>
#else
#define TRUE  1
#define FALSE 0

#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED

#include <OpenGL/gl.h>
#include <OpenGL/gl3.h>
#include <OpenGL/glu.h>


#endif


#ifdef APL
// #include <Carbon/Carbon.h>
#endif




#endif // BASE_C_INCLUDE_H_

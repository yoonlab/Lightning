
#ifndef	__BASE_OPENGL_PROJECT_H__
#define __BASE_OPENGL_PROJECT_H__


// include OpenGL header files
#include "GL/glew.h"		// include GLEW and new version of GL on Windows
#include "GL/freeglut.h"
#include "GLFW/glfw3.h"		// GLFW helper library

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "CheckError.h"

#include <iostream>


// type definition
#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif


const GLfloat  DegreesToRadians = (GLfloat)( M_PI / 180.0 ) ;


#endif	// __BASE_OPENGL_PROJECT_H__

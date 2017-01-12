

#ifndef	__SHADER_HELPER_H__
#define __SHADER_HELPER_H__

#include "Base.h"


// shader helper function
GLuint	InitShader( const char* vertexShaderFile, const char* geometryShaderFile, const char* fragmentShaderFile, GLuint* vShader, GLuint* gShader, GLuint* fShader ) ;
GLuint	LinkShader( GLuint program ) ;


#endif	// __SHADER_HELPER_H__

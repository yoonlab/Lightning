
#include "Shader.h"

// Create a NULL-terminated string by reading the provided file
static char*	readShaderSource( const char* shaderFile )
{
    FILE* fp = fopen(shaderFile, "r");

    if ( fp == NULL ) { return NULL; }

    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);

    char* buf = new char[size + 1];
	///////////////////////////////////////////////
	//fread(buf, 1, size, fp);
	///////////////////////////////////////////////
	size = fread(buf, 1, size, fp);

    buf[size] = '\0';
    fclose(fp);

    return buf;
}

// Link shader program
GLuint	LinkShader( GLuint prog )
{
	/* link  and error check */
    glLinkProgram( prog ) ;

    GLint  linked ;
    glGetProgramiv( prog, GL_LINK_STATUS, &linked ) ;
    if ( !linked )
	{
		std::cerr << "Shader program failed to link" << std::endl ;
		GLint  logSize ;
		glGetProgramiv( prog, GL_INFO_LOG_LENGTH, &logSize) ;
		char* logMsg = new char[ logSize ] ;
		glGetProgramInfoLog( prog, logSize, NULL, logMsg ) ;
		std::cerr << logMsg << std::endl ;
		delete [] logMsg ;

		exit( EXIT_FAILURE ) ;
    }

    /* use program object */
    glUseProgram( prog ) ;

    return prog ;
}

// Create a GLSL program object from vertex and fragment shader files
GLuint	InitShader(const char* vShaderFile, const char* gShaderFile, const char* fShaderFile, GLuint* vShader, GLuint* gShader, GLuint* fShader )
{
    struct Shader
	{
		const char*  filename ;
		GLenum       type ;
		GLchar*      source ;
    } shaders[ 3 ] =
	{
		{ vShaderFile, GL_VERTEX_SHADER, NULL },
		{ gShaderFile, GL_GEOMETRY_SHADER, NULL },
		{ fShaderFile, GL_FRAGMENT_SHADER, NULL }
    } ;

    GLuint program = glCreateProgram() ;
    
	GLuint tempShader[ 3 ];
	
    for ( int i = 0; i < 3; ++i )
	{
		Shader& s = shaders[ i ] ;
		if ( NULL != s.filename )
		{
			s.source = readShaderSource( s.filename ) ;
			if ( shaders[i].source == NULL )
			{
				std::cerr << "Failed to read " << s.filename << std::endl ;
				exit( EXIT_FAILURE ) ;
			}

			GLuint shader = glCreateShader( s.type ) ;

			tempShader[ i ] = shader ;

			glShaderSource( shader, 1, (const GLchar**) &s.source, NULL ) ;
			glCompileShader( shader ) ;

			GLint  compiled ;
			glGetShaderiv( shader, GL_COMPILE_STATUS, &compiled ) ;
			if ( !compiled )
			{
				std::cerr << s.filename << " failed to compile:" << std::endl ;
				GLint  logSize ;
				glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &logSize ) ;
				char* logMsg = new char[ logSize ] ;
				glGetShaderInfoLog( shader, logSize, NULL, logMsg ) ;
				std::cerr << logMsg << std::endl ;
				delete [] logMsg ;

				exit( EXIT_FAILURE ) ;
			}

			delete [] s.source ;

			glAttachShader( program, shader ) ;
		}
    }

	if ( vShader )	*vShader = tempShader[ 0 ] ;
	if ( gShader )	*gShader = tempShader[ 1 ] ;
	if ( fShader )	*fShader = tempShader[ 2 ] ;

	return LinkShader( program ) ;

/*/
    // link  and error check
    glLinkProgram( program ) ;

    GLint  linked ;
    glGetProgramiv( program, GL_LINK_STATUS, &linked ) ;
    if ( !linked )
	{
		std::cerr << "Shader program failed to link" << std::endl ;
		GLint  logSize ;
		glGetProgramiv( program, GL_INFO_LOG_LENGTH, &logSize) ;
		char* logMsg = new char[ logSize ] ;
		glGetProgramInfoLog( program, logSize, NULL, logMsg ) ;
		std::cerr << logMsg << std::endl ;
		delete [] logMsg ;

		exit( EXIT_FAILURE ) ;
    }

    // use program object 
    glUseProgram( program ) ;

    return program ;
//*/
}

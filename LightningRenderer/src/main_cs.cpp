//#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")

#include "Angel.h"
#include "Mesh.h"

#include "../cgm/acgm_map.h"
#include "../cgm/new_acgm_map.h"

#include <vector>
#include <random>
#include <ctime>

using namespace std ;


// --------------------------------------------------------------------------------------------------------------------
// for opengl & shader

GLuint				g_texture ;
GLuint				g_renderShader ;
GLuint				g_computeShader ;

int					g_index ;


// Window size
GLuint				g_width	= 960 ;
GLuint				g_height = 960 ;

bool				g_bWireFrame = false ;

// Update rate
double				g_FPS ;

// To implement Quaternion
struct Quaternion
{
	float x ;
	float y ;
	float z ;
	float w ;
} ;

// --------------------------------------------------------------------------------------------------------------------
// functions
// utility
mat4	Rotation( vec3 d, float degree ) ;
void	calculateFPS() ;

// shader
void	setLight() ;
void	setModel() ;
void	setView() ;
void	setProjection() ;

// callback
void	init() ;
void	display() ;
void	cleanup() ;
void	move( int x, int y ) ;
void	mouse( int button, int state, int x, int y ) ;
void	idle() ;
void	keySelect( unsigned char key, int x, int y ) ;
void	reshape( int width, int height ) ;

// 
void	destroyShaders() ;
vec2	newXY( int x, int y ) ;

// --------------------------------------------------------------------------------------------------------------------
void	initGrid() ;
void	drawGrid() ;

void	initObstacles() ;
void	drawObstalces() ;

void	initLightning() ;
void	drawLightning() ;
void	drawLightningStepByStep() ;

void	drawStartEndPos() ;


// Rotation with arbitrary axis by using quaternion
mat4	Rotation( vec3 d, float degree )
{
	d = normalize( d ) ;

	Quaternion local ;
	float angle = float( ( degree / 180.0f ) * M_PI ) ;
	float result = (float)sin( angle / 2.0f ) ;

	local.w = cos( angle / 2.0f ) ;
	local.x = float( d.x * result ) ;
	local.y = float( d.y * result ) ;
	local.z = float( d.z * result ) ;

	mat4 localMat ;
	localMat = mat4( 1.0f - 2.0f * ( local.y * local.y + local.z * local.z ), 2.0f * ( local.x * local.y - local.w * local.z ), 2.0f * ( local.x * local.z + local.w * local.y ), 0.0f, 
					 2.0f * ( local.x * local.y + local.w * local.z ), 1.0f - 2.0f * ( local.x * local.x + local.z * local.z ), 2.0f * ( local.y * local.z - local.w * local.x ), 0.0f,
					 2.0f * ( local.x * local.z - local.w * local.y ), 2.0f * ( local.y * local.z + local.w * local.x ), 1.0f - 2.0f * ( local.x * local.x + local.y * local.y ), 0.0f,
					 0.0f, 0.0f, 0.0f, 1.0f ) ;

	return localMat ;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////

void	checkError( std::string strDesc )
{
	GLenum e = glGetError() ;
	if ( GL_NO_ERROR != e )
	{
		fprintf( stderr, "OpenGL error in \"%s\": %s (%d)\n", strDesc.c_str(), gluErrorString( e ), e ) ;
		exit( 20 ) ;
	}
}

GLuint	genTexture( int w, int h ) 
{
	// create a single float channel w x h texture
	GLuint	texHandle ;
	glGenTextures( 1, &texHandle ) ;
	checkError( "Gen texture" ) ;

	glActiveTexture( GL_TEXTURE0 ) ;
	checkError( "Gen texture" ) ;

	glBindTexture( GL_TEXTURE_2D, texHandle ) ;
	checkError( "Gen texture" ) ;

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) ;
	checkError( "Gen texture" ) ;

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) ;
	checkError( "Gen texture" ) ;

	glTexImage2D( GL_TEXTURE_2D, 0, GL_R32F, w, h, 0, GL_RED, GL_FLOAT, NULL ) ;
	checkError( "Gen texture" ) ;

	// because we're also using this texture as an image in order to write to it.
	// bind it to an image until as well
	glBindImageTexture( 0, texHandle, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F ) ;
	checkError( "Gen texture" ) ;

	return texHandle ;
}

GLuint	genRenderProgram( GLuint texHandle )
{
	GLuint program = glCreateProgram() ;
	GLuint vs = glCreateShader( GL_VERTEX_SHADER ) ;
	GLuint fs = glCreateShader( GL_FRAGMENT_SHADER ) ;

	const char* vsSrc[] =
	{
		"#version 430 \n",
		"in vec2 pos ; \
		 out vec2 texCoord ; \
		 void main() \
		 { \
			texCoord = pos * 0.5f + 0.5f ; \
			gl_Position = vec4( pos.x, pos.y, 0.0, 1.0 ) ; \
		 }"
	} ;

	const char* fsSrc[] =
	{
		"#version 430 \n",
		"uniform sampler2D srcTex ; \
		 in vec2 texCoord ; \
		 out vec4 color ; \
		 void main() \
		 { \
			float c = texture( srcTex, texCoord ).x ; \
			color = vec4( c, 1.0, 1.0, 1.0 ) ; \
		 }"
	} ;

	glShaderSource( vs, 2, vsSrc, NULL ) ;
	glShaderSource( fs, 2, fsSrc, NULL ) ;

	
	glCompileShader( vs ) ;
	int ret ;
	glGetShaderiv( vs, GL_COMPILE_STATUS, &ret ) ;
	if ( !ret )
	{
		fprintf( stderr, "Error in compile vertex shader\n" ) ;
		exit( 30 ) ;
	}
	glAttachShader( program, vs ) ;

	glCompileShader( fs ) ;
	glGetShaderiv( fs, GL_COMPILE_STATUS, &ret ) ;
	if ( !ret )
	{
		fprintf( stderr, "Error in compile fragment shader\n" ) ;
		exit( 31 ) ;
	}
	glAttachShader( program, fs ) ;


	glBindFragDataLocation( program, 0, "color" ) ;
	glLinkProgram( program ) ;

	glGetProgramiv( program, GL_LINK_STATUS, &ret ) ;
	if ( !ret )
	{
		fprintf( stderr, "Error in linking shader program\n" ) ;
		exit( 32 ) ;
	}


	glUseProgram( program ) ;
	glUniform1i( glGetUniformLocation( program, "srcTex" ), 0 ) ;

	GLuint vertexArray ;
	glGenVertexArrays( 1, &vertexArray ) ;
	glBindVertexArray( vertexArray ) ;

	GLuint	posBuffer ;
	glGenBuffers( 1, &posBuffer ) ;
	glBindBuffer( GL_ARRAY_BUFFER, posBuffer ) ;

	float data[] =
	{
		-1.0f, -1.0f,
		-1.0f, 1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f
	} ;

	glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * 8, data, GL_STREAM_DRAW ) ;

	GLint posPtr = glGetAttribLocation( program, "pos" ) ;
	glVertexAttribPointer( posPtr, 2, GL_FLOAT, GL_FALSE, 0, 0 ) ;
	glEnableVertexAttribArray( posPtr ) ;

	checkError( "Render shaders" ) ;

	return program ;
}

GLuint	genComputeProgram( GLuint texHandle )
{
	// create the compute shader and the program object containing the shader
	GLuint program = glCreateProgram() ;
	GLuint cs = glCreateShader( GL_COMPUTE_SHADER ) ;


	// in order to write to a texture, we have to introduce it as image2D.
	// local_siz_x/y/z layout variables define the work group size.
	// gl_GlobalInvocationID is a uvec3 variable giving the global ID of the thread.
	// gl_LocalInvocationID is the local index within the work group, and
	// gl_WorkGroupID is the work group's index

	const char* src[] =
	{
		"#version 430 \n",
		"uniform float roll ;\
		 uniform image2D destTex ; \
		 layout ( local_size_x = 16, local_size_y = 16 ) in ; \
		 void main() \
		 { \
			ivec2 storePos = ivec2( gl_GlobalInvocationID.xy ) ; \
			float localCoef = length( vec2( ivec2( gl_LocalInvocationID.xy ) - 8 ) / 8.0 ) ; \
			float globalCoef = sin( float( gl_WorkGroupID.x + gl_WorkGroupID. y ) * 0.1 + roll ) * 0.5 ; \
			imageStore( destTex, storePos, vec4( 1.0 - globalCoef * localCoef, 0.0, 0.0, 0.0 ) ) ; \
		 }"
	} ;


	glShaderSource( cs, 2, src, NULL ) ;
	
	glCompileShader( cs ) ;
	int ret ;
	glGetShaderiv( cs, GL_COMPILE_STATUS, &ret ) ;
	if ( !ret )
	{
		fprintf( stderr, "Error in compiling the compute shader\n" ) ;
		GLchar szLog[ 10240 ] ;
		GLsizei length ;
		glGetShaderInfoLog( cs, 10239, &length, szLog ) ;
		fprintf( stderr, "Compiler log:\n%s\n", szLog ) ;
		exit( 40 ) ;
	}
	glAttachShader( program, cs ) ;
	   

	glLinkProgram( program ) ;
	glGetProgramiv( program, GL_LINK_STATUS, &ret ) ;
	if ( !ret )
	{
		fprintf( stderr, "Error in linking the compute shader\n" ) ;
		GLchar szLog[ 10240 ] ;
		GLsizei length ;
		glGetShaderInfoLog( cs, 10239, &length, szLog ) ;
		fprintf( stderr, "Linker log:\n%s\n", szLog ) ;
		exit( 41 ) ;
	}

	glUseProgram( program ) ;
	
	glUniform1i( glGetUniformLocation( program, "destTex" ), 0 ) ;

	checkError( "Compute shader" ) ;

	return program ;
}

void	updateTexture( int frame )
{
	glUseProgram( g_computeShader ) ;
	glUniform1f( glGetUniformLocation( g_computeShader, "roll" ), (float)frame * 0.01f ) ;
	glDispatchCompute( 512 / 16, 512 / 16, 1 ) ;	// 512 x 512 threads in blocks of 16 x 16

	checkError( "Dispatch compute shader" ) ;
}

void	draw()
{
	glClearColor( 0.0, 0.0, 0.0, 1.0 ) ;
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) ;

	glUseProgram( g_renderShader ) ;
	glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 ) ;

	glutSwapBuffers() ;

	checkError( "Draw screen" ) ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void	init()
{
	g_texture = genTexture( 512, 512 ) ;
	g_renderShader = genRenderProgram( g_texture ) ;
	g_computeShader = genComputeProgram( g_texture ) ;

	g_index = 0 ;
}

void	display()
{
	if ( g_index < 1024 )
	{
		updateTexture( g_index ) ;

		++g_index ;
	}

	draw() ;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

void	destroyShaders()
{
/*/
	GLenum ErrorCheckValue = glGetError() ;

	glUseProgram( 0 ) ;

	glDetachShader( g_IDShader, g_vertexShaderID ) ;
	glDetachShader( g_IDShader, g_geometryShaderID ) ;
	glDetachShader( g_IDShader, g_fragmentShaderID ) ;

	glDeleteShader( g_vertexShaderID ) ;
	glDeleteShader( g_geometryShaderID ) ;
	glDeleteShader( g_fragmentShaderID ) ;

	glDeleteProgram( g_IDShader ) ;
//*/

	glDeleteProgram( g_renderShader ) ;
	glDeleteProgram( g_computeShader ) ;
}

void	cleanup()
{
	destroyShaders() ;
}

void	calculateFPS()
{
	static int  timePreviousFrame = glutGet( GLUT_ELAPSED_TIME ) ;
	static int	frameCount = 0 ;
	int			timeCurrentFrame ;

	++frameCount ;
	
	// Number of milliseconds since  glutInit called (or first call to  glutGet(GLUT_ELAPSED_TIME)).
	timeCurrentFrame = glutGet( GLUT_ELAPSED_TIME ) ;
	
	int timeElapsed = timeCurrentFrame - timePreviousFrame ;
	if ( timeElapsed > 1000 / 360 )
	{
		g_FPS = 1000.0 * frameCount / timeElapsed ;
		timePreviousFrame = timeCurrentFrame ;
		frameCount = 0 ;
		//printf( "FPS: %.2lf \n", g_FPS ) ;
	}
}

// Helper function
int	min( int x, int y )		{	if ( x < y ) return x ;	else return y ;	}

void	idle()
{
	calculateFPS() ;
	glutPostRedisplay() ;
}

void	keySelect( unsigned char key, int x, int y )
{
	switch( key )
	{
		case 27 :
		{
			exit( 0 ) ;
			cleanup() ;
		}
		break ;
	}

	glutPostRedisplay() ;
}


void	reshape( int width, int height )
{
	double dWidth, dHeight ;

	int LM_WH = min( width, height ) ;

	dWidth = LM_WH ;
	dHeight = LM_WH ;

	glViewport( ( width - dWidth ) / 2.0, ( height - dHeight ) / 2.0, dWidth, dHeight ) ;
}


int		main( int argc, char** argv )
{
	glutInit				( &argc, argv ) ;
	
	/////////////////////////////////////////////////////////////////
	// Set the bit mask to select a double buffered window. 
	/////////////////////////////////////////////////////////////////
	glutInitDisplayMode		( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE ) ;
	
	glutInitWindowSize		( g_width, g_height ) ;

	glutSetOption			( GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS ) ;
	glutCloseFunc			( cleanup ) ;

	glutInitContextVersion	( 4, 3 ) ;
	checkError( "Gen texture" ) ;
	glutInitContextFlags	( GLUT_FORWARD_COMPATIBLE ) ;
	glutInitContextProfile	( GLUT_CORE_PROFILE ) ;
	
	char szTitle[ 256 ] ;
	sprintf( szTitle, "Lightning rendering - ETA : %d", 0 ) ;
	glutCreateWindow		( szTitle );

	//////////////////////////////////////////////
	glewExperimental = GL_TRUE ;
	//////////////////////////////////////////////

	glewInit				( ) ;
	init					( ) ;
	
	glutDisplayFunc			( display ) ;
	glutReshapeFunc			( reshape ) ;

	glutIdleFunc			( idle ) ;

	glutKeyboardFunc		( keySelect ) ;
	glutMainLoop			( ) ;

	return 0 ;
}

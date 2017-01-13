//#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")

#include "Base.h"
#include "Mesh.h"
#include "Shader.h"


#define USE_OUR_METHOD
//#define USE_CGM_3D

#ifdef	USE_OUR_METHOD
	#include "lightning_generator_3D.h"
	using namespace Lightning3D ;
#endif

#ifdef USE_CGM_3D
	#include "cgm_map.h"
	using namespace Cgm3D ;
#endif
	

#include <vector>
#include <random>
#include <ctime>

using namespace glm ;
using namespace std ;


#define ETA						2
#define RHO						3

#define BASE_THICKNESS			4//3
#define ITENSITY_ATTENUATION	0.8f
//#define ITENSITY_ATTENUATION	1.0f

#define SCENE_SIZE				720.0f
#define HALF_SCENE_SIZE			SCENE_SIZE / 2.0f

#define ELPSILON				0.0001f

//#define USE_WHITE_BACKGROUND

//#define RECURSIVE_HALF_THICKNESS
#define FIXED_HALF_THICKNESS


// --------------------------------------------------------------------------------------------------------------------
// for opengl & shader

float degree = 0.0 ;
float prevDegree = 0.0 ;
float constant = 0.75 ;
GLfloat				g_theta = 0.0 ;

bool bLeftClick = false ;
//float fZoom = 1.0f ;
float fOldX = 0 ;
float fOldY = 0 ;
float fAngleV = 0.0f ;
float fAngleH = 0.0f ;
float fOldAngleV = 0.0f ;
float fOldAngleH = 0.0f ;
vec3 vOldTo ;

// View matrix
vec3 eye = vec3( -8.468f, 1.932f, 17.992f ) ;
vec3 to = vec3( -8.001f, 1.803f, 17.116f ) ;
vec3 up = vec3( 0.0, 1.0, 0.0 ) ;
float fZoom = 0.013636f ;



// control values
bool				g_bDrawGrid = false ;
bool				g_bDrawStartEndPoints = true ;
bool				g_bDrawObstacles = false ;
bool				g_bDrawBoundary = true ;
bool				g_bProcessStarted = false ;
bool				g_bJitter = true ;
bool				g_bBloom = true ;
bool				g_bApplyThickLine = true ;
int					g_fGridScale = 1.0f ;
float				g_fExposure = 1.0f ;

//vec4				g_vLightningColor = vec4( 0.0f, 0.0f, 1.0f, 1.0f ) ;	// blue
//vec4				g_vLightningColor = vec4( 1.0f, 0.0f, 0.0f, 1.0f ) ;	// red
//vec4				g_vLightningColor = vec4( 1.0f, 1.0f, 1.0f, 1.0f ) ;	// white
//vec4				g_vLightningColor = vec4( 0.47f, 0.08f, 0.43f, 1.0f ) ;	// purple
vec4				g_vLightningColor = vec4( 0.7f, 0.6f, 1.0f, 1.0f ) ;	// white


// mesh objects
Mesh				g_meshGrid ;
Mesh				g_meshStartPos ;
Mesh				g_meshEndPos ;
Mesh				g_meshBoundary ;
Mesh				g_meshScene ;
Mesh				g_meshObstacles ;
Mesh				g_meshLightningRect ;
Mesh				g_meshLightningLine ;
Mesh				g_meshLightningCylinder ;
Mesh				g_meshLightningSphere ;

GLuint				g_texScene ;

#ifdef USE_OUR_METHOD
	LightningGenerator3D	g_lightningGenerator ;
#endif

#ifdef USE_CGM_3D
	CGMMap					g_lightningGenerator ;
#endif

// frame buffers to divide scene & lightning
GLuint				g_FBO ;
GLuint				g_colorBuffers[ 2 ] ;		// 0: scene, 1: lightning line for blurring

// frame buffers to blur lightning line (two-pass Gaussian Blur)
GLuint				g_blurFBO[ 2 ] ;			// toggle to use as frame buffer or texture
GLuint				g_blurColorBuffers[ 2 ] ;

// for quad
GLuint				g_quadVAO = 0 ;
GLuint				g_quadVBO ;


// shader ID
GLuint				g_shaderGrid ;		// shader for grid rendering
GLuint				g_vsGrid ;
GLuint				g_fsGrid ;

GLuint				g_shaderPoints ;	// shader for start & end points rendering
GLuint				g_vsPoints ;
GLuint				g_fsPoints ;

GLuint				g_shaderScene ;		// shader for scene rendering
GLuint				g_vsScene ;
GLuint				g_fsScene ;

GLuint				g_shaderObstacles ;	// shader for obstacle rendering
GLuint				g_vsObstacles ;
GLuint				g_fsObstacles ;

GLuint				g_shaderLightning ;	// shader for lightning line rendering
GLuint				g_vsLightning ;
GLuint				g_fsLightning ;

GLuint				g_shaderLightningCylinder ;	// shader for lightning line rendering (by cylinder & sphere)
GLuint				g_vsLightningCylinder ;
GLuint				g_gsLightningCylinder ;
GLuint				g_fsLightningCylinder ;

GLuint				g_shaderLightningSphere ;	// shader for lightning line rendering (by cylinder & sphere)
GLuint				g_vsLightningSphere ;
GLuint				g_gsLightningSphere ;
GLuint				g_fsLightningSphere ;

GLuint				g_shaderBlur ;		// shader for lightning line blurring
GLuint				g_vsBlur ;
GLuint				g_fsBlur ;

GLuint				g_shaderQuad ;		// shader for final quad rendering
GLuint				g_vsQuad ;
GLuint				g_fsQuad ;

GLuint				g_shaderDebug ;		// shader to show texture for debugging
GLuint				g_vsDebug ;
GLuint				g_fsDebug ;


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
void	setLight( GLuint shader ) ;
void	setModel( GLuint shader ) ;
void	setView( GLuint shader ) ;
void	setProjection( GLuint shader ) ;

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
void	deleteShader( GLuint shader, GLuint vs, GLuint fs, GLuint gs = -1 ) ;
void	destroyShaders() ;
vec2	newXY( int x, int y ) ;

// --------------------------------------------------------------------------------------------------------------------

void	generate_framebuffers() ;		// generate frame buffers to divide rendering scene & brighter
void	generate_blur_framebuffers() ;	// generate frame buffer for blurring the brighter

void	initGrid() ;
void	drawGrid() ;
void	drawBoundary() ;

void	initStartEndPoints() ;
void	drawStartEndPoints() ;

void	initObstacles() ;
void	drawObstalces() ;

void	initScene() ;
void	drawScene() ;

void	initLightning() ;
void	updateLightning() ;
void	drawLightning() ;

void	drawTexture( GLuint texture ) ;	// for debugging, draw quad with texture
void	drawQuad() ;

void	AddCircle( Mesh& mesh, float cx, float cy, float cz, float fRadius, int iSegment, float fIntensity = 1.0f ) ;
void	AddCube( Mesh& mesh, float cx, float cy, float cz, float fSize ) ;
void	AddCylinder( Mesh& mesh, float sx, float sy, float sz, float ex, float ey, float ez, float fRadius, int iSegment, float fStartIntensity, float fEndIntensity ) ;
void	AddSphere( Mesh& mesh, float sx, float sy, float sz, float fRadius, int iSegment, float fIntensity = 1.0f ) ;



bool	LoadTextureFromBMP( const char* pszImagePath, GLuint& textureID, bool bGenMipMap = false ) ;

void	CheckNumberOfBranches( int iBranches ) ;


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
// Setting light's modelMatrix
void	setLight( GLuint shader )
{
	mat4 lightmodelMat( 1.0f ) ;	// identity matrix
	glUniformMatrix4fv( glGetUniformLocation( shader, "mlModel" ), 1, GL_FALSE, value_ptr( lightmodelMat ) ) ;
}

// Setting object's modelMatrix
void	setModel( GLuint shader )
{
	mat4 modelMat( 1.0f ) ;
	glUniformMatrix4fv( glGetUniformLocation( shader, "mModel" ), 1, GL_FALSE, value_ptr( modelMat ) ) ;
}

// Setting object's viewMatrix
void	setView( GLuint shader )
{
/*/
	float fZoom = 1.0f ;

	vec3 eye = vec3( -1.0, 0.0, 0.0 ) ;
	vec3 to = vec3( 1.0, 0.0, 0.0 ) ;
	vec3 up = vec3( 0.0, 1.0, 0.0 ) ;
//*/

	// World to Camera(Eye - wc)
	mat4 matScale = glm::mat4( fZoom ) ;
	matScale[ 3 ][ 3 ] = 1.0f ;

	mat4 viewMat = glm::lookAt( eye, to, up ) * matScale ;
//	mat4 viewMat = glm::lookAt( eye, to, up ) ;
	glUniformMatrix4fv( glGetUniformLocation( shader, "mView" ), 1, GL_FALSE, value_ptr( viewMat ) ) ;
}

mat4 Perspective( const GLfloat fovy, const GLfloat aspect, const GLfloat zNear, const GLfloat zFar)
{
	const GLfloat  DegreesToRadians = (GLfloat)( 3.141592f / 180.0f ) ;

    GLfloat top   = tan( fovy * DegreesToRadians / 2 ) * zNear;
    GLfloat right = top * aspect;

    mat4 c;
    c[0][0] = zNear/right;
    c[1][1] = zNear/top;
    c[2][2] = -(zFar + zNear)/(zFar - zNear);
    c[2][3] = -2.0*zFar*zNear/(zFar - zNear);
    c[3][2] = -1.0;
    return c;
}

// Setting object's projectionMatrix
void	setProjection( GLuint shader )
{
	const GLfloat  DegreesToRadians = (GLfloat)( 3.141592f / 180.0f ) ;

	// Camera to Clipping
	mat4 projectionMat = glm::perspective( 45.0f * DegreesToRadians, (float)g_width / (float)g_height, 1.0f, 1000.0f ) ;
	glUniformMatrix4fv( glGetUniformLocation( shader, "mProjection" ), 1, GL_FALSE, value_ptr( projectionMat ) ) ;

	CheckError() ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void	generate_framebuffers()
{
	glGenFramebuffers( 1, &g_FBO ) ;
	glBindFramebuffer( GL_FRAMEBUFFER, g_FBO ) ;

	// create 2 floating point color buffers (1 for common rendering, other for brightness threshold values)
	glGenTextures( 2, g_colorBuffers ) ;
	for ( int i = 0; i < 2; ++i )
	{
		glBindTexture( GL_TEXTURE_2D, g_colorBuffers[ i ] ) ;
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB16F, g_width, g_height, 0, GL_RGB, GL_FLOAT, NULL) ;
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) ;
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) ;
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ) ;  // We clamp to the edge as the blur filter would otherwise sample repeated texture values!
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE ) ;
		
		// attach texture to framebuffer
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, g_colorBuffers[ i ], 0 ) ;
	}

	// create & attach other depth buffer (renderbuffer)
	GLuint rboDepth ;
	glGenRenderbuffers( 1, &rboDepth ) ;
	glBindRenderbuffer( GL_RENDERBUFFER, rboDepth ) ;
	glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT, g_width, g_height ) ;
	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth ) ;

	// - Tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	GLuint attachments[ 2 ] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 } ;
	glDrawBuffers( 2, attachments ) ;

	// - Finally check if framebuffer is complete
	if ( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
	{
		std::cout << "Framebuffer not complete!" << std::endl ;
	}

	glBindFramebuffer( GL_FRAMEBUFFER, 0 ) ;
}

void	generate_blur_framebuffers()
{
	// create toggle framebuffers for blurring
	glGenFramebuffers( 2, g_blurFBO ) ;
	glGenTextures( 2, g_blurColorBuffers ) ;

	for ( int i = 0; i < 2; ++i )
	{
		glBindFramebuffer( GL_FRAMEBUFFER, g_blurFBO[ i ] ) ;
		
		glBindTexture( GL_TEXTURE_2D, g_blurColorBuffers[ i ] ) ;
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB16F, g_width, g_height, 0, GL_RGB, GL_FLOAT, NULL ) ;
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) ; 
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) ;
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ) ; // We clamp to the edge as the blur filter would otherwise sample repeated texture values!
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE ) ;
		
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_blurColorBuffers[ i ], 0 ) ;
		
		// Also check if framebuffers are complete (no need for depth buffer)
		if ( glCheckFramebufferStatus( GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE )
		{
			std::cout << "Framebuffer not complete!" << std::endl ;
		}
	}
}

void	initGrid()
{
	g_meshGrid.clearBuffer() ;
	g_meshBoundary.clearBuffer() ;

	int iGridSize = g_lightningGenerator.GetGridSize() ;

	float fDiff = ( -( SCENE_SIZE ) / iGridSize ) / g_fGridScale ;
	float x ;
	float y ;
	float z ;

	int iDrawGridSize = iGridSize * g_fGridScale ;

	for ( int k =0; k < iDrawGridSize + 1; ++k )
	{
		z = fDiff * k + ( HALF_SCENE_SIZE ) ;

		for ( int i = 0; i < iDrawGridSize + 1; ++i )
		{
			y = fDiff * i + ( HALF_SCENE_SIZE ) ;

			g_meshGrid.m_vVertices.push_back( vec4( -( HALF_SCENE_SIZE ), y, z, 1.0f ) ) ;
			g_meshGrid.m_vVertices.push_back( vec4( ( HALF_SCENE_SIZE ), y, z, 1.0f ) ) ;
		}

		for ( int i = 0; i < iDrawGridSize + 1; ++i )
		{
			x = -fDiff * i - ( HALF_SCENE_SIZE ) ;

			g_meshGrid.m_vVertices.push_back( vec4( x, -( HALF_SCENE_SIZE ), z, 1.0f ) ) ;
			g_meshGrid.m_vVertices.push_back( vec4( x, ( HALF_SCENE_SIZE ), z, 1.0f ) ) ;
		}
	}

	for ( int k =0; k < iDrawGridSize + 1; ++k )
	{
		x = fDiff * k + ( HALF_SCENE_SIZE ) ;

		for ( int i = 0; i < iDrawGridSize + 1; ++i )
		{
			y = fDiff * i + ( HALF_SCENE_SIZE ) ;

			g_meshGrid.m_vVertices.push_back( vec4( x, y, ( HALF_SCENE_SIZE ), 1.0f ) ) ;
			g_meshGrid.m_vVertices.push_back( vec4( x, y, -( HALF_SCENE_SIZE ), 1.0f ) ) ;
		}
	}

	g_meshGrid.setDrawLines( true ) ;
	g_meshGrid.setArray() ;
	g_meshGrid.setBuffer() ;

	glUseProgram( g_shaderGrid ) ;

	GLuint vPosition = glGetAttribLocation( g_shaderGrid, "vPosition" ) ;
	g_meshGrid.setAttrLocPosition( vPosition ) ;

	GLuint vColor = glGetUniformLocation( g_shaderGrid, "vColor" ) ;
	g_meshGrid.setUniformLocDiffuse( vColor, vec4( 0.3f, 0.3f, 0.3f, 1.0f ) ) ;



	// generate map boundary
	g_meshBoundary.clearBuffer() ;

	g_meshBoundary.m_vVertices.push_back( vec4( -( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), 1.0f ) ) ;
	g_meshBoundary.m_vVertices.push_back( vec4( ( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), 1.0f ) ) ;

	g_meshBoundary.m_vVertices.push_back( vec4( ( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), 1.0f ) ) ;
	g_meshBoundary.m_vVertices.push_back( vec4( ( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), 1.0f ) ) ;

	g_meshBoundary.m_vVertices.push_back( vec4( ( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), 1.0f ) ) ;
	g_meshBoundary.m_vVertices.push_back( vec4( -( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), 1.0f ) ) ;

	g_meshBoundary.m_vVertices.push_back( vec4( -( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), 1.0f ) ) ;
	g_meshBoundary.m_vVertices.push_back( vec4( -( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), 1.0f ) ) ;


	g_meshBoundary.m_vVertices.push_back( vec4( -( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), 1.0f ) ) ;
	g_meshBoundary.m_vVertices.push_back( vec4( ( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), 1.0f ) ) ;

	g_meshBoundary.m_vVertices.push_back( vec4( ( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), 1.0f ) ) ;
	g_meshBoundary.m_vVertices.push_back( vec4( ( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), 1.0f ) ) ;

	g_meshBoundary.m_vVertices.push_back( vec4( ( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), 1.0f ) ) ;
	g_meshBoundary.m_vVertices.push_back( vec4( -( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), 1.0f ) ) ;

	g_meshBoundary.m_vVertices.push_back( vec4( -( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), 1.0f ) ) ;
	g_meshBoundary.m_vVertices.push_back( vec4( -( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), 1.0f ) ) ;


	g_meshBoundary.m_vVertices.push_back( vec4( -( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), 1.0f ) ) ;
	g_meshBoundary.m_vVertices.push_back( vec4( -( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), 1.0f ) ) ;


	g_meshBoundary.m_vVertices.push_back( vec4( ( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), 1.0f ) ) ;
	g_meshBoundary.m_vVertices.push_back( vec4( ( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), 1.0f ) ) ;


	g_meshBoundary.m_vVertices.push_back( vec4( ( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), 1.0f ) ) ;
	g_meshBoundary.m_vVertices.push_back( vec4( ( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), 1.0f ) ) ;


	g_meshBoundary.m_vVertices.push_back( vec4( -( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), 1.0f ) ) ;
	g_meshBoundary.m_vVertices.push_back( vec4( -( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), 1.0f ) ) ;


	g_meshBoundary.setDrawLines( true ) ;
	g_meshBoundary.setArray() ;
	g_meshBoundary.setBuffer() ;

	g_meshBoundary.setAttrLocPosition( vPosition ) ;
	g_meshBoundary.setUniformLocDiffuse( vColor, vec4( 0.3f, 0.3f, 0.3f, 1.0f ) ) ;
}

void	drawGrid()
{
	glUseProgram( g_shaderGrid ) ;

	setLight( g_shaderGrid ) ;
	setModel( g_shaderGrid ) ;
	setView( g_shaderGrid ) ;
	setProjection( g_shaderGrid ) ;

	g_meshGrid.drawBuffer() ;
}

void	drawBoundary()
{
	glUseProgram( g_shaderGrid ) ;

	setLight( g_shaderGrid ) ;
	setModel( g_shaderGrid ) ;
	setView( g_shaderGrid ) ;
	setProjection( g_shaderGrid ) ;

	g_meshBoundary.drawBuffer() ;
}

void	initStartEndPoints()
{
	glUseProgram( g_shaderPoints ) ;

	int iGridSize = g_lightningGenerator.GetGridSize() ;

	float fDiff = -( SCENE_SIZE ) / iGridSize ;
	float fHalfSize = -fDiff / 2.0f ;
	float x ;
	float y ;
	float z ;

	int iStartCount = g_lightningGenerator.GetStartCellCount() ;
	if ( iStartCount > 0 )
	{
		for ( int i = 0; i < iStartCount; ++i )
		{
#ifdef	USE_OUR_METHOD
			const Cell* pCell = g_lightningGenerator.GetStartCell( i ) ;
#endif

#ifdef USE_CGM_3D
			const SimpleCell* pCell = g_lightningGenerator.GetStartCell( i ) ;
#endif

			if ( pCell )
			{
				x = -fDiff * pCell->m_iX - ( HALF_SCENE_SIZE ) + fHalfSize ;
				y = fDiff * pCell->m_iY + ( HALF_SCENE_SIZE ) - fHalfSize ;
				z = fDiff * pCell->m_iZ + ( HALF_SCENE_SIZE ) - fHalfSize ;

				AddCube( g_meshStartPos, x, y, z, fDiff ) ;
			}
		}

		g_meshStartPos.setArray() ;
		g_meshStartPos.setBuffer() ;

		GLuint vPosition = glGetAttribLocation( g_shaderPoints, "vPosition" ) ;
		g_meshStartPos.setAttrLocPosition( vPosition ) ;

		GLuint vColor = glGetUniformLocation( g_shaderPoints, "vColor" ) ;
		g_meshStartPos.setUniformLocDiffuse( vColor, vec4( 1.0f, 0.0f, 0.0f, 1.0f ) ) ;
	}

	int iEndCount = g_lightningGenerator.GetEndCellCount() ;
	if ( iEndCount > 0 )
	{
		for ( int i = 0; i < iEndCount; ++i )
		{
#ifdef USE_OUR_METHOD
			const Cell* pCell = g_lightningGenerator.GetEndCell( i ) ;
#endif

#ifdef USE_CGM_3D
			const SimpleCell* pCell = g_lightningGenerator.GetEndCell( i ) ;
#endif
			if ( pCell )
			{
				x = -fDiff * pCell->m_iX - ( HALF_SCENE_SIZE ) + fHalfSize ;
				y = fDiff * pCell->m_iY + ( HALF_SCENE_SIZE ) - fHalfSize ;
				z = fDiff * pCell->m_iZ + ( HALF_SCENE_SIZE ) - fHalfSize ;

				AddCube( g_meshEndPos, x, y, z, fDiff ) ;
			}
		}

		g_meshEndPos.setArray() ;
		g_meshEndPos.setBuffer() ;

		GLuint vPosition = glGetAttribLocation( g_shaderPoints, "vPosition" ) ;
		g_meshEndPos.setAttrLocPosition( vPosition ) ;

		GLuint vColor = glGetUniformLocation( g_shaderPoints, "vColor" ) ;
		g_meshEndPos.setUniformLocDiffuse( vColor, vec4( 0.0f, 0.0f, 1.0f, 1.0f ) ) ;
	}
}

void	drawStartEndPoints()
{
	glUseProgram( g_shaderPoints ) ;

	setLight( g_shaderPoints ) ;
	setModel( g_shaderPoints ) ;
	setView( g_shaderPoints ) ;
	setProjection( g_shaderPoints ) ;

	// draw 
	g_meshStartPos.drawBuffer() ;
	g_meshEndPos.drawBuffer() ;
}


void	initObstacles()
{
	glUseProgram( g_shaderObstacles ) ;

	// there are only obstacle objects in the scene or empty scene
	int iCount = g_lightningGenerator.GetObstacleCellCount() ;
	if ( iCount > 0 )
	{
		int iGridSize = g_lightningGenerator.GetGridSize() ;

		float fDiff = -( SCENE_SIZE ) / iGridSize ;
		float fHalfSize = -fDiff / 2.0f ;
		float x ;
		float y ;
		float z ;

		for ( int i = 0; i < iCount; ++i )
		{
#ifdef USE_OUR_METHOD 
			const Cell* pCell = g_lightningGenerator.GetObstacleCell( i ) ;
#endif

#ifdef USE_CGM_3D
			const SimpleCell* pCell = g_lightningGenerator.GetObstacleCell( i ) ;
#endif
			
			if ( pCell )
			{
				x = -fDiff * pCell->m_iX - ( HALF_SCENE_SIZE ) + fHalfSize ;
				y = fDiff * pCell->m_iY + ( HALF_SCENE_SIZE ) - fHalfSize ;
				z = fDiff * pCell->m_iZ + ( HALF_SCENE_SIZE ) - fHalfSize ;

				AddCube( g_meshObstacles, x, y, z, fDiff ) ;
			}
		}

		g_meshObstacles.setArray() ;
		g_meshObstacles.setBuffer() ;

		GLuint vPosition = glGetAttribLocation( g_shaderObstacles, "vPosition" ) ;
		g_meshObstacles.setAttrLocPosition( vPosition ) ;

		GLuint vColor = glGetUniformLocation( g_shaderObstacles, "vColor" ) ;
		g_meshObstacles.setUniformLocDiffuse( vColor, vec4( 0.8f, 0.45f, 0.24f, 1.0f ) ) ;
	}
}

void	drawObstacles()
{
	glUseProgram( g_shaderObstacles ) ;

	setLight( g_shaderObstacles ) ;
	setModel( g_shaderObstacles ) ;
	setView( g_shaderObstacles ) ;
	setProjection( g_shaderObstacles ) ;

	if ( !g_meshObstacles.m_vVertices.empty() )
	{
		g_meshObstacles.drawBuffer() ;
	}
}

void	initScene()
{
	glUseProgram( g_shaderScene ) ;

	g_meshScene.m_vVertices.push_back( vec4( -( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), 0.0f, 1.0f ) ) ;		g_meshScene.m_vTexCoords.push_back( vec2( 0, 1 ) ) ;
	g_meshScene.m_vVertices.push_back( vec4( -( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), 0.0f, 1.0f ) ) ;		g_meshScene.m_vTexCoords.push_back( vec2( 0, 0 ) ) ;
	g_meshScene.m_vVertices.push_back( vec4( ( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), 0.0f, 1.0f ) ) ;			g_meshScene.m_vTexCoords.push_back( vec2( 1, 1 ) ) ;

	g_meshScene.m_vVertices.push_back( vec4( -( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), 0.0f, 1.0f ) ) ;		g_meshScene.m_vTexCoords.push_back( vec2( 0, 0 ) ) ;
	g_meshScene.m_vVertices.push_back( vec4( ( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), 0.0f, 1.0f ) ) ;			g_meshScene.m_vTexCoords.push_back( vec2( 1, 1 ) ) ;
	g_meshScene.m_vVertices.push_back( vec4( ( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), 0.0f, 1.0f ) ) ;		g_meshScene.m_vTexCoords.push_back( vec2( 1, 0 ) ) ;

	g_meshScene.setArray() ;
	g_meshScene.setBuffer() ;

	GLuint vPosition = glGetAttribLocation( g_shaderScene, "vPosition" ) ;
	g_meshScene.setAttrLocPosition( vPosition ) ;

	GLuint vTexCoords = glGetAttribLocation( g_shaderScene, "texCoords" ) ;
	g_meshScene.setAttrLocTexCoord( vTexCoords ) ;

	GLuint vColor = glGetUniformLocation( g_shaderScene, "vColor" ) ;
#ifdef USE_WHITE_BACKGROUND
	g_meshScene.setUniformLocDiffuse( vColor, vec4( 1.0f, 1.0f, 1.0f, 1.0f ) ) ;	// white rect scene
#else
	g_meshScene.setUniformLocDiffuse( vColor, vec4( 0.0f, 0.0f, 0.0f, 1.0f ) ) ;	// black rect scene
#endif
}

void	drawScene()
{
	glUseProgram( g_shaderScene ) ;

	GLint texLoc = glGetUniformLocation( g_shaderScene, "textureScene" ) ;
	glUniform1i( texLoc, 0 ) ;

	glActiveTexture( GL_TEXTURE0 ) ;
	glBindTexture( GL_TEXTURE_2D, g_texScene ) ;
	
	setLight( g_shaderScene ) ;
	setModel( g_shaderScene ) ;
	setView( g_shaderScene ) ;
	setProjection( g_shaderScene ) ;

	if ( !g_meshScene.m_vVertices.empty() )
	{
		g_meshScene.drawBuffer() ;
	}
}

void	initLightning()
{
	g_bProcessStarted = true ;

//	std::string strTimeLog = "timeLog.txt" ;
//	g_lightningGenerator.Process( E_PT_ALL_STEP, true, strTimeLog ) ;
	
int timeStart = glutGet( GLUT_ELAPSED_TIME ) ;

	g_lightningGenerator.Process( E_PT_ALL_STEP ) ;

int timeEnd = glutGet( GLUT_ELAPSED_TIME ) ;
int timeElapsed = timeEnd - timeStart ;

#ifdef USE_OUR_METHOD
	int timeInit = g_lightningGenerator.GetInitializationTime() ;
	printf( "\n\nTime : %d ms (segments: %d)\n", timeElapsed + timeInit, g_lightningGenerator.GetLightningTree().GetNodeList().size() ) ;
	printf( "  - Initialization : %d ms\n", timeInit ) ;
	printf( "  - Process        : %d ms\n", timeElapsed ) ;
#endif

#ifdef USE_CGM_3D
	printf( "\n\nTime : %d ms (segments: %d)\n", timeElapsed, g_lightningGenerator.GetLightningTree().GetNodeList().size() ) ;
#endif

	//float fd = g_lightningGenerator.CalculateFractalDimension() ;

	updateLightning() ;
}

void	initLightningSteps()
{
	if ( !g_bProcessStarted )
	{
		return ;
	}

//	std::string strTimeLog = "timeLog.txt" ;
//	g_lightningGenerator.Process( E_PT_ALL_STEP, true, strTimeLog ) ;
	
//int timeStart = glutGet( GLUT_ELAPSED_TIME ) ;

	bool bRet = g_lightningGenerator.Process( E_PT_ONE_STEP ) ;
	if ( !bRet )
	{
		char szTitle[ 256 ] ;
#ifdef USE_CGM_3D
		sprintf( szTitle, "Lightning rendering - ETA : %d (Finished)", ETA ) ;
#else
		sprintf( szTitle, "Lightning rendering - ETA : %d, RHO: %d (Finished)", ETA, RHO ) ;
#endif
		glutSetWindowTitle( szTitle );

		return ;
	}

//int timeEnd = glutGet( GLUT_ELAPSED_TIME ) ;
//int timeElapsed = timeEnd - timeStart ;
/*/
#ifdef USE_OUR_METHOD
	int timeInit = g_lightningGenerator.GetInitializationTime() ;
	printf( "\n\nTime : %d ms (segments: %d)\n", timeElapsed + timeInit, g_lightningGenerator.GetLightningTree().GetNodeList().size() ) ;
	printf( "  - Initialization : %d ms\n", timeInit ) ;
	printf( "  - Process        : %d ms\n", timeElapsed ) ;
#endif

#ifdef USE_CGM_3D
	printf( "\n\nTime : %d ms (segments: %d)\n", timeElapsed, g_lightningGenerator.GetLightningTree().GetNodeList().size() ) ;
#endif
//*/

	updateLightning() ;
}

void	updateLightning()
{
	// TODO : it is not optimized !!
	// clear old vertices
	g_meshLightningLine.clearBuffer() ;
	//g_meshLightningRect.clearBuffer() ;
	g_meshLightningCylinder.clearBuffer() ;
	g_meshLightningSphere.clearBuffer() ;


	// create lightning mesh
	float fDiff = -( SCENE_SIZE ) / g_lightningGenerator.GetGridSize() ;
	float fCenter = -fDiff / 2.0f ;
	float sx, sy, sz ;
	float tx, ty, tz ;
	float fThickness ;
	float fStartIntensity ;
	float fEndIntensity ;

	//const LightningTreeNode* pNode ;
	//const LightningTree& tree = g_lightningGenerator.GetLightningTree() ;
	//const std::vector< LightningTreeNode* > vNodes = tree.GetNodeList() ;
	LightningTreeNode* pNode ;
	LightningTree& tree = g_lightningGenerator.GetLightningTree() ;
	std::vector< LightningTreeNode* > vNodes = tree.GetNodeList() ;	// TODO : change this function to const function

	CheckNumberOfBranches( vNodes.size() - 1 ) ;

	std::vector< LightningTreeNode* >::const_iterator itr = vNodes.begin() ;
	while ( itr != vNodes.end() )
	{
		pNode = *itr ;

		if ( pNode && pNode->m_pParent )
		{
#ifdef	RECURSIVE_HALF_THICKNESS
			fThickness = pNode->m_fThickness ;
#else
	#ifdef	FIXED_HALF_THICKNESS
			fThickness = BASE_THICKNESS ;
			if ( false == pNode->m_bMainChannel )
			{
				fThickness /= 2.0f ;
			}
	#else
		fThickness = BASE_THICKNESS ;
	#endif
#endif

			fStartIntensity = pNode->m_pParent->m_fIntensity ;
			fEndIntensity = pNode->m_fIntensity ;

			if ( g_bJitter && pNode->m_pParent->m_bJittered )
			{
				sx = pNode->m_pParent->m_fXJittered ;
				sy = pNode->m_pParent->m_fYJittered ;
				sz = pNode->m_pParent->m_fZJittered ;
			}
			else
			{
				sx = -fDiff * pNode->m_pParent->m_iX - ( HALF_SCENE_SIZE ) + fCenter ;
				sy = fDiff * pNode->m_pParent->m_iY + ( HALF_SCENE_SIZE ) - fCenter ;
				sz = fDiff * pNode->m_pParent->m_iZ + ( HALF_SCENE_SIZE ) - fCenter ;
				
				if ( g_bJitter )
				{
					//float fJitterSize = -fDiff - ( 2 * fThickness ) ;
					//float fJitterSize = -fDiff ;
					float fJitterSize = -fDiff * 2.0f / 3.0f ;
					if ( fJitterSize < 1 )
					{
						fJitterSize = 1 ; 
					}

					sx += rand() % (int)( fJitterSize ) - ( fJitterSize / 2.0f ) ;
					sy += rand() % (int)( fJitterSize ) - ( fJitterSize / 2.0f ) ;
					sz += rand() % (int)( fJitterSize ) - ( fJitterSize / 2.0f ) ;

					pNode->m_pParent->m_bJittered = true ;
					pNode->m_pParent->m_fXJittered = sx ;
					pNode->m_pParent->m_fYJittered = sy ;
					pNode->m_pParent->m_fZJittered = sz ;
				}
			}

			if ( g_bJitter && pNode->m_bJittered )
			{
				tx = pNode->m_fXJittered ;
				ty = pNode->m_fYJittered ;
				tz = pNode->m_fZJittered ;
			}
			else
			{
				tx = -fDiff * pNode->m_iX - ( HALF_SCENE_SIZE ) + fCenter ;
				ty = fDiff * pNode->m_iY + ( HALF_SCENE_SIZE ) - fCenter ;
				tz = fDiff * pNode->m_iZ + ( HALF_SCENE_SIZE ) - fCenter ;

				if ( g_bJitter )
				{
					//float fJitterSize = -fDiff - ( 2 * fThickness ) ;
					//float fJitterSize = -fDiff ;
					float fJitterSize = -fDiff * 2.0f / 3.0f ;
					if ( fJitterSize < 1 )
					{
						fJitterSize = 1 ; 
					}

					tx += rand() % (int)( fJitterSize ) - ( fJitterSize / 2.0f ) ;
					ty += rand() % (int)( fJitterSize ) - ( fJitterSize / 2.0f ) ;
					tz += rand() % (int)( fJitterSize ) - ( fJitterSize / 2.0f ) ;

					pNode->m_bJittered = true ;
					pNode->m_fXJittered = tx ;
					pNode->m_fYJittered = ty ;
					pNode->m_fZJittered = tz ;
				}
			}

			if ( !g_bApplyThickLine || fThickness <= 1.0f )
			{
				g_meshLightningLine.m_vVertices.push_back( vec4( sx, sy, sz, 1.0f ) ) ;	g_meshLightningLine.m_vIntensities.push_back( fStartIntensity ) ;
				g_meshLightningLine.m_vVertices.push_back( vec4( tx, ty, tz, 1.0f ) ) ;	g_meshLightningLine.m_vIntensities.push_back( fEndIntensity ) ;
			}
			else
			{
				float fHalfWidth = fThickness * 0.5f ;

				g_meshLightningCylinder.m_vVertices.push_back( vec4( sx, sy, sz, 1.0f ) ) ;	g_meshLightningCylinder.m_vIntensities.push_back( fStartIntensity ) ;	g_meshLightningCylinder.m_vRadius.push_back( fHalfWidth ) ;
				g_meshLightningCylinder.m_vVertices.push_back( vec4( tx, ty, tz, 1.0f ) ) ;	g_meshLightningCylinder.m_vIntensities.push_back( fEndIntensity ) ;		g_meshLightningCylinder.m_vRadius.push_back( fHalfWidth ) ;

				g_meshLightningSphere.m_vVertices.push_back( vec4( sx, sy, sz, 1.0f ) ) ;	g_meshLightningSphere.m_vIntensities.push_back( fStartIntensity ) ;		g_meshLightningSphere.m_vRadius.push_back( fHalfWidth ) ;
				g_meshLightningSphere.m_vVertices.push_back( vec4( tx, ty, tz, 1.0f ) ) ;	g_meshLightningSphere.m_vIntensities.push_back( fEndIntensity ) ;		g_meshLightningSphere.m_vRadius.push_back( fHalfWidth ) ;

/*/
				float fHalfWidth = fThickness * 0.5f ;

				AddCylinder( g_meshLightningRect, sx, sy, sz, tx, ty, tz, fHalfWidth, 10, fStartIntensity, fEndIntensity ) ;
				AddSphere( g_meshLightningRect, sx, sy, sz, fHalfWidth, 10, fStartIntensity ) ;
				AddSphere( g_meshLightningRect, tx, ty, tz, fHalfWidth, 10, fEndIntensity ) ;
//*/
			}
		}

		++itr ;
	}

/*/
	if ( !g_meshLightningRect.m_vVertices.empty() )
	{
		g_meshLightningRect.setArray() ;
		g_meshLightningRect.setBuffer() ;

		g_meshLightningRect.setAttrLocPosition( vPosition ) ;
		g_meshLightningRect.setAttrLocIntensity( fAttri_Intensity ) ;
#ifdef USE_WHITE_BACKGROUND
		g_meshLightningRect.setUniformLocDiffuse( vColor, vec4( 0.0f, 0.0f, 0.0f, 1.0f ) ) ;
#else
		g_meshLightningRect.setUniformLocDiffuse( vColor, g_vLightningColor ) ;
#endif
	}
//*/
	
	if ( !g_meshLightningCylinder.m_vVertices.empty() )
	{
		glUseProgram( g_shaderLightningCylinder ) ;

		GLuint vPosition = glGetAttribLocation( g_shaderLightningCylinder, "vPosition" ) ;
		GLuint fAttri_Intensity = glGetAttribLocation( g_shaderLightningCylinder, "fIntensity" ) ;
		GLuint fAttri_Radius = glGetAttribLocation( g_shaderLightningCylinder, "fRadius" ) ;
		GLuint vColor = glGetUniformLocation( g_shaderLightningCylinder, "vColor" ) ;

		g_meshLightningCylinder.setDrawLines( true ) ;
		g_meshLightningCylinder.setArray() ;
		g_meshLightningCylinder.setBuffer() ;

		g_meshLightningCylinder.setAttrLocPosition( vPosition ) ;
		g_meshLightningCylinder.setAttrLocIntensity( fAttri_Intensity ) ;
		g_meshLightningCylinder.setAttrLocRadius( fAttri_Radius ) ;
#ifdef USE_WHITE_BACKGROUND
		g_meshLightningCylinder.setUniformLocDiffuse( vColor, vec4( 0.0f, 0.0f, 0.0f, 1.0f ) ) ;
#else
		g_meshLightningCylinder.setUniformLocDiffuse( vColor, g_vLightningColor ) ;
#endif
	}

	if ( !g_meshLightningSphere.m_vVertices.empty() )
	{
		glUseProgram( g_shaderLightningSphere ) ;

		GLuint vPosition = glGetAttribLocation( g_shaderLightningSphere, "vPosition" ) ;
		GLuint fAttri_Intensity = glGetAttribLocation( g_shaderLightningSphere, "fIntensity" ) ;
		GLuint fAttri_Radius = glGetAttribLocation( g_shaderLightningSphere, "fRadius" ) ;
		GLuint vColor = glGetUniformLocation( g_shaderLightningSphere, "vColor" ) ;

		g_meshLightningSphere.setDrawLines( true ) ;
		g_meshLightningSphere.setArray() ;
		g_meshLightningSphere.setBuffer() ;

		g_meshLightningSphere.setAttrLocPosition( vPosition ) ;
		g_meshLightningSphere.setAttrLocIntensity( fAttri_Intensity ) ;
		g_meshLightningSphere.setAttrLocRadius( fAttri_Radius ) ;
#ifdef USE_WHITE_BACKGROUND
		g_meshLightningSphere.setUniformLocDiffuse( vColor, vec4( 0.0f, 0.0f, 0.0f, 1.0f ) ) ;
#else
		g_meshLightningSphere.setUniformLocDiffuse( vColor, g_vLightningColor ) ;
#endif
	}

	if ( !g_meshLightningLine.m_vVertices.empty() )
	{
		glUseProgram( g_shaderLightning ) ;

		GLuint vPosition = glGetAttribLocation( g_shaderLightning, "vPosition" ) ;
		GLuint fAttri_Intensity = glGetAttribLocation( g_shaderLightning, "fIntensity" ) ;
		GLuint vColor = glGetUniformLocation( g_shaderLightning, "vColor" ) ;

		g_meshLightningLine.setDrawLines( true ) ;
		g_meshLightningLine.setArray() ;
		g_meshLightningLine.setBuffer() ;

		g_meshLightningLine.setAttrLocPosition( vPosition ) ;
		g_meshLightningLine.setAttrLocIntensity( fAttri_Intensity ) ;
#ifdef USE_WHITE_BACKGROUND
		g_meshLightningLine.setUniformLocDiffuse( vColor, vec4( 0.0f, 0.0f, 0.0f, 1.0f ) ) ;
#else
		g_meshLightningLine.setUniformLocDiffuse( vColor, g_vLightningColor ) ;
#endif
	}
}

void	drawLightning()
{
//	updateLightning() ;

	if ( !g_meshLightningCylinder.m_vVertices.empty() )
	{
		glUseProgram( g_shaderLightningCylinder ) ;

		setLight( g_shaderLightningCylinder ) ;
		setModel( g_shaderLightningCylinder ) ;
		setView( g_shaderLightningCylinder ) ;
		setProjection( g_shaderLightningCylinder ) ;

		g_meshLightningCylinder.drawBuffer() ;
	}

	if ( !g_meshLightningSphere.m_vVertices.empty() )
	{
		glUseProgram( g_shaderLightningSphere ) ;

		setLight( g_shaderLightningSphere ) ;
		setModel( g_shaderLightningSphere ) ;
		setView( g_shaderLightningSphere ) ;
		setProjection( g_shaderLightningSphere ) ;

		g_meshLightningSphere.drawBuffer() ;
	}

/*/
	if ( !g_meshLightningRect.m_vVertices.empty() )
	{
		g_meshLightningRect.drawBuffer() ;
	}
//*/
	
	if ( !g_meshLightningLine.m_vVertices.empty() )
	{
		glUseProgram( g_shaderLightning ) ;

		setLight( g_shaderLightning ) ;
		setModel( g_shaderLightning ) ;
		setView( g_shaderLightning ) ;
		setProjection( g_shaderLightning ) ;

		g_meshLightningLine.drawBuffer() ;
	}
}

void	AddCircle( Mesh& mesh, float cx, float cy, float cz, float fRadius, int iSegment, float fIntensity )
{
	float angleDiff = 2 * M_PI / (float)iSegment ;
	float angle = 0.0f ;

	for ( int i = 0; i < iSegment; ++i )
	{
		// center
		mesh.m_vVertices.push_back( vec4( cx, cy, cz, 1.0f ) ) ;
		mesh.m_vIntensities.push_back( fIntensity ) ;
		
		// first point
		mesh.m_vVertices.push_back( vec4( cx + sinf( angle ) * fRadius, cy + cosf( angle ) * fRadius, cz, 1.0f ) ) ;
		mesh.m_vIntensities.push_back( fIntensity ) ;

		angle += angleDiff ;
		if ( angle >= 2 * M_PI )
		{
			angle = 0.0f ;
		}

		// next angel point
		mesh.m_vVertices.push_back( vec4( cx + sinf( angle ) * fRadius, cy + cosf( angle ) * fRadius, cz, 1.0f ) ) ;
		mesh.m_vIntensities.push_back( fIntensity ) ;
	}
}

void	AddCube( Mesh& mesh, float cx, float cy, float cz, float fSize )
{
	float fHalfSize = fSize / 2.0f ;

	std::vector< vec4 > vCubeVertices ;
	vCubeVertices.push_back( vec4( cx - fHalfSize, cy - fHalfSize, cz + fHalfSize, 1.0f ) ) ;	// 0
	vCubeVertices.push_back( vec4( cx - fHalfSize, cy + fHalfSize, cz + fHalfSize, 1.0f ) ) ;	// 1
	vCubeVertices.push_back( vec4( cx + fHalfSize, cy + fHalfSize, cz + fHalfSize, 1.0f ) ) ;	// 2
	vCubeVertices.push_back( vec4( cx + fHalfSize, cy - fHalfSize, cz + fHalfSize, 1.0f ) ) ;	// 3
	vCubeVertices.push_back( vec4( cx - fHalfSize, cy - fHalfSize, cz - fHalfSize, 1.0f ) ) ;	// 4
	vCubeVertices.push_back( vec4( cx - fHalfSize, cy + fHalfSize, cz - fHalfSize, 1.0f ) ) ;	// 5
	vCubeVertices.push_back( vec4( cx + fHalfSize, cy + fHalfSize, cz - fHalfSize, 1.0f ) ) ;	// 6
	vCubeVertices.push_back( vec4( cx + fHalfSize, cy - fHalfSize, cz - fHalfSize, 1.0f ) ) ;	// 7


	mesh.m_vVertices.push_back( vCubeVertices[ 1 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 2 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 0 ] ) ;

	mesh.m_vVertices.push_back( vCubeVertices[ 3 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 0 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 2 ] ) ;

	mesh.m_vVertices.push_back( vCubeVertices[ 1 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 5 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 2 ] ) ;

	mesh.m_vVertices.push_back( vCubeVertices[ 6 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 2 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 5 ] ) ;

	mesh.m_vVertices.push_back( vCubeVertices[ 3 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 2 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 7 ] ) ;

	mesh.m_vVertices.push_back( vCubeVertices[ 6 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 7 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 2 ] ) ;

	mesh.m_vVertices.push_back( vCubeVertices[ 1 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 0 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 5 ] ) ;

	mesh.m_vVertices.push_back( vCubeVertices[ 4 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 5 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 0 ] ) ;

	mesh.m_vVertices.push_back( vCubeVertices[ 3 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 7 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 0 ] ) ;

	mesh.m_vVertices.push_back( vCubeVertices[ 4 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 0 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 7 ] ) ;

	mesh.m_vVertices.push_back( vCubeVertices[ 6 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 7 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 5 ] ) ;

	mesh.m_vVertices.push_back( vCubeVertices[ 4 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 5 ] ) ;
	mesh.m_vVertices.push_back( vCubeVertices[ 7 ] ) ;
}

void	AddCylinder( Mesh& mesh, float sx, float sy, float sz, float ex, float ey, float ez, float fRadius, int iSegment, float fStartIntensity, float fEndIntensity )
{
	float fLength = sqrt( ( ex - sx ) * ( ex - sx ) + ( ey - sy ) * ( ey - sy ) + ( ez - sz ) * ( ez - sz ) ) ;

	std::vector< vec3 > vTopVertices ;
	std::vector< vec3 > vBottomVertices ;

	float x, y, z ;

	// create base vectors
	vec3 vStartToEnd( ex - sx, ey - sy, ez - sz ) ;
	vStartToEnd = normalize( vStartToEnd ) ;

	vec3 vXAxis( 1, 0, 0 ) ;
	vec3 vYAxis( 0, 1, 0 ) ;

	vec3 vRight = cross( vStartToEnd, vXAxis ) ;
	if ( vRight.x <= ELPSILON && vRight.y <= ELPSILON && vRight.z <= ELPSILON )
	{
		vRight = cross( vStartToEnd, vYAxis ) ;
	}
	vRight = normalize( vRight ) ;

	vec3 vUp = cross( vStartToEnd, vRight ) ;
	vUp = normalize( vUp ) ;


	// create top/bottom vertices
	// p(s, t) = s * C + r (A cos(t) + B sin(t))
	float angleDiff = 2 * M_PI / (float)iSegment ;
	float angle = 0.0f ;
		
	for ( int i = 0; i < iSegment; ++i )
	{
		// top vertices
		x = sx + ( vRight.x * cosf( angle ) + vUp.x * sinf( angle ) ) * fRadius ;//+ vStartToEnd.x * 0 ;
		y = sy + ( vRight.y * cosf( angle ) + vUp.y * sinf( angle ) ) * fRadius ;//+ vStartToEnd.y * 0 ;
		z = sz + ( vRight.z * cosf( angle ) + vUp.z * sinf( angle ) ) * fRadius ;//+ vStartToEnd.z * 0 ;

		vTopVertices.push_back( vec3( x, y, z ) ) ;

		// bottom vertices
		x = sx + ( vRight.x * cosf( angle ) + vUp.x * sinf( angle ) ) * fRadius + vStartToEnd.x * fLength ;
		y = sy + ( vRight.y * cosf( angle ) + vUp.y * sinf( angle ) ) * fRadius + vStartToEnd.y * fLength ;
		z = sz + ( vRight.z * cosf( angle ) + vUp.z * sinf( angle ) ) * fRadius + vStartToEnd.z * fLength ;

		vBottomVertices.push_back( vec3( x, y, z ) ) ;

		angle += angleDiff ;
		if ( angle >= 2 * M_PI )
		{
			angle = 0.0f ;
		}
	}

	// generate triangles
	int iIndex ;
	int iNextIndex ;

	// - top circle
	for ( int i = 0; i < iSegment; ++i )
	{
		iIndex = i ;
		iNextIndex = i + 1 ;

		if ( iNextIndex == iSegment )
		{
			iNextIndex = 0 ;
		}

		// center
		mesh.m_vVertices.push_back( vec4( sx, sy, sz, 1.0f ) ) ;
		mesh.m_vIntensities.push_back( fStartIntensity ) ;

		// first vertex
		mesh.m_vVertices.push_back( vec4( vTopVertices[ iIndex ].x, vTopVertices[ iIndex ].y, vTopVertices[ iIndex ].z, 1.0f ) ) ;
		mesh.m_vIntensities.push_back( fStartIntensity ) ;

		// next vertex
		mesh.m_vVertices.push_back( vec4( vTopVertices[ iNextIndex ].x, vTopVertices[ iNextIndex ].y, vTopVertices[ iNextIndex ].z, 1.0f ) ) ;
		mesh.m_vIntensities.push_back( fStartIntensity ) ;
	}

	// - bottom circle
	for ( int i = 0; i < iSegment; ++i )
	{
		iIndex = i ;
		iNextIndex = i + 1 ;

		if ( iNextIndex == iSegment )
		{
			iNextIndex = 0 ;
		}

		// center
		mesh.m_vVertices.push_back( vec4( ex, ey, ez, 1.0f ) ) ;
		mesh.m_vIntensities.push_back( fEndIntensity ) ;

		// first vertex
		mesh.m_vVertices.push_back( vec4( vBottomVertices[ iIndex ].x, vBottomVertices[ iIndex ].y, vBottomVertices[ iIndex ].z, 1.0f ) ) ;
		mesh.m_vIntensities.push_back( fEndIntensity ) ;

		// next vertex
		mesh.m_vVertices.push_back( vec4( vBottomVertices[ iNextIndex ].x, vBottomVertices[ iNextIndex ].y, vBottomVertices[ iNextIndex ].z, 1.0f ) ) ;
		mesh.m_vIntensities.push_back( fEndIntensity ) ;
	}

	// - side plates
	for ( int i = 0; i < iSegment; ++i )
	{
		iIndex = i ;
		iNextIndex = i + 1 ;

		if ( iNextIndex == iSegment )
		{
			iNextIndex = 0 ;
		}

		mesh.m_vVertices.push_back( vec4( vTopVertices[ iNextIndex ].x, vTopVertices[ iNextIndex ].y, vTopVertices[ iNextIndex ].z, 1.0f ) ) ;				mesh.m_vIntensities.push_back( fStartIntensity ) ;
		mesh.m_vVertices.push_back( vec4( vTopVertices[ iIndex ].x, vTopVertices[ iIndex ].y, vTopVertices[ iIndex ].z, 1.0f ) ) ;							mesh.m_vIntensities.push_back( fStartIntensity ) ;
		mesh.m_vVertices.push_back( vec4( vBottomVertices[ iIndex ].x, vBottomVertices[ iIndex ].y, vBottomVertices[ iIndex ].z, 1.0f ) ) ;					mesh.m_vIntensities.push_back( fEndIntensity ) ;

		mesh.m_vVertices.push_back( vec4( vTopVertices[ iNextIndex ].x, vTopVertices[ iNextIndex ].y, vTopVertices[ iNextIndex ].z, 1.0f ) ) ;				mesh.m_vIntensities.push_back( fStartIntensity ) ;
		mesh.m_vVertices.push_back( vec4( vBottomVertices[ iIndex ].x, vBottomVertices[ iIndex ].y, vBottomVertices[ iIndex ].z, 1.0f ) ) ;					mesh.m_vIntensities.push_back( fEndIntensity ) ;
		mesh.m_vVertices.push_back( vec4( vBottomVertices[ iNextIndex ].x, vBottomVertices[ iNextIndex ].y, vBottomVertices[ iNextIndex ].z, 1.0f ) ) ;		mesh.m_vIntensities.push_back( fEndIntensity ) ;
	}
}

void	AddSphere( Mesh& mesh, float sx, float sy, float sz, float fRadius, int iSegment, float fIntensity )
{
	float x, y, z ;
	float nx, ny, nz ;

	std::vector< vec3 > vVertices ;

	const float fDeltaPhi = ( 2 * M_PI ) / (float)iSegment ;
	const float fDeltaTheta = M_PI / (float)( iSegment + 2 ) ;	// 2 pole position

	float fTheta = 0 ;
	float fPhi = 0 ;

	// north pole ( sx, sy, sz + fRadius ) // (0, 0, 1)
	vec3 vNorthPole( sx, sy, sz + fRadius ) ;
	
	for ( int i = 0; i < iSegment; ++i )
	{
		fTheta += fDeltaTheta ;

		for ( int j = 0; j < iSegment; ++j )
		{
			fPhi += fDeltaPhi ;

			x = sx + fRadius * sinf( fTheta ) * cosf( fPhi ) ;
			y = sy + fRadius * sinf( fTheta ) * sinf( fPhi ) ;
			z = sz + fRadius * cosf( fTheta ) ;

			// add vertex(x,y,z)
			vVertices.push_back( vec3( x, y, z ) ) ;
		}
	}

	// south pole ( sx, sy, sz - fRadius ) // ( 0, 0, -1 )
	vec3 vSouthPole( sx, sy, sz - fRadius ) ;


	// generate triangles
	int iIndex ;
	int iNext ;
	int iStart ;

	int iIndex2 ;	// for the next ring
	int iNext2 ;	// for the next ring
	int iStart2 ;	// for the next ring


	// - north pole
	for ( int i = 0; i < iSegment; ++i )
	{
		iNext = i + 1 ;

		if ( i == iSegment - 1 )
		{
			iNext = 0 ;
		}

		mesh.m_vVertices.push_back( vec4( vNorthPole.x, vNorthPole.y, vNorthPole.z, 1.0f ) ) ;							mesh.m_vIntensities.push_back( fIntensity ) ;
		mesh.m_vVertices.push_back( vec4( vVertices[ i ].x, vVertices[ i ].y, vVertices[ i ].z, 1.0f ) ) ;				mesh.m_vIntensities.push_back( fIntensity ) ;
		mesh.m_vVertices.push_back( vec4( vVertices[ iNext ].x, vVertices[ iNext ].y, vVertices[ iNext ].z, 1.0f ) ) ;	mesh.m_vIntensities.push_back( fIntensity ) ;
	}

	// south pole
	iIndex = ( iSegment - 1 ) * iSegment ;
	iStart = iIndex ;

	for ( int i = 0; i < iSegment; ++i )
	{
		iNext = iIndex + 1 ;

		if ( i == iSegment - 1 )
		{
			iNext = iStart ;
		}

		mesh.m_vVertices.push_back( vec4( vVertices[ iNext ].x, vVertices[ iNext ].y, vVertices[ iNext ].z, 1.0f ) ) ;		mesh.m_vIntensities.push_back( fIntensity ) ;
		mesh.m_vVertices.push_back( vec4( vVertices[ iIndex ].x, vVertices[ iIndex ].y, vVertices[ iIndex ].z, 1.0f ) ) ;	mesh.m_vIntensities.push_back( fIntensity ) ;
		mesh.m_vVertices.push_back( vec4( vSouthPole.x, vSouthPole.y, vSouthPole.z, 1.0f ) ) ;								mesh.m_vIntensities.push_back( fIntensity ) ;

		++iIndex ;
	}

	// body
	for ( int i = 0; i < iSegment - 1; ++i )
	{
		iIndex = i * iSegment ;
		iIndex2 = ( i + 1 ) * iSegment ;
		iStart = iIndex ;
		iStart2 = iIndex2 ;
		
		for ( int j = 0; j < iSegment; ++j )
		{
			iNext = iIndex + 1 ;
			iNext2 = iIndex2 + 1 ;

			if ( j == iSegment - 1 )
			{
				iNext = iStart ;
				iNext2 = iStart2 ;
			}

			mesh.m_vVertices.push_back( vec4( vVertices[ iNext ].x, vVertices[ iNext ].y, vVertices[ iNext ].z, 1.0f ) ) ;			mesh.m_vIntensities.push_back( fIntensity ) ;
			mesh.m_vVertices.push_back( vec4( vVertices[ iIndex ].x, vVertices[ iIndex ].y, vVertices[ iIndex ].z, 1.0f ) ) ;		mesh.m_vIntensities.push_back( fIntensity ) ;
			mesh.m_vVertices.push_back( vec4( vVertices[ iIndex2 ].x, vVertices[ iIndex2 ].y, vVertices[ iIndex2 ].z, 1.0f ) ) ;	mesh.m_vIntensities.push_back( fIntensity ) ;

			mesh.m_vVertices.push_back( vec4( vVertices[ iNext ].x, vVertices[ iNext ].y, vVertices[ iNext ].z, 1.0f ) ) ;			mesh.m_vIntensities.push_back( fIntensity ) ;
			mesh.m_vVertices.push_back( vec4( vVertices[ iIndex2 ].x, vVertices[ iIndex2 ].y, vVertices[ iIndex2 ].z, 1.0f ) ) ;	mesh.m_vIntensities.push_back( fIntensity ) ;
			mesh.m_vVertices.push_back( vec4( vVertices[ iNext2 ].x, vVertices[ iNext2 ].y, vVertices[ iNext2 ].z, 1.0f ) ) ;		mesh.m_vIntensities.push_back( fIntensity ) ;

			++iIndex ;
			++iIndex2 ;
		}
	}
}

void	drawTexture( GLuint texture )
{
	GLint texLoc = glGetUniformLocation( g_shaderDebug, "textureToShow" ) ;
	glUniform1i( texLoc, 0 ) ;

	glActiveTexture( GL_TEXTURE0 ) ;
	glBindTexture( GL_TEXTURE_2D, texture ) ;

	drawQuad() ;
}

// drawQuad() Renders a 1x1 quad in NDC, best used for framebuffer color targets and post-processing effects.
void	drawQuad()
{
	if ( 0 == g_quadVAO )
	{
		GLfloat quadVertices[ ] =
		{
			// Positions        // Texture Coords
			-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
			1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		} ;

		// Setup plane VAO
		glGenVertexArrays( 1, &g_quadVAO ) ;
		glGenBuffers( 1, &g_quadVBO ) ;
		glBindVertexArray( g_quadVAO ) ;
		glBindBuffer( GL_ARRAY_BUFFER, g_quadVBO ) ;
		glBufferData( GL_ARRAY_BUFFER, sizeof( quadVertices ), &quadVertices, GL_STATIC_DRAW ) ;
		
		glEnableVertexAttribArray( 0 ) ;
		glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof( GLfloat ), (GLvoid*)0 ) ;
		
		glEnableVertexAttribArray( 1 ) ;
		glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof( GLfloat ), (GLvoid*)( 3 * sizeof(GLfloat) ) ) ;
	}

	glBindVertexArray( g_quadVAO ) ;
	glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 ) ;
	glBindVertexArray( 0 ) ;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////

void	init()
{
	// load shaders
	g_shaderGrid = InitShader( "shader/grid_vs.glsl", NULL, "shader/grid_fs.glsl", &g_vsGrid, NULL, &g_fsGrid ) ;
	g_shaderPoints = InitShader( "shader/points_vs.glsl", NULL, "shader/points_fs.glsl", &g_vsPoints, NULL, &g_fsPoints ) ;
	g_shaderObstacles = InitShader( "shader/obstacles_vs.glsl", NULL, "shader/obstacles_fs.glsl", &g_vsPoints, NULL, &g_fsPoints ) ;

	g_shaderScene = InitShader( "shader/scene_vs.glsl", NULL, "shader/scene_fs.glsl", &g_vsScene, NULL, &g_fsScene ) ;
	g_shaderLightning = InitShader( "shader/lightning_vs.glsl", NULL, "shader/lightning_fs.glsl", &g_vsLightning, NULL, &g_fsLightning ) ;
	g_shaderLightningCylinder = InitShader( "shader/lightning_cyl_vs.glsl", "shader/lightning_cyl_gs.glsl", "shader/lightning_cyl_fs.glsl", &g_vsLightningCylinder, &g_gsLightningCylinder, &g_fsLightningCylinder ) ;
	g_shaderLightningSphere = InitShader( "shader/lightning_sph_vs.glsl", "shader/lightning_sph_gs.glsl", "shader/lightning_sph_fs.glsl", &g_vsLightningSphere, &g_gsLightningSphere, &g_fsLightningSphere ) ;
	g_shaderBlur = InitShader( "shader/blur_vs.glsl", NULL, "shader/blur_fs.glsl", &g_vsBlur, NULL, &g_fsBlur ) ;
	g_shaderQuad = InitShader( "shader/quad_vs.glsl", NULL, "shader/quad_fs.glsl", &g_vsQuad, NULL, &g_fsQuad ) ;
	
	g_shaderDebug = InitShader( "shader/debug_vs.glsl", NULL, "shader/debug_fs.glsl", &g_vsDebug, NULL, &g_fsDebug ) ;

	glEnable( GL_DEPTH_TEST ) ;
	glEnable( GL_MULTISAMPLE ) ;

	// load map
	g_lightningGenerator.SetEta( ETA ) ;

#ifdef USE_OUR_METHOD
	g_lightningGenerator.SetPowOfR( RHO ) ;
#endif

	g_lightningGenerator.SetBaseThickness( BASE_THICKNESS ) ;
	g_lightningGenerator.SetIntensityAttenuation( ITENSITY_ATTENUATION ) ;

#ifdef USE_OUR_METHOD
	std::vector< Pos > vTargets ;
	Pos pos ;

//	pos.x = 7 ; pos.y = 15 ; pos.z = 7 ;
//	vTargets.push_back( pos ) ;
//	g_lightningGenerator.Load( E_LT_SINGLE_TARGET, 16, 2, 7, 0, 7, vTargets ) ;

	pos.x = 31 ; pos.y = 63 ; pos.z = 31 ;
	vTargets.push_back( pos ) ;
	g_lightningGenerator.Load( E_LT_SINGLE_TARGET, 64, 8, 31, 0, 31, vTargets ) ;
	
	//g_lightningGenerator.Load( 32, 4, 15, 0, 15, 15, 31, 15 ) ;
	//g_lightningGenerator.Load( 128, 16, 63, 0, 63, 63, 127, 63 ) ;
#endif

#ifdef USE_CGM_3D
	//g_lightningGenerator.Load( 16, 7, 0, 7, 7, 15, 7 ) ;
	//g_lightningGenerator.Load( 32, 15, 0, 15, 15, 31, 15 ) ;
	g_lightningGenerator.Load( 64, 31, 0, 31, 31, 63, 31 ) ;
	//g_lightningGenerator.Load( 128, 63, 0, 63, 63, 127, 63 ) ;
#endif


	// load scene texture
	LoadTextureFromBMP( "./res/scene.bmp", g_texScene ) ;

	// load mesh objects
	initGrid() ;
	initObstacles() ;
	initStartEndPoints() ;
	initScene() ;

//	initLightning() ;

	// generate frame buffers to divide rendering scene & brighter
	generate_framebuffers() ;

	// generate frame buffer for blurring the brighter
	generate_blur_framebuffers() ;
}


void	display()
{
	initLightningSteps() ;

#ifdef USE_WHITE_BACKGROUND
	glClearColor( 1.0, 1.0, 1.0, 1.0 ) ;
#else
	glClearColor( 0.0, 0.0, 0.0, 1.0 ) ;
#endif

	// --------------------------------------------------------------
	// deferred rendering
	
	// 1. render scene into first color buffer, render brighter(lightning) into 2nd color buffer
	glBindFramebuffer( GL_FRAMEBUFFER, g_FBO ) ;

		// clear
		glEnable( GL_DEPTH_TEST ) ;
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) ;
			
		// draw scene
		drawScene() ;

		glDisable( GL_DEPTH_TEST ) ;

		drawLightning() ;

		// draw grid
		if ( g_bDrawGrid )
		{
			drawGrid() ;
		}

		// draw start & end point
		if ( g_bDrawStartEndPoints )// && g_bProcessStarted )
		{
			drawStartEndPoints() ;
		}

		// draw obstacles
		if ( g_bDrawObstacles )
		{
			drawObstacles() ;
		}

		if ( g_bDrawBoundary )
		{
			drawBoundary() ;
		}

	glBindFramebuffer( GL_FRAMEBUFFER, 0 ) ;


	// 2. blurring with brighter color texture (two-pass Gaussian Blur)
//*/

	// clear frame buffers
	for ( int i = 0; i < 2; ++i )
	{
		glBindFramebuffer( GL_FRAMEBUFFER, g_blurFBO[ i ] ) ;
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) ;
	}

	GLboolean horz = true ;
	GLuint framebufferTexture = g_colorBuffers[ 1 ] ;	// if it is first iteration, use brighter scene framebuffer

	int	amount = 10 ;

	glUseProgram( g_shaderBlur ) ;
	glActiveTexture( GL_TEXTURE0 ) ;

	for ( int i = 0; i < amount; ++i )
	{
		glBindFramebuffer( GL_FRAMEBUFFER, g_blurFBO[ horz ] ) ;
		glUniform1i( glGetUniformLocation( g_shaderBlur, "horizontal" ), horz ) ;
		
		glBindTexture( GL_TEXTURE_2D, framebufferTexture ) ;	// bind texture of other framebuffer

		drawQuad() ;

		framebufferTexture = g_blurColorBuffers[ horz ] ;
		horz = !horz ;
	}
	
	glBindFramebuffer( GL_FRAMEBUFFER, 0 ) ;
//*/


	// 3. final quad rendering
//*/
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) ;
	
	glUseProgram( g_shaderQuad ) ;

	GLint texScene = glGetUniformLocation( g_shaderQuad, "scene" ) ;
	GLint texBloomBlur = glGetUniformLocation( g_shaderQuad, "bloomBlur" ) ;
	glUniform1i( texScene, 0 ) ;
	glUniform1i( texBloomBlur, 1 ) ;

	glActiveTexture( GL_TEXTURE0 ) ;
	glBindTexture( GL_TEXTURE_2D, g_colorBuffers[ 0 ] ) ;
	glActiveTexture( GL_TEXTURE1 ) ;
	glBindTexture( GL_TEXTURE_2D, g_blurColorBuffers[ 0 ] ) ;	// g_toggleColorBuffers[ !horz ]
	
	glUniform1i( glGetUniformLocation( g_shaderQuad, "bloom" ), g_bBloom ) ;
	glUniform1f( glGetUniformLocation( g_shaderQuad, "exposure" ), g_fExposure ) ; 
		
	drawQuad() ;
//*/

	// for debugging
/*/
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) ;

	glUseProgram( g_shaderDebug ) ;

	//glDisable( GL_DEPTH_TEST ) ;
	//glEnable( GL_BLEND ) ;
	//glBlendFunc( GL_SRC_ALPHA, GL_ONE ) ;

	//drawTexture( g_colorBuffers[ 0 ] ) ;		// scene
	//drawTexture( g_colorBuffers[ 1 ] ) ;		// lightning

	drawTexture( g_blurColorBuffers[ 0 ] ) ;	// blurred lightning

	glEnable( GL_DEPTH_TEST ) ;
	glDisable( GL_BLEND ) ;
//*/
	// --------------------------------------------------------------
	
	glutSwapBuffers() ;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

void	deleteShader( GLuint shader, GLuint vs, GLuint fs, GLuint gs )
{
	glDetachShader( shader, vs ) ;
	glDetachShader( shader, fs ) ;
	
	if ( -1 != gs )
	{
		glDetachShader( shader, gs ) ;
	}

	glDeleteShader( vs ) ;
	glDeleteShader( fs ) ;
	
	if ( -1 != gs )
	{
		glDeleteShader( gs ) ;
	}

	glDeleteProgram( shader ) ;
}

void	destroyShaders()
{
	GLenum ErrorCheckValue = glGetError() ;

	glUseProgram( 0 ) ;

	deleteShader( g_shaderGrid, g_vsGrid, g_fsGrid ) ;
	deleteShader( g_shaderPoints, g_vsPoints, g_fsPoints ) ;
	deleteShader( g_shaderObstacles, g_vsObstacles, g_fsObstacles ) ;
	deleteShader( g_shaderScene, g_vsScene, g_fsScene ) ;
	deleteShader( g_shaderLightning, g_vsLightning, g_fsLightning ) ;
	deleteShader( g_shaderLightningCylinder, g_vsLightningCylinder, g_fsLightningCylinder, g_gsLightningCylinder ) ;
	deleteShader( g_shaderLightningSphere, g_vsLightningSphere, g_fsLightningSphere, g_gsLightningSphere ) ;
	deleteShader( g_shaderBlur, g_vsBlur, g_fsBlur ) ;
	deleteShader( g_shaderQuad, g_vsQuad, g_fsQuad ) ;
	deleteShader( g_shaderDebug, g_vsDebug, g_fsDebug ) ;
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


// Set new X and Y
vec2	newXY( int x, int y )
{
	float halfOfWidth = (float)g_width / 2.0 ;
	float halfOfHeight = (float)g_height / 2.0 ;

	float newX ;
	float newY ;

	double degree = 0.0 ;

	if ( g_width > g_height )
	{
		newX = (float)( ( x - ( g_width - g_height ) / 2.0 ) - halfOfHeight ) / halfOfHeight ;
		newY = (float)( ( -y + halfOfHeight ) / halfOfHeight ) ; 
	}
	else if ( g_height > g_width )
	{
		newX = (float)( ( x - halfOfWidth ) / halfOfWidth ) ;
		newY = (float)( ( -y - ( g_width - g_height ) / 2.0 ) + halfOfWidth ) / halfOfWidth ;
	}
	else
	{
		newX = (float)( ( x - halfOfWidth ) / halfOfWidth ) ;
		newY = (float)( ( -y + halfOfHeight ) / halfOfHeight ) ;
	}

	return vec2( newX, newY ) ;
}

void	move( int x, int y )
{
	vec2 newxy = newXY( x, y ) ;
	float newX = newxy.x ;
	float newY = newxy.y ;
	float fError = newX * newX + newY * newY ;

	if ( bLeftClick && fError <= 2.0f )
	{
		float dx = newX - fOldX ;
		float dy = newY - fOldY ;

		fAngleH = fOldAngleH + -dx * 1.0f ;
		fAngleV = fOldAngleV + -dy * 1.0f ;

		if ( fAngleV > M_PI / 2.0f )		fAngleV = M_PI / 2.0f ;
		else if ( fAngleV < -M_PI / 2.0f )	fAngleV = -M_PI / 2.0f ;

		to = vec3( eye.x - cos( fAngleV ) * sin( fAngleH ), eye.y - sin( fAngleV ), eye.z - cos( fAngleV ) * cos( fAngleH ) ) ;

		//printf( "mouse move !!\n" ) ;

//		updateLightning() ;
	}
}

void	mouse( int button, int state, int x, int y )
{
	if ( GLUT_LEFT_BUTTON == button )
	{
		vec2 newxy = newXY( x, y ) ;
		float newX = newxy.x ;
		float newY = newxy.y ;
		float fError = newX * newX + newY * newY ;

		if ( GLUT_DOWN == state && fError <= 2.0f )
		{
			bLeftClick = true ;

			fOldX = newX ;
			fOldY = newY ;

			fOldAngleV = fAngleV ;
			fOldAngleH = fAngleH ;

			//printf( "mouse clicked !!\n" ) ;
		}
		else if ( GLUT_UP == state )
		{
			bLeftClick = false ;

			//printf( "mouse released !!\n" ) ;

			printf( "eye - x: %f,  y: %f,  z: %f\n", eye.x, eye.y, eye.z ) ;
			printf( "to -  x: %f,  y: %f,  z: %f\n\n", to.x, to.y, to.z ) ;
		}
	}
}

void	wheel( int button, int direction, int x, int y )
{
	if ( direction > 0 )
	{
		fZoom *= 1.1f ;
	}
	else
	{
		fZoom /= 1.1f ;
	}

	printf( "zoom : %f\n\n", fZoom ) ;

//	updateLightning() ;
}


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

		case '1' :
		{
			if ( 1.0f != g_fGridScale )
			{
				g_fGridScale = 1.0f ;
				initGrid() ;
			}
		}
		break ;

		case '2' :
		{
			if ( 2.0f != g_fGridScale )
			{
				g_fGridScale = 2.0f ;
				initGrid() ;
			}
		}
		break ;

		case '3' :
		{
			if ( 3.0f != g_fGridScale )
			{
				g_fGridScale = 3.0f ;
				initGrid() ;
			}
		}
		break ;

		case 'g':
		{
			g_bDrawGrid = !g_bDrawGrid ;
		}
		break ;

		case 'p':
		{
			g_bDrawStartEndPoints = !g_bDrawStartEndPoints ;
		}
		break ;

		case 'o' :
		{
			g_bDrawObstacles = !g_bDrawObstacles ;
		}
		break ;

		case 'b' :
		{
			g_bDrawBoundary = !g_bDrawBoundary ;
		}
		break ;

		case 'e':
		{
			g_bBloom = !g_bBloom ;
		}
		break ;

		case 'j':
		{
			g_bJitter = !g_bJitter ;

#ifndef	DRAW_STEP_BY_STEP
			initLightning() ;
#endif
		}
		break ;

		case 'q':
		{
			g_bWireFrame = !g_bWireFrame ;

			if ( g_bWireFrame )
			{
				glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ) ;
			}
			else
			{
				glPolygonMode( GL_FRONT_AND_BACK, GL_FILL ) ;
			}
		}
		break ;

		case 'u':
		{
					initLightning() ;
		}
		break ;





		case 'w':
		{
			vec3 vForward = ( to - eye ) ;
			eye += vForward * 0.1f ;
			to += vForward * 0.1f ;

			printf( "eye - x: %f,  y: %f,  z: %f\n", eye.x, eye.y, eye.z ) ;
			printf( "to -  x: %f,  y: %f,  z: %f\n\n", to.x, to.y, to.z ) ;

//			updateLightning() ;
		}
		break ;

		case 's':
		{
			vec3 vBackward = -( to - eye ) ;
			eye += vBackward * 0.1f ;
			to += vBackward * 0.1f ;

			printf( "eye - x: %f,  y: %f,  z: %f\n", eye.x, eye.y, eye.z ) ;
			printf( "to -  x: %f,  y: %f,  z: %f\n\n", to.x, to.y, to.z ) ;

//			updateLightning() ;
		}
		break ;

		case 'z':
		{
			eye += up * 0.1f ;
			to += up * 0.1f ;

			printf( "eye - x: %f,  y: %f,  z: %f\n", eye.x, eye.y, eye.z ) ;
			printf( "to -  x: %f,  y: %f,  z: %f\n\n", to.x, to.y, to.z ) ;

//			updateLightning() ;
		}
		break ;

		case 'x':
		{
			eye -= up * 0.1f ;
			to -= up * 0.1f ;

			printf( "eye - x: %f,  y: %f,  z: %f\n", eye.x, eye.y, eye.z ) ;
			printf( "to -  x: %f,  y: %f,  z: %f\n\n", to.x, to.y, to.z ) ;

//			updateLightning() ;
		}
		break ;

		case 'a':
		{
//			fAngleH += 0.1f ;
//			to = vec3( eye.x - cos( fAngleV ) * sin( fAngleH ), eye.y - sin( fAngleV ), eye.z - cos( fAngleV ) * cos( fAngleH ) ) ;

			vec3 vForward = ( to - eye ) ;
			vForward = glm::normalize( vForward ) ;

			vec3 vRight = glm::cross( vForward, up ) ;
			vRight = glm::normalize( vRight ) ;

			vec3 vUp = glm::cross( vRight, vForward ) ;
			vUp = glm::normalize( vUp ) ;

			float angle = DegreesToRadians * 0.5f ;

			// rotation by arbitrary axis r
			// | cos + ( 1 - cos ) * r.x * r.x			( 1 - cos ) * r.x * r.y - r.z * sin		( 1 - cos ) * r.x * r.z + r.y * sin		|
			// | ( 1 - cos ) * r.x * r.y + r.z * sin	cos + ( 1 - cos ) * r.y * r.y			( 1 - cos ) * r.y * r.z - r.x * sin		|
			// | ( 1 - cos ) * r.z * r.z - r.y * sin	( 1 - cos ) * r.y * r.z + r.x * sin		cos + ( 1 - cos ) * r.z * r.z			|

			vec3 newEye ;
			newEye.x = eye.x * ( cosf( angle ) + ( 1 - cosf( angle ) ) * vUp.x * vUp.x ) +
					   eye.y * ( ( 1 - cosf( angle ) ) * vUp.x * vUp.y + vUp.z * sinf( angle ) ) +
					   eye.z * ( ( 1 - cosf( angle ) ) * vUp.z * vUp.z - vUp.y * sin( angle ) ) ;

			newEye.y = eye.x * ( ( 1 - cosf( angle ) ) * vUp.x * vUp.y - vUp.z * sinf( angle ) ) +
					   eye.y * ( cosf( angle ) + ( 1 - cosf( angle ) ) * vUp.y * vUp.y ) +
					   eye.z * ( ( 1 - cosf( angle ) ) * vUp.y * vUp.z + vUp.x * sinf( angle ) ) ;

			newEye.z = eye.x * ( ( 1 - cosf( angle ) ) * vUp.x * vUp.z + vUp.y * sinf( angle ) ) +
					   eye.y * ( ( 1 - cosf( angle ) ) * vUp.y * vUp.z - vUp.x * sinf( angle ) ) +
					   eye.z * ( cosf( angle ) + ( 1 - cosf( angle ) ) * vUp.z * vUp.z ) ;

			eye = newEye ;
			to = vec3( eye.x - cos( fAngleV ) * sin( fAngleH ), eye.y - sin( fAngleV ), eye.z - cos( fAngleV ) * cos( fAngleH ) ) ;

			printf( "eye - x: %f,  y: %f,  z: %f\n", eye.x, eye.y, eye.z ) ;
			printf( "to -  x: %f,  y: %f,  z: %f\n\n", to.x, to.y, to.z ) ;

//			updateLightning() ;
		}
		break ;

		case 'd':
		{
//			fAngleH -= 0.1f ;
//			to = vec3( eye.x - cos( fAngleV ) * sin( fAngleH ), eye.y - sin( fAngleV ), eye.z - cos( fAngleV ) * cos( fAngleH ) ) ;

			vec3 vForward = ( to - eye ) ;
			vForward = glm::normalize( vForward ) ;

			vec3 vRight = glm::cross( vForward, up ) ;
			vRight = glm::normalize( vRight ) ;

			vec3 vUp = glm::cross( vRight, vForward ) ;
			vUp = glm::normalize( vUp ) ;

			float angle = -DegreesToRadians * 0.5f ;

			// rotation by arbitrary axis r
			// | cos + ( 1 - cos ) * r.x * r.x			( 1 - cos ) * r.x * r.y - r.z * sin		( 1 - cos ) * r.x * r.z + r.y * sin		|
			// | ( 1 - cos ) * r.x * r.y + r.z * sin	cos + ( 1 - cos ) * r.y * r.y			( 1 - cos ) * r.y * r.z - r.x * sin		|
			// | ( 1 - cos ) * r.z * r.z - r.y * sin	( 1 - cos ) * r.y * r.z + r.x * sin		cos + ( 1 - cos ) * r.z * r.z			|

			vec3 newEye ;
			newEye.x = eye.x * ( cosf( angle ) + ( 1 - cosf( angle ) ) * vUp.x * vUp.x ) +
					   eye.y * ( ( 1 - cosf( angle ) ) * vUp.x * vUp.y + vUp.z * sinf( angle ) ) +
					   eye.z * ( ( 1 - cosf( angle ) ) * vUp.z * vUp.z - vUp.y * sin( angle ) ) ;

			newEye.y = eye.x * ( ( 1 - cosf( angle ) ) * vUp.x * vUp.y - vUp.z * sinf( angle ) ) +
					   eye.y * ( cosf( angle ) + ( 1 - cosf( angle ) ) * vUp.y * vUp.y ) +
					   eye.z * ( ( 1 - cosf( angle ) ) * vUp.y * vUp.z + vUp.x * sinf( angle ) ) ;

			newEye.z = eye.x * ( ( 1 - cosf( angle ) ) * vUp.x * vUp.z + vUp.y * sinf( angle ) ) +
					   eye.y * ( ( 1 - cosf( angle ) ) * vUp.y * vUp.z - vUp.x * sinf( angle ) ) +
					   eye.z * ( cosf( angle ) + ( 1 - cosf( angle ) ) * vUp.z * vUp.z ) ;

			eye = newEye ;
			to = vec3( eye.x - cos( fAngleV ) * sin( fAngleH ), eye.y - sin( fAngleV ), eye.z - cos( fAngleV ) * cos( fAngleH ) ) ;

			printf( "eye - x: %f,  y: %f,  z: %f\n", eye.x, eye.y, eye.z ) ;
			printf( "to -  x: %f,  y: %f,  z: %f\n\n", to.x, to.y, to.z ) ;

//			updateLightning() ;
		}
		break ;

		case ' ':
		{
			char szTitle[ 256 ] ;
#ifdef USE_CGM_3D
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started)", ETA ) ;
#else
			sprintf( szTitle, "Lightning rendering - ETA : %d, RHO: %d (Started)", ETA, RHO ) ;
#endif
			glutSetWindowTitle( szTitle ) ;

			g_lightningGenerator.Reset() ;

			g_meshLightningLine.clearBuffer() ;
			g_meshLightningRect.clearBuffer() ;

			initLightning() ;
//			initLightningSteps() ;

			g_bProcessStarted = true ;
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

	//glViewport( ( width - dWidth ) / 2.0, ( height - dHeight ) / 2.0, dWidth, dHeight ) ;
	glViewport( 0, 0, g_width, g_height ) ;
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
	glutInitContextFlags	( GLUT_FORWARD_COMPATIBLE ) ;
	glutInitContextProfile	( GLUT_CORE_PROFILE ) ;
	
	char szTitle[ 256 ] ;
#ifdef USE_CGM_3D
	sprintf( szTitle, "Lightning rendering - ETA : %d", ETA ) ;
#else
	sprintf( szTitle, "Lightning rendering - ETA : %d, RHO: %d", ETA, RHO ) ;
#endif

	glutCreateWindow		( szTitle );

	//////////////////////////////////////////////
	glewExperimental = GL_TRUE ;
	//////////////////////////////////////////////

	glewInit				( ) ;
	CheckError() ;

	init					( ) ;
	
	glutDisplayFunc			( display ) ;
	glutReshapeFunc			( reshape ) ;

	glutIdleFunc			( idle ) ;

	glutKeyboardFunc		( keySelect ) ;
	glutMouseFunc			( mouse ) ;
	glutMotionFunc			( move ) ;
	glutMouseWheelFunc		( wheel ) ;
	glutMainLoop			( ) ;

	return 0 ;
}


bool	LoadTextureFromBMP( const char* pszImagePath, GLuint& textureID, bool bGenMipMap )
{
	if ( !pszImagePath )
	{
		return false ;
	}

	unsigned char	header[ 54 ] ;	// header
	unsigned int	data_pos ;
	unsigned int	width ;
	unsigned int	height ;
	unsigned int	data_size ;	// w * h * 3 for 24 bit bmp

	unsigned char*	data ;			// data pointer

	// read header
	FILE* fp = fopen( pszImagePath, "rb" ) ;
	if ( !fp )
	{
		fprintf( stderr, "ERROR: failed to open bitmap file : %s\n", pszImagePath ) ;
		return false ;
	}

	if ( 54 != fread( header, 1, 54, fp ) )
	{
		fprintf( stderr, "ERROR: bitmap header size is not correct : %s\n", pszImagePath ) ;

		fclose( fp ) ;
		return false ;
	}

	if ( 'B' != header[ 0 ] || 'M' != header[ 1 ] )
	{
		fprintf( stderr, "ERROR: invalid bitmap file : %s\n", pszImagePath ) ;

		fclose( fp ) ;
		return false ;
	}

	// read size & data position
	data_pos	= *( int* )&( header[ 0x0A ] ) ;
	data_size	= *( int* )&( header[ 0x22 ] ) ;
	width		= *( int* )&( header[ 0x12 ] ) ;
	height		= *( int* )&( header[ 0x16 ] ) ;

	if ( 0 == data_size )
	{
		data_size = width * height * 3 ;
	}

	if ( 0 == data_pos )
	{
		data_pos = 54 ;
	}

	// create image data buffer
	data = new unsigned char [ data_size ] ;

	// read image data
	fread( data, 1, data_size, fp ) ;

	// close file
	fclose( fp ) ;


	// create texture
	glGenTextures( 1, &textureID ) ;

	glBindTexture( GL_TEXTURE_2D, textureID ) ;
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data ) ;

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP ) ;			// or GL_REPEAT
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP ) ;			// or GL_REPEAT
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) ;	// or GL_NEAREST
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) ;	// or GL_NEAREST

	if ( bGenMipMap )
	{
		glGenerateMipmap( GL_TEXTURE_2D ) ;
	}

	// delete memory
	delete [] data ;

	return true ;
}


void		CheckNumberOfBranches( int iBranches )
{
	char szTitle[ 256 ] ;

	switch ( iBranches )
	{
		case 100 :
		{
#ifdef USE_CGM_3D
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started, 100)", ETA ) ;
#else
			sprintf( szTitle, "Lightning rendering - ETA : %d, RHO: %d (Started, 100)", ETA, RHO ) ;
#endif
			glutSetWindowTitle( szTitle );
		}
		break ;

		case 200 :
		{
#ifdef USE_CGM_3D
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started, 100, 200)", ETA ) ;
#else
			sprintf( szTitle, "Lightning rendering - ETA : %d, RHO: %d (Started, 100, 200)", ETA, RHO ) ;
#endif
			glutSetWindowTitle( szTitle );
		}
		break ;

		case 300 :
		{
#ifdef USE_CGM_3D
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started, 100, 200, 300)", ETA ) ;
#else
			sprintf( szTitle, "Lightning rendering - ETA : %d, RHO: %d (Started, 100, 200, 300)", ETA, RHO ) ;
#endif
			glutSetWindowTitle( szTitle );
		}
		break ;

		case 400 :
		{
#ifdef USE_CGM_3D
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started, 100, 200, 300, 400)", ETA ) ;
#else
			sprintf( szTitle, "Lightning rendering - ETA : %d, RHO: %d (Started, 100, 200, 300, 400)", ETA, RHO ) ;
#endif
			glutSetWindowTitle( szTitle );
		}
		break ;

		case 500 :
		{
#ifdef USE_CGM_3D
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started, 100, 200, 300, 400, 500)", ETA ) ;
#else
			sprintf( szTitle, "Lightning rendering - ETA : %d, RHO: %d (Started, 100, 200, 300, 400, 500)", ETA, RHO ) ;
#endif
			glutSetWindowTitle( szTitle );
		}
		break ;

		case 600 :
		{
#ifdef USE_CGM_3D
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started, 100, 200, 300, 400, 500, 600)", ETA ) ;
#else
			sprintf( szTitle, "Lightning rendering - ETA : %d, RHO: %d (Started, 100, 200, 300, 400, 500, 600)", ETA, RHO ) ;
#endif
			glutSetWindowTitle( szTitle );
		}
		break ;

		case 700 :
		{
#ifdef USE_CGM_3D
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started, 100, 200, 300, 400, 500, 600, 700)", ETA ) ;
#else
			sprintf( szTitle, "Lightning rendering - ETA : %d, RHO: %d (Started, 100, 200, 300, 400, 500, 600, 700)", ETA, RHO ) ;
#endif
			glutSetWindowTitle( szTitle );
		}
		break ;

		case 800 :
		{
#ifdef USE_CGM_3D
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started, 100, 200, 300, 400, 500, 600, 700, 800)", ETA ) ;
#else
			sprintf( szTitle, "Lightning rendering - ETA : %d, RHO: %d (Started, 100, 200, 300, 400, 500, 600, 700, 800)", ETA, RHO ) ;
#endif
			glutSetWindowTitle( szTitle );
		}
		break ;

		case 900 :
		{
#ifdef USE_CGM_3D
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started, 100, 200, 300, 400, 500, 600, 700, 800, 900)", ETA ) ;
#else
			sprintf( szTitle, "Lightning rendering - ETA : %d, RHO: %d (Started, 100, 200, 300, 400, 500, 600, 700, 800, 900)", ETA, RHO ) ;
#endif
			glutSetWindowTitle( szTitle );
		}
		break ;

		case 1000 :
		{
#ifdef USE_CGM_3D
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000)", ETA ) ;
#else
			sprintf( szTitle, "Lightning rendering - ETA : %d, RHO: %d (Started, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000)", ETA, RHO ) ;
#endif
			glutSetWindowTitle( szTitle );
		}
		break ;
	}
}

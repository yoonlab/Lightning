//#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")

#include "Base.h"
#include "Mesh.h"
#include "Shader.h"

#include "lightning_generator.h"

#include <vector>
#include <random>
#include <ctime>

using namespace Lightning ;
using namespace glm ;
using namespace std ;


#define ETA						6
#define BASE_THICKNESS			3
#define ITENSITY_ATTENUATION	1.0f//0.7f

#define SCENE_SIZE				720.0f
#define HALF_SCENE_SIZE			SCENE_SIZE / 2.0f

#define USE_WHITE_BACKGROUND
//#define CHECK_FRACTAL_DIMENSION


// --------------------------------------------------------------------------------------------------------------------
// for opengl & shader

// control values
bool				g_bDrawGrid = false ;
bool				g_bDrawStartEndPoints = true ;
bool				g_bDrawObstacles = true ;
bool				g_bDrawBoundary = true ;
bool				g_bProcessStarted = false ;
bool				g_bJitter = false ;
bool				g_bBloom = false ;
bool				g_bApplyThickLine = false ;
int					g_fGridScale = 1.0f ;
float				g_fExposure = 1.0f ;

std::vector< int >	g_vTimeCheckSteps ;

//vec4				g_vLightningColor = vec4( 0.0f, 0.0f, 1.0f, 1.0f ) ;	// blue
//vec4				g_vLightningColor = vec4( 1.0f, 0.0f, 0.0f, 1.0f ) ;	// red
vec4				g_vLightningColor = vec4( 1.0f, 1.0f, 1.0f, 1.0f ) ;	// white
//vec4				g_vLightningColor = vec4( 0.47f, 0.08f, 0.43f, 1.0f ) ;	// purple
//vec4				g_vLightningColor = vec4( 0.7f, 0.6f, 1.0f, 1.0f ) ;	// white


// mesh objects
Mesh				g_meshGrid ;
Mesh				g_meshStartPos ;
Mesh				g_meshEndPos ;
Mesh				g_meshBoundary ;
Mesh				g_meshScene ;
Mesh				g_meshObstacles ;
Mesh				g_meshLightningRect ;
Mesh				g_meshLightningLine ;

GLuint				g_texScene ;

LightningGenerator	g_lightningGenerator ;

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
void	initLightningSteps() ;
void	drawLightning() ;

void	drawTexture( GLuint texture ) ;	// for debugging, draw quad with texture
void	drawQuad() ;

void	AddCircle( Mesh& mesh, float cx, float cy, float fRadius, int iSegment, float fIntensity = 1.0f ) ;


bool	LoadTextureFromBMP( const char* pszImagePath, GLuint& textureID, bool bGenMipMap = false ) ;

void	CalcFractalDimension( int iGridSize, float iSceneSize, const Lightning::LightningTree& lightning, float& outOrigin, float& outJittered ) ;

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
	//mat4 lightmodelMat = Angel::RotateY( g_theta ) ;
	mat4 lightmodelMat ;	// identity matrix
	glUniformMatrix4fv( glGetUniformLocation( shader, "mlModel" ), 1, GL_TRUE, value_ptr( lightmodelMat ) ) ;
}

// Setting object's modelMatrix
void	setModel( GLuint shader )
{
	mat4 modelMat( 1.0f ) ;
	glUniformMatrix4fv( glGetUniformLocation( shader, "mModel" ), 1, GL_TRUE, value_ptr( modelMat ) ) ;
}

// Setting object's viewMatrix
void	setView( GLuint shader )
{
	float fZoom = 1.0f ;

	vec3 eye = vec3( 0.0, 0.0, 50.0 ) ;
	vec3 to = vec3( 0.0, 0.0, 0.0 ) ;
	vec3 up = vec3( 0.0, 1.0, 0.0 ) ;

	// World to Camera(Eye - wc)
	mat4 viewMat = glm::lookAt( eye, to, up ) * glm::mat4( fZoom ) ;
	glUniformMatrix4fv( glGetUniformLocation( shader, "mView" ), 1, GL_TRUE, value_ptr( viewMat ) ) ;
}

// Setting object's projectionMatrix
void	setProjection( GLuint shader )
{
/*/
	// Camera to Clipping
	mat4 projectionMat = glm::perspective( 45.0f, (float)g_width / (float)g_height, 1.0f, 100.0f ) ;
	glUniformMatrix4fv( glGetUniformLocation( g_IDShader, "mProjection" ), 1, GL_TRUE, projectionMat ) ;
//*/

	mat4 projectionMat = glm::ortho( -400.0f, 400.0f, -400.0f, 400.0f, 1.0f, 100.0f ) ;
	glUniformMatrix4fv( glGetUniformLocation( shader, "mProjection" ), 1, GL_TRUE, value_ptr( projectionMat ) ) ;
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

	int iDrawGridSize = iGridSize * g_fGridScale ;

	for ( int i = 0; i < iDrawGridSize + 1; ++i )
	{
		y = fDiff * i + ( HALF_SCENE_SIZE ) ;

		g_meshGrid.m_vVertices.push_back( vec4( -( HALF_SCENE_SIZE ), y, 0.0f, 1.0f ) ) ;
		g_meshGrid.m_vVertices.push_back( vec4( ( HALF_SCENE_SIZE ), y, 0.0f, 1.0f ) ) ;
	}

	for ( int i = 0; i < iDrawGridSize + 1; ++i )
	{
		x = -fDiff * i - ( HALF_SCENE_SIZE ) ;

		g_meshGrid.m_vVertices.push_back( vec4( x, -( HALF_SCENE_SIZE ), 0.0f, 1.0f ) ) ;
		g_meshGrid.m_vVertices.push_back( vec4( x, ( HALF_SCENE_SIZE ), 0.0f, 1.0f ) ) ;
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

	g_meshBoundary.m_vVertices.push_back( vec4( -( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), 0.0f, 1.0f ) ) ;
	g_meshBoundary.m_vVertices.push_back( vec4( ( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), 0.0f, 1.0f ) ) ;

	g_meshBoundary.m_vVertices.push_back( vec4( ( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), 0.0f, 1.0f ) ) ;
	g_meshBoundary.m_vVertices.push_back( vec4( ( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), 0.0f, 1.0f ) ) ;

	g_meshBoundary.m_vVertices.push_back( vec4( ( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), 0.0f, 1.0f ) ) ;
	g_meshBoundary.m_vVertices.push_back( vec4( -( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), 0.0f, 1.0f ) ) ;

	g_meshBoundary.m_vVertices.push_back( vec4( -( HALF_SCENE_SIZE ), -( HALF_SCENE_SIZE ), 0.0f, 1.0f ) ) ;
	g_meshBoundary.m_vVertices.push_back( vec4( -( HALF_SCENE_SIZE ), ( HALF_SCENE_SIZE ), 0.0f, 1.0f ) ) ;

	g_meshBoundary.setDrawLines( true ) ;
	g_meshBoundary.setArray() ;
	g_meshBoundary.setBuffer() ;

	g_meshBoundary.setAttrLocPosition( vPosition ) ;
#ifdef USE_WHITE_BACKGROUND
	g_meshBoundary.setUniformLocDiffuse( vColor, vec4( 0.0f, 0.0f, 0.0f, 1.0f ) ) ;
#else
	g_meshBoundary.setUniformLocDiffuse( vColor, vec4( 1.0f, 1.0f, 1.0f, 1.0f ) ) ;
#endif
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
	float x ;
	float y ;

	int iStartCount = g_lightningGenerator.GetStartCellCount() ;
	if ( iStartCount > 0 )
	{
		for ( int i = 0; i < iStartCount; ++i )
		{
			const Cell* pCell = g_lightningGenerator.GetStartCell( i ) ;
			if ( pCell )
			{
//*/
				x = -fDiff * pCell->m_iX - ( HALF_SCENE_SIZE ) ;
				y = fDiff * pCell->m_iY + ( HALF_SCENE_SIZE ) ;
				
				AddCircle( g_meshStartPos, x, y, -fDiff, 10 ) ;
//*/
				
/*/
				x = -fDiff * pCell->m_iX - ( HALF_SCENE_SIZE ) ;
				y = fDiff * pCell->m_iY + ( HALF_SCENE_SIZE ) ;
				
				g_meshStartPos.m_vVertices.push_back( vec4( x, y, 0.0f, 1.0f ) ) ;
				g_meshStartPos.m_vVertices.push_back( vec4( x, y + fDiff, 0.0f, 1.0f ) ) ;
				g_meshStartPos.m_vVertices.push_back( vec4( x - fDiff, y, 0.0f, 1.0f ) ) ;

				g_meshStartPos.m_vVertices.push_back( vec4( x, y + fDiff, 0.0f, 1.0f ) ) ;
				g_meshStartPos.m_vVertices.push_back( vec4( x - fDiff, y, 0.0f, 1.0f ) ) ;
				g_meshStartPos.m_vVertices.push_back( vec4( x - fDiff, y + fDiff, 0.0f, 1.0f ) ) ;
//*/
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
			const Cell* pCell = g_lightningGenerator.GetEndCell( i ) ;
			if ( pCell )
			{
//*/
				x = -fDiff * pCell->m_iX - ( HALF_SCENE_SIZE ) ;
				y = fDiff * pCell->m_iY + ( HALF_SCENE_SIZE ) ;
				y += fDiff ;
				
				AddCircle( g_meshEndPos, x, y, -fDiff, 10 ) ;
//*/

/*/
				x = -fDiff * pCell->m_iX - ( HALF_SCENE_SIZE ) ;
				y = fDiff * pCell->m_iY + ( HALF_SCENE_SIZE ) ;
				
				g_meshEndPos.m_vVertices.push_back( vec4( x, y, 0.0f, 1.0f ) ) ;
				g_meshEndPos.m_vVertices.push_back( vec4( x, y + fDiff, 0.0f, 1.0f ) ) ;
				g_meshEndPos.m_vVertices.push_back( vec4( x - fDiff, y, 0.0f, 1.0f ) ) ;

				g_meshEndPos.m_vVertices.push_back( vec4( x, y + fDiff, 0.0f, 1.0f ) ) ;
				g_meshEndPos.m_vVertices.push_back( vec4( x - fDiff, y, 0.0f, 1.0f ) ) ;
				g_meshEndPos.m_vVertices.push_back( vec4( x - fDiff, y + fDiff, 0.0f, 1.0f ) ) ;
//*/
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
		float x ;
		float y ;

		for ( int i = 0; i < iCount; ++i )
		{
			const Cell* pCell = g_lightningGenerator.GetObstacleCell( i ) ;
			if ( pCell )
			{
				x = -fDiff * pCell->m_iX - ( HALF_SCENE_SIZE ) ;
				y = fDiff * pCell->m_iY + ( HALF_SCENE_SIZE ) ;


				g_meshObstacles.m_vVertices.push_back( vec4( x, y, 0.0f, 1.0f ) ) ;
				g_meshObstacles.m_vVertices.push_back( vec4( x, y + fDiff, 0.0f, 1.0f ) ) ;
				g_meshObstacles.m_vVertices.push_back( vec4( x - fDiff, y, 0.0f, 1.0f ) ) ;

				g_meshObstacles.m_vVertices.push_back( vec4( x, y + fDiff, 0.0f, 1.0f ) ) ;
				g_meshObstacles.m_vVertices.push_back( vec4( x - fDiff, y, 0.0f, 1.0f ) ) ;
				g_meshObstacles.m_vVertices.push_back( vec4( x - fDiff, y + fDiff, 0.0f, 1.0f ) ) ;
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

#ifdef CHECK_FRACTAL_DIMENSION
	g_bJitter = true ;
#endif

//	std::string strTimeLog = "timeLog.txt" ;
//	g_lightningGenerator.Process( E_PT_ALL_STEP, true, strTimeLog ) ;
	
int timeStart = glutGet( GLUT_ELAPSED_TIME ) ;

	g_lightningGenerator.Process( E_PT_ALL_STEP ) ;

int timeEnd = glutGet( GLUT_ELAPSED_TIME ) ;
int timeElapsed = timeEnd - timeStart ;
int timeInit = g_lightningGenerator.GetInitializationTime() ;
printf( "\n\nTime : %d ms (segments: %d)\n", timeElapsed + timeInit, g_lightningGenerator.GetLightningTree().GetNodeList().size() ) ;
printf( "  - Initialization : %d ms\n", timeInit ) ;
printf( "  - Process        : %d ms\n", timeElapsed ) ;

printf( "\n" ) ;
const std::vector< int > vTimesPerSteps = g_lightningGenerator.GetTimesPerSteps() ;
for ( int i = 0; i < vTimesPerSteps.size(); ++i )
{
	printf( "%4d : %d (%d + %d) ms\n", g_vTimeCheckSteps[ i ], timeInit + vTimesPerSteps[ i ], timeInit, vTimesPerSteps[ i ] ) ;
}

	//float fd = g_lightningGenerator.CalculateFractalDimension() ;

	float fDiff = -( SCENE_SIZE ) / g_lightningGenerator.GetGridSize() ;
	float fCenter = -fDiff / 2.0f ;
	float sx, sy ;
	float tx, ty ;
	float fThickness ;
	float fStartIntensity ;
	float fEndIntensity ;

	//const LightningTreeNode* pNode ;
	//const LightningTree& tree = g_lightningGenerator.GetLightningTree() ;
	//const std::vector< LightningTreeNode* > vNodes = tree.GetNodeList() ;
	LightningTreeNode* pNode ;
	LightningTree& tree = g_lightningGenerator.GetLightningTree() ;
	std::vector< LightningTreeNode* > vNodes = tree.GetNodeList() ;	// TODO : change this function to const function

	std::vector< LightningTreeNode* >::const_iterator itr = vNodes.begin() ;
	while ( itr != vNodes.end() )
	{
		pNode = *itr ;

		if ( pNode && pNode->m_pParent )
		{
			fThickness = pNode->m_fThickness ;
			fStartIntensity = pNode->m_pParent->m_fIntensity ;
			fEndIntensity = pNode->m_fIntensity ;

			if ( g_bJitter && pNode->m_pParent->m_bJittered )
			{
				sx = pNode->m_pParent->m_fXJittered ;
				sy = pNode->m_pParent->m_fYJittered ;
			}
			else
			{
				sx = -fDiff * pNode->m_pParent->m_iX - ( HALF_SCENE_SIZE ) + fCenter ;
				sy = fDiff * pNode->m_pParent->m_iY + ( HALF_SCENE_SIZE ) - fCenter ;

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

					pNode->m_pParent->m_bJittered = true ;
					pNode->m_pParent->m_fXJittered = sx ;
					pNode->m_pParent->m_fYJittered = sy ;
				}
			}

			if ( g_bJitter && pNode->m_bJittered )
			{
				tx = pNode->m_fXJittered ;
				ty = pNode->m_fYJittered ;
			}
			else
			{
				tx = -fDiff * pNode->m_iX - ( HALF_SCENE_SIZE ) + fCenter ;
				ty = fDiff * pNode->m_iY + ( HALF_SCENE_SIZE ) - fCenter ;

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

					pNode->m_bJittered = true ;
					pNode->m_fXJittered = tx ;
					pNode->m_fYJittered = ty ;
				}
			}

			if ( !g_bApplyThickLine || fThickness <= 1.0f )
			{
				g_meshLightningLine.m_vVertices.push_back( vec4( sx, sy, 0.0f, 1.0f ) ) ;	g_meshLightningLine.m_vIntensities.push_back( fStartIntensity ) ;
				g_meshLightningLine.m_vVertices.push_back( vec4( tx, ty, 0.0f, 1.0f ) ) ;	g_meshLightningLine.m_vIntensities.push_back( fEndIntensity ) ;
			}
			else
			{
				vec2 vCorner ;
				float xDiff = tx - sx ;
				float yDiff = ty - sy ;
//				float fHalfWidth = ( fThickness * 0.5f ) / 4.0f ;
				float fHalfWidth = fThickness * 0.5f ;

				float fThetaInRadians = atan2f( yDiff, xDiff ) ;
				if ( 0 == yDiff && xDiff < 0 ) fThetaInRadians = 0.0f ;

				vCorner.x = sinf( fThetaInRadians ) * fHalfWidth ;
				vCorner.y = cosf( fThetaInRadians ) * fHalfWidth ;

				//if ( vCorner.x > 0 && vCorner.x <= 0.25 ) vCorner.x = 0.5f ;
				//if ( vCorner.y > 0 && vCorner.y <= 0.25 ) vCorner.y = 0.5f ;

				// TODO : remove this check routine !!!
				bool bHasSamePosition1 = false ;
				bool bHasSamePosition2 = false ;

				if ( sx + vCorner.x == sx - vCorner.x && sy - vCorner.y == sy + vCorner.y )			bHasSamePosition1 = true ;
				else if ( sx + vCorner.x == tx - vCorner.x && sy - vCorner.y == ty + vCorner.y )	bHasSamePosition1 = true ;
				else if ( sx - vCorner.x == tx - vCorner.x && sy + vCorner.y == ty + vCorner.y )	bHasSamePosition1 = true ;

				if ( sx + vCorner.x == tx - vCorner.x && sy - vCorner.y == ty + vCorner.y )			bHasSamePosition2 = true ;
				else if ( sx + vCorner.x == tx + vCorner.x && sy - vCorner.y == ty - vCorner.y )	bHasSamePosition2 = true ;
				else if ( tx - vCorner.x == tx + vCorner.x && ty + vCorner.y == ty - vCorner.y )	bHasSamePosition2 = true ;

				if ( bHasSamePosition1 || bHasSamePosition2 )
				{
					printf( "Has Same Position !!!!!\n" ) ;
				}
				else
				{
					g_meshLightningRect.m_vVertices.push_back( vec4( sx + vCorner.x, sy - vCorner.y, 0.0f, 1.0f ) ) ;	g_meshLightningRect.m_vIntensities.push_back( fStartIntensity ) ;
					g_meshLightningRect.m_vVertices.push_back( vec4( sx - vCorner.x, sy + vCorner.y, 0.0f, 1.0f ) ) ;	g_meshLightningRect.m_vIntensities.push_back( fStartIntensity ) ;
					g_meshLightningRect.m_vVertices.push_back( vec4( tx - vCorner.x, ty + vCorner.y, 0.0f, 1.0f ) ) ;	g_meshLightningRect.m_vIntensities.push_back( fEndIntensity ) ;

					g_meshLightningRect.m_vVertices.push_back( vec4( sx + vCorner.x, sy - vCorner.y, 0.0f, 1.0f ) ) ;	g_meshLightningRect.m_vIntensities.push_back( fStartIntensity ) ;
					g_meshLightningRect.m_vVertices.push_back( vec4( tx - vCorner.x, ty + vCorner.y, 0.0f, 1.0f ) ) ;	g_meshLightningRect.m_vIntensities.push_back( fEndIntensity ) ;
					g_meshLightningRect.m_vVertices.push_back( vec4( tx + vCorner.x, ty - vCorner.y, 0.0f, 1.0f ) ) ;	g_meshLightningRect.m_vIntensities.push_back( fEndIntensity ) ;

					AddCircle( g_meshLightningRect, sx, sy, fHalfWidth, 10, fStartIntensity ) ;
					AddCircle( g_meshLightningRect, tx, ty, fHalfWidth, 10, fEndIntensity ) ;
				}
			}
		}

		++itr ;
	}

	glUseProgram( g_shaderLightning ) ;

	GLuint vPosition = glGetAttribLocation( g_shaderLightning, "vPosition" ) ;
	GLuint fAttri_Intensity = glGetAttribLocation( g_shaderLightning, "fIntensity" ) ;
	GLuint vColor = glGetUniformLocation( g_shaderLightning, "vColor" ) ;
		
	if ( !g_meshLightningLine.m_vVertices.empty() )
	{
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


#ifdef CHECK_FRACTAL_DIMENSION
	float fOrgDimension ;
	float fJitteredDimension ;
	
	CalcFractalDimension( g_lightningGenerator.GetGridSize(), SCENE_SIZE, g_lightningGenerator.GetLightningTree(), fOrgDimension, fJitteredDimension ) ;
	printf( "\n\nFractal dimension - original position: %f, jittered position: %f\n", fOrgDimension, fJitteredDimension ) ;
#endif
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
		sprintf( szTitle, "Lightning rendering - ETA : %d (Finished)", ETA ) ;
		glutSetWindowTitle( szTitle );

		return ;
	}

	g_meshLightningLine.clearBuffer() ;
	g_meshLightningRect.clearBuffer() ;

//int timeEnd = glutGet( GLUT_ELAPSED_TIME ) ;
//int timeElapsed = timeEnd - timeStart ;
//int timeInit = g_lightningGenerator.GetInitializationTime() ;
//printf( "\n\nTime : %d ms (segments: %d)\n", timeElapsed + timeInit, g_lightningGenerator.GetLightningTree().GetNodeList().size() ) ;
//printf( "  - Initialization : %d ms\n", timeInit ) ;
//printf( "  - Process        : %d ms\n", timeElapsed ) ;

//printf( "\n" ) ;
//const std::vector< int > vTimesPerSteps = g_lightningGenerator.GetTimesPerSteps() ;
//for ( int i = 0; i < vTimesPerSteps.size(); ++i )
//{
//	printf( "%4d : %d (%d + %d) ms\n", g_vTimeCheckSteps[ i ], timeInit + vTimesPerSteps[ i ], timeInit, vTimesPerSteps[ i ] ) ;
//}

	//float fd = g_lightningGenerator.CalculateFractalDimension() ;

	float fDiff = -( SCENE_SIZE ) / g_lightningGenerator.GetGridSize() ;
	float fCenter = -fDiff / 2.0f ;
	float sx, sy ;
	float tx, ty ;
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
			fThickness = pNode->m_fThickness ;
			fStartIntensity = pNode->m_pParent->m_fIntensity ;
			fEndIntensity = pNode->m_fIntensity ;

			if ( g_bJitter && pNode->m_pParent->m_bJittered )
			{
				sx = pNode->m_pParent->m_fXJittered ;
				sy = pNode->m_pParent->m_fYJittered ;
			}
			else
			{
				sx = -fDiff * pNode->m_pParent->m_iX - ( HALF_SCENE_SIZE ) + fCenter ;
				sy = fDiff * pNode->m_pParent->m_iY + ( HALF_SCENE_SIZE ) - fCenter ;

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

					pNode->m_pParent->m_bJittered = true ;
					pNode->m_pParent->m_fXJittered = sx ;
					pNode->m_pParent->m_fYJittered = sy ;
				}
			}

			if ( g_bJitter && pNode->m_bJittered )
			{
				tx = pNode->m_fXJittered ;
				ty = pNode->m_fYJittered ;
			}
			else
			{
				tx = -fDiff * pNode->m_iX - ( HALF_SCENE_SIZE ) + fCenter ;
				ty = fDiff * pNode->m_iY + ( HALF_SCENE_SIZE ) - fCenter ;

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

					pNode->m_bJittered = true ;
					pNode->m_fXJittered = tx ;
					pNode->m_fYJittered = ty ;
				}
			}

			if ( !g_bApplyThickLine || fThickness <= 1.0f )
			{
				g_meshLightningLine.m_vVertices.push_back( vec4( sx, sy, 0.0f, 1.0f ) ) ;	g_meshLightningLine.m_vIntensities.push_back( fStartIntensity ) ;
				g_meshLightningLine.m_vVertices.push_back( vec4( tx, ty, 0.0f, 1.0f ) ) ;	g_meshLightningLine.m_vIntensities.push_back( fEndIntensity ) ;
			}
			else
			{
				vec2 vCorner ;
				float xDiff = tx - sx ;
				float yDiff = ty - sy ;
//				float fHalfWidth = ( fThickness * 0.5f ) / 4.0f ;
				float fHalfWidth = fThickness * 0.5f ;

				float fThetaInRadians = atan2f( yDiff, xDiff ) ;
				if ( 0 == yDiff && xDiff < 0 ) fThetaInRadians = 0.0f ;

				vCorner.x = sinf( fThetaInRadians ) * fHalfWidth ;
				vCorner.y = cosf( fThetaInRadians ) * fHalfWidth ;

				//if ( vCorner.x > 0 && vCorner.x <= 0.25 ) vCorner.x = 0.5f ;
				//if ( vCorner.y > 0 && vCorner.y <= 0.25 ) vCorner.y = 0.5f ;

				// TODO : remove this check routine !!!
				bool bHasSamePosition1 = false ;
				bool bHasSamePosition2 = false ;

				if ( sx + vCorner.x == sx - vCorner.x && sy - vCorner.y == sy + vCorner.y )			bHasSamePosition1 = true ;
				else if ( sx + vCorner.x == tx - vCorner.x && sy - vCorner.y == ty + vCorner.y )	bHasSamePosition1 = true ;
				else if ( sx - vCorner.x == tx - vCorner.x && sy + vCorner.y == ty + vCorner.y )	bHasSamePosition1 = true ;

				if ( sx + vCorner.x == tx - vCorner.x && sy - vCorner.y == ty + vCorner.y )			bHasSamePosition2 = true ;
				else if ( sx + vCorner.x == tx + vCorner.x && sy - vCorner.y == ty - vCorner.y )	bHasSamePosition2 = true ;
				else if ( tx - vCorner.x == tx + vCorner.x && ty + vCorner.y == ty - vCorner.y )	bHasSamePosition2 = true ;

				if ( bHasSamePosition1 || bHasSamePosition2 )
				{
					printf( "Has Same Position !!!!!\n" ) ;
				}
				else
				{
					g_meshLightningRect.m_vVertices.push_back( vec4( sx + vCorner.x, sy - vCorner.y, 0.0f, 1.0f ) ) ;	g_meshLightningRect.m_vIntensities.push_back( fStartIntensity ) ;
					g_meshLightningRect.m_vVertices.push_back( vec4( sx - vCorner.x, sy + vCorner.y, 0.0f, 1.0f ) ) ;	g_meshLightningRect.m_vIntensities.push_back( fStartIntensity ) ;
					g_meshLightningRect.m_vVertices.push_back( vec4( tx - vCorner.x, ty + vCorner.y, 0.0f, 1.0f ) ) ;	g_meshLightningRect.m_vIntensities.push_back( fEndIntensity ) ;

					g_meshLightningRect.m_vVertices.push_back( vec4( sx + vCorner.x, sy - vCorner.y, 0.0f, 1.0f ) ) ;	g_meshLightningRect.m_vIntensities.push_back( fStartIntensity ) ;
					g_meshLightningRect.m_vVertices.push_back( vec4( tx - vCorner.x, ty + vCorner.y, 0.0f, 1.0f ) ) ;	g_meshLightningRect.m_vIntensities.push_back( fEndIntensity ) ;
					g_meshLightningRect.m_vVertices.push_back( vec4( tx + vCorner.x, ty - vCorner.y, 0.0f, 1.0f ) ) ;	g_meshLightningRect.m_vIntensities.push_back( fEndIntensity ) ;

					AddCircle( g_meshLightningRect, sx, sy, fHalfWidth, 10, fStartIntensity ) ;
					AddCircle( g_meshLightningRect, tx, ty, fHalfWidth, 10, fEndIntensity ) ;
				}
			}
		}

		++itr ;
	}

	glUseProgram( g_shaderLightning ) ;

	GLuint vPosition = glGetAttribLocation( g_shaderLightning, "vPosition" ) ;
	GLuint fAttri_Intensity = glGetAttribLocation( g_shaderLightning, "fIntensity" ) ;
	GLuint vColor = glGetUniformLocation( g_shaderLightning, "vColor" ) ;
		
	if ( !g_meshLightningLine.m_vVertices.empty() )
	{
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
}

void	drawLightning()
{
	glUseProgram( g_shaderLightning ) ;

	setLight( g_shaderLightning ) ;
	setModel( g_shaderLightning ) ;
	setView( g_shaderLightning ) ;
	setProjection( g_shaderLightning ) ;

	if ( !g_meshLightningLine.m_vVertices.empty() )
	{
		g_meshLightningLine.drawBuffer() ;
	}

	if ( !g_meshLightningRect.m_vVertices.empty() )
	{
		g_meshLightningRect.drawBuffer() ;
	}
}

void	AddCircle( Mesh& mesh, float cx, float cy, float fRadius, int iSegment, float fIntensity )
{
	float angleDiff = 2 * M_PI / (float)iSegment ;
	float angle = 0.0f ;

	for ( int i = 0; i < iSegment; ++i )
	{
		// center
		mesh.m_vVertices.push_back( vec4( cx, cy, 0.0f, 1.0f ) ) ;
		mesh.m_vIntensities.push_back( fIntensity ) ;
		
		// first point
		mesh.m_vVertices.push_back( vec4( cx + sinf( angle ) * fRadius, cy + cosf( angle ) * fRadius, 0.0f, 1.0f ) ) ;
		mesh.m_vIntensities.push_back( fIntensity ) ;

		angle += angleDiff ;
		if ( angle >= 2 * M_PI )
		{
			angle = 0.0f ;
		}

		// next angel point
		mesh.m_vVertices.push_back( vec4( cx + sinf( angle ) * fRadius, cy + cosf( angle ) * fRadius, 0.0f, 1.0f ) ) ;
		mesh.m_vIntensities.push_back( fIntensity ) ;
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
	g_shaderBlur = InitShader( "shader/blur_vs.glsl", NULL, "shader/blur_fs.glsl", &g_vsBlur, NULL, &g_fsBlur ) ;
	g_shaderQuad = InitShader( "shader/quad_vs.glsl", NULL, "shader/quad_fs.glsl", &g_vsQuad, NULL, &g_fsQuad ) ;
	
	g_shaderDebug = InitShader( "shader/debug_vs.glsl", NULL, "shader/debug_fs.glsl", &g_vsDebug, NULL, &g_fsDebug ) ;

	glEnable( GL_DEPTH_TEST ) ;
	glEnable( GL_MULTISAMPLE ) ;

	// load map
//	g_lightningGenerator.SetEta( ETA ) ;
	g_lightningGenerator.SetEta( 6 ) ;	// 10
	g_lightningGenerator.SetPhysicalHeight( 1000 ) ;	// 100000
	g_lightningGenerator.SetWeight( 128.0f ) ;
	g_lightningGenerator.SetBaseThickness( BASE_THICKNESS ) ;
	g_lightningGenerator.SetIntensityAttenuation( ITENSITY_ATTENUATION ) ;


	//g_lightningGenerator.Load( 256, 127, 0, 127, 255 ) ;
	g_lightningGenerator.Load( 128, 63, 0, 63, 127 ) ;
	//g_lightningGenerator.Load( 128, 63, 63, 63, 127 ) ;
	//g_lightningGenerator.Load( 128, 63, 0 ) ;
	//g_lightningGenerator.Load( 128, 63, 63 ) ;

	// ----------------------------------------------------------------------------
	// for checking time per steps
/*/
	g_lightningGenerator.Load( 512, 255, 0, 255, 511 ) ;
	//g_lightningGenerator.Load( 512, 255, 0 ) ;

	g_vTimeCheckSteps.clear() ;
	g_vTimeCheckSteps.push_back( 100 ) ;
	g_vTimeCheckSteps.push_back( 200 ) ;
	g_vTimeCheckSteps.push_back( 300 ) ;
	g_vTimeCheckSteps.push_back( 400 ) ;
	g_vTimeCheckSteps.push_back( 500 ) ;
	g_vTimeCheckSteps.push_back( 1000 ) ;
	g_vTimeCheckSteps.push_back( 2000 ) ;
	g_vTimeCheckSteps.push_back( 3000 ) ;
	g_vTimeCheckSteps.push_back( 4000 ) ;
	g_vTimeCheckSteps.push_back( 5000 ) ;
	g_lightningGenerator.SetTimeCheckSteps( g_vTimeCheckSteps ) ;
//*/





	//g_lightningGenerator.Load( "./res/lightning_32.map" ) ;
	//g_lightningGenerator.Load( "./res/lightning_64.map" ) ;
	//g_lightningGenerator.Load( "./res/lightning_128.map" ) ;
	//g_lightningGenerator.Load( "./res/lightning_128_c.map" ) ;
		
	//g_lightningGenerator.Load( 128, 64, 0, 64, 127 ) ;
	//g_lightningGenerator.Load( 256, 127, 0, 127, 255 ) ;
	//g_lightningGenerator.Load( 512, 255, 127, 255, 383 ) ;
	//g_lightningGenerator.Load( 512, 255, 255, 255, 511 ) ;
	
	//g_lightningGenerator.Load( "./res/lightning_64_obs.map" ) ;
	//g_lightningGenerator.Load( "./res/lightning_64_obs_complex.map" ) ;
	//g_lightningGenerator.Load( "./res/lightning_64_obs_complex_waypoint.map" ) ;

	//g_lightningGenerator.Load( "./res/lightning_64_obs_waypoint.map" ) ;
	//g_lightningGenerator.Load( "./res/lightning_64_obs_complex_waypoint.map" ) ;
	
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
		if ( g_bDrawStartEndPoints )//&& g_bProcessStarted )
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

void	deleteShader( GLuint shader, GLuint vs, GLuint fs )
{
	glDetachShader( shader, vs ) ;
	//glDetachShader( shader, gs ) ;
	glDetachShader( shader, fs ) ;

	glDeleteShader( vs ) ;
	//glDeleteShader( gs ) ;
	glDeleteShader( fs ) ;

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

		case 'd' :
		{
			g_bDrawBoundary = !g_bDrawBoundary ;
		}
		break ;

		case 'b':
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

		case ' ':
		{
			char szTitle[ 256 ] ;
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started)", ETA ) ;
			glutSetWindowTitle( szTitle ) ;

			g_meshLightningLine.clearBuffer() ;
			g_meshLightningRect.clearBuffer() ;

#ifdef CHECK_FRACTAL_DIMENSION
			initLightning() ;
#else
			//initLightning() ;
			initLightningSteps() ;
#endif

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
	sprintf( szTitle, "Lightning rendering - ETA : %d", ETA ) ;
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


struct LineSeg
{
	float x1, y1 ;
	float x2, y2 ;
} ;

bool	Intersect( const glm::vec2& s1, const glm::vec2& e1, const glm::vec2& s2, const glm::vec2& e2, glm::vec2& out )
{
	out = glm::vec2( 0, 0 ) ;

	glm::vec2 b = e1 - s1 ;
	glm::vec2 d = e2 - s2 ;

	float fDot = b.x * d.y - b.y * d.x ;
	if ( 0 == fDot )
	{
		return false ;	// parallel
	}

	glm::vec2 c = s2 - s1 ;
	
	float t = ( c.x * d.y - c.y * d.x ) / fDot ;
	if ( t < 0 || t > 1 )
	{
		return false ;
	}

	float u = ( c.x * b.y - c.y * b.x ) / fDot ;
	if ( u < 0 || u > 1 )
	{
		return false ;
	}

	out = s1 + t * b ;

	return true ;
}

void	GetNodeSegments( int iGridSize, float fSceneSize, const LightningTreeNode* pNode, std::vector< LineSeg >& vSegments, std::vector< LineSeg >& vSegmentsJittered )
{
	float fHalfSceneSize = fSceneSize / 2.0f ;
	float fDiff = ( -fSceneSize / iGridSize ) ;

	if ( pNode && pNode->m_pParent )
	{
		LineSeg seg ;
		
		seg.x1 = -fDiff * pNode->m_pParent->m_iX - fHalfSceneSize ;
		seg.y1 = fDiff * pNode->m_pParent->m_iY + fHalfSceneSize ;

		seg.x2 = -fDiff * pNode->m_iX - fHalfSceneSize ;
		seg.y2 = fDiff * pNode->m_iY + fHalfSceneSize ;

		vSegments.push_back( seg ) ;

		seg.x1 = pNode->m_pParent->m_fXJittered ;
		seg.y1 = pNode->m_pParent->m_fYJittered ;

		seg.x2 = pNode->m_fXJittered ;
		seg.y2 = pNode->m_fYJittered ;

		vSegmentsJittered.push_back( seg ) ;


		int iChildCount = pNode->m_vChildren.size() ;
		for ( int i = 0; i < iChildCount; ++i )
		{
			GetNodeSegments( iGridSize, fSceneSize, pNode->m_vChildren[ i ], vSegments, vSegmentsJittered ) ;
		}
	}
}

void		CalcFractalDimension( int iGridSize, float fSceneSize, const Lightning::LightningTree& lightning, float& outOrigin, float& outJittered )
{
	printf( "\n\n--- Calculate fractal dimension\n" ) ;

	int iBaseOrg = 0 ;
	int iScaledOrg = 0 ;
	int iBaseJitter = 0 ;
	int iScaledJitter = 0 ;
	
	std::vector< int > vBaseOrg ;
	std::vector< int > vScaledOrg ;
	std::vector< int > vBaseJitter ;
	std::vector< int > vScaledJitter ;

	vBaseOrg.assign( iGridSize * iGridSize, 0 ) ;
	vBaseJitter.assign( iGridSize * iGridSize, 0 ) ;

	vScaledOrg.assign( iGridSize * iGridSize * 4, 0 ) ;
	vScaledJitter.assign( iGridSize * iGridSize * 4, 0 ) ;


	// -----------------------------------------------------
	// get line segments
	std::vector< LineSeg > vSegments ;
	std::vector< LineSeg > vSegmentsJittered ;

	const Lightning::LightningTreeNode* pRoot = lightning.GetRoot() ;
	if ( pRoot )
	{
		int iChildCount = pRoot->m_vChildren.size() ;
		for ( int i = 0; i < iChildCount; ++i )
		{
			GetNodeSegments( iGridSize, fSceneSize, pRoot->m_vChildren[ i ], vSegments, vSegmentsJittered ) ;
		}
	}

	printf( "   - Lightning segments : %d\n", (int)vSegments.size() ) ;

	int iIndex ;
	float rx1, rx2, rx3, rx4 ;	// top, right, bottom, left
	float ry1, ry2, ry3, ry4 ;
	bool bIntersect ;
	glm::vec2 out ;

	float fHalfSceneSize = fSceneSize / 2.0f ;
	float fDiff = ( -fSceneSize / iGridSize ) ;
	float fScaledDiff = ( -fSceneSize / ( iGridSize * 2 ) ) ;


	// -----------------------------------------------------
	// base position

	// for all grid rectangle
	for ( int gy = 0; gy < iGridSize; ++gy )
	{
		for ( int gx = 0; gx < iGridSize; ++gx )
		{
			iIndex = gy * iGridSize + gx ;
			
			if ( 0 == vBaseOrg[ iIndex ] )
			{
				rx1 = -fDiff * gx - fHalfSceneSize ;
				rx2 = -fDiff * ( gx + 1 ) - fHalfSceneSize - 1 ;
				rx3 = -fDiff * ( gx + 1 ) - fHalfSceneSize - 1 ;
				rx4 = -fDiff * gx - fHalfSceneSize ;

				ry1 = fDiff * gy + fHalfSceneSize ;
				ry2 = fDiff * gy + fHalfSceneSize ;
				ry3 = fDiff * ( gy + 1 ) + fHalfSceneSize + 1 ;
				ry4 = fDiff * ( gy + 1 ) + fHalfSceneSize + 1 ;

				bIntersect = false ;

				// loop for all segments
				std::vector< LineSeg >::iterator itr = vSegments.begin() ;
				while ( itr != vSegments.end() )
				{
					// if contains
					if ( rx1 <= itr->x1 && itr->x1 <= rx2 && itr->y1 <= ry1 && ry3 <= itr->y1 &&
						 rx1 <= itr->x2 && itr->x2 <= rx2 && itr->y2 <= ry1 && ry3 <= itr->y2 )
					{
						bIntersect = true ;
						break ;
					}

					// 4 boundary lines
					if ( Intersect( glm::vec2( itr->x1, itr->y1 ), glm::vec2( itr->x2, itr->y2 ), glm::vec2( rx1, ry1 ), glm::vec2( rx2, ry2 ), out ) )
					{
						bIntersect = true ;
						break ;
					}

					if ( Intersect( glm::vec2( itr->x1, itr->y1 ), glm::vec2( itr->x2, itr->y2 ), glm::vec2( rx2, ry2 ), glm::vec2( rx3, ry3 ), out ) )
					{
						bIntersect = true ;
						break ;
					}

					if ( Intersect( glm::vec2( itr->x1, itr->y1 ), glm::vec2( itr->x2, itr->y2 ), glm::vec2( rx3, ry3 ), glm::vec2( rx4, ry4 ), out ) )
					{
						bIntersect = true ;
						break ;
					}

					if ( Intersect( glm::vec2( itr->x1, itr->y1 ), glm::vec2( itr->x2, itr->y2 ), glm::vec2( rx4, ry4 ), glm::vec2( rx1, ry1 ), out ) )
					{
						bIntersect = true ;
						break ;
					}

					++itr ;
				}

				if ( bIntersect )
				{
					++iBaseOrg ;
					vBaseOrg[ iIndex ] = 1 ;
				}
			}
		}
	}

	printf( "   - # of cells for original position : %d\n", iBaseOrg ) ;


	// for all grid rectangle
	for ( int gy = 0; gy < iGridSize * 2; ++gy )
	{
		for ( int gx = 0; gx < iGridSize * 2; ++gx )
		{
			iIndex = gy * iGridSize * 2 + gx ;
			
			if ( 0 == vScaledOrg[ iIndex ] )
			{
				rx1 = -fScaledDiff * gx - fHalfSceneSize ;
				rx2 = -fScaledDiff * ( gx + 1 ) - fHalfSceneSize - 1 ;
				rx3 = -fScaledDiff * ( gx + 1 ) - fHalfSceneSize - 1 ;
				rx4 = -fScaledDiff * gx - fHalfSceneSize ;

				ry1 = fScaledDiff * gy + fHalfSceneSize ;
				ry2 = fScaledDiff * gy + fHalfSceneSize ;
				ry3 = fScaledDiff * ( gy + 1 ) + fHalfSceneSize + 1 ;
				ry4 = fScaledDiff * ( gy + 1 ) + fHalfSceneSize + 1 ;

				bIntersect = false ;

				// loop for all segments
				std::vector< LineSeg >::iterator itr = vSegments.begin() ;
				while ( itr != vSegments.end() )
				{
					// if contains
					if ( rx1 <= itr->x1 && itr->x1 <= rx2 && itr->y1 <= ry1 && ry3 <= itr->y1 &&
						 rx1 <= itr->x2 && itr->x2 <= rx2 && itr->y2 <= ry1 && ry3 <= itr->y2 )
					{
						bIntersect = true ;
						break ;
					}

					// 4 boundary lines
					if ( Intersect( glm::vec2( itr->x1, itr->y1 ), glm::vec2( itr->x2, itr->y2 ), glm::vec2( rx1, ry1 ), glm::vec2( rx2, ry2 ), out ) )
					{
						bIntersect = true ;
						break ;
					}

					if ( Intersect( glm::vec2( itr->x1, itr->y1 ), glm::vec2( itr->x2, itr->y2 ), glm::vec2( rx2, ry2 ), glm::vec2( rx3, ry3 ), out ) )
					{
						bIntersect = true ;
						break ;
					}

					if ( Intersect( glm::vec2( itr->x1, itr->y1 ), glm::vec2( itr->x2, itr->y2 ), glm::vec2( rx3, ry3 ), glm::vec2( rx4, ry4 ), out ) )
					{
						bIntersect = true ;
						break ;
					}

					if ( Intersect( glm::vec2( itr->x1, itr->y1 ), glm::vec2( itr->x2, itr->y2 ), glm::vec2( rx4, ry4 ), glm::vec2( rx1, ry1 ), out ) )
					{
						bIntersect = true ;
						break ;
					}

					++itr ;
				}

				if ( bIntersect )
				{
					++iScaledOrg ;
					vScaledOrg[ iIndex ] = 1 ;
				}
			}
		}
	}

	printf( "   - # of cells for original position (scaled) : %d\n", iScaledOrg ) ;


	// -----------------------------------------------------
	// jittered

	// for all grid rectangle
	for ( int gy = 0; gy < iGridSize; ++gy )
	{
		for ( int gx = 0; gx < iGridSize; ++gx )
		{
			iIndex = gy * iGridSize + gx ;
			
			if ( 0 == vBaseJitter[ iIndex ] )
			{
				rx1 = -fDiff * gx - fHalfSceneSize ;
				rx2 = -fDiff * ( gx + 1 ) - fHalfSceneSize - 1 ;
				rx3 = -fDiff * ( gx + 1 ) - fHalfSceneSize - 1 ;
				rx4 = -fDiff * gx - fHalfSceneSize ;

				ry1 = fDiff * gy + fHalfSceneSize ;
				ry2 = fDiff * gy + fHalfSceneSize ;
				ry3 = fDiff * ( gy + 1 ) + fHalfSceneSize + 1 ;
				ry4 = fDiff * ( gy + 1 ) + fHalfSceneSize + 1 ;

				bIntersect = false ;

				// loop for all segments
				std::vector< LineSeg >::iterator itr = vSegmentsJittered.begin() ;
				while ( itr != vSegmentsJittered.end() )
				{
					// if contains
					if ( rx1 <= itr->x1 && itr->x1 <= rx2 && itr->y1 <= ry1 && ry3 <= itr->y1 &&
						 rx1 <= itr->x2 && itr->x2 <= rx2 && itr->y2 <= ry1 && ry3 <= itr->y2 )
					{
						bIntersect = true ;
						break ;
					}

					// 4 boundary lines
					if ( Intersect( glm::vec2( itr->x1, itr->y1 ), glm::vec2( itr->x2, itr->y2 ), glm::vec2( rx1, ry1 ), glm::vec2( rx2, ry2 ), out ) )
					{
						bIntersect = true ;
						break ;
					}

					if ( Intersect( glm::vec2( itr->x1, itr->y1 ), glm::vec2( itr->x2, itr->y2 ), glm::vec2( rx2, ry2 ), glm::vec2( rx3, ry3 ), out ) )
					{
						bIntersect = true ;
						break ;
					}

					if ( Intersect( glm::vec2( itr->x1, itr->y1 ), glm::vec2( itr->x2, itr->y2 ), glm::vec2( rx3, ry3 ), glm::vec2( rx4, ry4 ), out ) )
					{
						bIntersect = true ;
						break ;
					}

					if ( Intersect( glm::vec2( itr->x1, itr->y1 ), glm::vec2( itr->x2, itr->y2 ), glm::vec2( rx4, ry4 ), glm::vec2( rx1, ry1 ), out ) )
					{
						bIntersect = true ;
						break ;
					}

					++itr ;
				}

				if ( bIntersect )
				{
					++iBaseJitter ;
					vBaseJitter[ iIndex ] = 1 ;
				}
			}
		}
	}

	printf( "   - # of cells for jittered position : %d\n", iBaseJitter ) ;


	// for all grid rectangle
	for ( int gy = 0; gy < iGridSize * 2; ++gy )
	{
		for ( int gx = 0; gx < iGridSize * 2; ++gx )
		{
			iIndex = gy * iGridSize * 2 + gx ;
			
			if ( 0 == vScaledJitter[ iIndex ] )
			{
				rx1 = -fScaledDiff * gx - fHalfSceneSize ;
				rx2 = -fScaledDiff * ( gx + 1 ) - fHalfSceneSize - 1 ;
				rx3 = -fScaledDiff * ( gx + 1 ) - fHalfSceneSize - 1 ;
				rx4 = -fScaledDiff * gx - fHalfSceneSize ;

				ry1 = fScaledDiff * gy + fHalfSceneSize ;
				ry2 = fScaledDiff * gy + fHalfSceneSize ;
				ry3 = fScaledDiff * ( gy + 1 ) + fHalfSceneSize + 1 ;
				ry4 = fScaledDiff * ( gy + 1 ) + fHalfSceneSize + 1 ;

				bIntersect = false ;

				// loop for all segments
				std::vector< LineSeg >::iterator itr = vSegmentsJittered.begin() ;
				while ( itr != vSegmentsJittered.end() )
				{
					// if contains
					if ( rx1 <= itr->x1 && itr->x1 <= rx2 && itr->y1 <= ry1 && ry3 <= itr->y1 &&
						 rx1 <= itr->x2 && itr->x2 <= rx2 && itr->y2 <= ry1 && ry3 <= itr->y2 )
					{
						bIntersect = true ;
						break ;
					}

					// 4 boundary lines
					if ( Intersect( glm::vec2( itr->x1, itr->y1 ), glm::vec2( itr->x2, itr->y2 ), glm::vec2( rx1, ry1 ), glm::vec2( rx2, ry2 ), out ) )
					{
						bIntersect = true ;
						break ;
					}

					if ( Intersect( glm::vec2( itr->x1, itr->y1 ), glm::vec2( itr->x2, itr->y2 ), glm::vec2( rx2, ry2 ), glm::vec2( rx3, ry3 ), out ) )
					{
						bIntersect = true ;
						break ;
					}

					if ( Intersect( glm::vec2( itr->x1, itr->y1 ), glm::vec2( itr->x2, itr->y2 ), glm::vec2( rx3, ry3 ), glm::vec2( rx4, ry4 ), out ) )
					{
						bIntersect = true ;
						break ;
					}

					if ( Intersect( glm::vec2( itr->x1, itr->y1 ), glm::vec2( itr->x2, itr->y2 ), glm::vec2( rx4, ry4 ), glm::vec2( rx1, ry1 ), out ) )
					{
						bIntersect = true ;
						break ;
					}

					++itr ;
				}

				if ( bIntersect )
				{
					++iScaledJitter ;
					vScaledJitter[ iIndex ] = 1 ;
				}
			}
		}
	}

	printf( "   - # of cells for jittered position (scaled) : %d\n", iScaledJitter ) ;


	// calculate fractal dimension
	// d = log( new created count ) / log( Scale )
	float N = (float)iScaledOrg / (float)iBaseOrg ;
	float S = 2.0f ;
	outOrigin = log( N ) / log( S ) ;

	N = (float)iScaledJitter / (float)iBaseJitter ;
	outJittered = long( N ) / log( S ) ;
}

void		CheckNumberOfBranches( int iBranches )
{
	switch ( iBranches )
	{
		case 100 :
		{
			char szTitle[ 256 ] ;
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started, 100)", ETA ) ;
			glutSetWindowTitle( szTitle );
		}
		break ;

		case 200 :
		{
			char szTitle[ 256 ] ;
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started, 100, 200)", ETA ) ;
			glutSetWindowTitle( szTitle );
		}
		break ;

		case 300 :
		{
			char szTitle[ 256 ] ;
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started, 100, 200, 300)", ETA ) ;
			glutSetWindowTitle( szTitle );
		}
		break ;

		case 400 :
		{
			char szTitle[ 256 ] ;
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started, 100, 200, 300, 400)", ETA ) ;
			glutSetWindowTitle( szTitle );
		}
		break ;

		case 500 :
		{
			char szTitle[ 256 ] ;
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started, 100, 200, 300, 400, 500)", ETA ) ;
			glutSetWindowTitle( szTitle );
		}
		break ;

		case 600 :
		{
			char szTitle[ 256 ] ;
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started, 100, 200, 300, 400, 500, 600)", ETA ) ;
			glutSetWindowTitle( szTitle );
		}
		break ;

		case 700 :
		{
			char szTitle[ 256 ] ;
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started, 100, 200, 300, 400, 500, 600, 700)", ETA ) ;
			glutSetWindowTitle( szTitle );
		}
		break ;

		case 800 :
		{
			char szTitle[ 256 ] ;
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started, 100, 200, 300, 400, 500, 600, 700, 800)", ETA ) ;
			glutSetWindowTitle( szTitle );
		}
		break ;

		case 900 :
		{
			char szTitle[ 256 ] ;
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started, 100, 200, 300, 400, 500, 600, 700, 800, 900)", ETA ) ;
			glutSetWindowTitle( szTitle );
		}
		break ;

		case 1000 :
		{
			char szTitle[ 256 ] ;
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000)", ETA ) ;
			glutSetWindowTitle( szTitle );
		}
		break ;
	}
}
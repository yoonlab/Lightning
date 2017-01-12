//#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")

#include "Base.h"
#include "Mesh.h"
#include "Shader.h"

#include "../cgm/cgm_map.h"

#include <vector>
#include <random>
#include <ctime>

using namespace std ;
using namespace glm ;


#define ETA			2

#define SCENE_SIZE				720.0f
#define HALF_SCENE_SIZE			SCENE_SIZE / 2.0f

#define USE_WHITE_BACKGROUND
//#define CHECK_FRACTAL_DIMENSION

// --------------------------------------------------------------------------------------------------------------------
// for opengl & shader

bool				g_bDrawGrid = false ;
bool				g_bDrawStartEndPoints = true ;
bool				g_bProcessStarted = false ;
bool				g_bJitter = false ;

Mesh				g_meshGrid ;
Mesh				g_meshStartPos ;
Mesh				g_meshEndPos ;
Mesh				g_meshBoundary ;
Mesh				g_meshScene ;
Mesh				g_meshLightning ;
CGMMap				g_map ;

std::vector< int >	g_vTimeCheckSteps ;

// Shader ID
GLuint				g_IDShader ;
GLuint				g_vertexShaderID ;
GLuint				g_geometryShaderID ;
GLuint				g_fragmentShaderID ;

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

void	initStartEndPoints() ;
void	drawStartEndPoints() ;

void	initScene() ;
void	drawScene() ;

void	initObstacles() ;
void	drawObstalces() ;

void	initLightning() ;
void	initLightningSteps() ;
void	drawLightning() ;


void	CalcFractalDimension( int iGridSize, float iSceneSize, const LightningTree& lightning, float& outOrigin, float& outJittered ) ;

void	AddCircle( Mesh& mesh, float cx, float cy, float fRadius, int iSegment, float fIntensity = 1.0f ) ;
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
void	setLight()
{
	//mat4 lightmodelMat = Angel::RotateY( g_theta ) ;
	mat4 lightmodelMat ;	// identity matrix
	glUniformMatrix4fv( glGetUniformLocation( g_IDShader, "mlModel" ), 1, GL_TRUE, value_ptr( lightmodelMat ) ) ;
}

// Setting object's modelMatrix
void	setModel()
{
	mat4 modelMat( 1.0f ) ;
	glUniformMatrix4fv( glGetUniformLocation( g_IDShader, "mModel" ), 1, GL_TRUE, value_ptr( modelMat ) ) ;
}

// Setting object's viewMatrix
void	setView()
{
	float fZoom = 1.0f ;

	vec3 eye = vec3( 0.0, 0.0, 50 ) ;
	vec3 to = vec3( 0.0, 0.0, 0.0 ) ;
	vec3 up = vec3( 0.0, 1.0, 0.0 ) ;

	// World to Camera(Eye - wc)
	mat4 viewMat = glm::lookAt( eye, to, up ) * glm::mat4( fZoom ) ;
	glUniformMatrix4fv( glGetUniformLocation( g_IDShader, "mView" ), 1, GL_TRUE, value_ptr( viewMat ) ) ;
}

// Setting object's projectionMatrix
void	setProjection()
{
/*/
	// Camera to Clipping
	mat4 projectionMat = glm::perspective( 45.0f, (float)g_width / (float)g_height, 1.0f, 100.0f ) ;
	glUniformMatrix4fv( glGetUniformLocation( g_IDShader, "mProjection" ), 1, GL_TRUE, value_ptr( projectionMat ) ) ;
//*/

	mat4 projectionMat = glm::ortho( -400.0f, 400.0f, -400.0f, 400.0f, 1.0f, 100.0f ) ;
	glUniformMatrix4fv( glGetUniformLocation( g_IDShader, "mProjection" ), 1, GL_TRUE, value_ptr( projectionMat ) ) ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void	initGrid()
{
	g_meshGrid.clearBuffer() ;

	int iGridSize = g_map.GetGridSize() ;

	float fDiff = ( -200.0f / iGridSize ) ;
	float x ;
	float y ;

	int iDrawGridSize = iGridSize ;

	for ( int i = 0; i < iDrawGridSize + 1; ++i )
	{
		y = fDiff * i + 100.0f ;

		g_meshGrid.m_vVertices.push_back( vec4( -100.0f, y, 0.0f, 1.0f ) ) ;
		g_meshGrid.m_vVertices.push_back( vec4( 100.0f, y, 0.0f, 1.0f ) ) ;
	}

	for ( int i = 0; i < iDrawGridSize + 1; ++i )
	{
		x = -fDiff * i - 100.0f ;

		g_meshGrid.m_vVertices.push_back( vec4( x, -100.0f, 0.0f, 1.0f ) ) ;
		g_meshGrid.m_vVertices.push_back( vec4( x, 100.0f, 0.0f, 1.0f ) ) ;
	}

	g_meshGrid.setDrawLines( true ) ;
	g_meshGrid.setArray() ;
	g_meshGrid.setBuffer() ;

	glUseProgram( g_IDShader ) ;

	GLuint vPosition = glGetAttribLocation( g_IDShader, "vPosition" ) ;
	g_meshGrid.setAttrLocPosition( vPosition ) ;

	GLuint vColor = glGetUniformLocation( g_IDShader, "vColor" ) ;
	g_meshGrid.setUniformLocDiffuse( vColor, vec4( 0.3f, 0.3f, 0.3f, 1.0f ) ) ;
}

void	drawGrid()
{
	glUseProgram( g_IDShader ) ;

	g_meshGrid.drawBuffer() ;
}

void	initScene()
{
	glUseProgram( g_IDShader ) ;

	// there are only obstacle objects in the scene or empty scene
	int iCount = g_map.GetObstacleCellCount() ;
	if ( iCount > 0 )
	{
		int iGridSize = g_map.GetGridSize() ;

		float fDiff = -SCENE_SIZE / iGridSize ;
		float x ;
		float y ;

		for ( int i = 0; i < iCount; ++i )
		{
			const SimpleCell* pCell = g_map.GetObstacleCell( i ) ;
			if ( pCell )
			{
				x = -fDiff * pCell->m_iX - HALF_SCENE_SIZE ;
				y = fDiff * pCell->m_iY + HALF_SCENE_SIZE ;


				g_meshScene.m_vVertices.push_back( vec4( x, y, 0.0f, 1.0f ) ) ;
				g_meshScene.m_vVertices.push_back( vec4( x, y + fDiff, 0.0f, 1.0f ) ) ;
				g_meshScene.m_vVertices.push_back( vec4( x - fDiff, y, 0.0f, 1.0f ) ) ;

				g_meshScene.m_vVertices.push_back( vec4( x, y + fDiff, 0.0f, 1.0f ) ) ;
				g_meshScene.m_vVertices.push_back( vec4( x - fDiff, y, 0.0f, 1.0f ) ) ;
				g_meshScene.m_vVertices.push_back( vec4( x - fDiff, y + fDiff, 0.0f, 1.0f ) ) ;
			}
		}

		g_meshScene.setArray() ;
		g_meshScene.setBuffer() ;

		GLuint vPosition = glGetAttribLocation( g_IDShader, "vPosition" ) ;
		g_meshScene.setAttrLocPosition( vPosition ) ;

		GLuint vColor = glGetUniformLocation( g_IDShader, "vColor" ) ;
		g_meshScene.setUniformLocDiffuse( vColor, vec4( 0.8f, 0.45f, 0.24f, 1.0f ) ) ;
	}

	// generate map boundary
	g_meshBoundary.clearBuffer() ;

	g_meshBoundary.m_vVertices.push_back( vec4( -HALF_SCENE_SIZE, HALF_SCENE_SIZE, 0.0f, 1.0f ) ) ;
	g_meshBoundary.m_vVertices.push_back( vec4( HALF_SCENE_SIZE, HALF_SCENE_SIZE, 0.0f, 1.0f ) ) ;

	g_meshBoundary.m_vVertices.push_back( vec4( HALF_SCENE_SIZE, HALF_SCENE_SIZE, 0.0f, 1.0f ) ) ;
	g_meshBoundary.m_vVertices.push_back( vec4( HALF_SCENE_SIZE, -HALF_SCENE_SIZE, 0.0f, 1.0f ) ) ;

	g_meshBoundary.m_vVertices.push_back( vec4( HALF_SCENE_SIZE, -HALF_SCENE_SIZE, 0.0f, 1.0f ) ) ;
	g_meshBoundary.m_vVertices.push_back( vec4( -HALF_SCENE_SIZE, -HALF_SCENE_SIZE, 0.0f, 1.0f ) ) ;

	g_meshBoundary.m_vVertices.push_back( vec4( -HALF_SCENE_SIZE, -HALF_SCENE_SIZE, 0.0f, 1.0f ) ) ;
	g_meshBoundary.m_vVertices.push_back( vec4( -HALF_SCENE_SIZE, HALF_SCENE_SIZE, 0.0f, 1.0f ) ) ;

	g_meshBoundary.setDrawLines( true ) ;
	g_meshBoundary.setArray() ;
	g_meshBoundary.setBuffer() ;

	glUseProgram( g_IDShader ) ;

	GLuint vPosition = glGetAttribLocation( g_IDShader, "vPosition" ) ;
	g_meshBoundary.setAttrLocPosition( vPosition ) ;

	GLuint vColor = glGetUniformLocation( g_IDShader, "vColor" ) ;
#ifdef USE_WHITE_BACKGROUND
	g_meshBoundary.setUniformLocDiffuse( vColor, vec4( 0.0f, 0.0f, 0.0f, 1.0f ) ) ;
#else
	g_meshBoundary.setUniformLocDiffuse( vColor, vec4( 1.0f, 1.0f, 1.0f, 1.0f ) ) ;
#endif
}

void	drawScene()
{
	glUseProgram( g_IDShader ) ;

	if ( !g_meshScene.m_vVertices.empty() )
	{
		g_meshScene.drawBuffer() ;
	}

	g_meshBoundary.drawBuffer() ;
}


void	initStartEndPoints()
{
	glUseProgram( g_IDShader ) ;

	int iGridSize = g_map.GetGridSize() ;

	float fDiff = -( SCENE_SIZE ) / iGridSize ;
	float x ;
	float y ;

	int iStartCount = g_map.GetStartCellCount() ;
	if ( iStartCount > 0 )
	{
		for ( int i = 0; i < iStartCount; ++i )
		{
			const SimpleCell* pCell = g_map.GetStartCell( i ) ;
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

		GLuint vPosition = glGetAttribLocation( g_IDShader, "vPosition" ) ;
		g_meshStartPos.setAttrLocPosition( vPosition ) ;

		GLuint vColor = glGetUniformLocation( g_IDShader, "vColor" ) ;
		g_meshStartPos.setUniformLocDiffuse( vColor, vec4( 1.0f, 0.0f, 0.0f, 1.0f ) ) ;
	}

	int iEndCount = g_map.GetEndCellCount() ;
	if ( iEndCount > 0 )
	{
		for ( int i = 0; i < iEndCount; ++i )
		{
			const SimpleCell* pCell = g_map.GetEndCell( i ) ;
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

		GLuint vPosition = glGetAttribLocation( g_IDShader, "vPosition" ) ;
		g_meshEndPos.setAttrLocPosition( vPosition ) ;

		GLuint vColor = glGetUniformLocation( g_IDShader, "vColor" ) ;
		g_meshEndPos.setUniformLocDiffuse( vColor, vec4( 0.0f, 0.0f, 1.0f, 1.0f ) ) ;
	}
}

void	drawStartEndPoints()
{
	glUseProgram( g_IDShader ) ;

	// draw 
	g_meshStartPos.drawBuffer() ;
	g_meshEndPos.drawBuffer() ;
}

void	initLightning()
{
	g_bProcessStarted = true  ;

#ifdef CHECK_FRACTAL_DIMENSION
	g_bJitter = true ;
#endif

int timeStart = glutGet( GLUT_ELAPSED_TIME ) ;
	
	g_map.Process( E_PT_ALL_STEP, true ) ;

int timeEnd = glutGet( GLUT_ELAPSED_TIME ) ;
int timeElapsed = timeEnd - timeStart ;
printf( "\n\nTime : %d ms (segments: %d, process: %d)\n", timeElapsed, g_map.GetLightningTree().GetNodeList().size(), g_map.GetProcessIndex() ) ;

printf( "\n" ) ;
const std::vector< int > vTimesPerSteps = g_map.GetTimesPerSteps() ;
for ( int i = 0; i < vTimesPerSteps.size(); ++i )
{
	printf( "%4d : %d ms\n", g_vTimeCheckSteps[ i ], vTimesPerSteps[ i ] ) ;
}

//	float fd = g_map.CalculateFractalDimension() ;

	float fDiff = -SCENE_SIZE / g_map.GetGridSize() ;
	float fCenter = -fDiff / 2.0f ;
	float sx, sy ;
	float tx, ty ;

	//const LightningTreeNode* pNode ;
	//const LightningTree& tree = g_map.GetLightningTree() ;
	//const std::vector< LightningTreeNode* > vNodes = tree.GetNodeList() ;
	LightningTreeNode* pNode ;
	LightningTree& tree = g_map.GetLightningTree() ;
	std::vector< LightningTreeNode* > vNodes = tree.GetNodeList() ;
	
	//std::vector< LightningTreeNode* >::const_iterator itr = vNodes.begin() ;
	std::vector< LightningTreeNode* >::iterator itr = vNodes.begin() ;
	while ( itr != vNodes.end() )
	{
		pNode = *itr ;
		if ( pNode && pNode->m_pParent )
		{
			if ( g_bJitter && pNode->m_pParent->m_bJittered )
			{
				sx = pNode->m_pParent->m_fXJittered ;
				sy = pNode->m_pParent->m_fYJittered ;
			}
			else
			{
				sx = -fDiff * pNode->m_pParent->m_iX - HALF_SCENE_SIZE + fCenter ;
				sy = fDiff * pNode->m_pParent->m_iY + HALF_SCENE_SIZE - fCenter ;

				if ( g_bJitter )
				{
					float fJitterSize = -fDiff ;
					int iJitterSize = int( -fDiff + 0.5f ) ;
					if ( iJitterSize < 1 )
					{
						iJitterSize = 1 ; 
					}

					sx += rand() % (int)( iJitterSize ) - ( fJitterSize / 2.0f ) ;
					sy += rand() % (int)( iJitterSize ) - ( fJitterSize / 2.0f ) ;

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
				tx = -fDiff * pNode->m_iX - HALF_SCENE_SIZE + fCenter ;
				ty = fDiff * pNode->m_iY + HALF_SCENE_SIZE - fCenter ;

				if ( g_bJitter )
				{
					float fJitterSize = -fDiff ;
					int iJitterSize = int( -fDiff + 0.5f ) ;
					if ( iJitterSize < 1 )
					{
						iJitterSize = 1 ; 
					}

					tx += rand() % (int)( iJitterSize ) - ( fJitterSize / 2.0f ) ;
					ty += rand() % (int)( iJitterSize ) - ( fJitterSize / 2.0f ) ;

					pNode->m_bJittered = true ;
					pNode->m_fXJittered = tx ;
					pNode->m_fYJittered = ty ;
				}
			}
		}

		g_meshLightning.m_vVertices.push_back( vec4( sx, sy, 0.0f, 1.0f ) ) ;
		g_meshLightning.m_vVertices.push_back( vec4( tx, ty, 0.0f, 1.0f ) ) ;

		++itr ;
	}
	
	g_meshLightning.setDrawLines( true ) ;
	g_meshLightning.setArray() ;
	g_meshLightning.setBuffer() ;

	GLuint vPosition = glGetAttribLocation( g_IDShader, "vPosition" ) ;
	g_meshLightning.setAttrLocPosition( vPosition ) ;

	GLuint vDiffuse = glGetUniformLocation( g_IDShader, "vColor" ) ;
#ifdef USE_WHITE_BACKGROUND
	g_meshLightning.setUniformLocDiffuse( vDiffuse, vec4( 0.0f, 0.0f, 0.0f, 1.0f ) ) ;
#else
	g_meshLightning.setUniformLocDiffuse( vDiffuse, vec4( 1.0f, 1.0f, 1.0f, 1.0f ) ) ;
#endif


#ifdef CHECK_FRACTAL_DIMENSION
	float fOrgDimension ;
	float fJitteredDimension ;
	
	CalcFractalDimension( g_map.GetGridSize(), SCENE_SIZE, g_map.GetLightningTree(), fOrgDimension, fJitteredDimension ) ;
	printf( "\n\nFractal dimension - original position: %f, jittered position: %f\n", fOrgDimension, fJitteredDimension ) ;
#endif
}

void	initLightningSteps()
{
	if ( !g_bProcessStarted )
	{
		return ;
	}

//int timeStart = glutGet( GLUT_ELAPSED_TIME ) ;
	
	bool bRet = g_map.Process( E_PT_ONE_STEP ) ;
	if ( !bRet )
	{
		char szTitle[ 256 ] ;
		sprintf( szTitle, "Lightning rendering - ETA : %d (Finished)", ETA ) ;
		glutSetWindowTitle( szTitle );

		return ;
	}

	g_meshLightning.clearBuffer() ;

//int timeEnd = glutGet( GLUT_ELAPSED_TIME ) ;
//int timeElapsed = timeEnd - timeStart ;
//printf( "\n\nTime : %d ms (segments: %d, process: %d)\n", timeElapsed, g_map.GetLightningTree().GetNodeList().size(), g_map.GetProcessIndex() ) ;

//printf( "\n" ) ;
//const std::vector< int > vTimesPerSteps = g_map.GetTimesPerSteps() ;
//for ( int i = 0; i < vTimesPerSteps.size(); ++i )
//{
//	printf( "%4d : %d ms\n", g_vTimeCheckSteps[ i ], vTimesPerSteps[ i ] ) ;
//}

//	float fd = g_map.CalculateFractalDimension() ;

	float fDiff = -SCENE_SIZE / g_map.GetGridSize() ;
	float fCenter = -fDiff / 2.0f ;
	float x ;
	float y ;

	//const LightningTreeNode* pNode ;
	//const LightningTree& tree = g_map.GetLightningTree() ;
	//const std::vector< LightningTreeNode* > vNodes = tree.GetNodeList() ;
	LightningTreeNode* pNode ;
	LightningTree& tree = g_map.GetLightningTree() ;
	std::vector< LightningTreeNode* > vNodes = tree.GetNodeList() ;

	CheckNumberOfBranches( vNodes.size() - 1 ) ;
	
	//std::vector< LightningTreeNode* >::const_iterator itr = vNodes.begin() ;
	std::vector< LightningTreeNode* >::iterator itr = vNodes.begin() ;
	while ( itr != vNodes.end() )
	{
		pNode = *itr ;
		if ( pNode && pNode->m_pParent )
		{
			x = -fDiff * pNode->m_pParent->m_iX - HALF_SCENE_SIZE + fCenter ;
			y = fDiff * pNode->m_pParent->m_iY + HALF_SCENE_SIZE - fCenter ;

			g_meshLightning.m_vVertices.push_back( vec4( x, y, 0.0f, 1.0f ) ) ;

			x = -fDiff * pNode->m_iX - HALF_SCENE_SIZE + fCenter ;
			y = fDiff * pNode->m_iY + HALF_SCENE_SIZE - fCenter ;

			g_meshLightning.m_vVertices.push_back( vec4( x, y, 0.0f, 1.0f ) ) ;
		}

		++itr ;
	}
	
	g_meshLightning.setDrawLines( true ) ;
	g_meshLightning.setArray() ;
	g_meshLightning.setBuffer() ;

	GLuint vPosition = glGetAttribLocation( g_IDShader, "vPosition" ) ;
	g_meshLightning.setAttrLocPosition( vPosition ) ;

	GLuint vDiffuse = glGetUniformLocation( g_IDShader, "vColor" ) ;
#ifdef USE_WHITE_BACKGROUND
	g_meshLightning.setUniformLocDiffuse( vDiffuse, vec4( 0.0f, 0.0f, 0.0f, 1.0f ) ) ;
#else
	g_meshLightning.setUniformLocDiffuse( vDiffuse, vec4( 1.0f, 1.0f, 1.0f, 1.0f ) ) ;
#endif
}

void	drawLightning()
{
	glUseProgram( g_IDShader ) ;

	g_meshLightning.drawBuffer() ;
}


///////
/*/
void	initGrid()
{
	int iGridSize = g_map.GetGridSize() ;

	float fDiff = -200.0f / iGridSize ;
	float x ;
	float y ;

	for ( int i = 0; i < iGridSize + 1; ++i )
	{
		y = fDiff * i + 100.0f ;

		g_meshGrid.m_vVertices.push_back( vec4( -100.0f, y, 0.0f, 1.0f ) ) ;
		g_meshGrid.m_vVertices.push_back( vec4( 100.0f, y, 0.0f, 1.0f ) ) ;
	}

	for ( int i = 0; i < iGridSize + 1; ++i )
	{
		x = -fDiff * i - 100.0f ;

		g_meshGrid.m_vVertices.push_back( vec4( x, -100.0f, 0.0f, 1.0f ) ) ;
		g_meshGrid.m_vVertices.push_back( vec4( x, 100.0f, 0.0f, 1.0f ) ) ;
	}

	g_meshGrid.setDrawLinesON() ;
	g_meshGrid.setArray() ;
	g_meshGrid.setBuffer() ;

	GLuint vPosition = glGetAttribLocation( g_IDShader, "vPosition" ) ;
	g_meshGrid.setAttrLocPosition( vPosition ) ;

	GLuint vDiffuse = glGetUniformLocation( g_IDShader, "vDiffuse" ) ;
	g_meshGrid.setUniformLocDiffuse( vDiffuse, vec4( 0.3f, 0.3f, 0.3f, 1.0f ) ) ;
}

void	drawGrid()
{
	g_meshGrid.drawBuffer() ;
}

void	initObstacles()
{
	int iCount = g_map.GetObstacleCellCount() ;
	int iGridSize = g_map.GetGridSize() ;

	float fDiff = -200.0f / iGridSize ;
	float x ;
	float y ;

	for ( int i = 0; i < iCount; ++i )
	{
		const SimpleCell* pCell = g_map.GetObstacleCell( i ) ;
		if ( pCell )
		{
			x = -fDiff * pCell->m_iX - 100.0f ;
			y = fDiff * pCell->m_iY + 100.0f ;
						
			g_meshObstacle.m_vVertices.push_back( vec4( x, y, 0.0f, 1.0f ) ) ;

			x += -fDiff ;
			y += fDiff ;

			g_meshObstacle.m_vVertices.push_back( vec4( x, y, 0.0f, 1.0f ) ) ;

			x -= -fDiff ;

			g_meshObstacle.m_vVertices.push_back( vec4( x, y, 0.0f, 1.0f ) ) ;

			x += -fDiff ;
			y -= fDiff ;

			g_meshObstacle.m_vVertices.push_back( vec4( x, y, 0.0f, 1.0f ) ) ;
		}
	}

	g_meshObstacle.setDrawLinesON() ;
	g_meshObstacle.setArray() ;
	g_meshObstacle.setBuffer() ;

	GLuint vPosition = glGetAttribLocation( g_IDShader, "vPosition" ) ;
	g_meshObstacle.setAttrLocPosition( vPosition ) ;

	GLuint vDiffuse = glGetUniformLocation( g_IDShader, "vDiffuse" ) ;
	g_meshObstacle.setUniformLocDiffuse( vDiffuse, vec4( 0.0f, 0.0f, 1.0f, 1.0f ) ) ;
}

void	drawObstalces()
{
	g_meshObstacle.drawBuffer() ;
}

void	initLightning()
{
	g_map.SetEta( ETA ) ;
	g_map.Process( E_PT_ALL_STEP, true ) ;

	float fd = g_map.CalculateFractalDimension() ;

	float fDiff = -200.0f / g_map.GetGridSize() ;
	float fCenter = -fDiff / 2.0f ;
	float x ;
	float y ;

	//const LightningTreeNode* pNode ;
	//const LightningTree& tree = g_map.GetLightningTree() ;
	//const std::vector< LightningTreeNode* > vNodes = tree.GetNodeList() ;
	LightningTreeNode* pNode ;
	LightningTree& tree = g_map.GetLightningTree() ;
	std::vector< LightningTreeNode* > vNodes = tree.GetNodeList() ;
	
	//std::vector< LightningTreeNode* >::const_iterator itr = vNodes.begin() ;
	std::vector< LightningTreeNode* >::iterator itr = vNodes.begin() ;
	while ( itr != vNodes.end() )
	{
		pNode = *itr ;
		if ( pNode && pNode->m_pParent )
		{
			x = -fDiff * pNode->m_pParent->m_iX - 100.0f + fCenter ;
			y = fDiff * pNode->m_pParent->m_iY + 100.0f - fCenter ;

			g_meshLightning.m_vVertices.push_back( vec4( x, y, 0.0f, 1.0f ) ) ;

			x = -fDiff * pNode->m_iX - 100.0f + fCenter ;
			y = fDiff * pNode->m_iY + 100.0f - fCenter ;

			g_meshLightning.m_vVertices.push_back( vec4( x, y, 0.0f, 1.0f ) ) ;
		}

		++itr ;
	}
	
	g_meshLightning.setDrawLinesON() ;
	g_meshLightning.setArray() ;
	g_meshLightning.setBuffer() ;

	GLuint vPosition = glGetAttribLocation( g_IDShader, "vPosition" ) ;
	g_meshLightning.setAttrLocPosition( vPosition ) ;

	GLuint vDiffuse = glGetUniformLocation( g_IDShader, "vDiffuse" ) ;
	//g_meshLightning.setUniformLocDiffuse( vDiffuse, vec4( 0.5f, 1.0f, 1.0f, 1.0f ) ) ;
	//g_meshLightning.setUniformLocDiffuse( vDiffuse, vec4( 1.0f, 0.0f, 0.0f, 1.0f ) ) ;
	g_meshLightning.setUniformLocDiffuse( vDiffuse, vec4( 1.0f, 1.0f, 1.0f, 1.0f ) ) ;
}

void	drawLightning()
{
	g_meshLightning.drawBuffer() ;
}
//*/
////////////////////////////////////////////////////////////////////////////////////////////////////////

void	init()
{
	g_IDShader = InitShader( "shader/vertex.glsl", NULL, "shader/fragment.glsl", &g_vertexShaderID, NULL, &g_fragmentShaderID ) ;
	glUseProgram( g_IDShader ) ;
	
	glEnable( GL_DEPTH_TEST ) ;
	glEnable( GL_MULTISAMPLE ) ;

	// load cgm map
	g_map.SetEta( ETA ) ;
	
	//g_map.Load( "./res/lightning_32.map" ) ;
	//g_map.Load( "./res/lightning_64.map" ) ;
	//g_map.Load( "./res/lightning_128.map" ) ;
	//g_map.Load( "./res/lightning_256.map" ) ;


	//g_map.Load( 512, 255, 0, 255, 511 ) ;	// eta = 2
	//g_map.Load( 512, 10, 0, 502, 511 ) ;	// eta = 2
	//g_map.Load( 256, 127, 0, 127, 255 ) ;	// eta = 2
	g_map.Load( 128, 63, 0, 63, 127 ) ;	// eta = 2
	//g_map.Load( 64, 32, 0, 32, 63 ) ;	// eta = 2
	
/*/
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
	g_map.SetTimeCheckSteps( g_vTimeCheckSteps ) ;
//*/
	
	
	// load grid mesh
	initGrid() ;
	initStartEndPoints() ;
	initScene() ;
	
	// generate lightning tree
//	initLightning() ;
			
	setLight() ;		// uniform vec4
	setModel() ;		// uniform mat4 - block
	setView() ;			// uniform mat4
	setProjection() ;	// uniform mat4
}


void	display()
{
	initLightningSteps() ;

#ifdef USE_WHITE_BACKGROUND
	glClearColor( 1.0, 1.0, 1.0, 1.0 ) ;
#else
	glClearColor( 0.0, 0.0, 0.0, 1.0 ) ;
#endif
	
	
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) ;

	setView() ;
	setLight() ;

	// Draw scene
	drawScene() ;
	drawLightning() ;

	if ( g_bDrawGrid )
	{
		drawGrid() ;
	}

	if ( g_bDrawStartEndPoints )
	{
		drawStartEndPoints() ;
	}

	glutSwapBuffers() ;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

void	destroyShaders()
{
	GLenum ErrorCheckValue = glGetError() ;

	glUseProgram( 0 ) ;

	glDetachShader( g_IDShader, g_vertexShaderID ) ;
	glDetachShader( g_IDShader, g_geometryShaderID ) ;
	glDetachShader( g_IDShader, g_fragmentShaderID ) ;

	glDeleteShader( g_vertexShaderID ) ;
	glDeleteShader( g_geometryShaderID ) ;
	glDeleteShader( g_fragmentShaderID ) ;

	glDeleteProgram( g_IDShader ) ;
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

		case 'g':
		{
			g_bDrawGrid = !g_bDrawGrid ;
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

		case ' ':
		{
			char szTitle[ 256 ] ;
			sprintf( szTitle, "Lightning rendering - ETA : %d (Started)", ETA ) ;
			glutSetWindowTitle( szTitle ) ;

			g_meshLightning.clearBuffer() ;

#ifdef CHECK_FRACTAL_DIMENSION
			initLightning() ;
#else
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

	glutInitContextVersion	( 3, 2 ) ;
	glutInitContextFlags	( GLUT_FORWARD_COMPATIBLE ) ;
	glutInitContextProfile	( GLUT_CORE_PROFILE ) ;
	
	char szTitle[ 256 ] ;
	sprintf( szTitle, "Lightning rendering (T. Kim) - ETA : %d", ETA ) ;
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

void		CalcFractalDimension( int iGridSize, float fSceneSize, const LightningTree& lightning, float& outOrigin, float& outJittered )
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

	const LightningTreeNode* pRoot = lightning.GetRoot() ;
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

void	AddCircle( Mesh& mesh, float cx, float cy, float fRadius, int iSegment, float fIntensity )
{
	float angleDiff = 2 * M_PI / (float)iSegment ;
	float angle = 0.0f ;

	for ( int i = 0; i < iSegment; ++i )
	{
		// center
		mesh.m_vVertices.push_back( glm::vec4( cx, cy, 0.0f, 1.0f ) ) ;
		mesh.m_vIntensities.push_back( fIntensity ) ;
		
		// first point
		mesh.m_vVertices.push_back( glm::vec4( cx + sinf( angle ) * fRadius, cy + cosf( angle ) * fRadius, 0.0f, 1.0f ) ) ;
		mesh.m_vIntensities.push_back( fIntensity ) ;

		angle += angleDiff ;
		if ( angle >= 2 * M_PI )
		{
			angle = 0.0f ;
		}

		// next angel point
		mesh.m_vVertices.push_back( glm::vec4( cx + sinf( angle ) * fRadius, cy + cosf( angle ) * fRadius, 0.0f, 1.0f ) ) ;
		mesh.m_vIntensities.push_back( fIntensity ) ;
	}
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
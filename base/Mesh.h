

#ifndef	__MESH_H__
#define __MESH_H__

#include "base.h"
#include <vector>


class Mesh
{
public :

	Mesh() 
		: m_strName(""), m_VAID( -1 )
		, m_VBIDVertices( -1 ), m_VBIDIntensities( -1 ), m_VBIDRadius( -1 ), m_VBIDTexCoords( -1 ), m_VBIDNormals( -1 ), m_IBIDIndices( -1 )
		, m_AttrLocPosition( -1 ), m_AttrLocIntensity( -1 ), m_AttrLocRadius( -1 ), m_AttrLocTexCoord( -1 ), m_AttrLocNormal( -1 )
		, m_UniformLocTrans( -1 ), m_UniformLocAmbient( -1 ), m_UniformLocDiffuse( -1 ), m_UniformLocSpecular( -1 ), m_UniformLocShininess( -1 ), m_UniformLocUseColor( -1 )
		, m_bDrawLine( false ), m_bDrawPoint( false )
	{
	}

	Mesh( std::string strName ) 
		:  m_strName( strName ), m_VAID( -1 )
		, m_VBIDVertices( -1 ), m_VBIDIntensities( -1 ), m_VBIDRadius( -1 ), m_VBIDTexCoords( -1 ), m_VBIDNormals( -1 ), m_IBIDIndices( -1 )
		, m_AttrLocPosition( -1 ), m_AttrLocIntensity( -1 ), m_AttrLocRadius( -1 ), m_AttrLocTexCoord( -1 ), m_AttrLocNormal( -1 )
		, m_UniformLocTrans( -1 ), m_UniformLocAmbient( -1 ), m_UniformLocDiffuse( -1 ), m_UniformLocSpecular( -1 ), m_UniformLocShininess( -1 ), m_UniformLocUseColor( -1 )
		, m_bDrawLine( false ), m_bDrawPoint( false )
	{
	}

	~Mesh()
	{
		clearBuffer() ;
	}


	// getter & setter
	std::string		getName() const					{ return m_strName ;					}
	void			setName( std::string& strName )	{ m_strName = strName ;					}

	GLuint			getVAID()						{ return m_VAID ;						}
	GLuint			getVBIDVertices()				{ return m_VBIDVertices ;				}
	GLuint			getVBIDIntensities()			{ return m_VBIDIntensities ;			}
	GLuint			getVBIDTexCoords()				{ return m_VBIDTexCoords ;				}
	GLuint			getVBIDNormals()				{ return m_VBIDNormals ;				}
	GLuint			getIBIDIndices()				{ return m_IBIDIndices ;				}

	bool			hasNormal() const				{ return ( m_vNormals.size() > 0 ) ;	}
		

	// set attribute & uniform location
	void	setAttrLocPosition( GLuint loc )		{ m_AttrLocPosition = loc ;				}
	void	setAttrLocIntensity( GLuint loc )		{ m_AttrLocIntensity = loc ;			}
	void	setAttrLocRadius( GLuint loc )			{ m_AttrLocRadius = loc ;				}
	void	setAttrLocTexCoord( GLuint loc )		{ m_AttrLocTexCoord = loc ;				}
	void	setAttrLocNormal( GLuint loc )			{ m_AttrLocNormal = loc ;				}

	void	setUniformLocTrans( GLuint loc, glm::mat4& matTrasform )	{ m_UniformLocTrans = loc ; m_matTrans = matTrasform ;					}
	void	setUniformLocAmbient( GLuint loc, glm::vec4& ambient )		{ m_UniformLocAmbient = loc ; m_vAmbient = ambient ;					}
	void	setUniformLocDiffuse( GLuint loc, glm::vec4& diffuse )		{ m_UniformLocDiffuse = loc ; m_vDiffuse = diffuse ;					}
	void	setUniformLocSpecular( GLuint loc, glm::vec4& specular )	{ m_UniformLocSpecular = loc ; m_vSpecular = specular ;					}
	void	setUniformLocShininess( GLuint loc, float shininess )		{ m_UniformLocShininess = loc ; m_fShininess = shininess ;				}
	void	setUniformLocUseColor( GLuint loc, bool bUse )				{ m_UniformLocUseColor = loc ; m_fUseColor = ( bUse ? 1.0f : 0.0f ) ;	}

	
	// drawing modes
	void	setDrawLines( bool bSet )		{ m_bDrawLine = bSet ;	}
	void	setDrawPoints( bool bSet )		{ m_bDrawPoint = bSet ;	}


	// mesh functions
	void	setArray() ;
	void	setBuffer() ;
	void	clearBuffer() ;
	void	drawBuffer() ;

	void	computeBoundingBox() ;
	void	computeNormal() ;
	

private :

	std::string		m_strName ;
	GLuint			m_VAID ;

	GLuint			m_VBIDVertices ;
	GLuint			m_VBIDIntensities ;
	GLuint			m_VBIDRadius ;
	GLuint			m_VBIDTexCoords ;
	GLuint			m_VBIDNormals ;
	GLuint			m_IBIDIndices ;
	
	GLuint			m_AttrLocPosition ;
	GLuint			m_AttrLocIntensity ;
	GLuint			m_AttrLocRadius ;
	GLuint			m_AttrLocTexCoord ;
	GLuint			m_AttrLocNormal ;

	GLuint			m_UniformLocTrans ;			glm::mat4		m_matTrans ;
	GLuint			m_UniformLocAmbient ;		glm::vec4		m_vAmbient ;
	GLuint			m_UniformLocDiffuse ;		glm::vec4		m_vDiffuse ;
	GLuint			m_UniformLocSpecular ;		glm::vec4		m_vSpecular ;
	GLuint			m_UniformLocShininess ;		float			m_fShininess ;
	GLuint			m_UniformLocUseColor ;		float			m_fUseColor ;

	// drawing modes
	bool			m_bDrawLine ;
	bool			m_bDrawPoint ;

public :

	// mesh data
	std::vector< glm::vec4 >		m_vVertices ;
	std::vector< GLfloat >			m_vIntensities ;
	std::vector< GLfloat >			m_vRadius ;
	std::vector< glm::vec2 >		m_vTexCoords ;
	std::vector< glm::vec3 >		m_vNormals ;
	std::vector< GLuint >			m_vIndices ;

	// bounding box
	GLfloat			m_BBMinX, m_BBMaxX, m_BBMinY, m_BBMaxY, m_BBMinZ, m_BBMaxZ ;
	glm::vec3		m_vBBSize ;
	glm::vec3		m_vBBCenter ;
	glm::mat4		m_matBBTrans ;

} ;

#endif

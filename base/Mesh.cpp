
#include "Mesh.h"
#include <iostream>


void	Mesh::setArray()
{
	glGenVertexArrays( 1, &m_VAID ) ;
	glBindVertexArray( m_VAID ) ;
}


void	Mesh::setBuffer()
{
	// move the data to GPU memory
	if ( m_vVertices.size() > 0 )
	{
		glGenBuffers( 1, &m_VBIDVertices ) ;
		glBindBuffer( GL_ARRAY_BUFFER, m_VBIDVertices ) ;
		glBufferData( GL_ARRAY_BUFFER, m_vVertices.size() * sizeof( m_vVertices[ 0 ] ), m_vVertices.data(), GL_STATIC_DRAW ) ;
	}

	if ( m_vIntensities.size() > 0 )
	{
		glGenBuffers( 1, &m_VBIDIntensities ) ;
		glBindBuffer( GL_ARRAY_BUFFER, m_VBIDIntensities ) ;
		glBufferData( GL_ARRAY_BUFFER, m_vIntensities.size() * sizeof( m_vIntensities[ 0 ] ), m_vIntensities.data(), GL_STATIC_DRAW ) ;
	}

	if ( m_vRadius.size() > 0 )
	{
		glGenBuffers( 1, &m_VBIDRadius ) ;
		glBindBuffer( GL_ARRAY_BUFFER, m_VBIDRadius ) ;
		glBufferData( GL_ARRAY_BUFFER, m_vRadius.size() * sizeof( m_vRadius[ 0 ] ), m_vRadius.data(), GL_STATIC_DRAW ) ;
	}

	if ( m_vTexCoords.size() > 0 )
	{
		glGenBuffers( 1, &m_VBIDTexCoords ) ;
		glBindBuffer( GL_ARRAY_BUFFER, m_VBIDTexCoords ) ;
		glBufferData( GL_ARRAY_BUFFER, m_vTexCoords.size() * sizeof( m_vTexCoords[ 0 ] ), m_vTexCoords.data(), GL_STATIC_DRAW ) ;
	}

	if ( m_vNormals.size() > 0 )
	{
		glGenBuffers( 1, &m_VBIDNormals ) ;
		glBindBuffer( GL_ARRAY_BUFFER, m_VBIDNormals ) ;
		glBufferData( GL_ARRAY_BUFFER, m_vNormals.size() * sizeof( m_vNormals[ 0 ] ), m_vNormals.data(), GL_STATIC_DRAW ) ;
	}

	if ( m_vIndices.size() > 0 )
	{
		glGenBuffers( 1, &m_IBIDIndices ) ;
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_IBIDIndices ) ;
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, m_vIndices.size() * sizeof( m_vIndices[ 0 ] ), m_vIndices.data(), GL_STATIC_DRAW ) ;
	}
}


void	Mesh::clearBuffer()
{
	if ( -1 != m_VBIDVertices )		glDeleteBuffers( 1, &m_VBIDVertices ) ;
	if ( -1 != m_VBIDIntensities )	glDeleteBuffers( 1, &m_VBIDIntensities ) ;
	if ( -1 != m_VBIDRadius )		glDeleteBuffers( 1, &m_VBIDRadius ) ;
	if ( -1 != m_VBIDTexCoords )	glDeleteBuffers( 1, &m_VBIDTexCoords ) ;
	if ( -1 != m_VBIDNormals )		glDeleteBuffers( 1, &m_VBIDNormals ) ;
	if ( -1 != m_IBIDIndices )		glDeleteBuffers( 1, &m_IBIDIndices ) ;

	m_vVertices.clear() ;
	m_vIntensities.clear() ;
	m_vRadius.clear() ;
	m_vTexCoords.clear() ;
	m_vNormals.clear() ;
	m_vIndices.clear() ;

	if ( -1 != m_VAID )				glDeleteVertexArrays( 1, &m_VAID ) ;
	m_VAID = -1 ;
}


void	Mesh::drawBuffer()
{
	glBindVertexArray( m_VAID ) ;

	if ( -1 != m_VBIDVertices && -1 != m_AttrLocPosition )
	{
		glEnableVertexAttribArray( m_AttrLocPosition ) ;
		glBindBuffer( GL_ARRAY_BUFFER, this->m_VBIDVertices ) ;
		glVertexAttribPointer( m_AttrLocPosition, 4, GL_FLOAT, GL_FALSE, 0, 0 ) ;
	}

	if ( -1 != m_VBIDIntensities && -1 != m_AttrLocIntensity )
	{
		glEnableVertexAttribArray( m_AttrLocIntensity ) ;
		glBindBuffer( GL_ARRAY_BUFFER, m_VBIDIntensities ) ;
		glVertexAttribPointer( m_AttrLocIntensity, 1, GL_FLOAT, GL_FALSE, 0, 0 ) ;
	}

	if ( -1 != m_VBIDRadius && -1 != m_AttrLocRadius )
	{
		glEnableVertexAttribArray( m_AttrLocRadius ) ;
		glBindBuffer( GL_ARRAY_BUFFER, m_VBIDRadius ) ;
		glVertexAttribPointer( m_AttrLocRadius, 1, GL_FLOAT, GL_FALSE, 0, 0 ) ;
	}

	if ( -1 != m_VBIDTexCoords && -1 != m_AttrLocTexCoord )
	{
		glEnableVertexAttribArray( this->m_AttrLocTexCoord ) ;
		glBindBuffer( GL_ARRAY_BUFFER, this->m_VBIDTexCoords ) ;
		glVertexAttribPointer( this->m_AttrLocTexCoord, 2, GL_FLOAT, GL_FALSE, 0, 0 ) ;
	}
	
	if ( -1 != m_VBIDNormals && -1 != m_AttrLocNormal )
	{
		glEnableVertexAttribArray( m_AttrLocNormal ) ;
		glBindBuffer( GL_ARRAY_BUFFER, m_VBIDNormals ) ;
		glVertexAttribPointer( m_AttrLocNormal, 3, GL_FLOAT, GL_FALSE, 0, 0 ) ;
	}


	if ( -1 == m_UniformLocTrans )		m_matTrans = glm::mat4( 1.0 ) ;
	glUniformMatrix4fv( m_UniformLocTrans, 1, GL_TRUE, glm::value_ptr( m_matTrans ) ) ;

	if ( -1 == m_UniformLocAmbient )	m_vAmbient = glm::vec4( 0.2, 0.2, 0.2, 1.0 ) ;
	glUniform4fv( m_UniformLocAmbient, 1, glm::value_ptr( m_vAmbient ) ) ;

	if ( -1 == m_UniformLocDiffuse )	m_vDiffuse = glm::vec4( 1.0, 1.0, 1.0, 1.0 ) ;
	glUniform4fv( m_UniformLocDiffuse, 1, glm::value_ptr( m_vDiffuse ) ) ;

	if ( -1 == m_UniformLocSpecular )	m_vSpecular = glm::vec4( 1.0, 1.0, 1.0, 1.0 ) ;
	glUniform4fv( m_UniformLocSpecular, 1, glm::value_ptr( m_vSpecular ) ) ;

	if ( -1 == m_UniformLocShininess )	m_fShininess = 5.0 ;
	glUniform1f( m_UniformLocShininess, m_fShininess ) ;

	if ( -1 == m_UniformLocUseColor ) m_fUseColor = 0.0f ;
	glUniform1f( m_UniformLocUseColor, m_fUseColor ) ;


	if ( -1 != m_IBIDIndices )
	{
		int iSize ;
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_IBIDIndices ) ;
		glGetBufferParameteriv( GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &iSize ) ;
		glDrawElements( GL_TRIANGLES, iSize / sizeof( GLuint ), GL_UNSIGNED_INT, 0 ) ;
	}
	else
	{
		if ( m_bDrawPoint )
		{
			glDrawArrays( GL_POINTS, 0, m_vVertices.size() ) ;
		}
		else if ( m_bDrawLine )
		{
			glDrawArrays( GL_LINES, 0, m_vVertices.size() ) ;
		}
		else
		{
			glDrawArrays( GL_TRIANGLES, 0, m_vVertices.size() ) ;
		}
	}
}


void	Mesh::computeBoundingBox()
{
	if ( m_vVertices.size() <= 0 )
	{
		std::cerr << "There are no vertices !!" << std::endl ;
		return ;
	}

	m_BBMinX = m_BBMaxX == m_vVertices[ 0 ].x ;
	m_BBMinY = m_BBMaxY == m_vVertices[ 0 ].y ;
	m_BBMinZ = m_BBMaxZ == m_vVertices[ 0 ].z ;

	for ( int i = 0; i < m_vVertices.size(); ++i )
	{
		if ( m_vVertices[ i ].x < m_BBMinX ) m_BBMinX = m_vVertices[ i ].x ;
		if ( m_vVertices[ i ].x > m_BBMaxX ) m_BBMaxX = m_vVertices[ i ].x ;

		if ( m_vVertices[ i ].y < m_BBMinY ) m_BBMinY = m_vVertices[ i ].y ;
		if ( m_vVertices[ i ].y > m_BBMaxY ) m_BBMaxY = m_vVertices[ i ].y ;

		if ( m_vVertices[ i ].z < m_BBMinZ ) m_BBMinZ = m_vVertices[ i ].z ;
		if ( m_vVertices[ i ].z > m_BBMaxZ ) m_BBMaxZ = m_vVertices[ i ].z ;
	}

	m_vBBSize = glm::vec3( m_BBMaxX - m_BBMinX, m_BBMaxY - m_BBMinY, m_BBMaxZ - m_BBMinZ ) ;
	m_vBBCenter = glm::vec3( ( m_BBMaxX + m_BBMinX ) / 2.0f, ( m_BBMaxY + m_BBMinY ) / 2.0f, ( m_BBMaxZ + m_BBMinZ ) / 2.0f ) ;

	glm::mat4 matScale( 1.0f ) ;
	matScale[ 0 ][ 0 ] = m_vBBSize[ 0 ] ;
	matScale[ 1 ][ 1 ] = m_vBBSize[ 1 ] ;
	matScale[ 2 ][ 2 ] = m_vBBSize[ 2 ] ;

	glm::mat4 matTrans( 1.0f ) ;
	matTrans[ 0 ][ 3 ] = m_vBBCenter[ 0 ] ;
	matTrans[ 1 ][ 3 ] = m_vBBCenter[ 1 ] ;
	matTrans[ 2 ][ 3 ] = m_vBBCenter[ 2 ] ;

	m_matBBTrans = matScale * matTrans ;
}


void	Mesh::computeNormal()
{
	int iVertexSize = m_vVertices.size() ;
	int iFaceSize = m_vIndices.size() / 3 ;

	m_vNormals.clear() ;

	std::vector< int >	vecSharedFaceCount ;
	vecSharedFaceCount.assign( iVertexSize, 0 ) ;

	m_vNormals.assign( iVertexSize, glm::vec3( 0.0f, 0.0f, 0.0f ) ) ;

	int a, b, c ;
	glm::vec3 va, vb, vc ;
	glm::vec3 n ;

	for ( int i = 0; i < iFaceSize; ++i )
	{
		// calculate face normal vector
		a = m_vIndices[ i * 3 ] ;
		b = m_vIndices[ i * 3 + 1 ] ;
		c = m_vIndices[ i * 3 + 2 ] ;

		va = m_vVertices[ a ] ;
		vb = m_vVertices[ b ] ;
		vc = m_vVertices[ c ] ;

		n = glm::normalize( glm::cross( vb - va, vc - va ) ) ;

		m_vNormals[ a ] += n ;
		m_vNormals[ b ] += n ;
		m_vNormals[ c ] += n ;

		++vecSharedFaceCount[ a ] ;
		++vecSharedFaceCount[ b ] ;
		++vecSharedFaceCount[ c ] ;
	}

	for ( int i = 0; i < iVertexSize; ++i )
	{
		if ( 0 != vecSharedFaceCount[ i ] )
		{
			m_vNormals[ i ] /= vecSharedFaceCount[ i ] ;
		}
	}
}


#version 330 core

layout ( lines ) in ;
layout ( triangle_strip, max_vertices = 180 ) out ;

in VS_OUT {
    float intensity ;
	float radius ;
} gs_in[] ;

out float	fOutIntensity ;

// MVP Matrices
uniform mat4	mModel ;
uniform mat4	mView ;
uniform mat4	mProjection ;


void	AddSphere( float sx, float sy, float sz, float fRadius, float fIntensity )
{
	float ELPSILON = 0.0001f ;
	float M_PI = 3.14159265358979323846f ;
	int iDivide = 4 ;
	int iSegment = 6 ;

	mat4 MVP = mProjection * mView * mModel ;

	float x, y, z ;
	float nx, ny, nz ;

	vec3 vVertices[ 4 * 6 ] ;	// iDivide * iSegment

	float fDeltaPhi = ( 2 * M_PI ) / iSegment ;
	float fDeltaTheta = M_PI / ( iSegment + 2 ) ;	// 2 pole position

	float fTheta = 0 ;
	float fPhi = 0 ;

	// north pole ( sx, sy, sz + fRadius ) // (0, 0, 1)
	vec3 vNorthPole = vec3( sx, sy, sz + fRadius ) ;
	
	int iIndex = 0 ;
	for ( int i = 0; i < iSegment; ++i )
	{
		fTheta += fDeltaTheta ;

		for ( int j = 0; j < iSegment; ++j )
		{
			fPhi += fDeltaPhi ;

			x = sx + fRadius * sin( fTheta ) * cos( fPhi ) ;
			y = sy + fRadius * sin( fTheta ) * sin( fPhi ) ;
			z = sz + fRadius * cos( fTheta ) ;

			// add vertex(x,y,z)
			vVertices[ iIndex++ ] = vec3( x, y, z ) ;
		}
	}

	// south pole ( sx, sy, sz - fRadius ) // ( 0, 0, -1 )
	vec3 vSouthPole = vec3( sx, sy, sz - fRadius ) ;


	// generate triangles
	int iNext ;
	int iStart ;

	int iIndex2 ;	// for the next ring
	int iNext2 ;	// for the next ring
	int iStart2 ;	// for the next ring


	fOutIntensity = fIntensity ;

	// - north pole
	for ( int i = 0; i < iSegment; ++i )
	{
		iNext = i + 1 ;

		if ( i == iSegment - 1 )
		{
			iNext = 0 ;
		}

		gl_Position = MVP * vec4( vNorthPole.x, vNorthPole.y, vNorthPole.z, 1.0f ) ;
		EmitVertex() ;

		gl_Position = MVP * vec4( vVertices[ i ].x, vVertices[ i ].y, vVertices[ i ].z, 1.0f ) ;
		EmitVertex() ;

		gl_Position = MVP * vec4( vVertices[ iNext ].x, vVertices[ iNext ].y, vVertices[ iNext ].z, 1.0f ) ;
		EmitVertex() ;
		EndPrimitive() ;
	}

	// south pole
	iIndex = ( iDivide - 1 ) * iSegment ;
	iStart = iIndex ;

	for ( int i = 0; i < iSegment; ++i )
	{
		iNext = iIndex + 1 ;

		if ( i == iSegment - 1 )
		{
			iNext = iStart ;
		}

		gl_Position = MVP * vec4( vVertices[ iNext ].x, vVertices[ iNext ].y, vVertices[ iNext ].z, 1.0f ) ;
		EmitVertex() ;

		gl_Position = MVP * vec4( vVertices[ iIndex ].x, vVertices[ iIndex ].y, vVertices[ iIndex ].z, 1.0f ) ;
		EmitVertex() ;

		gl_Position = MVP * vec4( vSouthPole.x, vSouthPole.y, vSouthPole.z, 1.0f ) ;
		EmitVertex() ;
		EndPrimitive() ;

		++iIndex ;
	}

	// body
	for ( int i = 0; i < iDivide - 1; ++i )
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

			gl_Position = MVP * vec4( vVertices[ iNext ].x, vVertices[ iNext ].y, vVertices[ iNext ].z, 1.0f ) ;
			EmitVertex() ;

			gl_Position = MVP * vec4( vVertices[ iIndex ].x, vVertices[ iIndex ].y, vVertices[ iIndex ].z, 1.0f ) ;
			EmitVertex() ;

			gl_Position = MVP * vec4( vVertices[ iIndex2 ].x, vVertices[ iIndex2 ].y, vVertices[ iIndex2 ].z, 1.0f ) ;
			EmitVertex() ;
			EndPrimitive() ;


			gl_Position = MVP * vec4( vVertices[ iNext ].x, vVertices[ iNext ].y, vVertices[ iNext ].z, 1.0f ) ;
			EmitVertex() ;

			gl_Position = MVP * vec4( vVertices[ iIndex2 ].x, vVertices[ iIndex2 ].y, vVertices[ iIndex2 ].z, 1.0f ) ;
			EmitVertex() ;

			gl_Position = MVP * vec4( vVertices[ iNext2 ].x, vVertices[ iNext2 ].y, vVertices[ iNext2 ].z, 1.0f ) ;
			EmitVertex() ;
			EndPrimitive() ;


			++iIndex ;
			++iIndex2 ;
		}
	}
}



void main()
{
//	AddCylinder( gl_in[ 0 ].gl_Position.x, gl_in[ 0 ].gl_Position.y, gl_in[ 0 ].gl_Position.z,
//				 gl_in[ 1 ].gl_Position.x, gl_in[ 1 ].gl_Position.y, gl_in[ 1 ].gl_Position.z,
//				 gs_in[ 0 ].radius, gs_in[ 0 ].intensity, gs_in[ 1 ].intensity ) ;

//	AddSphere(gl_in[ 0 ].gl_Position.x, gl_in[ 0 ].gl_Position.y, gl_in[ 0 ].gl_Position.z, gs_in[ 0 ].radius, gs_in[ 0 ].intensity ) ;
	AddSphere(gl_in[ 1 ].gl_Position.x, gl_in[ 1 ].gl_Position.y, gl_in[ 1 ].gl_Position.z, gs_in[ 1 ].radius, gs_in[ 1 ].intensity ) ;
}  
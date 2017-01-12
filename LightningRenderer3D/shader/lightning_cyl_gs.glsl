#version 330 core

layout ( lines ) in ;
layout ( triangle_strip, max_vertices = 96 ) out ;

in VS_OUT {
    float intensity ;
	float radius ;
} gs_in[] ;

out float	fOutIntensity ;

// MVP Matrices
uniform mat4	mModel ;
uniform mat4	mView ;
uniform mat4	mProjection ;


void	AddCylinder( float sx, float sy, float sz, float ex, float ey, float ez, float fRadius, float fStartIntensity, float fEndIntensity )
{
	float ELPSILON = 0.0001f ;
	float M_PI = 3.14159265358979323846f ;
	int iSegment = 8 ;

	mat4 MVP = mProjection * mView * mModel ;

	float fLength = sqrt( ( ex - sx ) * ( ex - sx ) + ( ey - sy ) * ( ey - sy ) + ( ez - sz ) * ( ez - sz ) ) ;

	vec3 vTopVertices[ 8 ] ;		// iSegment
	vec3 vBottomVertices[ 8 ] ;		// iSegment

	float x, y, z ;

	// create base vectors
	vec3 vStartToEnd = vec3( ex - sx, ey - sy, ez - sz ) ;
	vStartToEnd = normalize( vStartToEnd ) ;

	vec3 vXAxis = vec3( 1, 0, 0 ) ;
	vec3 vYAxis = vec3( 0, 1, 0 ) ;

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
	float angleDiff = 2 * M_PI / iSegment ;
	float angle = 0.0f ;
		
	for ( int i = 0; i < iSegment; ++i )
	{
		// top vertices
		x = sx + ( vRight.x * cos( angle ) + vUp.x * sin( angle ) ) * fRadius ;//+ vStartToEnd.x * 0 ;
		y = sy + ( vRight.y * cos( angle ) + vUp.y * sin( angle ) ) * fRadius ;//+ vStartToEnd.y * 0 ;
		z = sz + ( vRight.z * cos( angle ) + vUp.z * sin( angle ) ) * fRadius ;//+ vStartToEnd.z * 0 ;

		vTopVertices[ i ] = vec3( x, y, z ) ;

		// bottom vertices
		x = sx + ( vRight.x * cos( angle ) + vUp.x * sin( angle ) ) * fRadius + vStartToEnd.x * fLength ;
		y = sy + ( vRight.y * cos( angle ) + vUp.y * sin( angle ) ) * fRadius + vStartToEnd.y * fLength ;
		z = sz + ( vRight.z * cos( angle ) + vUp.z * sin( angle ) ) * fRadius + vStartToEnd.z * fLength ;

		vBottomVertices[ i ] = vec3( x, y, z ) ;

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

		fOutIntensity = fStartIntensity ;

		// center
		gl_Position = MVP * vec4( sx, sy, sz, 1.0f ) ;
		EmitVertex() ;

		// first vertex
		gl_Position = MVP * vec4( vTopVertices[ iIndex ].x, vTopVertices[ iIndex ].y, vTopVertices[ iIndex ].z, 1.0f ) ;
		EmitVertex() ;

		// first vertex
		gl_Position = MVP * vec4( vTopVertices[ iNextIndex ].x, vTopVertices[ iNextIndex ].y, vTopVertices[ iNextIndex ].z, 1.0f ) ;
		EmitVertex() ;
		EndPrimitive() ;
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

		fOutIntensity = fEndIntensity ;

		// center
		gl_Position = MVP * vec4( ex, ey, ez, 1.0f ) ;
		EmitVertex() ;

		// first vertex
		gl_Position = MVP * vec4( vBottomVertices[ iIndex ].x, vBottomVertices[ iIndex ].y, vBottomVertices[ iIndex ].z, 1.0f ) ;
		EmitVertex() ;

		// next vertex
		gl_Position = MVP * vec4( vBottomVertices[ iNextIndex ].x, vBottomVertices[ iNextIndex ].y, vBottomVertices[ iNextIndex ].z, 1.0f ) ;
		EmitVertex() ;
		EndPrimitive() ;
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

		fOutIntensity = fStartIntensity ;

		gl_Position = MVP * vec4( vTopVertices[ iNextIndex ].x, vTopVertices[ iNextIndex ].y, vTopVertices[ iNextIndex ].z, 1.0f ) ;
		EmitVertex() ;

		gl_Position = MVP * vec4( vTopVertices[ iIndex ].x, vTopVertices[ iIndex ].y, vTopVertices[ iIndex ].z, 1.0f ) ;
		EmitVertex() ;

		fOutIntensity = fEndIntensity ;
		gl_Position = MVP * vec4( vBottomVertices[ iIndex ].x, vBottomVertices[ iIndex ].y, vBottomVertices[ iIndex ].z, 1.0f ) ;
		EmitVertex() ;
		EndPrimitive() ;


		fOutIntensity = fStartIntensity ;
		gl_Position = MVP * vec4( vTopVertices[ iNextIndex ].x, vTopVertices[ iNextIndex ].y, vTopVertices[ iNextIndex ].z, 1.0f ) ;
		EmitVertex() ;

		fOutIntensity = fEndIntensity ;
		gl_Position = MVP * vec4( vBottomVertices[ iIndex ].x, vBottomVertices[ iIndex ].y, vBottomVertices[ iIndex ].z, 1.0f ) ;
		EmitVertex() ;

		gl_Position = MVP * vec4( vBottomVertices[ iNextIndex ].x, vBottomVertices[ iNextIndex ].y, vBottomVertices[ iNextIndex ].z, 1.0f ) ;
		EmitVertex() ;
		EndPrimitive() ;
	}
}

void	AddSphere( float sx, float sy, float sz, float fRadius, float fIntensity )
{
	float ELPSILON = 0.0001f ;
	float M_PI = 3.14159265358979323846f ;
	int iSegment = 8 ;

	mat4 MVP = mProjection * mView * mModel ;

	float x, y, z ;
	float nx, ny, nz ;

	vec3 vVertices[ 8 * 8 ] ;	// iSegment * iSegment

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
	iIndex = ( iSegment - 1 ) * iSegment ;
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
	AddCylinder( gl_in[ 0 ].gl_Position.x, gl_in[ 0 ].gl_Position.y, gl_in[ 0 ].gl_Position.z,
				 gl_in[ 1 ].gl_Position.x, gl_in[ 1 ].gl_Position.y, gl_in[ 1 ].gl_Position.z,
				 gs_in[ 0 ].radius, gs_in[ 0 ].intensity, gs_in[ 1 ].intensity ) ;

//	AddSphere(gl_in[ 0 ].gl_Position.x, gl_in[ 0 ].gl_Position.y, gl_in[ 0 ].gl_Position.z, gs_in[ 0 ].radius, gs_in[ 0 ].intensity ) ;
//	AddSphere(gl_in[ 1 ].gl_Position.x, gl_in[ 1 ].gl_Position.y, gl_in[ 1 ].gl_Position.z, gs_in[ 1 ].radius, gs_in[ 1 ].intensity ) ;
}  
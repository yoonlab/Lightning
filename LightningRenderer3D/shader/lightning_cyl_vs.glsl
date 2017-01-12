#version 330

// Input from the application
layout ( location = 0 ) in vec3		vPosition ;
layout ( location = 1 ) in float	fIntensity ;
layout ( location = 2 ) in float	fRadius ;

// MVP Matrices
uniform mat4	mModel ;
uniform mat4	mView ;
uniform mat4	mProjection ;

out VS_OUT
{
	float		intensity ;
	float		radius ;
} vs_out ;


void main()
{
//	gl_Position = mProjection * mView * mModel * vec4( vPosition, 1.0 ) ;
	gl_Position = vec4( vPosition, 1.0 ) ;

	vs_out.intensity = fIntensity ;
	vs_out.radius = fRadius ;
}

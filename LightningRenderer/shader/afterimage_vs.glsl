#version 330

// Input from the application
layout ( location = 0 ) in vec3		vPosition ;
layout ( location = 1 ) in float	fIntensity ;

// MVP Matrices
uniform mat4	mModel ;
uniform mat4	mView ;
uniform mat4	mProjection ;

uniform float	fAttenuation ;

out float		outIntensity ;


void main()
{
	gl_Position = mProjection * mView * mModel * vec4( vPosition, 1.0 ) ;

	outIntensity = fIntensity * fAttenuation ;
	//outIntensity = fIntensity ;
}

#version 330

// Input from the application
layout ( location = 0 ) in vec3	vPosition ;

// MVP Matrices
uniform mat4	mModel ;
uniform mat4	mView ;
uniform mat4	mProjection ;


void main()
{
	gl_Position = mProjection * mView * mModel * vec4( vPosition, 1.0 ) ;

	float a = 0 ;
}

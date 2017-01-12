#version 150

// Input from the application
in	vec4	vPosition ;

// MVP Matrices
uniform mat4	mModel ;
uniform mat4	mView ;
uniform mat4	mProjection ;

uniform vec4	vDiffuse ;

void main()
{
	gl_Position = mProjection * mView * mModel * vPosition ;
}

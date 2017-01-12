#version 330

// Input from the application
layout ( location = 0 ) in vec3	vPosition ;
layout ( location = 1 ) in vec2 texCoords ;

// MVP Matrices
uniform mat4	mModel ;
uniform mat4	mView ;
uniform mat4	mProjection ;

out vec2 TexCoords ;

void main()
{
	gl_Position = mProjection * mView * mModel * vec4( vPosition, 1.0 ) ;
	TexCoords = texCoords ;
}

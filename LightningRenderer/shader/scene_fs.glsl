#version 330 core

layout ( location = 0 ) out vec4 FragColor ;
layout ( location = 1 ) out vec4 BrightColor ;

uniform float	useColor ;
uniform vec4	vColor ;
uniform sampler2D textureScene ;

in vec2 TexCoords ;


void main()
{
	if ( 1.0f == useColor )
	{
		FragColor = vColor ;
	}
	else
	{
		vec3 color = texture( textureScene, TexCoords ).rgb ;
		FragColor = vec4( color, 1.0 ) ;
	}
		
	BrightColor = vec4( 0.0, 0.0, 0.0, 1.0 ) ;
}

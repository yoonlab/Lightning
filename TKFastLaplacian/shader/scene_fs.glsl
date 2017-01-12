#version 330 core

layout ( location = 0 ) out vec4 FragColor ;
layout ( location = 1 ) out vec4 BrightColor ;

uniform vec4	vColor ;
uniform sampler2D textureScene ;

in vec2 TexCoords ;


void main()
{
//	vec3 color = texture( textureScene, TexCoords ).rgb ;
	//FragColor = vec4( vColor, 1.0 ) ;
	//FragColor = vec4( 0.0, 0.0, 0.0, 1.0 ) ;
	FragColor = vColor ;
	
	BrightColor = vec4( 0.0, 0.0, 0.0, 1.0 ) ;
}

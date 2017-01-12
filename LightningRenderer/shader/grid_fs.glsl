#version 330 core

layout ( location = 0 ) out vec4 FragColor ;
layout ( location = 1 ) out vec4 BrightColor ;

uniform vec4	vColor ;


void main()
{
	FragColor = vColor ;
	
	BrightColor = vec4( 0.0, 0.0, 0.0, 1.0 ) ;
}

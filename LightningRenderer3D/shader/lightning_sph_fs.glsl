#version 330 core

layout ( location = 0 ) out vec4 FragColor ;
layout ( location = 1 ) out vec4 BrightColor ;

uniform vec4	vColor ;

in float		fOutIntensity ;



void main()
{
	FragColor = vColor * fOutIntensity ;
	BrightColor = vColor * fOutIntensity ;
}

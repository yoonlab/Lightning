#version 330 core

layout ( location = 0 ) out vec4 FragColor ;
layout ( location = 1 ) out vec4 BrightColor ;

uniform vec4		vColor ;

in float			outIntensity ;


void main()
{
	float intentsity = outIntensity ;

	if ( intentsity < 0.2 )
	{
		intentsity = 0.2 ;
	}

	FragColor = vColor * intentsity ;
	BrightColor = vColor * intentsity ;
}

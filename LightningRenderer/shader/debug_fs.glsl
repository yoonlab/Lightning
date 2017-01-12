#version 330 core

out vec4 fColor ;

in vec2 TexCoords ;

uniform sampler2D textureToShow ;


void main()
{
	vec3 color = texture( textureToShow, TexCoords ).rgb ;

	fColor = vec4( color, 1.0 ) ;
}

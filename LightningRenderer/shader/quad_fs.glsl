#version 330 core

out vec4 fColor ;
in vec2 TexCoords ;

uniform sampler2D scene ;
uniform sampler2D bloomBlur ;

uniform bool bloom ;
uniform float exposure ;

void main()
{             
	const float gamma = 2.2 ;
    
	vec3 hdrColor = texture( scene, TexCoords ).rgb ;
	vec3 bloomColor = texture( bloomBlur, TexCoords ).rgb ;

	if ( bloom )
	{
		hdrColor += bloomColor ; // additive blending
	}

	// tone mapping
/*/
    vec3 result = vec3( 1.0 ) - exp( -hdrColor * exposure ) ;

    // also gamma correct while we're at it       
    result = pow( result, vec3( 1.0 / gamma ) ) ;
    
	fColor = vec4( result, 1.0f ) ;
//*/
	fColor = vec4( hdrColor, 1.0f ) ;
}

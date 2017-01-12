#version 430
#extension GL_ARB_compute_variable_group_size : enable

// uniform variable for grid map size
uniform int		grid_size ;
uniform float	pow_of_r ;
uniform int		positive_pos_count ;

struct positive_pos
{
	float x ;
	float y ;
	float z ;
	float padding ;
} ;

layout ( local_size_variable ) in ;
layout ( rgba32f, binding = 0 ) uniform image2D img_p ;
layout ( std430, binding = 1 ) buffer input
{
	positive_pos cell[ ] ;
} positive_pos_buffer ;


void main()
{
	uvec2 gid = gl_GlobalInvocationID.xy ;

	float P = 0 ;
	float r ;
	
	for ( int i = 0; i < positive_pos_count; ++i )
	{
		r = distance( vec2( gid ), vec2( positive_pos_buffer.cell[ i ].x, positive_pos_buffer.cell[ i ].y ) ) ;
		if ( pow_of_r > 1 )
		{
			r = pow( r, pow_of_r ) ;
		}

		P += 1.0 / r ;
	}

	imageStore( img_p, ivec2( gid ), vec4( P, 0, 0, 1 ) ) ;
}


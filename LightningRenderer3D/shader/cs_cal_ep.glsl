#version 430
#extension GL_ARB_compute_variable_group_size : enable

// uniform variable for grid map size
uniform int		grid_size ;
uniform int		clustered_grid_size ;
uniform float	pow_of_r ;

struct candidate_cell
{
	vec4	pos ;

	float	potential ;
	float	padding[ 3 ] ;
} ;

layout ( rgba32f, binding = 0 ) uniform image2D img_b ;
layout ( rgba32f, binding = 1 ) uniform image2D img_o ;
layout ( rgba32f, binding = 2 ) uniform image2D img_p ;
layout ( rgba32f, binding = 3 ) uniform image2D img_n ;
layout ( rgba32f, binding = 4 ) uniform image2D img_clustered_n ;

layout ( std430, binding = 5 ) buffer Input
{
	candidate_cell cell[ ] ;
} candi_buffer ;

layout ( local_size_variable ) in ;


int	get_clustered_x( int x )				{ return ( x / ( grid_size / clustered_grid_size ) ) ;	}
int	get_clustered_y( int y )				{ return ( y / ( grid_size / clustered_grid_size ) ) ;	}
int	get_clustered_index( int x, int y )		{ int cx = get_clustered_x( x ) ; int cy = get_clustered_y( y ) ; return ( cy * clustered_grid_size + cx ) ;	}

void main()
{
	uint gid = gl_GlobalInvocationID.x ;
	ivec2 cell = ivec2( candi_buffer.cell[ gid ].pos.xy ) ;

	// get B
	float B = imageLoad( img_b, cell ).r ;

	// get Obs
	float Obs = imageLoad( img_o, cell ).r ;

	// get P
	float P = imageLoad( img_p, cell ).r ;
		
	// compute N
	float r ;
	float N = 0 ;

	int iClusteredX = get_clustered_x( cell.x ) ;
	int iClusteredY = get_clustered_y( cell.y ) ;
	int iClusteredIndex = iClusteredY * clustered_grid_size + iClusteredX ;
	
	int iIndex ;
	float count ;
	vec4 value ;

	for ( int i = 0; i < clustered_grid_size; ++i )
	{
		for ( int j = 0; j < clustered_grid_size; ++j )
		{
			iIndex = i * clustered_grid_size + j ;

			if ( iIndex != iClusteredIndex )		// not in the same cluster
			{
				value = imageLoad( img_clustered_n, ivec2( j, i ) ) ;

				if ( 0 != value.a )	// count is not zero
				{
					r = distance( candi_buffer.cell[ gid ].pos.xy, vec2( value.r, value.g ) ) ;
					if ( pow_of_r > 1 )
					{
						r = pow( r, pow_of_r ) ;
					}

					N += value.a / r ;
				}
			}
			else									// in the same cluster
			{
				count = imageLoad( img_n, ivec2( 0, iClusteredIndex ) ).r ;

				for ( int k = 0; k < count; ++k )
				{
					value = imageLoad( img_n, ivec2( k + 1, iClusteredIndex ) ) ;

					r = distance( candi_buffer.cell[ gid ].pos.xy, vec2( value.r, value.g ) ) ;
					if ( pow_of_r > 1 )
					{
						r = pow( r, pow_of_r ) ;
					}

					N += 1.0f / r ;
				}
			}
		}
	}
	

	// compute potential
	float fPotential = 0 ;

	fPotential = P / ( B * N ) ;
	if ( 0 != Obs )
	{
		fPotential *= 1.0f / Obs ;
	}
	
	candi_buffer.cell[ gid ].potential = fPotential ;
}




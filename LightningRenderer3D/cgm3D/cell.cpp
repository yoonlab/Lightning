

#include "cell.h"

using namespace Cgm3D ;
using namespace std ;


const int	Cell::DIR_X_DIFF[ MAX_NEIGHBOR_DIRECTION ]	= {  0, 1, 0, -1,  0, 0 } ;
const int	Cell::DIR_Y_DIFF[ MAX_NEIGHBOR_DIRECTION ]	= { -1, 0, 1,  0,  0, 0 } ;
const int	Cell::DIR_Z_DIFF[ MAX_NEIGHBOR_DIRECTION ]	= {  0, 0, 0,  0, -1, 1 } ;

// front						    // middle					    // back
const int	SimpleCell::DIR_WAY_X_DIFF[ 26 ]		= { -1,  0,  1, -1,  0,  1, -1,  0,  1, -1,  0,  1, -1,  1, -1,  0,  1, -1,  0,  1, -1,  0,  1, -1,  0,  1 } ;
const int	SimpleCell::DIR_WAY_Y_DIFF[ 26 ]		= { -1, -1, -1,  0,  0,  0,  1,  1,  1, -1, -1, -1,  0,  0,  1,  1,  1, -1, -1, -1,  0,  0,  0,  1,  1,  1 } ;
const int	SimpleCell::DIR_WAY_Z_DIFF[ 26 ]		= { -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1 } ;


Cell::Cell( int x, int y, int z )
	: m_eType( E_CT_EMPTY ), m_bBoundary( false ), m_iX( x ), m_iY( y ), m_iZ( z )
	, m_fPotential( 0.0f ), m_fStencil( 0.0f ), m_fB( 0.0f ), m_fR( 0.0f )
	, m_fXDiff( 0.0f ), m_fYDiff( 0.0f ), m_fZDiff( 0.0f ), m_f2Diff( 0.0f )
{
	for ( int i = 0; i < MAX_NEIGHBOR_DIRECTION; ++i )
	{
		m_fNeighborStencil[ i ] = 0.0f ;
	}
}

Cell::Cell( int x, int y, int z, CellType eType, float fPotential )
	: m_eType( eType ), m_iX( x ), m_iY( y ), m_iZ( z )
	, m_fPotential( fPotential ), m_fStencil( 0.0f ), m_fB( 0.0f ), m_fR( 0.0f )
	, m_fXDiff( 0.0f ), m_fYDiff( 0.0f ), m_fZDiff( 0.0f ), m_f2Diff( 0.0f )
{
	if ( E_CT_EMPTY == eType )	m_bBoundary = false ;
	else						m_bBoundary = true ;
		
	for ( int i = 0; i < MAX_NEIGHBOR_DIRECTION; ++i )
	{
		m_fNeighborStencil[ i ] = 0.0f ;
	}
}

Cell::~Cell()
{
}


void	Cell::SetCellType( CellType eType )
{
	m_eType = eType ;

	switch ( eType )
	{
		case E_CT_EMPTY :
		{
			m_bBoundary = false ;
			m_fPotential = 0.0f ;
		}
		break ;

		case E_CT_START :
		{
			m_bBoundary = true ;
			m_fPotential = 0.0f ;
		}
		break ;

		case E_CT_END :
		{
			m_bBoundary = true ;
			m_fPotential = 1.0f ;
		}
		break ;

		case E_CT_OBSTACLE :
		{
			m_bBoundary = true ;
			m_fPotential = 0.0f ;
		}
		break ;
	}
}

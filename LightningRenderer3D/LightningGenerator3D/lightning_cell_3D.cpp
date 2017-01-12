

#include "lightning_cell_3D.h"

using namespace Lightning3D ;
using namespace std ;

									            // front						    // middle					    // back
const int	Cell::DIR_WAY_X_DIFF[ 26 ]		= { -1,  0,  1, -1,  0,  1, -1,  0,  1, -1,  0,  1, -1,  1, -1,  0,  1, -1,  0,  1, -1,  0,  1, -1,  0,  1 } ;
const int	Cell::DIR_WAY_Y_DIFF[ 26 ]		= { -1, -1, -1,  0,  0,  0,  1,  1,  1, -1, -1, -1,  0,  0,  1,  1,  1, -1, -1, -1,  0,  0,  0,  1,  1,  1 } ;
const int	Cell::DIR_WAY_Z_DIFF[ 26 ]		= { -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1 } ;


Cell::Cell()
	: m_eType( E_CT_EMPTY ), m_bBoundary( false )
	, m_iX( -1 ), m_iY( -1 ), m_iZ( -1 )
	, m_iParentX( -1 ), m_iParentY( -1 ), m_iParentZ( -1 )
	, m_fPotential( 0.0f ), m_fProb( 0.0f )
	, m_fLastN( 0.0f ), m_fP( 0.0f ), m_fB( 0.0f ), m_fObs( 0.0f )
{
}

Cell::Cell( int x, int y, int z )
	: m_eType( E_CT_EMPTY ), m_bBoundary( false )
	, m_iX( x ), m_iY( y ), m_iZ( z )
	, m_iParentX( -1 ), m_iParentY( -1 ), m_iParentZ( -1 )
	, m_fPotential( 0.0f ), m_fProb( 0.0f )
	, m_fLastN( 0.0f ), m_fP( 0.0f ), m_fB( 0.0f ), m_fObs( 0.0f )
{
}

Cell::Cell( int x, int y, int z, CellType eType, float fPotential )
	: m_eType( E_CT_EMPTY ), m_bBoundary( false )
	, m_iX( x ), m_iY( y ), m_iZ( z )
	, m_iParentX( -1 ), m_iParentY( -1 ), m_iParentZ( -1 )
	, m_fPotential( fPotential ), m_fProb( 0.0f )
	, m_fLastN( 0.0f ), m_fP( 0.0f ), m_fB( 0.0f ), m_fObs( 0.0f )
{
	if ( E_CT_EMPTY == eType )	m_bBoundary = false ;
	else						m_bBoundary = true ;
}

Cell::~Cell()
{
}


void	Cell::SetCellType( CellType eType )
{
	m_eType = eType ;

	switch ( eType )
	{
		case E_CT_EMPTY :		m_bBoundary = false ;	m_fPotential = 0.0f ;	break ;
		case E_CT_START :		m_bBoundary = true ;	m_fPotential = 0.0f ;	break ;
		case E_CT_END :			m_bBoundary = true ;	m_fPotential = 1.0f ;	break ;
		case E_CT_OBSTACLE :	m_bBoundary = true ;	m_fPotential = 0.0f ;	break ;
	}
}

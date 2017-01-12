

#include "lightning_cell.h"

using namespace Lightning ;
using namespace std ;


const int	Cell::DIR_8_WAY_X_DIFF[ 8 ]		= {  0,  1, 1, 1, 0, -1, -1, -1 } ;
const int	Cell::DIR_8_WAY_Y_DIFF[ 8 ]		= { -1, -1, 0, 1, 1,  1,  0, -1 } ;


Cell::Cell()
	: m_eType( E_CT_EMPTY ), m_bBoundary( false ), m_iX( -1 ), m_iY( -1 ), m_iParentX( -1 ), m_iParentY( -1 )
	, m_fLastPotential( 0.0f ), m_fPotential( 0.0f ), m_fProb( 0.0f )
{
}

Cell::Cell( int x, int y )
	: m_eType( E_CT_EMPTY ), m_bBoundary( false ), m_iX( x ), m_iY( y ), m_iParentX( -1 ), m_iParentY( -1 )
	,  m_fLastPotential( 0.0f ), m_fPotential( 0.0f ), m_fProb( 0.0f )
{
}

Cell::Cell( int x, int y, CellType eType, float fPotential )
	: m_eType( E_CT_EMPTY ), m_bBoundary( false ), m_iX( x ), m_iY( y ), m_iParentX( -1 ), m_iParentY( -1 )
	,  m_fLastPotential( 0.0f ), m_fPotential( fPotential ), m_fProb( 0.0f )
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

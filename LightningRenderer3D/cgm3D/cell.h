

#ifndef	__CGM_CELL_3D_H__
#define	__CGM_CELL_3D_H__

namespace Cgm3D
{

enum CellType
{
	E_CT_EMPTY = 0,
	E_CT_START,
	E_CT_END,
	E_CT_OBSTACLE,

	MAX_CELL_TYPE
} ;


enum NeighborDir
{
	E_ND_UP = 0,
	E_ND_RIGHT,
	E_ND_DOWN,
	E_ND_LEFT,
	E_ND_FRONT,
	E_ND_BACK,

	MAX_NEIGHBOR_DIRECTION
} ;


class Cell
{
public :

	// for computing x & y position of neighbor cell
	static const int	DIR_X_DIFF[ MAX_NEIGHBOR_DIRECTION ] ;
	static const int	DIR_Y_DIFF[ MAX_NEIGHBOR_DIRECTION ] ;
	static const int	DIR_Z_DIFF[ MAX_NEIGHBOR_DIRECTION ] ;

public :

	// constructor & destructor
	Cell( int x, int y, int z ) ;
	Cell( int x, int y, int z, CellType eType, float fPotential ) ;
	virtual ~Cell() ;

	// set type
	void	SetCellType( CellType eType ) ;

public :

	CellType	m_eType ;			// cell type
	bool		m_bBoundary ;		// cell is boundary

	int			m_iX ;				// x position on grid
	int			m_iY ;				// y position on grid
	int			m_iZ ;				// z position on grid

	float		m_fPotential ;		// x
	float		m_fStencil ;		// A
	float		m_fNeighborStencil[ MAX_NEIGHBOR_DIRECTION ] ;	
	float		m_fB ;				// b
	float		m_fR ;				// r (residual)

	float		m_fXDiff ;
	float		m_fYDiff ;
	float		m_fZDiff ;
	float		m_f2Diff ;

} ;


class SimpleCell
{
public :

	// start from north(12 o'clock) by clock wise direction for 2D map
	// front, middle, back z-order
	//  - front : top left to bottom right (9)
	//  - middle : top left to bottom right except center (8)
	//  - back : top left to bottom right (9)
	
	static const int	DIR_WAY_X_DIFF[ 26 ] ;	
	static const int	DIR_WAY_Y_DIFF[ 26 ] ;
	static const int	DIR_WAY_Z_DIFF[ 26 ] ;

public :

	// constructor & destructor
	SimpleCell() : m_iX( 0 ), m_iY( 0 ), m_iZ( 0 ), m_iParentX( 0 ), m_iParentY( 0 ), m_fProb( 0.0f )	{	}
	virtual ~SimpleCell()																				{	}

public :

	int		m_iX ;
	int		m_iY ;
	int		m_iZ ;

	int		m_iParentX ;
	int		m_iParentY ;
	int		m_iParentZ ;

	float	m_fProb ;

} ;	// SimpleCell


}	// namespace Cgm3D

#endif	// __CGM_CELL_3D_H__

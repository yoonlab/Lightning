

#ifndef	__CGM_CELL_H__
#define	__CGM_CELL_H__


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

	MAX_NEIGHBOR_DIRECTION
} ;


class Cell
{
public :

	// for computing x & y position of neighbor cell
	static const int	DIR_X_DIFF[ MAX_NEIGHBOR_DIRECTION ] ;
	static const int	DIR_Y_DIFF[ MAX_NEIGHBOR_DIRECTION ] ;

public :

	// constructor & destructor
	Cell( int x, int y ) ;
	Cell( int x, int y, CellType eType, float fPotential ) ;
	virtual ~Cell() ;

	// set type
	void	SetCellType( CellType eType ) ;

public :

	CellType	m_eType ;			// cell type
	bool		m_bBoundary ;		// cell is boundary

	int			m_iX ;				// x position on grid
	int			m_iY ;				// y position on grid

	float		m_fPotential ;		// x
	float		m_fStencil ;		// A
	float		m_fNeighborStencil[ MAX_NEIGHBOR_DIRECTION ] ;	
	float		m_fB ;				// b
	float		m_fR ;				// r (residual)

	float		m_fXDiff ;
	float		m_fYDiff ;
	float		m_f2Diff ;

} ;


class SimpleCell
{
public :

	static const int	DIR_8_WAY_X_DIFF[ 8 ] ;
	static const int	DIR_8_WAY_Y_DIFF[ 8 ] ;

public :

	// constructor & destructor
	SimpleCell() : m_iX( 0 ), m_iY( 0 ), m_iParentX( 0 ), m_iParentY( 0 ), m_fProb( 0.0f ), m_iChildCount( 0 ), m_iAge( 0 )	{	}
	virtual ~SimpleCell()																									{	}

public :

	int		m_iX ;
	int		m_iY ;

	int		m_iParentX ;
	int		m_iParentY ;

	float	m_fProb ;

	int		m_iChildCount ;		// number of child nodes
	int		m_iAge ;			// count that was candidate cell

} ;	// SimpleCell


class Obstacle
{
public :

	// constructor & destructor
	Obstacle() ;
	Obstacle( int x, int y, int w, int h ) ;
	virtual ~Obstacle() ;

protected :

	void	CalcCenter() ;

public :

	int		m_iX ;
	int		m_iY ;
	int		m_iWidth ;
	int		m_iHeight ;

	float	m_fCenterX ;
	float	m_fCenterY ;

} ;

#endif	// __CGM_CELL_H__

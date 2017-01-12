

#ifndef	__LIGHTNING_CELL_3D_H__
#define	__LIGHTNING_CELL_3D_H__

#include <vector>


namespace Lightning3D
{


// lightning cell type
enum CellType
{
	E_CT_EMPTY = 0,
	E_CT_START,
	E_CT_END,
	E_CT_OBSTACLE,
	E_CT_WAY_POINT,
	E_CT_TEMP,			// for multiple target & chain lightning

	MAX_CELL_TYPE
} ;


// lightning cell
class Cell
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
	Cell() ;
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

	int			m_iParentX ;		// x position of parent node on grid
	int			m_iParentY ;		// y position of parent node on grid
	int			m_iParentZ ;		// z position of parent node on grid

	float		m_fPotential ;		// electric potential
	float		m_fProb ;			// probability

	float		m_fLastN ;			// previous electric potential for negative charges
	float		m_fP ;				// pre-computed values
	float		m_fB ;
	float		m_fObs ;
} ;


// multi scaled cluster cell
class ClusteredCell
{
public :

	// constructor & destructor
	ClusteredCell()
		: m_iX( -1 ), m_iY( -1 ), m_iZ( -1 )
		, m_iXSum( 0 ), m_iYSum( 0 ), m_iZSum( 0 )
		, m_fAvgX( 0 ), m_fAvgY( 0 ), m_fAvgZ( 0 )
	{
		m_vCells.clear() ;
	}

	virtual ~ClusteredCell()	{	}

public :

	int		m_iX ;							// x position of multi scaled grid map
	int		m_iY ;							// y position of multi scaled grid map
	int		m_iZ ;							// z position of multi scaled grid map

	int		m_iXSum ;						// sum of x position in the same cluster
	int		m_iYSum ;						// sum of y position in the same cluster
	int		m_iZSum ;						// sum of z position in the same cluster

	float	m_fAvgX ;						// average of x position in the same cluster
	float	m_fAvgY ;						// average of y position in the same cluster
	float	m_fAvgZ ;						// average of z position in the same cluster

	std::vector< Cell >	m_vCells ;			// list of cells in the same cluster

} ;


}	// namespace of Lightning

#endif	// __LIGHTNING_CELL_3D_H__


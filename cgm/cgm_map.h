

#ifndef	__CGM_MAP_H__
#define __CGM_MAP_H__

#include "cgm_solver.h"
#include "lightning_tree.h"
#include "base.h"

#include <vector>
#include <string>
#include <random>



// -------------------------------------------------------------------------------------------------------
// abstract class for cell map

enum ValueType
{
	E_VT_X = 0,
	E_VT_A,
	E_VT_B,
	E_VT_R,
	E_VT_P,

	E_VT_X_DIFF,
	E_VT_Y_DIFF,
	E_VT_2_DIFF,

	MAX_VALUE_TYPE
} ;

enum ProcessType
{
	E_PT_ALL_STEP = 0,
	E_PT_ONE_STEP,
	E_PT_REGION_STEP,

	MAX_PROCESS_TYPE
} ;


class CellMap
{
public :

	static const int	DEFAULT_ETA		= 2 ;

public :

	// constructor & destructor
	CellMap() ;
	CellMap( int iEta ) ;
	virtual ~CellMap() ;

	// load map
	virtual bool	Load( const std::string& strPath ) ;
		
	// process lightning tree generation
	virtual bool	Process( ProcessType eType, bool bShowTime = false, bool bShowValue = false ) PURE ;

		
	// get lightning tree
	//const LightningTree&	GetLightningTree() const					{ return m_tree ;			}
	LightningTree&			GetLightningTree()							{ return m_tree ;			}
	int						GetProcessIndex() const						{ return m_iProcessIndex ;	}

	// getter & setter
	int						GetGridSize() const							{ return m_iGridSize ;		}

	void					SetEta( int iEta )							{ m_iEta = iEta ;			}
	int						GetEta() const								{ return m_iEta ;			}

	void					SetBaseThickness( float fThickness )			{ m_fBaseThickness = fThickness ;	}
	float					GetBaseThickness() const						{ return m_fBaseThickness ;			}

	void					SetIntensityAttenuation( float fAttenuation )	{ m_fIntensityAttenuation = fAttenuation ;	}
	float					GetIntensityAttenuation() const					{ return m_fIntensityAttenuation ;			}

	
	// log
	void					SetLogFile( const std::string& strPath )	{ m_strLogFile = strPath ;	}
	void					SetLog( bool bEnable )						{ m_bLogEnabled = bEnable ;	}
	void					ClearLog() ;

	// cells
	const SimpleCell*		GetStartCell( int iIndex ) const ;
	const SimpleCell*		GetEndCell( int iIndex ) const ;
	const SimpleCell*		GetObstacleCell( int iIndex ) const ;

	int						GetStartCellCount() const					{ return (int)m_vStartCells.size() ;	}
	int						GetEndCellCount() const						{ return (int)m_vEndCells.size() ;		}
	int						GetObstacleCellCount() const				{ return (int)m_vObstacleCells.size() ;	}

	// write data
	bool					WriteValue( const std::string& strPath ) ;
	bool					WriteMap( const std::string& strPath ) ;

	// fractal dimension
	float					CalculateFractalDimension() ;

protected :
	
	std::vector< Cell* >&		GetCells()						{ return m_vCells ;						}
	
	std::vector< SimpleCell >&	GetBoundaryCells()				{ return m_vBoundaryCells ;				}
	std::vector< SimpleCell >&	GetNegativeCells()				{ return m_vNegativeCells ;				}
	std::vector< SimpleCell >&	GetPositiveCells()				{ return m_vPositiveCells ;				}

	CellType					GetCellType( int iIndex ) const ;
	void						SetCellType( int iIndex, CellType eType ) ;
		
	bool						IsNearEndCell( int iIndex ) const ;
	bool						IsNearEndCell( int x, int y, int tx, int ty ) const ;
	bool						IsNearEndCell( int x, int y, int tx, int ty, int iRadius ) const ;
	
	// for debugging
	void						PrintValue( ValueType eType ) ;

protected :

	void	Clear() ;

	void	InitTree() ;

	virtual void	UpdateCandidate() ;

protected :

	int								m_iGridSize ;
	int								m_iEta ;

	std::vector< Cell* >			m_vCells ;
	
	std::vector< SimpleCell >		m_vBoundaryCells ;
	std::vector< SimpleCell >		m_vNegativeCells ;
	std::vector< SimpleCell >		m_vPositiveCells ;

	// initial state
	std::vector< SimpleCell >		m_vStartCells ;
	std::vector< SimpleCell >		m_vEndCells ;
	std::vector< SimpleCell >		m_vObstacleCells ;

	// for lightning tree
	std::vector< SimpleCell >		m_vCandidates ;

	SimpleCell						m_nextCell ;
	LightningTree					m_tree ;

	int								m_iProcessIndex ;
	
	std::mt19937					m_randGen ;

	bool							m_bProcessFinished ;

	std::string						m_strLogFile ;
	bool							m_bLogEnabled ;

	float							m_fBaseThickness ;
	float							m_fIntensityAttenuation ;
	
} ;	// Cell Map


// -------------------------------------------------------------------------------------------------------
// Cell map for CGM

class CGMMap : public CellMap
{
public :

	static const int	MAX_ITERATION	= 2000 ;
	
public :

	// constructor & destructor
	CGMMap() ;
	CGMMap( int iMaxIteration, int iEta ) ;
	virtual ~CGMMap() ;

	// load map
	virtual bool	Load( const std::string& strPath ) ;
	virtual bool	Load( int iGridSize, int sx, int sy, int ex = -1, int ey = -1 ) ;	// default : bottom row is positive charges (ground)

	// process lightning tree generation
	virtual bool	Process( ProcessType eType, bool bShowTime = false, bool bShowValue = false ) ;

	// getter & setter
	void			SetMaxIteration( int iMaxIter )		{ m_iMaxIteration = iMaxIter ;	}
	int				GetMaxIteration() const				{ return m_iMaxIteration ;		}


	void					SetTimeCheckSteps( const std::vector< int >& vSteps ) ;
	const std::vector< int > GetTimesPerSteps() const ;

protected :

private :

	int				m_iMaxIteration ;
	
	CGMSolver		m_solver ;


	int								m_iTimeProcesStart ;
	int								m_iTimeIndex ;
	std::vector< int >				m_vTimeCheckSteps ;
	std::vector< int >				m_vTimePerSteps ;

} ;	// CGMMap

#endif	// __CGM_MAP_H__

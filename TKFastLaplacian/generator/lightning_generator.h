

#ifndef	__LIGHTNING_SHAPE_GENERATOR_H__
#define __LIGHTNING_SHAPE_GENERATOR_H__

#include "lightning_cell.h"
#include "lightning_tree.h"

#include "GL/glew.h"

#include <vector>
#include <map>
#include <string>
#include <random>


namespace Lightning
{

enum ProcessType
{
	E_PT_ALL_STEP = 0,
	E_PT_ONE_STEP,

	MAX_PROCESS_TYPE
} ;

enum ComputationType
{
	E_CT_CPU	= 0,

	MAX_COMPUTATION_TYPE
} ;


// lightning shape generator
class LightningGenerator
{
public :

	// constructor & destructor
	LightningGenerator() ;
	LightningGenerator( int iEta ) ;
	virtual ~LightningGenerator() ;


	// load map
	bool	Load( const std::string& strPath ) ;
	bool	Load( int iGridSize, int sx, int sy, int tx = -1, int ty = -1 ) ;

	// process lightning tree generation
	bool	Process( ProcessType eType ) ;
		

	// getter & setter
	LightningTree&			GetLightningTree()								{ return m_tree ;							}

	int						GetGridSize() const								{ return m_iGridSize ;						}
	int						GetClusterGridSize() const						{ return m_iClusterGridSize	;				}

	void					SetEta( int iEta )								{ m_iEta = iEta ;							}
	int						GetEta() const									{ return m_iEta ;							}

	void					SetPhysicalHeight( float h )					{ m_fH = h ;								}
	float					GetPhysicalHeight() const						{ return m_fH ;								}

	void					SetWeight( float fWeight )						{ m_fWeight = fWeight ;						}
	float					GetWeight() const								{ return m_fWeight ;						}

	void					SetBaseThickness( float fThickness )			{ m_fBaseThickness = fThickness ;			}
	float					GetBaseThickness() const						{ return m_fBaseThickness ;					}

	void					SetIntensityAttenuation( float fAttenuation )	{ m_fIntensityAttenuation = fAttenuation ;	}
	float					GetIntensityAttenuation() const					{ return m_fIntensityAttenuation ;			}

	int						GetInitializationTime() const					{ return m_iTimeInitEnd - m_iTimeInitStart ;	}

	void					SetTimeCheckSteps( const std::vector< int >& vSteps ) ;
	const std::vector< int > GetTimesPerSteps() const ;

	CellType				GetCellType( int iIndex ) const ;
	void					SetCellType( int iIndex, CellType eType ) ;


	const std::vector< Cell* >&	GetCells() const							{ return m_vCells ;							}

	int						GetStartCellCount() const						{ return (int)m_vStartCells.size() ;		}
	int						GetEndCellCount() const							{ return (int)m_vEndCells.size() ;			}
	int						GetBoundaryCellCount() const					{ return (int)m_vBoundaryCells.size() ;		}
	int						GetObstacleCellCount() const					{ return (int)m_vObstacleCells.size() ;		}
	int						GetNegativeCellCount() const					{ return (int)m_vNegativeCells.size() ;		}
	int						GetPositiveCellCount() const					{ return (int)m_vPositiveCells.size() ;		}

	const std::vector< Cell >&	GetStartCells() const						{ return m_vStartCells ;					}
	const std::vector< Cell >&	GetEndCells() const							{ return m_vEndCells ;						}
	const std::vector< Cell >&	GetBoundaryCells() const					{ return m_vBoundaryCells ;					}
	const std::vector< Cell >&	GetObstacleCells() const					{ return m_vObstacleCells ;					}
	const std::vector< Cell >&	GetNegativeCells() const					{ return m_vNegativeCells ;					}
	const std::vector< Cell >&	GetPositiveCells() const					{ return m_vPositiveCells ;					}

	const Cell*				GetStartCell( int iIndex ) const				{ if ( 0 <= iIndex && iIndex < m_vStartCells.size() )		return &m_vStartCells[ iIndex ] ;		else return NULL ;	}
	const Cell*				GetEndCell( int iIndex ) const					{ if ( 0 <= iIndex && iIndex < m_vEndCells.size() )			return &m_vEndCells[ iIndex ] ;			else return NULL ;	}
	const Cell*				GetBoundaryCell( int iIndex ) const				{ if ( 0 <= iIndex && iIndex < m_vBoundaryCells.size() )	return &m_vBoundaryCells[ iIndex ] ;	else return NULL ;	}
	const Cell*				GetObstacleCell( int iIndex ) const				{ if ( 0 <= iIndex && iIndex < m_vObstacleCells.size() )	return &m_vObstacleCells[ iIndex ] ;	else return NULL ;	}
	const Cell*				GetNegativeCell( int iIndex ) const				{ if ( 0 <= iIndex && iIndex < m_vNegativeCells.size() )	return &m_vNegativeCells[ iIndex ] ;	else return NULL ;	}
	const Cell*				GetPositiveCell( int iIndex ) const				{ if ( 0 <= iIndex && iIndex < m_vPositiveCells.size() )	return &m_vPositiveCells[ iIndex ] ;	else return NULL ;	}

	
	// logging
	void					SetLogFile( const std::string& strPath )		{ m_strLogFile = strPath ;					}
	void					SetLog( bool bEnable )							{ m_bLogEnabled = bEnable ;					}
	void					ClearLog() ;

	// write data
	bool					WriteValue( const std::string& strPath ) ;
	bool					WriteMap( const std::string& strPath ) ;

	// for heatmap visualization
	void					MakeAllCellsToCandidates() ;
	void					Normalize( float fMax ) ;


	// fractal dimension
	float					CalculateFractalDimension() ;

protected :

	// clear all data
	void	Clear() ;

	// load map data
	bool	LoadMap( const std::string& strPath ) ;

	// initialize lightning tree
	void	InitTree() ;


	//------------------------------------------------------------
	// CPU functions
	bool	ProcessByCPU( ProcessType eType ) ;

	void	CreateCandidateMap() ;
	void	UpdateCandidateMap( const Cell& nextCell ) ;

	void	CreateBoundaryCells() ;

	// Calculation function
	int		CalcElectricPotential() ;
	float	CalcElectricPotential( int gx, int gy ) ;
	
	// update candidate
	void	UpdateCandidate() ;
	
	// select candidate
	bool	SelectCandidate( Cell& outNextCell ) ;


	// update new path information
	void	AddNewLightningPath( const Cell& newPath ) ;
	//------------------------------------------------------------


	// apply intensity & thickness to final lightning tree
	void	ApplyIntensityAndThickness( float fBaseThickness, float fIntensityAttenuation ) ;
	

	// utility functions
	float	Distance( float x1, float y1, float x2, float y2 ) const ;
	float	Distance2( float x1, float y1, float x2, float y2 ) const ;	// squared distance

	bool	IsNearToEndCell( int x, int y, int& outEndX, int& outEndY ) const ;
	bool	IsNearToWaypoint( int x, int y, int& outEndX, int& outEndY ) const ;

	bool	ExistFile( const std::string& strPath ) const ;

	
	// for debugging
	void	PrintValue() ;

private :

	// ---------------------------------------------------------------------
	// common variables

	int								m_iGridSize ;			// size of grid map	( 64 x 64 )
	int								m_iClusterGridSize ;	// size of multi scaled cluster grid map ( 8 x 8 )
		
	int								m_iEta ;
	float							m_fH ;					// physical height of grid cell
	float							m_fWeight ;				// weight for positive charges

	ComputationType					m_eCompType ;			// CPU or GPU

	// cell map
	std::vector< Cell* >			m_vCells ;

	// initial state
	std::vector< Cell >				m_vStartCells ;			// initial state
	std::vector< Cell >				m_vEndCells ;		

	// for lightning tree
	std::map< int, Cell* >			m_mapCandidates ;		// store candidate cells that does not include the same position cells. This is for calculation potential.
	std::vector< Cell >				m_vCandidates ;			// store candidate cells that includes the same position cells which has different parent. This is for selecting next lightning growth.

	LightningTree					m_tree ;

	bool							m_bProcessFinished ;
	int								m_iProcessIndex ;

	float							m_fBaseThickness ;
	float							m_fIntensityAttenuation ;
	
	std::mt19937					m_randGen ;


	// ---------------------------------------------------------------------
	// variables for CPU

	// cells of each type
	std::vector< Cell >				m_vBoundaryCells ;		// for CPU
	std::vector< Cell >				m_vObstacleCells ;
	std::vector< Cell >				m_vNegativeCells ;
	std::vector< Cell >				m_vPositiveCells ;


	// ---------------------------------------------------------------------
	// others
	
	// for logging
	std::string						m_strLogFile ;
	bool							m_bLogEnabled ;

	// time variables
	double							m_timeGenCandidate ;
	double							m_timeCalcB ;
	double							m_timeCalcN ;
	double							m_timeCalcP ;

	double							m_timeCalcTotalSum ;
	double							m_timeCalcBSum ;
	double							m_timeCalcNSum ;
	double							m_timeCalcPSum ;
	double							m_timeUpdateCandidateSum ;
	double							m_timeSelectCandidateSum ;


	int								m_iTimeInitStart ;
	int								m_iTimeInitEnd ;

	int								m_iTimeProcesStart ;
	int								m_iTimeIndex ;
	std::vector< int >				m_vTimeCheckSteps ;
	std::vector< int >				m_vTimePerSteps ;

} ;


}	// namespace of Lightning

#endif	// __LIGHTNING_SHAPE_GENERATOR_H__


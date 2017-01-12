

#ifndef	__LIGHTNING_SHAPE_GENERATOR_3D_H__
#define __LIGHTNING_SHAPE_GENERATOR_3D_H__

#include "lightning_cell_3D.h"
#include "lightning_tree_3D.h"

#include "GL/glew.h"

#include <vector>
#include <map>
#include <string>
#include <random>


namespace Lightning3D
{

enum LightningType
{
	E_LT_SINGLE_TARGET = 0,
	E_LT_GROUND_TARGET,
	E_LT_MULTIPLE_TARGET,
	E_LT_CHAIN_LIGHTNING,

	MAX_LIGHTNING_TYPE
} ;

enum ProcessType
{
	E_PT_ALL_STEP = 0,
	E_PT_ONE_STEP,
	E_PT_SAMPLE_STEP,

	MAX_PROCESS_TYPE
} ;

class Pos
{
public :

	Pos() : x( 0 ), y( 0 ), z( 0 )								{	}
	Pos( int _x, int _y, int _z ) : x( _x ), y( _y ), z( _z )	{	}

	int x ;
	int y ;
	int z ;
} ;


// lightning shape generator
class LightningGenerator3D
{
public :

	// constructor & destructor
	LightningGenerator3D() ;
	LightningGenerator3D( int iEta ) ;
	virtual ~LightningGenerator3D() ;


	// load map
	bool	Load( LightningType eType, int iGridSize, int iClusteredGridSize, int sx, int sy, int sz, const std::vector< Pos >& vTargets ) ;

	// process lightning tree generation
	bool	Process( ProcessType eType ) ;

	void	Reset() ;
	void	SetProcessSampleCount( int iCount )								{ m_iProcessSampleCount = iCount ;			}


	// getter & setter
	LightningTree&			GetLightningTree()								{ return m_tree ;							}

	int						GetGridSize() const								{ return m_iGridSize ;						}
	int						GetClusterGridSize() const						{ return m_iClusterGridSize	;				}

	void					SetEta( int iEta )								{ m_iEta = iEta ;							}
	int						GetEta() const									{ return m_iEta ;							}

	void					SetPowOfR( float fWeight )						{ m_fPowOfR = fWeight ;						}
	float					GetPowOfR() const								{ return m_fPowOfR ;						}

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

	// for heatmap visualization
	void					MakeAllCellsToCandidates() ;
	void					Normalize( float fMax ) ;

protected :

	// clear all data
	void	Clear() ;

	// initialize lightning tree
	void	InitTree() ;


	//-----------------------------------------------------------------------------------------------------
	// pre-computation step

	// create boundary cells for pre-computation
	void	CreateBoundaryCells() ;

	void	LoadBoundaryData( const std::string& strPath ) ;
	void	SaveBoundaryData( const std::string& strPath ) ;

	// pre-computation of electric potential for each type of cells
	void	CalcBoundaryElectricPotential() ;
	void	CalcObstacleElectricPotential() ;
	void	CalcPositiveElectricPotential() ;


	//-----------------------------------------------------------------------------------------------------

	// initialize clustered negative map
	void	InitNegativeElectricPotential() ;
	void	CreateMultiScaledClusterMap( int iMultiScaledClusterMapSize ) ;

	// create candidate map that can be next lightning cell
	void	CreateCandidateMap() ;

	// calculate electric potential 
	void	CalcElectricPotential() ;				// for all candidate cells
	void	CalcElectricPotential( Cell* pCell ) ;	// for particular cell


	// generate candidates that can be next lightning cell (It can include same cell position that has different parent position)
	void	UpdateCandidate() ;
	
	// choose next lightning cell among the candidates
	bool	SelectCandidate( Cell& outNextCell ) ;


	// add new path information
	void	AddNewLightningPath( const Cell& newPath, bool bIsEndCell = false, bool bTargetCell = false ) ;

	// update candidate map and clustered negative map after selecting the next lightning cell
	void	UpdateCandidateMap( const Cell& nextCell ) ;
	void	UpdateClusteredMap( const Cell& nextCell ) ;

	// apply intensity & thickness to final lightning tree after reaching the target
	void	ApplyIntensityAndThickness( float fBaseThickness, float fIntensityAttenuation ) ;
	void	ApplyIntensityAndThicknessForMultipleTarget( float fBaseThickness, float fIntensityAttenuation ) ;


	//-----------------------------------------------------------------------------------------------------
	// process by lightning type

	bool	ProcessSingleTarget( ProcessType eType ) ;
	bool	ProcessMultipleTarget( ProcessType eType ) ;
	bool	ProcessChainLightning( ProcessType eType ) ;


	
	//-----------------------------------------------------------------------------------------------------
	// utility functions
	float	Distance( float x1, float y1, float z1, float x2, float y2, float z2 ) const ;
	float	Distance2( float x1, float y1, float z1, float x2, float y2, float z2 ) const ;	// squared distance

	bool	IsNearToEndCell( int x, int y, int z, int& outEndX, int& outEndY, int& outEndZ ) const ;
	bool	IsNearToWaypoint( int x, int y, int z, int& outEndX, int& outEndY, int& outEndZ ) const ;

	bool	ExistFile( const std::string& strPath ) const ;


private :

	// ---------------------------------------------------------------------
	// common variables

	LightningType					m_eLightningType ;		// lightning type

	int								m_iGridSize ;			// size of grid map	( 64 x 64 )
	int								m_iClusterGridSize ;	// size of multi scaled cluster grid map ( 8 x 8 )
		
	int								m_iEta ;				// eta
	float							m_fPowOfR ;				// rho

	// target list for multiple target & chain lightning
	std::vector< Pos >				m_vTargets ;
	int								m_iTargetIndex ;

	// cell map
	std::vector< Cell* >			m_vCells ;
	std::vector< ClusteredCell >	m_vClusteredCells ;

	// initial state
	std::vector< Cell >				m_vStartCells ;			// initial negative charges
	std::vector< Cell >				m_vEndCells ;			// initial positive charges

	// cells of each type
	std::vector< Cell >				m_vBoundaryCells ;
	std::vector< Cell >				m_vObstacleCells ;
	std::vector< Cell >				m_vNegativeCells ;
	std::vector< Cell >				m_vPositiveCells ;
	std::vector< Cell >				m_vWaypointsCells ;

	// potential values for each type
	std::vector< float >			m_vBoundaryPotential ;
	std::vector< float >			m_vObstaclePotential ;
	std::vector< float >			m_vNegativePotential ;
	std::vector< float >			m_vPositivePotential ;
	
	// for lightning tree
	std::map< int, Cell* >			m_mapCandidates ;		// store candidate cells that does not include the same position cells. This is for calculation potential.
	std::vector< Cell >				m_vCandidates ;			// store candidate cells that includes the same position cells which has different parent. This is for selecting next lightning growth.

	LightningTree					m_tree ;

	bool							m_bProcessFinished ;
	int								m_iProcessIndex ;
	int								m_iProcessSampleIndex ;
	int								m_iProcessSampleCount ;

	float							m_fBaseThickness ;
	float							m_fIntensityAttenuation ;
	
	std::mt19937					m_randGen ;


	// ---------------------------------------------------------------------
	// others
	
	// for logging
	std::string						m_strLogFile ;
	bool							m_bLogEnabled ;

	// time variables
	int								m_iTimeInitStart ;
	int								m_iTimeInitEnd ;

	int								m_iTimeProcesStart ;
	int								m_iTimeIndex ;
	std::vector< int >				m_vTimeCheckSteps ;
	std::vector< int >				m_vTimePerSteps ;

} ;


}	// namespace of Lightning

#endif	// __LIGHTNING_SHAPE_GENERATOR_3D_H__




#include "lightning_generator_3D.h"
#include "base.h"
#include "GL/freeglut.h"

#include <iostream>
#include <ios>
#include <fstream>
#include <sstream>

#include <ctime>
#include <algorithm>
#include <functional>
#include <map>

#include <assert.h>

using namespace Lightning3D ;
using namespace std ;


#define DEFAULT_ETA						2
#define DEFAULT_POW_OF_R				3

#define DEFAULT_THICKNESS				1.0f
#define DEFAULT_INTENSITY_ATTENUATION	0.7f

//#define USE_TIME_CHECK


LightningGenerator3D::LightningGenerator3D()
	: m_eLightningType( E_LT_SINGLE_TARGET ), m_iGridSize( 0 ), m_iClusterGridSize( 0 ), m_iEta( DEFAULT_ETA ), m_fPowOfR( DEFAULT_POW_OF_R )
	, m_iTargetIndex( 0 )
	, m_bProcessFinished( false ), m_iProcessIndex( 0 ), m_iProcessSampleIndex( 0 ), m_iProcessSampleCount( 0 )
	, m_fBaseThickness( DEFAULT_THICKNESS ), m_fIntensityAttenuation( DEFAULT_INTENSITY_ATTENUATION )
	, m_strLogFile( "" ), m_bLogEnabled( false )
{
	m_vTargets.clear() ;

	m_vCells.clear() ;
	m_vClusteredCells.clear() ;

	m_vStartCells.clear() ;
	m_vEndCells.clear() ;

	m_vBoundaryCells.clear() ;
	m_vObstacleCells.clear() ;
	m_vNegativeCells.clear() ;
	m_vPositiveCells.clear() ;
	m_vWaypointsCells.clear() ;

	m_vBoundaryPotential.clear() ;
	m_vObstaclePotential.clear() ;
	m_vNegativePotential.clear() ;
	m_vPositivePotential.clear() ;
	
	m_mapCandidates.clear() ;
	m_vCandidates.clear() ;
	
	
	// random number generator
	std::random_device	rd ;
	m_randGen.seed( rd() ) ;

	srand( static_cast< unsigned >( time( 0 ) ) ) ;

	m_iTimeInitStart = 0 ;
	m_iTimeInitEnd = 0 ;

	m_iTimeProcesStart = 0 ;
	m_iTimeIndex = 0 ;
	m_vTimeCheckSteps.clear() ;
	m_vTimePerSteps.clear() ;
}

LightningGenerator3D::LightningGenerator3D( int iEta )
	: m_eLightningType( E_LT_SINGLE_TARGET ), m_iGridSize( 0 ), m_iClusterGridSize( 0 ), m_iEta( iEta ), m_fPowOfR( DEFAULT_POW_OF_R )
	, m_iTargetIndex( 0 )
	, m_bProcessFinished( false ), m_iProcessIndex( 0 ), m_iProcessSampleIndex( 0 ), m_iProcessSampleCount( 0 )
	, m_fBaseThickness( DEFAULT_THICKNESS ), m_fIntensityAttenuation( DEFAULT_INTENSITY_ATTENUATION )
	, m_strLogFile( "" ), m_bLogEnabled( false )
{
	m_vTargets.clear() ;

	m_vCells.clear() ;
	m_vClusteredCells.clear() ;

	m_vStartCells.clear() ;
	m_vEndCells.clear() ;

	m_vBoundaryCells.clear() ;
	m_vObstacleCells.clear() ;
	m_vNegativeCells.clear() ;
	m_vPositiveCells.clear() ;
	m_vWaypointsCells.clear() ;
	
	m_vBoundaryPotential.clear() ;
	m_vObstaclePotential.clear() ;
	m_vNegativePotential.clear() ;
	m_vPositivePotential.clear() ;
	
	m_mapCandidates.clear() ;
	m_vCandidates.clear() ;
	
	
	// random number generator
	std::random_device	rd ;
	m_randGen.seed( rd() ) ;

	srand( static_cast< unsigned >( time( 0 ) ) ) ;

	m_iTimeInitStart = 0 ;
	m_iTimeInitEnd = 0 ;


	m_iTimeProcesStart = 0 ;
	m_iTimeIndex = 0 ;
	m_vTimeCheckSteps.clear() ;
	m_vTimePerSteps.clear() ;
}

LightningGenerator3D::~LightningGenerator3D()
{
	Clear() ;
}

void	LightningGenerator3D::Clear()
{
	std::vector< Cell* >::iterator itr = m_vCells.begin() ;
	while ( itr != m_vCells.end() )
	{
		SAFE_DELETE( *itr ) ;
		++itr ;
	}

	m_vTargets.clear() ;

	m_vCells.clear() ;
	m_vClusteredCells.clear() ;

	m_vStartCells.clear() ;
	m_vEndCells.clear() ;
	
	m_vBoundaryCells.clear() ;
	m_vObstacleCells.clear() ;
	m_vNegativeCells.clear() ;
	m_vPositiveCells.clear() ;
	m_vWaypointsCells.clear() ;

	m_vBoundaryPotential.clear() ;
	m_vObstaclePotential.clear() ;
	m_vNegativePotential.clear() ;
	m_vPositivePotential.clear() ;
	
	m_mapCandidates.clear() ;
	m_vCandidates.clear() ;

	m_tree.Clear() ;

	m_bProcessFinished = false ;
	m_iProcessIndex = 0 ;
	m_iProcessSampleIndex = 0 ;
	m_iProcessSampleCount = 0 ;

	m_bLogEnabled = false ;
}

bool	LightningGenerator3D::Load( LightningType eType, int iGridSize, int iClusteredGridSize, int sx, int sy, int sz, const std::vector< Pos >& vTargets )
{
	Clear() ;

	m_eLightningType = eType ;
	m_iGridSize = iGridSize ;
	m_iClusterGridSize = iClusteredGridSize ;

	// create whole cells
	Cell* pCell ;

	for ( int z = 0; z < m_iGridSize; ++z )
	{
		for ( int y = 0; y < m_iGridSize; ++y )
		{
			for ( int x = 0; x < m_iGridSize; ++x )
			{
				pCell = new Cell( x, y, z, E_CT_EMPTY, 0 ) ;
				if ( pCell )
				{
					m_vCells.push_back( pCell ) ;

					if ( m_iGridSize - 1 == y && E_LT_GROUND_TARGET == eType && vTargets.empty() )
					{
						pCell->SetCellType( E_CT_END ) ;

						m_vEndCells.push_back( *pCell ) ;
						m_vPositiveCells.push_back( *pCell ) ;
					}
				}
			}
		}
	}

	// set start position
	int iIndex = sz * ( m_iGridSize * m_iGridSize ) + sy * m_iGridSize + sx ;
	if ( m_vCells[ iIndex ] )
	{
		m_vCells[ iIndex ]->SetCellType( E_CT_START ) ;

		m_vStartCells.push_back( *m_vCells[ iIndex ] ) ;
		m_vNegativeCells.push_back( *m_vCells[ iIndex ] ) ;

		// initialize lightning tree
		InitTree() ;
	}

	// set goal position if it does not use default setting
	switch ( eType )
	{
		case E_LT_SINGLE_TARGET :
		{
			if ( vTargets.size() > 0 )
			{
				int iIndex = vTargets[ 0 ].z * ( m_iGridSize * m_iGridSize ) + vTargets[ 0 ].y * m_iGridSize + vTargets[ 0 ].x ;
				if ( m_vCells[ iIndex ] )
				{
					m_vCells[ iIndex ]->SetCellType( E_CT_END ) ;

					m_vEndCells.push_back( *m_vCells[ iIndex ] ) ;
					m_vPositiveCells.push_back( *m_vCells[ iIndex ] ) ;
				}
			}
		}
		break ;
	
		case E_LT_GROUND_TARGET :
		{
			std::vector< Pos >::const_iterator itr = vTargets.begin() ;
			while ( itr != vTargets.end() )
			{
				int iIndex = itr->z * ( m_iGridSize * m_iGridSize ) + itr->y * m_iGridSize + itr->x ;
				if ( m_vCells[ iIndex ] )
				{
					m_vCells[ iIndex ]->SetCellType( E_CT_END ) ;

					m_vEndCells.push_back( *m_vCells[ iIndex ] ) ;
					m_vPositiveCells.push_back( *m_vCells[ iIndex ] ) ;
				}

				++itr ;
			}
		}
		break ;

		case E_LT_MULTIPLE_TARGET :
		case E_LT_CHAIN_LIGHTNING :
		{
			bool bFirst = true ;

			std::vector< Pos >::const_iterator itr = vTargets.begin() ;
			while ( itr != vTargets.end() )
			{
				int iIndex = itr->z * ( m_iGridSize * m_iGridSize ) + itr->y * m_iGridSize + itr->x ;
				if ( m_vCells[ iIndex ] )
				{
					if ( bFirst )
					{
						bFirst = false ;

						m_vCells[ iIndex ]->SetCellType( E_CT_END ) ;

						m_vEndCells.push_back( *m_vCells[ iIndex ] ) ;
						m_vPositiveCells.push_back( *m_vCells[ iIndex ] ) ;
					}
					else
					{
						m_vCells[ iIndex ]->SetCellType( E_CT_TEMP ) ;

						m_vEndCells.push_back( *m_vCells[ iIndex ] ) ;
					}
				}

				++itr ;
			}

			// copy target list
			m_vTargets = vTargets ;
			
			m_iTargetIndex = 0 ;
		}
		break ;
	}

	std::cout << "Finish generating map !!" << std::endl ;

	
	// pre-computation step		// TODO : it must changed to be loaded from file
	std::string strBoundaryData = "./res/boundary_" ;
	strBoundaryData += std::to_string( m_iGridSize ) + ".dat" ;

	if ( ExistFile( strBoundaryData ) )
	{
		LoadBoundaryData( strBoundaryData ) ;
	}
	else
	{
		CreateBoundaryCells() ;
		CalcBoundaryElectricPotential() ;

		SaveBoundaryData( strBoundaryData ) ;
	}
		
	CalcObstacleElectricPotential() ;

	if ( E_LT_SINGLE_TARGET == eType || E_LT_MULTIPLE_TARGET == eType || E_LT_CHAIN_LIGHTNING == eType )
	{
		m_iTimeInitStart = glutGet( GLUT_ELAPSED_TIME ) ;

		// one time computation for specific target
		CalcPositiveElectricPotential() ;
	}
	else if ( E_LT_GROUND_TARGET == eType )
	{
		// pre-computation for fixed target positions like ground
		CalcPositiveElectricPotential() ;

		m_iTimeInitStart = glutGet( GLUT_ELAPSED_TIME ) ;
	}
		

	// initialize for negative charges
	InitNegativeElectricPotential() ;
	CreateMultiScaledClusterMap( m_iClusterGridSize ) ;

	// generate initial candidate map
	CreateCandidateMap() ;

	// calculate electric potential for candidate cells
	CalcElectricPotential() ;

	m_iTimeInitEnd = glutGet( GLUT_ELAPSED_TIME ) ;

	std::cout << "Finish initialization !!" << std::endl ;

	return true ;
}

void	LightningGenerator3D::InitTree()
{
	Cell nextCell ;
	bool bRoot = true ;

	std::vector< Cell >::iterator itr = m_vStartCells.begin() ;
	while ( itr != m_vStartCells.end() )
	{
		nextCell.m_iParentX = nextCell.m_iX ;
		nextCell.m_iParentY = nextCell.m_iY ;
		nextCell.m_iParentZ = nextCell.m_iZ ;
		nextCell.m_iX = ( *itr ).m_iX ;
		nextCell.m_iY = ( *itr ).m_iY ;
		nextCell.m_iZ = ( *itr ).m_iZ ;

		if ( bRoot )
		{
			bRoot = false ;
			
			// set root
			LightningTreeNode* pRoot = new LightningTreeNode() ;
			if ( pRoot )
			{
				pRoot->m_iX = nextCell.m_iX ;
				pRoot->m_iY = nextCell.m_iY ;
				pRoot->m_iZ = nextCell.m_iZ ;
				pRoot->m_pParent = NULL ;

				m_tree.SetRoot( pRoot ) ;
			}
		}
		else
		{
			// add child
			m_tree.AddChild( nextCell.m_iParentX, nextCell.m_iParentY, nextCell.m_iParentZ, nextCell.m_iX, nextCell.m_iY, nextCell.m_iZ ) ;
		}

		++itr ;
	}
}

void	LightningGenerator3D::SetTimeCheckSteps( const std::vector< int >& vSteps )
{
	m_vTimeCheckSteps.clear() ;
	m_vTimePerSteps.clear() ;

	m_iTimeIndex = 0 ;
	m_iTimeProcesStart = 0 ;

	m_vTimeCheckSteps = vSteps ;
}

const std::vector< int > LightningGenerator3D::GetTimesPerSteps() const
{
	std::vector< int > vRet ;

	std::vector< int >::const_iterator itr = m_vTimePerSteps.begin() ;
	while ( itr != m_vTimePerSteps.end() )
	{
		vRet.push_back( *itr - m_iTimeProcesStart ) ;

		++itr ;
	}

	return vRet ;
}

CellType	LightningGenerator3D::GetCellType( int iIndex ) const
{
	if ( 0 <= iIndex && iIndex < (int)m_vCells.size() )
	{
		if ( m_vCells[ iIndex ] )
		{
			return m_vCells[ iIndex ]->m_eType ;
		}
	}

	return MAX_CELL_TYPE ;
}

void	LightningGenerator3D::SetCellType( int iIndex, CellType eType )
{
	if ( 0 <= iIndex && iIndex < (int)m_vCells.size() )
	{
		if ( m_vCells[ iIndex ] )
		{
			CellType eOldType = m_vCells[ iIndex ]->m_eType ;

			// only empty cell can be changed to other types
			//if ( E_CT_EMPTY == eOldType && E_CT_EMPTY != eType )
			if ( E_CT_EMPTY != eType )
			{
				m_vCells[ iIndex ]->SetCellType( eType ) ;
			
				switch ( eType )
				{
					case E_CT_START :		m_vNegativeCells.push_back( *m_vCells[ iIndex ] ) ;		break ;	// only use this type
					case E_CT_END :			m_vPositiveCells.push_back( *m_vCells[ iIndex ] ) ;		break ;
					case E_CT_OBSTACLE :	m_vObstacleCells.push_back( *m_vCells[ iIndex ] ) ;		break ;
					case E_CT_WAY_POINT :	m_vWaypointsCells.push_back( *m_vCells[ iIndex ] ) ;	break ;
				}
			}
		}
	}
}

bool	LightningGenerator3D::Process( ProcessType eType )
{
	switch ( m_eLightningType )
	{
		case E_LT_SINGLE_TARGET :
		case E_LT_GROUND_TARGET :
		{
			return ProcessSingleTarget( eType ) ;
		}
		break ;

		case E_LT_MULTIPLE_TARGET :
		{
			return ProcessMultipleTarget( eType ) ;
		}
		break ;

		case E_LT_CHAIN_LIGHTNING :
		{
			return ProcessChainLightning( eType ) ;
		}
		break ;
	}

	return false ;
}

void	LightningGenerator3D::Reset()
{
	// it maintains original start & target position

	for ( int i = 0; i < m_vCells.size(); ++i )
	{
		m_vCells[ i ]->m_eType = E_CT_EMPTY ;
	}

	m_mapCandidates.clear() ;
	m_vCandidates.clear() ;

	m_tree.Clear() ;

	m_vClusteredCells.clear() ;

	m_bProcessFinished = false ;
	m_iProcessIndex = 0 ;
	m_iProcessSampleIndex = 0 ;
	m_iProcessSampleCount = 0 ;

	int iIndex ;


	// for start cells
	m_vNegativeCells.clear() ;

	std::vector< Cell >::const_iterator nItr = m_vStartCells.begin() ;
	while ( nItr != m_vStartCells.end() )
	{
		iIndex = nItr->m_iZ * ( m_iGridSize * m_iGridSize ) + nItr->m_iY * m_iGridSize + nItr->m_iX ;
		if ( m_vCells[ iIndex ] )
		{
			m_vCells[ iIndex ]->SetCellType( E_CT_START ) ;

			m_vNegativeCells.push_back( *m_vCells[ iIndex ] ) ;
		}

		++nItr ;
	}
	
	// initialize lightning tree
	InitTree() ;
	
	// for target cells
	if ( E_LT_SINGLE_TARGET == m_eLightningType || E_LT_GROUND_TARGET == m_eLightningType )
	{
		std::vector< Cell >::const_iterator pItr = m_vEndCells.begin() ;
		while ( pItr != m_vEndCells.end() )
		{
			iIndex = pItr->m_iZ * ( m_iGridSize * m_iGridSize ) + pItr->m_iY * m_iGridSize + pItr->m_iX ;
			if ( m_vCells[ iIndex ] )
			{
				m_vCells[ iIndex ]->SetCellType( E_CT_END ) ;
			}

			++pItr ;
		}
	}
	else if ( E_LT_MULTIPLE_TARGET == m_eLightningType || E_LT_CHAIN_LIGHTNING == m_eLightningType )
	{
		m_vPositiveCells.clear() ;
		m_iTargetIndex = 0 ;

		bool bFirst = true ;

		std::vector< Pos >::const_iterator itr = m_vTargets.begin() ;
		while ( itr != m_vTargets.end() )
		{
			int iIndex = itr->z * ( m_iGridSize * m_iGridSize ) + itr->y * m_iGridSize + itr->x ;
			if ( m_vCells[ iIndex ] )
			{
				if ( bFirst )
				{
					bFirst = false ;

					m_vCells[ iIndex ]->SetCellType( E_CT_END ) ;

					m_vPositiveCells.push_back( *m_vCells[ iIndex ] ) ;
				}
				else
				{
					m_vCells[ iIndex ]->SetCellType( E_CT_TEMP ) ;
				}
			}

			++itr ;
		}

		// one time computation for specific target
		CalcPositiveElectricPotential() ;
	}

	// initialize for negative charges
	InitNegativeElectricPotential() ;
	CreateMultiScaledClusterMap( m_iClusterGridSize ) ;

	// generate initial candidate map
	CreateCandidateMap() ;

	// calculate electric potential for candidate cells
	CalcElectricPotential() ;
}

void	LightningGenerator3D::CreateBoundaryCells()
{
	std::cout << "Calculate boundary cells" << std::endl ;

	m_vBoundaryCells.clear() ;
	m_vBoundaryCells.reserve( m_iGridSize * m_iGridSize * 6 + 8 ) ;	// 6 plane + 8 corner

	Cell cell ;

	// add boundary charges
	// - front
	for ( int i = 0; i < m_iGridSize; ++i )
	{
		for ( int j = 0; j < m_iGridSize; ++j )
		{
			cell.m_iX = j ;		cell.m_iY = i ;		cell.m_iZ = -1 ;			m_vBoundaryCells.push_back( cell ) ;
		}
	}

	// - back
	for ( int i = 0; i < m_iGridSize; ++i )
	{
		for ( int j = 0; j < m_iGridSize; ++j )
		{
			cell.m_iX = j ;		cell.m_iY = i ;		cell.m_iZ = m_iGridSize ;	m_vBoundaryCells.push_back( cell ) ;
		}
	}

	// - top
	for ( int i = 0; i < m_iGridSize; ++i )
	{
		for ( int j = 0; j < m_iGridSize; ++j )
		{
			cell.m_iX = j ;		cell.m_iY = -1 ;			cell.m_iZ = i ;		m_vBoundaryCells.push_back( cell ) ;
		}
	}

	// - bottom
	for ( int i = 0; i < m_iGridSize; ++i )
	{
		for ( int j = 0; j < m_iGridSize; ++j )
		{
			cell.m_iX = j ;		cell.m_iY = m_iGridSize ;	cell.m_iZ = i ;		m_vBoundaryCells.push_back( cell ) ;
		}
	}

	// - left
	for ( int i = 0; i < m_iGridSize; ++i )
	{
		for ( int j = 0; j < m_iGridSize; ++j )
		{
			cell.m_iX = -1 ;		cell.m_iY = i ;			cell.m_iZ = j ;		m_vBoundaryCells.push_back( cell ) ;
		}
	}

	// - right
	for ( int i = 0; i < m_iGridSize; ++i )
	{
		for ( int j = 0; j < m_iGridSize; ++j )
		{
			cell.m_iX = m_iGridSize ;	cell.m_iY = i ;		cell.m_iZ = j ;		m_vBoundaryCells.push_back( cell ) ;
		}
	}

	// 8 corner
	cell.m_iX = -1 ;			cell.m_iY = -1 ;			cell.m_iZ = -1 ;				m_vBoundaryCells.push_back( cell ) ;
	cell.m_iX = m_iGridSize ;	cell.m_iY = -1 ;			cell.m_iZ = -1 ;				m_vBoundaryCells.push_back( cell ) ;
	cell.m_iX = -1 ;			cell.m_iY = m_iGridSize ;	cell.m_iZ = -1 ;				m_vBoundaryCells.push_back( cell ) ;
	cell.m_iX = m_iGridSize ;	cell.m_iY = m_iGridSize ;	cell.m_iZ = -1 ;				m_vBoundaryCells.push_back( cell ) ;

	cell.m_iX = -1 ;			cell.m_iY = -1 ;			cell.m_iZ = m_iGridSize ;		m_vBoundaryCells.push_back( cell ) ;
	cell.m_iX = m_iGridSize ;	cell.m_iY = -1 ;			cell.m_iZ = m_iGridSize ;		m_vBoundaryCells.push_back( cell ) ;
	cell.m_iX = -1 ;			cell.m_iY = m_iGridSize ;	cell.m_iZ = m_iGridSize ;		m_vBoundaryCells.push_back( cell ) ;
	cell.m_iX = m_iGridSize ;	cell.m_iY = m_iGridSize ;	cell.m_iZ = m_iGridSize ;		m_vBoundaryCells.push_back( cell ) ;

	std::cout << "Finish calculating boundary cells" << std::endl ;
}

void	LightningGenerator3D::CalcBoundaryElectricPotential()
{
	std::cout << "Calculate electric potential for boundary cells" << std::endl ;

	m_vBoundaryPotential.clear() ;
	m_vBoundaryPotential.reserve( m_iGridSize * m_iGridSize * m_iGridSize ) ;

	size_t iIndex = 0 ;
	Cell* pCell ;
	float B ;
	float r ;

	size_t iTotal = m_iGridSize * m_iGridSize * m_iGridSize ;
	size_t iCheckUnit = 100 ;
	size_t iCurTarget = iCheckUnit ;

	for ( int i = 0; i < m_iGridSize; ++i )
	{
		for ( int j = 0; j < m_iGridSize; ++j )
		{
			for ( int k = 0; k < m_iGridSize; ++k )
			{
				pCell = m_vCells[ iIndex ] ;
				B = 0 ;
			
				if ( pCell && E_CT_EMPTY == pCell->m_eType )
				{
					std::vector< Cell >::const_iterator bItr = m_vBoundaryCells.begin() ;
					while ( bItr != m_vBoundaryCells.end() )
					{
						r = Distance( bItr->m_iX, bItr->m_iY, bItr->m_iZ, pCell->m_iX, pCell->m_iY, pCell->m_iZ ) ;
						if ( m_fPowOfR > 1 )
						{
							r = pow( r, m_fPowOfR ) ;
						}

						B += 1.0f / r ;
				
						++bItr ;
					}
				}

				m_vBoundaryPotential.push_back( B ) ;

				++iIndex ;

				if ( iIndex >= iCurTarget )
				{
					std::cout << "  - calculating ... ( " << iIndex << " / " << iTotal << " )" << std::endl ;

					iCurTarget += iCheckUnit ;
				}
			}
		}
	}

	std::cout << "Finish calculating pre-computed electric potential for boundary cells" << std::endl ;
}

void	LightningGenerator3D::LoadBoundaryData( const std::string& strPath )
{
	m_vBoundaryPotential.clear() ;
	m_vBoundaryPotential.assign( m_iGridSize * m_iGridSize * m_iGridSize, 0 ) ;

	FILE* fp = fopen( strPath.c_str(), "rb" ) ;
	if ( fp )
	{
		fread( &m_vBoundaryPotential[ 0 ], sizeof( float ), m_vBoundaryPotential.size(), fp ) ;
		fclose( fp ) ;
	}
}

void	LightningGenerator3D::SaveBoundaryData( const std::string& strPath )
{
	FILE* fp = fopen( strPath.c_str(), "wb" ) ;
	if ( fp )
	{
		fwrite( &m_vBoundaryPotential[ 0 ], sizeof( float ), m_vBoundaryPotential.size(), fp ) ;
		fclose( fp ) ;
	}
}

void	LightningGenerator3D::CalcObstacleElectricPotential()
{
	std::cout << "Calculate electric potential for obstacle cells" << std::endl ;

	m_vObstaclePotential.clear() ;
	m_vObstaclePotential.assign( m_iGridSize * m_iGridSize * m_iGridSize, 0.0f ) ;

	if ( false == m_vObstacleCells.empty() )
	{
		size_t iIndex = 0 ;
		Cell* pCell ;
		float Obs ;
		float r ;

		size_t iTotal = m_iGridSize * m_iGridSize * m_iGridSize ;
		size_t iCheckUnit = 10 ;
		size_t iCurTarget = iCheckUnit ;

		for ( int i = 0; i < m_iGridSize; ++i )
		{
			for ( int j = 0; j < m_iGridSize; ++j )
			{
				for ( int k = 0; k < m_iGridSize; ++k )
				{
					pCell = m_vCells[ iIndex ] ;
					Obs = 0 ;
			
					if ( pCell && E_CT_EMPTY == pCell->m_eType )
					{
						std::vector< Cell >::const_iterator oItr = m_vObstacleCells.begin() ;
						while ( oItr != m_vObstacleCells.end() )
						{
							r = Distance( oItr->m_iX, oItr->m_iY, oItr->m_iZ, pCell->m_iX, pCell->m_iY, pCell->m_iZ ) ;
							if ( m_fPowOfR > 1 )
							{
								r = pow( r, m_fPowOfR ) ;
							}

							Obs += 1.0f / r ;
				
							++oItr ;
						}
					}

					m_vObstaclePotential[ iIndex ] = Obs ;

					++iIndex ;

					if ( iIndex >= iCurTarget )
					{
						std::cout << "  - calculating ... ( " << iIndex << " / " << iTotal << " )" << std::endl ;

						iCurTarget += iCheckUnit ;
					}
				}
			}
		}
	}

	std::cout << "Finish calculating pre-computed electric potential for obstacle cells" << std::endl ;
}

void	LightningGenerator3D::CalcPositiveElectricPotential()
{
	std::cout << "Calculate electric potential for positive cells" << std::endl ;

	m_vPositivePotential.clear() ;
	m_vPositivePotential.assign( m_iGridSize * m_iGridSize * m_iGridSize, 0.0f ) ;

	// ------------------------------------------------------------------------
	// OpenMP version
#pragma omp parallel
{
	#pragma omp for
	for ( int i = 0; i < m_iGridSize * m_iGridSize * m_iGridSize; ++i )
	{
		Cell* pCell = m_vCells[ i ] ;
		float P = 0 ;
		float r ;
			
		if ( pCell && E_CT_EMPTY == pCell->m_eType )
		{
			std::vector< Cell >::const_iterator pItr = m_vPositiveCells.begin() ;
			while ( pItr != m_vPositiveCells.end() )
			{
				r = Distance( pItr->m_iX, pItr->m_iY, pItr->m_iZ, pCell->m_iX, pCell->m_iY, pCell->m_iZ ) ;
				if ( m_fPowOfR > 1 )
				{
					r = pow( r, m_fPowOfR ) ;
				}

				P += 1.0f / r ;
		
				++pItr ;
			}
		}

		m_vPositivePotential[ i ] = P ;
	}
}

	// ------------------------------------------------------------------------
	// Single CPU version
/*/
	int iIndex = 0 ;
	Cell* pCell ;
	float P ;
	float r ;

	for ( int i = 0; i < m_iGridSize; ++i )
	{
		for ( int j = 0; j < m_iGridSize; ++j )
		{
			for ( int k = 0; k < m_iGridSize; ++k )
			{
				pCell = m_vCells[ iIndex ] ;
				P = 0 ;
			
				if ( pCell && E_CT_EMPTY == pCell->m_eType )
				{
					std::vector< Cell >::const_iterator pItr = m_vPositiveCells.begin() ;
					while ( pItr != m_vPositiveCells.end() )
					{
						r = Distance( pItr->m_iX, pItr->m_iY, pItr->m_iZ, pCell->m_iX, pCell->m_iY, pCell->m_iZ ) ;
						if ( m_fPowOfR > 1 )
						{
							r = pow( r, m_fPowOfR ) ;
						}

						P += 1.0f / r ;
			
						++pItr ;
					}
				}

				m_vPositivePotential[ iIndex ] = P ;

				++iIndex ;
			}
		}
	}
//*/

	std::cout << "Finish calculating one-time computed electric potential for positive cells" << std::endl ;
}

void	LightningGenerator3D::InitNegativeElectricPotential()
{
	m_vNegativePotential.clear() ;
	m_vNegativePotential.assign( m_iGridSize * m_iGridSize * m_iGridSize, 0.0f ) ;
}


void	LightningGenerator3D::CreateMultiScaledClusterMap( int iMultiScaledClusterMapSize )
{
	std::cout << "Initialize clustered map information" << std::endl ;

	m_iClusterGridSize = iMultiScaledClusterMapSize ;
	int iMultiScaledRegionSize = m_iGridSize / m_iClusterGridSize ;

	m_vClusteredCells.clear() ;
	m_vClusteredCells.reserve( m_iClusterGridSize * m_iClusterGridSize * m_iClusterGridSize ) ;

	ClusteredCell cell ;

	for ( int i = 0; i < m_iClusterGridSize; ++i )
	{
		for ( int j = 0; j < m_iClusterGridSize; ++j )
		{
			for ( int k = 0; k < m_iClusterGridSize; ++k )
			{
				cell.m_iX = k ;
				cell.m_iY = j ;
				cell.m_iZ = i ;

				m_vClusteredCells.push_back( cell ) ;
			}
		}
	}
	
	int x, y, z ;
	int iIndex ;

	std::vector< Cell >::iterator nItr = m_vNegativeCells.begin() ;
	while ( nItr != m_vNegativeCells.end() )
	{
		x = nItr->m_iX / iMultiScaledRegionSize ;
		y = nItr->m_iY / iMultiScaledRegionSize ;
		z = nItr->m_iZ / iMultiScaledRegionSize ;

		iIndex = z * ( m_iClusterGridSize * m_iClusterGridSize ) + y * m_iClusterGridSize + x ;

		m_vClusteredCells[ iIndex ].m_vCells.push_back( *nItr ) ;

		m_vClusteredCells[ iIndex ].m_iXSum += nItr->m_iX ;
		m_vClusteredCells[ iIndex ].m_iYSum += nItr->m_iY ;
		m_vClusteredCells[ iIndex ].m_iZSum += nItr->m_iZ ;

		m_vClusteredCells[ iIndex ].m_fAvgX = (float)m_vClusteredCells[ iIndex ].m_iXSum / m_vClusteredCells[ iIndex ].m_vCells.size() ;
		m_vClusteredCells[ iIndex ].m_fAvgY = (float)m_vClusteredCells[ iIndex ].m_iYSum / m_vClusteredCells[ iIndex ].m_vCells.size() ;
		m_vClusteredCells[ iIndex ].m_fAvgZ = (float)m_vClusteredCells[ iIndex ].m_iZSum / m_vClusteredCells[ iIndex ].m_vCells.size() ;
		
		++nItr ;
	}

	std::cout << "Finish initializing clustered map information" << std::endl ;
}

void	LightningGenerator3D::CreateCandidateMap()
{
	std::cout << "Create candidate cell map" << std::endl ;

	m_mapCandidates.clear() ;

	int iIndex ;
	int x, y, z ;
	int cx, cy, cz ;
	int iChildIndex ;

	std::vector< Cell >::iterator itr = m_vNegativeCells.begin() ;
	while ( itr != m_vNegativeCells.end() )
	{
		x = ( *itr ).m_iX ;
		y = ( *itr ).m_iY ;
		z = ( *itr ).m_iZ ;
		
		iIndex = z * m_iGridSize * m_iGridSize + y * m_iGridSize + x ;

		if ( m_vCells[ iIndex ] )
		{
			// check 26 direction
			for ( int dir = 0; dir < 26; ++dir )
			{
				cx = x + Cell::DIR_WAY_X_DIFF[ dir ] ;
				cy = y + Cell::DIR_WAY_Y_DIFF[ dir ] ;
				cz = z + Cell::DIR_WAY_Z_DIFF[ dir ] ;

				iChildIndex = cz * ( m_iGridSize * m_iGridSize ) + cy * m_iGridSize + cx ;

				if ( cx >= 0 && cx < m_iGridSize && cy >= 0 && cy < m_iGridSize && cz >= 0 && cz < m_iGridSize && m_vCells[ iChildIndex ] )
				{
					if ( E_CT_EMPTY == m_vCells[ iChildIndex ]->m_eType )
					{
						std::map< int, Cell* >::iterator itrCandidate = m_mapCandidates.find( iChildIndex ) ;
						if ( itrCandidate == m_mapCandidates.end() )
						{
							m_mapCandidates.insert( std::map< int, Cell* >::value_type( iChildIndex, m_vCells[ iChildIndex ] ) ) ;
						}
					}
				}
			}
		}

		++itr ;
	}

	std::cout << "Finish creating candidate cell map" << std::endl ;
}

void	LightningGenerator3D::CalcElectricPotential()
{
	int iTotalCellNum = m_iGridSize * m_iGridSize * m_iGridSize ;
	if ( m_vCells.size() < iTotalCellNum )
	{
		std::cerr << std::endl ;
		std::cerr << "cell size error !! " << std::endl ;

		return ;
	}

	// calculate electric field for only candidate cells
	float fPotential ;
	float r ;
	float B, N, P, Obs ;

	int iClusteredRegionSize = m_iGridSize / m_iClusterGridSize ;
	int iClusteredIndex ;

	int iCandidateClusteredX ;
	int iCandidateClusteredY ;
	int iCandidateClusteredZ ;
	int iCandidateClusteredIndex ;
	
	int iKey ;
	Cell* pCell ;

	if ( 0 == m_mapCandidates.size() )
	{
		std::cout << "There is no candidate cells to compute electric potential !!" << std::endl ;
	}

	// for candidate cells
	std::map< int, Cell* >::iterator mapItr = m_mapCandidates.begin() ;
	while ( mapItr != m_mapCandidates.end() )
	{
		iKey = mapItr->first ;
		pCell = mapItr->second ;

		if ( pCell && E_CT_EMPTY == pCell->m_eType )
		{
			// -----------------------------------------------------------
			// for boundaries, use pre-computed values
			B = m_vBoundaryPotential[ iKey ] ;

			// -----------------------------------------------------------
			// for obstacles, use pre-computed values
			Obs = m_vObstaclePotential[ iKey ] ;

			// -----------------------------------------------------------
			// for positive charges, use pre-computed value
			P = m_vPositivePotential[ iKey ] ;


			// -----------------------------------------------------------
			// for negative charges
			iCandidateClusteredX = pCell->m_iX / iClusteredRegionSize ;
			iCandidateClusteredY = pCell->m_iY / iClusteredRegionSize ;
			iCandidateClusteredZ = pCell->m_iZ / iClusteredRegionSize ;
			iCandidateClusteredIndex = iCandidateClusteredZ * ( m_iClusterGridSize * m_iClusterGridSize ) + iCandidateClusteredY * m_iClusterGridSize + iCandidateClusteredX ;

			iClusteredIndex = 0 ;

			N = 0 ;

			for ( int cz = 0; cz < m_iClusterGridSize; ++cz )
			{
				for ( int cy = 0; cy < m_iClusterGridSize; ++cy )
				{
					for ( int cx = 0; cx < m_iClusterGridSize; ++cx )
					{
						if ( !m_vClusteredCells[ iClusteredIndex ].m_vCells.empty() )
						{
							if ( iClusteredIndex != iCandidateClusteredIndex )
							{
								// negative charges are not in the same cluster of candidate cell
								r = Distance( m_vClusteredCells[ iClusteredIndex ].m_fAvgX, m_vClusteredCells[ iClusteredIndex ].m_fAvgY, m_vClusteredCells[ iClusteredIndex ].m_fAvgZ, pCell->m_iX, pCell->m_iY, pCell->m_iZ ) ;
								if ( m_fPowOfR > 1 )
								{
									r = pow( r, m_fPowOfR ) ;
								}

								N += m_vClusteredCells[ iClusteredIndex ].m_vCells.size() / r ;
							}
							else	// for the same cluster
							{
								std::vector< Cell >::const_iterator nItr = m_vClusteredCells[ iClusteredIndex ].m_vCells.begin() ;
								while ( nItr != m_vClusteredCells[ iClusteredIndex ].m_vCells.end() )
								{
									r = Distance( nItr->m_iX, nItr->m_iY, nItr->m_iZ, pCell->m_iX, pCell->m_iY, pCell->m_iZ ) ;
									if ( m_fPowOfR > 1 )
									{
										r = pow( r, m_fPowOfR ) ;
									}

									N += 1.0f / r ;

									++nItr ;
								}
							}
						}

						++iClusteredIndex ;
					}
				}
			}
			
			// -----------------------------------------------------------
			// compute final potential value
			if ( 0 == Obs )
			{
				fPotential = ( 1.0f / B ) * ( 1.0f / N ) * P ;
			}
			else
			{
				fPotential = ( 1.0f / B ) * ( 1.0f / N ) * P * ( 1.0f / Obs ) ;
			}

			pCell->m_fLastN = N ;
			pCell->m_fP = P ;
			pCell->m_fB = B ;
			pCell->m_fObs = Obs ;

			pCell->m_fPotential = fPotential ;
		}

		++mapItr ;
	}
}

void	LightningGenerator3D::CalcElectricPotential( Cell* pCell )
{
	float fPotential ;
	float r ;
	float B, N, P, Obs ;

	int iClusteredRegionSize = m_iGridSize / m_iClusterGridSize ;
	int iClusteredIndex ;

	int iCandidateClusteredX ;
	int iCandidateClusteredY ;
	int iCandidateClusteredZ ;
	int iCandidateClusteredIndex ;
	
	if ( pCell && E_CT_EMPTY == pCell->m_eType )
	{
		int iKey = pCell->m_iZ * ( m_iGridSize * m_iGridSize ) + pCell->m_iY * m_iGridSize + pCell->m_iX ;

		// -----------------------------------------------------------
		// for boundaries, use pre-computed values
		B = m_vBoundaryPotential[ iKey ] ;

		// -----------------------------------------------------------
		// for obstacles, use pre-computed values
		Obs = m_vObstaclePotential[ iKey ] ;

		// -----------------------------------------------------------
		// for positive charges, use pre-computed value
		P = m_vPositivePotential[ iKey ] ;
		
		// -----------------------------------------------------------
		// for negative charges
		N = 0 ;

		iCandidateClusteredX = pCell->m_iX / iClusteredRegionSize ;
		iCandidateClusteredY = pCell->m_iY / iClusteredRegionSize ;
		iCandidateClusteredZ = pCell->m_iZ / iClusteredRegionSize ;
		iCandidateClusteredIndex = iCandidateClusteredZ * ( m_iClusterGridSize * m_iClusterGridSize ) + iCandidateClusteredY * m_iClusterGridSize + iCandidateClusteredX ;

		iClusteredIndex = 0 ;

		for ( int cz = 0; cz < m_iClusterGridSize; ++cz )
		{
			for ( int cy = 0; cy < m_iClusterGridSize; ++cy )
			{
				for ( int cx = 0; cx < m_iClusterGridSize; ++cx )
				{
					if ( !m_vClusteredCells[ iClusteredIndex ].m_vCells.empty() )
					{
						if ( iClusteredIndex != iCandidateClusteredIndex )
						{
							// negative charges are not in the same cluster of candidate cell
							r = Distance( m_vClusteredCells[ iClusteredIndex ].m_fAvgX, m_vClusteredCells[ iClusteredIndex ].m_fAvgY, m_vClusteredCells[ iClusteredIndex ].m_fAvgZ, pCell->m_iX, pCell->m_iY, pCell->m_iZ ) ;
							if ( m_fPowOfR > 1 )
							{
								r = pow( r, m_fPowOfR ) ;
							}

							N += m_vClusteredCells[ iClusteredIndex ].m_vCells.size() / r ;
						}
						else	// for the same cluster
						{
							std::vector< Cell >::const_iterator nItr = m_vClusteredCells[ iClusteredIndex ].m_vCells.begin() ;
							while ( nItr != m_vClusteredCells[ iClusteredIndex ].m_vCells.end() )
							{
								r = Distance( nItr->m_iX, nItr->m_iY, nItr->m_iZ, pCell->m_iX, pCell->m_iY, pCell->m_iZ ) ;
								if ( m_fPowOfR > 1 )
								{
									r = pow( r, m_fPowOfR ) ;
								}

								N += 1.0f / r ;

								++nItr ;
							}
						}
					}

					++iClusteredIndex ;
				}
			}
		}

		// -----------------------------------------------------------
		// compute final potential value
		if ( 0 == Obs )
		{
			fPotential = ( 1.0f / B ) * ( 1.0f / N ) * P ;
		}
		else
		{
			fPotential = ( 1.0f / B ) * ( 1.0f / N ) * P * ( 1.0f / Obs ) ;
		}

		pCell->m_fLastN = N ;
		pCell->m_fP = P ;
		pCell->m_fB = B ;
		pCell->m_fObs = Obs ;

		pCell->m_fPotential = fPotential ;
	}
}

void	LightningGenerator3D::UpdateCandidate()
{
	// add candidates
	m_vCandidates.clear() ;

	int iIndex ;

	int px, py, pz ;
	int cx, cy, cz ;
	int iAdjIndex ;

	std::vector< Cell >::const_iterator itr = m_vNegativeCells.begin() ;
	while ( itr != m_vNegativeCells.end() )
	{
		px = ( *itr ).m_iX ;
		py = ( *itr ).m_iY ;
		pz = ( *itr ).m_iZ ;

		iIndex = pz * ( m_iGridSize * m_iGridSize ) + py * m_iGridSize + px ;

		if ( m_vCells[ iIndex ] )
		{
			// check 26 direction
			for ( int dir = 0; dir < 26; ++dir )
			{
				cx = px + Cell::DIR_WAY_X_DIFF[ dir ] ;
				cy = py + Cell::DIR_WAY_Y_DIFF[ dir ] ;
				cz = pz + Cell::DIR_WAY_Z_DIFF[ dir ] ;

				iAdjIndex = cz * ( m_iGridSize * m_iGridSize ) + cy * m_iGridSize + cx ;

				if ( cx >= 0 && cx < m_iGridSize && cy >= 0 && cy < m_iGridSize && cz >= 0 && cz < m_iGridSize && m_vCells[ iAdjIndex ] )
				{
					if ( E_CT_EMPTY == m_vCells[ iAdjIndex ]->m_eType )
					{
						if ( 0 != m_vCells[ iAdjIndex ]->m_fPotential )	// remove 0 potential cells
						{
							Cell candi ;
							candi.m_iX = cx ;
							candi.m_iY = cy ;
							candi.m_iZ = cz ;
							candi.m_iParentX = px ;
							candi.m_iParentY = py ;
							candi.m_iParentZ = pz ;
							candi.m_fProb = m_vCells[ iAdjIndex ]->m_fPotential ;

							m_vCandidates.push_back( candi ) ;
						}
					}
				}
			}
		}

		++itr ;
	}
}

bool	LightningGenerator3D::SelectCandidate( Cell& outNextCell )
{
	outNextCell.m_iX = -1 ;
	outNextCell.m_iY = -1 ;
	outNextCell.m_iZ = -1 ;

	bool bResult = true ;
	int iSelectedIndex = 0 ;
	
	std::vector< float > vProb ;
	std::vector< Cell >::iterator itr = m_vCandidates.begin() ;
	while ( itr != m_vCandidates.end() )
	{
		vProb.push_back( pow( fabs( ( *itr ).m_fProb ), m_iEta ) ) ;	// apply eta

		++itr ;
	}

	// -------------------------------------------------------------------------------
	// for VS 2015
	//std::discrete_distribution<> d( vProb.begin(), vProb.end() ) ;
	// -------------------------------------------------------------------------------

	// -------------------------------------------------------------------------------
	// for VS 2013
	std::size_t i( 0 ) ;
	std::discrete_distribution<> d( vProb.size(), 0, 1, [&vProb, &i](double)
	{
		auto w = vProb[ i ] ;
		++i ;
		return w ;
	} ) ;
	// -------------------------------------------------------------------------------

	int iErrorCount = 0 ;
	iSelectedIndex = d( m_randGen ) ;
		
	while ( iSelectedIndex == m_vCandidates.size() )
	{
		++iErrorCount ;
		std::cout << "Index error !!!!!" << std::endl ;

		if ( iErrorCount >= 10 )
		{
			bResult = false ;
			break ;
		}

		iSelectedIndex = d( m_randGen ) ;
	}

	if ( bResult )
	{
		outNextCell = m_vCandidates[ iSelectedIndex ] ;
	}

	return bResult ;
}

void	LightningGenerator3D::AddNewLightningPath( const Cell& newPath, bool bIsEndCell, bool bTargetCell )
{
	int iIndex = newPath.m_iZ * ( m_iGridSize * m_iGridSize ) + newPath.m_iY * m_iGridSize + newPath.m_iX ;

	if ( !bIsEndCell )
	{
		// update cell type
		SetCellType( iIndex, E_CT_START ) ;
	}

	// add lightning node to tree
	m_tree.AddChild( newPath.m_iParentX, newPath.m_iParentY, newPath.m_iParentZ, newPath.m_iX, newPath.m_iY, newPath.m_iZ, bTargetCell ) ;
}

void	LightningGenerator3D::UpdateCandidateMap( const Cell& nextCell )
{
	int x = nextCell.m_iX ;
	int y = nextCell.m_iY ;
	int z = nextCell.m_iZ ;
	
	int iIndex = z * ( m_iGridSize * m_iGridSize ) + y * m_iGridSize + x ;

	// remove cell from candidate map
	std::map< int, Cell* >::iterator itrCandidate = m_mapCandidates.find( iIndex ) ;
	if ( itrCandidate != m_mapCandidates.end() )
	{
		m_mapCandidates.erase( iIndex ) ;
	}

	// update electric potential for candidate cells
	float r ;
	Cell* pCell ;

	std::map< int, Cell* >::iterator itr = m_mapCandidates.begin() ;
	while ( itr != m_mapCandidates.end() )
	{
		pCell = itr->second ;
		if ( pCell )
		{
			r = Distance( x, y, z, pCell->m_iX, pCell->m_iY, pCell->m_iZ ) ;
			if ( m_fPowOfR > 1 )
			{
				r = pow( r, m_fPowOfR ) ;
			}

			pCell->m_fLastN += 1.0f / r ;

			if ( 0 == pCell->m_fObs )
			{
				pCell->m_fPotential = ( 1.0f / pCell->m_fB ) * ( 1.0f / pCell->m_fLastN ) * pCell->m_fP ;
			}
			else
			{
				pCell->m_fPotential = ( 1.0f / pCell->m_fB ) * ( 1.0f / pCell->m_fLastN ) * pCell->m_fP * ( 1.0f / pCell->m_fObs ) ;
			}
		}

		++itr ;
	}

	// add candidate cells for new lightning cell
	if ( m_vCells[ iIndex ] )
	{
		int cx, cy, cz ;
		int iChildIndex ;

		// check 26 direction
		for ( int dir = 0; dir < 26; ++dir )
		{
			cx = x + Cell::DIR_WAY_X_DIFF[ dir ] ;
			cy = y + Cell::DIR_WAY_Y_DIFF[ dir ] ;
			cz = z + Cell::DIR_WAY_Z_DIFF[ dir ] ;
			
			iChildIndex = cz * ( m_iGridSize * m_iGridSize ) + cy * m_iGridSize + cx ;

			if ( cx >= 0 && cx < m_iGridSize && cy >= 0 && cy < m_iGridSize && cz >= 0 && cz < m_iGridSize && m_vCells[ iChildIndex ] )
			{
				if ( E_CT_EMPTY == m_vCells[ iChildIndex ]->m_eType )
				{
					std::map< int, Cell* >::iterator itrCandidate = m_mapCandidates.find( iChildIndex ) ;
					if ( itrCandidate == m_mapCandidates.end() )
					{
						m_mapCandidates.insert( std::map< int, Cell* >::value_type( iChildIndex, m_vCells[ iChildIndex ] ) ) ;

						// calculate electric potential for candidate cells which are newly added
						CalcElectricPotential( m_vCells[ iChildIndex ] ) ;
					}
				}
			}
		}
	}
}

void	LightningGenerator3D::UpdateClusteredMap( const Cell& nextCell )
{
	int iIndex ;
	int x, y, z ;

	int iClusteredRegionSize = m_iGridSize / m_iClusterGridSize ;

	x = nextCell.m_iX / iClusteredRegionSize ;
	y = nextCell.m_iY / iClusteredRegionSize ;
	z = nextCell.m_iZ / iClusteredRegionSize ;
	
	iIndex = z * ( m_iClusterGridSize * m_iClusterGridSize ) + y * m_iClusterGridSize + x ;

	m_vClusteredCells[ iIndex ].m_vCells.push_back( nextCell ) ;

	m_vClusteredCells[ iIndex ].m_iXSum += nextCell.m_iX ;
	m_vClusteredCells[ iIndex ].m_iYSum += nextCell.m_iY ;
	m_vClusteredCells[ iIndex ].m_iZSum += nextCell.m_iZ ;

	m_vClusteredCells[ iIndex ].m_fAvgX = (float)m_vClusteredCells[ iIndex ].m_iXSum / m_vClusteredCells[ iIndex ].m_vCells.size() ;
	m_vClusteredCells[ iIndex ].m_fAvgY = (float)m_vClusteredCells[ iIndex ].m_iYSum / m_vClusteredCells[ iIndex ].m_vCells.size() ;
	m_vClusteredCells[ iIndex ].m_fAvgZ = (float)m_vClusteredCells[ iIndex ].m_iZSum / m_vClusteredCells[ iIndex ].m_vCells.size() ;
}

void	LightningGenerator3D::ApplyIntensityAndThickness( float fBaseThickness, float fIntensityAttenuation )
{
	m_tree.ApplyIntensityAndThickness( fBaseThickness, fIntensityAttenuation ) ;
}

void	LightningGenerator3D::ApplyIntensityAndThicknessForMultipleTarget( float fBaseThickness, float fIntensityAttenuation )
{
	m_tree.ApplyIntensityAndThicknessForMultipleTarget( fBaseThickness, fIntensityAttenuation, m_vTargets.size() ) ;
}

bool	LightningGenerator3D::ProcessSingleTarget( ProcessType eType )
{
	m_iTimeProcesStart = glutGet( GLUT_ELAPSED_TIME ) ;

	if ( m_bProcessFinished )
	{
		return false ;
	}

	bool bLoop = true ;

	while ( bLoop )
	{
		++m_iProcessIndex ;
		++m_iProcessSampleIndex ;

		if ( E_PT_ONE_STEP == eType )	// one step
		{
			bLoop = false ;
		}
		else if ( E_PT_SAMPLE_STEP == eType && m_iProcessSampleIndex >= m_iProcessSampleCount )
		{
			m_iProcessSampleIndex = 0 ;
			bLoop = false ;
		}

		// ----------------------------------------------------------------
		// get candidates
		UpdateCandidate() ;

		// ----------------------------------------------------------------
		// select next cell from candidates
		Cell nextCell ;
		bool bRet = SelectCandidate( nextCell ) ;

		// ----------------------------------------------------------------
		// set next cell & update cells
		if ( bRet )
		{
			bool bReachWaypoint = false ;
			bool bReachEndCell = false ;

			int iEndX, iEndY, iEndZ ;

			if ( m_vWaypointsCells.size() > 0 )
			{
				if ( IsNearToWaypoint( nextCell.m_iX, nextCell.m_iY, nextCell.m_iZ, iEndX, iEndY, iEndZ ) )
				{
					bReachWaypoint = true ;
				}
				else if ( IsNearToEndCell( nextCell.m_iX, nextCell.m_iY, nextCell.m_iZ, iEndX, iEndY, iEndZ ) )
				{
					bReachEndCell = true ;
				}
			}
			else
			{
				if ( IsNearToEndCell( nextCell.m_iX, nextCell.m_iY, nextCell.m_iZ, iEndX, iEndY, iEndZ ) )
				{
					bReachEndCell = true ;
				}
			}

			if ( bReachWaypoint )
			{
				AddNewLightningPath( nextCell ) ;

				// update multi scale map
				UpdateClusteredMap( nextCell ) ;

				// update candidate map
				UpdateCandidateMap( nextCell ) ;

				// add way point position
				nextCell.m_iParentX = nextCell.m_iX ;
				nextCell.m_iParentY = nextCell.m_iY ;
				nextCell.m_iParentZ = nextCell.m_iZ ;
				nextCell.m_iX = iEndX ;
				nextCell.m_iY = iEndY ;
				nextCell.m_iZ = iEndZ ;

				// update final target cell
				AddNewLightningPath( nextCell ) ;

				// update multi scale map
				UpdateClusteredMap( nextCell ) ;

				// update candidate map
				UpdateCandidateMap( nextCell ) ;

				// remove way point
				std::vector< Cell >::iterator wItr = m_vWaypointsCells.begin() ;
				while ( wItr != m_vWaypointsCells.end() )
				{
					if ( wItr->m_iX == iEndX && wItr->m_iY == iEndY && wItr->m_iZ == iEndZ )
					{
						m_vWaypointsCells.erase( wItr ) ;
						break ;
					}

					++wItr ;
				}
			}
			else if ( bReachEndCell )
			{
				// end loop
				bLoop = false ;
				m_bProcessFinished = true ;

				AddNewLightningPath( nextCell ) ;

				// update multi scale map
//				UpdateClusteredMap( nextCell ) ;

				// update candidate map
//				UpdateCandidateMap( nextCell ) ;


				// add final target position
				nextCell.m_iParentX = nextCell.m_iX ;
				nextCell.m_iParentY = nextCell.m_iY ;
				nextCell.m_iParentZ = nextCell.m_iZ ;
				nextCell.m_iX = iEndX ;
				nextCell.m_iY = iEndY ;
				nextCell.m_iZ = iEndZ ;

				// update final target cell
				AddNewLightningPath( nextCell, true ) ;

				// update multi scale map
//				UpdateClusteredMap( nextCell ) ;

				// update candidate map
//				UpdateCandidateMap( nextCell ) ;
			}
			else
			{
				AddNewLightningPath( nextCell ) ;

				// update multi scale map
				UpdateClusteredMap( nextCell ) ;

				// update candidate map
				UpdateCandidateMap( nextCell ) ;
			}
		}
	
		// ----------------------------------------------------------------

		if ( m_bProcessFinished )
		{
			ApplyIntensityAndThickness( GetBaseThickness(), GetIntensityAttenuation() ) ;
		}

		// time check per steps
		if ( !m_vTimeCheckSteps.empty() && m_iTimeIndex < m_vTimeCheckSteps.size() && m_iProcessIndex == m_vTimeCheckSteps[ m_iTimeIndex ] )
		{
			m_vTimePerSteps.push_back( glutGet( GLUT_ELAPSED_TIME ) ) ;

			++m_iTimeIndex ;
		}
	}
	
	m_iProcessIndex = 0 ;

	return true ;
}

bool	LightningGenerator3D::ProcessMultipleTarget( ProcessType eType )
{
	m_iTimeProcesStart = glutGet( GLUT_ELAPSED_TIME ) ;

	if ( m_bProcessFinished )
	{
		return false ;
	}

	bool bLoop = true ;

	while ( bLoop )
	{
		++m_iProcessIndex ;
		++m_iProcessSampleIndex ;

		if ( E_PT_ONE_STEP == eType )	// one step
		{
			bLoop = false ;
		}
		else if ( E_PT_SAMPLE_STEP == eType && m_iProcessSampleIndex >= m_iProcessSampleCount )
		{
			m_iProcessSampleIndex = 0 ;
			bLoop = false ;
		}
		
		// ----------------------------------------------------------------
		// get candidates
		UpdateCandidate() ;

		// ----------------------------------------------------------------
		// select next cell from candidates
		Cell nextCell ;
		bool bRet = SelectCandidate( nextCell ) ;

		// ----------------------------------------------------------------
		// set next cell & update cells
		if ( bRet )
		{
			int iEndX, iEndY, iEndZ ;

			if ( IsNearToEndCell( nextCell.m_iX, nextCell.m_iY, nextCell.m_iZ, iEndX, iEndY, iEndZ ) )
			{
				AddNewLightningPath( nextCell ) ;

				if ( m_iTargetIndex == m_vTargets.size() - 1 )
				{
					// end loop
					bLoop = false ;
					m_bProcessFinished = true ;

					// add final target position
					nextCell.m_iParentX = nextCell.m_iX ;
					nextCell.m_iParentY = nextCell.m_iY ;
					nextCell.m_iParentZ = nextCell.m_iZ ;
					nextCell.m_iX = iEndX ;
					nextCell.m_iY = iEndY ;
					nextCell.m_iZ = iEndZ ;

					// update final target cell
					AddNewLightningPath( nextCell, true, true ) ;
				}
				else
				{
					// update multi scale map
					UpdateClusteredMap( nextCell ) ;

					// update candidate map
					UpdateCandidateMap( nextCell ) ;

					// add final target position
					nextCell.m_iParentX = nextCell.m_iX ;
					nextCell.m_iParentY = nextCell.m_iY ;
					nextCell.m_iParentZ = nextCell.m_iZ ;
					nextCell.m_iX = iEndX ;
					nextCell.m_iY = iEndY ;
					nextCell.m_iZ = iEndZ ;

					// update final target cell
					AddNewLightningPath( nextCell, false, true ) ;

					// update multi scale map
					UpdateClusteredMap( nextCell ) ;

					// update candidate map
					UpdateCandidateMap( nextCell ) ;


					// change target for the next target
					++m_iTargetIndex ;

					int iIndex = m_vTargets[ m_iTargetIndex ].z * ( m_iGridSize * m_iGridSize ) + m_vTargets[ m_iTargetIndex ].y * m_iGridSize + m_vTargets[ m_iTargetIndex ].x ;
					if ( m_vCells[ iIndex ] && E_CT_TEMP == m_vCells[ iIndex ]->m_eType )
					{
						m_vCells[ iIndex ]->SetCellType( E_CT_END ) ;

						m_vPositiveCells.clear() ;
						m_vPositiveCells.push_back( *m_vCells[ iIndex ] ) ;

						CalcPositiveElectricPotential() ;
					}
				}
			}
			else
			{
				AddNewLightningPath( nextCell ) ;

				// update multi scale map
				UpdateClusteredMap( nextCell ) ;

				// update candidate map
				UpdateCandidateMap( nextCell ) ;
			}
		}
	
		// ----------------------------------------------------------------

		if ( m_bProcessFinished )
		{
			ApplyIntensityAndThicknessForMultipleTarget( GetBaseThickness(), GetIntensityAttenuation() ) ;
		}


		// time check per steps
		if ( !m_vTimeCheckSteps.empty() && m_iTimeIndex < m_vTimeCheckSteps.size() && m_iProcessIndex == m_vTimeCheckSteps[ m_iTimeIndex ] )
		{
			m_vTimePerSteps.push_back( glutGet( GLUT_ELAPSED_TIME ) ) ;

			++m_iTimeIndex ;
		}
	}

	m_iProcessIndex = 0 ;

	return true ;
}


bool	LightningGenerator3D::ProcessChainLightning( ProcessType eType )
{
	m_iTimeProcesStart = glutGet( GLUT_ELAPSED_TIME ) ;

	if ( m_bProcessFinished )
	{
		return false ;
	}

	bool bLoop = true ;

	while ( bLoop )
	{
		++m_iProcessIndex ;
		++m_iProcessSampleIndex ;

		if ( E_PT_ONE_STEP == eType )	// one step
		{
			bLoop = false ;
		}
		else if ( E_PT_SAMPLE_STEP == eType && m_iProcessSampleIndex >= m_iProcessSampleCount )
		{
			m_iProcessSampleIndex = 0 ;
			bLoop = false ;
		}
		
		// ----------------------------------------------------------------
		// get candidates
		UpdateCandidate() ;

		// ----------------------------------------------------------------
		// select next cell from candidates
		Cell nextCell ;
		bool bRet = SelectCandidate( nextCell ) ;

		// ----------------------------------------------------------------
		// set next cell & update cells
		if ( bRet )
		{
			int iEndX, iEndY, iEndZ ;

			if ( IsNearToEndCell( nextCell.m_iX, nextCell.m_iY, nextCell.m_iZ, iEndX, iEndY, iEndZ ) )
			{
				AddNewLightningPath( nextCell ) ;

				if ( m_iTargetIndex == m_vTargets.size() - 1 )
				{
					// end loop
					bLoop = false ;
					m_bProcessFinished = true ;

					// add final target position
					nextCell.m_iParentX = nextCell.m_iX ;
					nextCell.m_iParentY = nextCell.m_iY ;
					nextCell.m_iParentZ = nextCell.m_iZ ;
					nextCell.m_iX = iEndX ;
					nextCell.m_iY = iEndY ;
					nextCell.m_iZ = iEndZ ;

					// update final target cell
					AddNewLightningPath( nextCell, true, true ) ;
				}
				else
				{
/*/
					// reset all negative charge cells to temp state
					std::vector< Cell >::iterator nItr = m_vNegativeCells.begin() ;
					while ( nItr != m_vNegativeCells.end() )
					{
						iIndex = nItr->m_iZ * ( m_iGridSize * m_iGridSize ) + nItr->m_iY * m_iGridSize + nItr->m_iX ;
						if ( m_vCells[ iIndex ] && E_CT_START == m_vCells[ iIndex ]->m_eType )
						{
							m_vCells[ iIndex ]->m_eType = E_CT_TEMP ;
						}

						++nItr ;
					}

					// add final target position
					nextCell.m_iParentX = nextCell.m_iX ;
					nextCell.m_iParentY = nextCell.m_iY ;
					nextCell.m_iParentZ = nextCell.m_iZ ;
					nextCell.m_iX = iEndX ;
					nextCell.m_iY = iEndY ;
					nextCell.m_iZ = iEndZ ;

					// update final target cell
					AddNewLightningPath( nextCell, false, true ) ;

					// initialize for negative charges
					InitNegativeElectricPotential() ;
					CreateMultiScaledClusterMap( m_iClusterGridSize ) ;
					
					// generate initial candidate map
					m_vNegativeCells.clear() ;
					m_vNegativeCells.push_back( nextCell ) ;

					CreateCandidateMap() ;

					// calculate electric potential for candidate cells
					CalcElectricPotential() ;

					// change target for the next target
					++m_iTargetIndex ;

					iIndex = m_vTargets[ m_iTargetIndex ].z * ( m_iGridSize * m_iGridSize ) + m_vTargets[ m_iTargetIndex ].y * m_iGridSize + m_vTargets[ m_iTargetIndex ].x ;
					if ( m_vCells[ iIndex ] && E_CT_TEMP == m_vCells[ iIndex ]->m_eType )
					{
						m_vCells[ iIndex ]->SetCellType( E_CT_END ) ;

						m_vPositiveCells.clear() ;
						m_vPositiveCells.push_back( *m_vCells[ iIndex ] ) ;

						CalcPositiveElectricPotential() ;
					}
				}
//*/

//*/					
					int iIndex ;
					
					// reset all negative charge cells to temp state
					std::vector< Cell >::iterator nItr = m_vNegativeCells.begin() ;
					while ( nItr != m_vNegativeCells.end() )
					{
						iIndex = nItr->m_iZ * ( m_iGridSize * m_iGridSize ) + nItr->m_iY * m_iGridSize + nItr->m_iX ;
						if ( m_vCells[ iIndex ] && E_CT_START == m_vCells[ iIndex ]->m_eType )
						{
							m_vCells[ iIndex ]->m_eType = E_CT_TEMP ;
						}

						++nItr ;
					}

					// reset all candidate cells to temp state
					std::map< int, Cell* >::iterator cItr = m_mapCandidates.begin() ;
					while ( cItr != m_mapCandidates.end() )
					{
						( cItr->second )->m_eType = E_CT_TEMP ;

						++cItr ;
					}

					m_vNegativeCells.clear() ;

					// add final target position
					nextCell.m_iParentX = nextCell.m_iX ;
					nextCell.m_iParentY = nextCell.m_iY ;
					nextCell.m_iParentZ = nextCell.m_iZ ;
					nextCell.m_iX = iEndX ;
					nextCell.m_iY = iEndY ;
					nextCell.m_iZ = iEndZ ;

					// update final target cell
					AddNewLightningPath( nextCell, false, true ) ;

					// initialize for negative charges
					InitNegativeElectricPotential() ;
					CreateMultiScaledClusterMap( m_iClusterGridSize ) ;

					// generate initial candidate map
					CreateCandidateMap() ;

					// calculate electric potential for candidate cells
					CalcElectricPotential() ;

					// change target for the next target
					++m_iTargetIndex ;

					iIndex = m_vTargets[ m_iTargetIndex ].z * ( m_iGridSize * m_iGridSize ) + m_vTargets[ m_iTargetIndex ].y * m_iGridSize + m_vTargets[ m_iTargetIndex ].x ;
					if ( m_vCells[ iIndex ] && E_CT_TEMP == m_vCells[ iIndex ]->m_eType )
					{
						m_vCells[ iIndex ]->SetCellType( E_CT_END ) ;

						m_vPositiveCells.clear() ;
						m_vPositiveCells.push_back( *m_vCells[ iIndex ] ) ;

						CalcPositiveElectricPotential() ;
					}
				}
//*/
			}
			else
			{
				AddNewLightningPath( nextCell ) ;

				// update multi scale map
				UpdateClusteredMap( nextCell ) ;

				// update candidate map
				UpdateCandidateMap( nextCell ) ;
			}
		}
	
		// ----------------------------------------------------------------

		if ( m_bProcessFinished )
		{
			ApplyIntensityAndThickness( GetBaseThickness(), GetIntensityAttenuation() ) ;
		}


		// time check per steps
		if ( !m_vTimeCheckSteps.empty() && m_iTimeIndex < m_vTimeCheckSteps.size() && m_iProcessIndex == m_vTimeCheckSteps[ m_iTimeIndex ] )
		{
			m_vTimePerSteps.push_back( glutGet( GLUT_ELAPSED_TIME ) ) ;

			++m_iTimeIndex ;
		}
	}

	m_iProcessIndex = 0 ;

	return true ;
}

float	LightningGenerator3D::Distance( float x1, float y1, float z1, float x2, float y2, float z2 ) const
{
	return sqrt( ( x2 - x1 ) * ( x2 - x1 ) + ( y2 - y1 ) * ( y2 - y1 ) + ( z2 - z1 ) * ( z2 - z1 ) ) ;
}

float	LightningGenerator3D::Distance2( float x1, float y1, float z1, float x2, float y2, float z2 ) const
{
	return ( ( x2 - x1 ) * ( x2 - x1 ) + ( y2 - y1 ) * ( y2 - y1 ) + ( z2 - z1 ) * ( z2 - z1 ) ) ;
}

bool	LightningGenerator3D::IsNearToEndCell( int x, int y, int z, int& outEndX, int& outEndY, int& outEndZ ) const
{
	bool bResult = false ;

	outEndX = -1 ;
	outEndY = -1 ;
	outEndZ = -1 ;

	if ( 0 <= x && x < m_iGridSize && 0 <= y && y < m_iGridSize && 0 <= z && z < m_iGridSize )
	{
		int cx, cy, cz ;
		int iNearCellIndex ;

		std::vector< int > vOutEndX ;
		std::vector< int > vOutEndY ;
		std::vector< int > vOutEndZ ;

		// check 26 direction for 3D map
		for ( int dir = 0; dir < 26; ++dir )
		{
			cx = x + Cell::DIR_WAY_X_DIFF[ dir ] ;
			cy = y + Cell::DIR_WAY_Y_DIFF[ dir ] ;
			cz = z + Cell::DIR_WAY_Z_DIFF[ dir ] ;
			
			iNearCellIndex = cz * m_iGridSize * m_iGridSize + cy * m_iGridSize + cx ;

			if ( cx >= 0 && cx < m_iGridSize && cy >= 0 && cy < m_iGridSize && cz >= 0 && cz < m_iGridSize && m_vCells[ iNearCellIndex ] )
			{
				if ( E_CT_END == m_vCells[ iNearCellIndex ]->m_eType )
				{
					bResult = true ;

					// add candidates of final position
					vOutEndX.push_back( cx ) ;
					vOutEndY.push_back( cy ) ;
					vOutEndZ.push_back( cz ) ;
				}
			}
		}

		if ( vOutEndX.size() > 0 )
		{
			int iCandidateIndex = 0 ;

			if ( vOutEndX.size() > 1 )
			{
				// choose candidate randomly
				iCandidateIndex = rand() % vOutEndX.size() ;
			}

			outEndX = vOutEndX[ iCandidateIndex ] ;
			outEndY = vOutEndY[ iCandidateIndex ] ;
			outEndZ = vOutEndZ[ iCandidateIndex ] ;
		}
	}

	return bResult ;
}

bool	LightningGenerator3D::IsNearToWaypoint( int x, int y, int z, int& outEndX, int& outEndY, int& outEndZ ) const
{
	bool bResult = false ;

	outEndX = -1 ;
	outEndY = -1 ;
	outEndZ = -1 ;

	if ( 0 <= x && x < m_iGridSize && 0 <= y && y < m_iGridSize && 0 <= z && z < m_iGridSize )
	{
		int cx, cy, cz ;
		int iNearCellIndex ;

		std::vector< int > vOutEndX ;
		std::vector< int > vOutEndY ;
		std::vector< int > vOutEndZ ;

		// check 26 direction for 3D map
		for ( int dir = 0; dir < 26; ++dir )
		{
			cx = x + Cell::DIR_WAY_X_DIFF[ dir ] ;
			cy = y + Cell::DIR_WAY_Y_DIFF[ dir ] ;
			cz = z + Cell::DIR_WAY_Z_DIFF[ dir ] ;
			
			iNearCellIndex = cz * m_iGridSize * m_iGridSize + cy * m_iGridSize + cx ;

			if ( cx >= 0 && cx < m_iGridSize && cy >= 0 && cy < m_iGridSize && cz >= 0 && cz < m_iGridSize && m_vCells[ iNearCellIndex ] )
			{
				if ( E_CT_WAY_POINT == m_vCells[ iNearCellIndex ]->m_eType )
				{
					bResult = true ;

					// add candidates of final position
					vOutEndX.push_back( cx ) ;
					vOutEndY.push_back( cy ) ;
					vOutEndZ.push_back( cz ) ;
				}
			}
		}

		if ( vOutEndX.size() > 0 )
		{
			int iCandidateIndex = 0 ;

			if ( vOutEndX.size() > 1 )
			{
				// choose candidate randomly
				iCandidateIndex = rand() % vOutEndX.size() ;
			}

			outEndX = vOutEndX[ iCandidateIndex ] ;
			outEndY = vOutEndY[ iCandidateIndex ] ;
			outEndZ = vOutEndZ[ iCandidateIndex ] ;
		}
	}

	return bResult ;
}

bool	LightningGenerator3D::ExistFile( const std::string& strPath ) const
{
	FILE* fp = fopen( strPath.c_str(), "r" ) ;
	if ( fp )
	{
		fclose( fp ) ;
		return true ;
	}

	return false ;
}

void	LightningGenerator3D::ClearLog()
{
	if ( false == m_strLogFile.empty() )
	{
		std::ofstream log( m_strLogFile.c_str() ) ;
		if ( log )
		{
			log.clear() ;
			log.close() ;
		}
	}
}


void	LightningGenerator3D::MakeAllCellsToCandidates()
{
	m_mapCandidates.clear() ;

	int iIndex ;

	for ( int i = 0; i < m_iGridSize; ++i )
	{
		for ( int j = 0; j < m_iGridSize; ++j )
		{
			for ( int k = 0; k < m_iGridSize; ++k )
			{
				iIndex = i * ( m_iGridSize * m_iGridSize ) + j * m_iGridSize + k ;

				if ( m_vCells[ iIndex ] && E_CT_EMPTY == m_vCells[ iIndex ]->m_eType )
				{
					m_mapCandidates.insert( std::map< int, Cell* >::value_type( iIndex, m_vCells[ iIndex ] ) ) ;
				}
			}
		}
	}
}

void	LightningGenerator3D::Normalize( float fMax )
{
	int iIndex ;

	// find current max value
	float fCurMax = 0 ;

	for ( int i = 0; i < m_iGridSize; ++i )
	{
		for ( int j = 0; j < m_iGridSize; ++j )
		{
			for ( int k = 0; k < m_iGridSize; ++k )
			{
				iIndex = i * ( m_iGridSize * m_iGridSize ) + j * m_iGridSize + k ;

				if ( m_vCells[ iIndex ] && E_CT_EMPTY == m_vCells[ iIndex ]->m_eType )
				{
					if ( m_vCells[ iIndex ]->m_fPotential > fCurMax )
					{
						fCurMax = m_vCells[ iIndex ]->m_fPotential ;
					}
				}
			}
		}
	}

	// normalized it to fMax
	float fRatio = fMax / fCurMax ;

	for ( int i = 0; i < m_iGridSize; ++i )
	{
		for ( int j = 0; j < m_iGridSize; ++j )
		{
			for ( int k = 0; k < m_iGridSize; ++k )
			{
				iIndex = i * ( m_iGridSize * m_iGridSize ) + j * m_iGridSize + k ;

				if ( m_vCells[ iIndex ] && E_CT_EMPTY == m_vCells[ iIndex ]->m_eType )
				{
					m_vCells[ iIndex ]->m_fPotential *= fRatio ;
				}
			}
		}
	}
}

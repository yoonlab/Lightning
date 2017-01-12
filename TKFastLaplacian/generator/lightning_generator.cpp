

#include "lightning_generator.h"
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

using namespace Lightning ;
using namespace std ;


#define DEFAULT_ETA						2
#define DEFAULT_H						10

#define DEFAULT_THICKNESS				1.0f
#define DEFAULT_INTENSITY_ATTENUATION	0.7f

//#define USE_TIME_CHECK


LightningGenerator::LightningGenerator()
	: m_iGridSize( 0 ), m_iClusterGridSize( 0 ), m_iEta( DEFAULT_ETA ), m_fH( DEFAULT_H ), m_fWeight( 1.0f )
	, m_eCompType( E_CT_CPU ), m_bProcessFinished( false ), m_iProcessIndex( 0 )
	, m_fBaseThickness( DEFAULT_THICKNESS ), m_fIntensityAttenuation( DEFAULT_INTENSITY_ATTENUATION )
	, m_strLogFile( "" ), m_bLogEnabled( false )
{
	m_vCells.clear() ;

	m_vStartCells.clear() ;
	m_vEndCells.clear() ;

	m_vBoundaryCells.clear() ;
	m_vObstacleCells.clear() ;
	m_vNegativeCells.clear() ;
	m_vPositiveCells.clear() ;

	m_mapCandidates.clear() ;
	m_vCandidates.clear() ;
	
	
	// random number generator
	std::random_device	rd ;
	m_randGen.seed( rd() ) ;

	srand( static_cast< unsigned >( time( 0 ) ) ) ;

	m_timeCalcBSum = 0 ;
	m_timeCalcNSum = 0 ;
	m_timeCalcPSum = 0 ;
	m_timeCalcTotalSum = 0 ;
	m_timeUpdateCandidateSum = 0 ;
	m_timeSelectCandidateSum = 0 ;


	m_iTimeInitStart = 0 ;
	m_iTimeInitEnd = 0 ;

	m_iTimeProcesStart = 0 ;
	m_iTimeIndex = 0 ;
	m_vTimeCheckSteps.clear() ;
	m_vTimePerSteps.clear() ;
}

LightningGenerator::LightningGenerator( int iEta )
	: m_iGridSize( 0 ), m_iClusterGridSize( 0 ), m_iEta( iEta ), m_fH( DEFAULT_H ), m_fWeight( 1.0f )
	, m_eCompType( E_CT_CPU ), m_bProcessFinished( false ), m_iProcessIndex( 0 )
	, m_fBaseThickness( DEFAULT_THICKNESS ), m_fIntensityAttenuation( DEFAULT_INTENSITY_ATTENUATION )
	, m_strLogFile( "" ), m_bLogEnabled( false )
{
	m_vCells.clear() ;

	m_vStartCells.clear() ;
	m_vEndCells.clear() ;

	m_vBoundaryCells.clear() ;
	m_vObstacleCells.clear() ;
	m_vNegativeCells.clear() ;
	m_vPositiveCells.clear() ;
	
	m_mapCandidates.clear() ;
	m_vCandidates.clear() ;
	
	
	// random number generator
	std::random_device	rd ;
	m_randGen.seed( rd() ) ;

	srand( static_cast< unsigned >( time( 0 ) ) ) ;

	m_timeCalcBSum = 0 ;
	m_timeCalcNSum = 0 ;
	m_timeCalcPSum = 0 ;
	m_timeCalcTotalSum = 0 ;
	m_timeUpdateCandidateSum = 0 ;
	m_timeSelectCandidateSum = 0 ;


	m_iTimeInitStart = 0 ;
	m_iTimeInitEnd = 0 ;

	m_iTimeProcesStart = 0 ;
	m_iTimeIndex = 0 ;
	m_vTimeCheckSteps.clear() ;
	m_vTimePerSteps.clear() ;
}

LightningGenerator::~LightningGenerator()
{
	Clear() ;
}

void	LightningGenerator::Clear()
{
	std::vector< Cell* >::iterator itr = m_vCells.begin() ;
	while ( itr != m_vCells.end() )
	{
		SAFE_DELETE( *itr ) ;
		++itr ;
	}

	m_vCells.clear() ;

	m_vStartCells.clear() ;
	m_vEndCells.clear() ;
	
	m_vBoundaryCells.clear() ;
	m_vObstacleCells.clear() ;
	m_vNegativeCells.clear() ;
	m_vPositiveCells.clear() ;

	m_mapCandidates.clear() ;
	m_vCandidates.clear() ;

	m_tree.Clear() ;

	m_bProcessFinished = false ;
	m_iProcessIndex = 0 ;

	m_bLogEnabled = false ;
}


bool	LightningGenerator::Load( const std::string& strPath )
{
	Clear() ;

	bool bRet = LoadMap( strPath ) ;
	if ( bRet )
	{
		// create boundary charge cells
//		CreateBoundaryCells() ;

		// generate initial candidate map
		CreateCandidateMap() ;

		// calculate electric potential at first step.
		CalcElectricPotential() ;
	}

	return bRet ;
}

bool	LightningGenerator::Load( int iGridSize, int sx, int sy, int tx, int ty )
{
	Clear() ;

	m_iGridSize = iGridSize ;

	bool bUsePositiveGround = false ;
	if ( -1 == tx && -1 == ty )
	{
		bUsePositiveGround = true ;
	}

	// create whole cells
	Cell* pCell ;

	for ( int y = 0; y < m_iGridSize; ++y )
	{
		for ( int x = 0; x < m_iGridSize; ++x )
		{
			pCell = new Cell( x, y, E_CT_EMPTY, 0 ) ;
			if ( pCell )
			{
				m_vCells.push_back( pCell ) ;

				if ( bUsePositiveGround && m_iGridSize - 1 == y )
				{
					pCell->SetCellType( E_CT_END ) ;

					m_vEndCells.push_back( *pCell ) ;
					m_vPositiveCells.push_back( *pCell ) ;
				}
			}
		}
	}

	// set start position
	int iIndex = sy * m_iGridSize + sx ;
	if ( m_vCells[ iIndex ] )
	{
		m_vCells[ iIndex ]->SetCellType( E_CT_START ) ;

		m_vStartCells.push_back( *m_vCells[ iIndex ] ) ;
		m_vNegativeCells.push_back( *m_vCells[ iIndex ] ) ;

		// initialize lightning tree
		InitTree() ;
	}

	// set goal position if it does not use default setting
	if ( false == bUsePositiveGround )
	{
		int iIndex = ty * m_iGridSize + tx ;
		if ( m_vCells[ iIndex ] )
		{
			m_vCells[ iIndex ]->SetCellType( E_CT_END ) ;

			m_vEndCells.push_back( *m_vCells[ iIndex ] ) ;
			m_vPositiveCells.push_back( *m_vCells[ iIndex ] ) ;
		}
	}

	std::cout << "Finish generating map !!" << std::endl ;

	// create boundary charge cells
	CreateBoundaryCells() ;



	m_iTimeInitStart = glutGet( GLUT_ELAPSED_TIME ) ;

	// generate initial candidate map
	CreateCandidateMap() ;

	// calculate electric potential at first step.
	CalcElectricPotential() ;

	m_iTimeInitEnd = glutGet( GLUT_ELAPSED_TIME ) ;

	std::cout << "Finish initialization !!" << std::endl ;

	return true ;
}

bool	LightningGenerator::LoadMap( const std::string& strPath )
{
	std::cout << "Load map file : " << strPath.c_str() << std::endl ;

	// read map file
	std::ifstream in( strPath.c_str(), std::ios::in ) ;
	if ( !in )
	{
		std::cerr << "Cannot open " << strPath.c_str() << std::endl ;
		return false ;
	}

	// parse map
	std::string strLine ;

	int iLine = 0 ;
	float fDefaultX = 0.5f ;
	
	Cell* pCell ;
	bool bMapStart = false ;
	int iCellType ;
	int iCellIndex = 0 ;
	int iCellX ;
	int iCellY ;

	while ( std::getline( in, strLine ) )
	{
		++iLine ;

		if ( '#' == strLine[ 0 ] )
		{
			// ignore comments
		}
		else if ( strLine.substr( 0, 8 ) == "VERSION:" )
		{
			// TODO : check version
			// ignore version
		}
		else if ( strLine.substr( 0, 10 ) == "GRID_SIZE:" )
		{
			std::istringstream pos( strLine.substr( 10 ) ) ;
			pos >> m_iGridSize ;
		}
		else if ( strLine.substr( 0, 20 ) == "CLUSTERED_GRID_SIZE:" )
		{
			std::istringstream pos( strLine.substr( 20 ) ) ;
			pos >> m_iClusterGridSize ;
		}
		else if ( strLine.substr( 0, 10 ) == "MAP_START:" )
		{
			bMapStart = true ;
		}
		else if ( strLine.substr( 0, 8 ) == "MAP_END:" )
		{
			bMapStart = false ;
		}
		else if ( strLine.substr( 0, 2 ) == "M:" )
		{
			if ( bMapStart )
			{
				std::istringstream pos( strLine.substr( 2 ) ) ;

				for ( int i = 0; i < m_iGridSize; ++i )
				{
					pos >> iCellType ;

					iCellX = iCellIndex % m_iGridSize ;
					iCellY = iCellIndex / m_iGridSize ;

					pCell = new Cell( iCellX, iCellY, E_CT_EMPTY, fDefaultX ) ;
					if ( pCell )
					{
						m_vCells.push_back( pCell ) ;

						switch ( iCellType )
						{
							case E_CT_START :
							{
								pCell->SetCellType( E_CT_START ) ;

								m_vStartCells.push_back( *pCell ) ;
								m_vNegativeCells.push_back( *pCell ) ;
							}
							break ;

							case E_CT_END :
							{
								pCell->SetCellType( E_CT_END ) ;

								m_vEndCells.push_back( *pCell ) ;
								m_vPositiveCells.push_back( *pCell ) ;
							}
							break ;

							case E_CT_OBSTACLE :
							{
								pCell->SetCellType( E_CT_OBSTACLE ) ;

								m_vObstacleCells.push_back( *pCell ) ;
							}
							break ;
						}

						++iCellIndex ;
					}
				}
			}
			else
			{
				std::cerr << "Map file is invalid !! (line: " << iLine << ", file: " << strPath.c_str() << ")" << std::endl ;
			}
		}
	}

	if ( bMapStart )
	{
		std::cerr << "MAP_END: is missed !! (line: " << iLine << ", file: " << strPath.c_str() << ")" << std::endl ;
	}

	// initialize lightning tree
	InitTree() ;

	std::cout << "Finish loading map file (" << strPath.c_str() << ") !!" << std::endl ;

	return true ;
}

void	LightningGenerator::InitTree()
{
	Cell nextCell ;
	bool bRoot = true ;

	std::vector< Cell >::iterator itr = m_vStartCells.begin() ;
	while ( itr != m_vStartCells.end() )
	{
		nextCell.m_iParentX = nextCell.m_iX ;
		nextCell.m_iParentY = nextCell.m_iY ;
		nextCell.m_iX = ( *itr ).m_iX ;
		nextCell.m_iY = ( *itr ).m_iY ;

		if ( bRoot )
		{
			bRoot = false ;
			
			// set root
			LightningTreeNode* pRoot = new LightningTreeNode() ;
			if ( pRoot )
			{
				pRoot->m_iX = nextCell.m_iX ;
				pRoot->m_iY = nextCell.m_iY ;
				pRoot->m_pParent = NULL ;

				m_tree.SetRoot( pRoot ) ;
			}
		}
		else
		{
			// add child
			m_tree.AddChild( nextCell.m_iParentX, nextCell.m_iParentY, nextCell.m_iX, nextCell.m_iY ) ;
		}

		++itr ;
	}
}

void	LightningGenerator::SetTimeCheckSteps( const std::vector< int >& vSteps )
{
	m_vTimeCheckSteps.clear() ;
	m_vTimePerSteps.clear() ;

	m_iTimeIndex = 0 ;
	m_iTimeProcesStart = 0 ;

	m_vTimeCheckSteps = vSteps ;
}

const std::vector< int > LightningGenerator::GetTimesPerSteps() const
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

CellType	LightningGenerator::GetCellType( int iIndex ) const
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

void	LightningGenerator::SetCellType( int iIndex, CellType eType )
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
				}
			}
		}
	}
}

bool	LightningGenerator::Process( ProcessType eType )
{
	return ProcessByCPU( eType ) ;
}

void	LightningGenerator::CreateBoundaryCells()
{
	m_vBoundaryCells.clear() ;
	m_vBoundaryCells.reserve( m_iGridSize * 4 + 4 ) ;

	Cell cell ;

	// add boundary charges
	for ( int i = 0; i < m_iGridSize; ++ i )
	{
		// horizontal
		cell.m_iX = i ;				cell.m_iY = -1 ;				m_vBoundaryCells.push_back( cell ) ;

		if ( 1 == m_vEndCells.size() )
		{
			cell.m_iX = i ;			cell.m_iY = m_iGridSize ;		m_vBoundaryCells.push_back( cell ) ;
		}

		// vertical
		cell.m_iX = -1 ;			cell.m_iY = i ;					m_vBoundaryCells.push_back( cell ) ;
		cell.m_iX = m_iGridSize ;	cell.m_iY = i ;					m_vBoundaryCells.push_back( cell ) ;
	}

	// 4 corner
	cell.m_iX = -1 ;			cell.m_iY = -1 ;					m_vBoundaryCells.push_back( cell ) ;
	cell.m_iX = m_iGridSize ;	cell.m_iY = -1 ;					m_vBoundaryCells.push_back( cell ) ;
	cell.m_iX = m_iGridSize ;	cell.m_iY = m_iGridSize ;			m_vBoundaryCells.push_back( cell ) ;
	cell.m_iX = -1 ;			cell.m_iY = m_iGridSize ;			m_vBoundaryCells.push_back( cell ) ;
}

void	LightningGenerator::CreateCandidateMap()
{
	m_mapCandidates.clear() ;

	int iIndex ;
	int x, y ;
	int cx, cy ;
	int iChildIndex ;

	std::vector< Cell >::iterator itr = m_vNegativeCells.begin() ;
	while ( itr != m_vNegativeCells.end() )
	{
		x = ( *itr ).m_iX ;
		y = ( *itr ).m_iY ;
		iIndex = y * m_iGridSize + x ;

		if ( m_vCells[ iIndex ] )
		{
			// check 8 direction
			for ( int dir = 0; dir < 8; ++dir )
			{
				cx = x + Cell::DIR_8_WAY_X_DIFF[ dir ] ;
				cy = y + Cell::DIR_8_WAY_Y_DIFF[ dir ] ;
				iChildIndex = cy * m_iGridSize + cx ;

				if ( cx >= 0 && cx < m_iGridSize && cy >= 0 && cy < m_iGridSize && m_vCells[ iChildIndex ] )
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
}

void	LightningGenerator::UpdateCandidateMap( const Cell& nextCell )
{
	int x = nextCell.m_iX ;
	int y = nextCell.m_iY ;
	int iIndex = y * m_iGridSize + x ;

	// remove cell from candidate map
	std::map< int, Cell* >::iterator itrCandidate = m_mapCandidates.find( iIndex ) ;
	if ( itrCandidate != m_mapCandidates.end() )
	{
		m_mapCandidates.erase( iIndex ) ;
	}

	// update electric potential for existing candidate cells
	float R = m_fH / 2.0f ;
	float r ;
	Cell* pCell ;

	std::map< int, Cell* >::iterator itr = m_mapCandidates.begin() ;
	while ( itr != m_mapCandidates.end() )
	{
		pCell = itr->second ;
		if ( pCell )
		{
			r = Distance( x * m_fH, y * m_fH, pCell->m_iX * m_fH, pCell->m_iY * m_fH ) ;

			pCell->m_fPotential = pCell->m_fLastPotential + ( 1 - R / r ) ;
			pCell->m_fLastPotential = pCell->m_fPotential ;
		}

		++itr ;
	}

	// add candidate cells for new lightning cell
	if ( m_vCells[ iIndex ] )
	{
		int cx, cy ;
		int iChildIndex ;

		// check 8 direction
		for ( int dir = 0; dir < 8; ++dir )
		{
			cx = x + Cell::DIR_8_WAY_X_DIFF[ dir ] ;
			cy = y + Cell::DIR_8_WAY_Y_DIFF[ dir ] ;
			iChildIndex = cy * m_iGridSize + cx ;

			if ( cx >= 0 && cx < m_iGridSize && cy >= 0 && cy < m_iGridSize && m_vCells[ iChildIndex ] )
			{
				if ( E_CT_EMPTY == m_vCells[ iChildIndex ]->m_eType )
				{
					std::map< int, Cell* >::iterator itrCandidate = m_mapCandidates.find( iChildIndex ) ;
					if ( itrCandidate == m_mapCandidates.end() )
					{
						m_mapCandidates.insert( std::map< int, Cell* >::value_type( iChildIndex, m_vCells[ iChildIndex ] ) ) ;

						// calculate electric potential for candidate cells which are newly added
						m_vCells[ iChildIndex ]->m_fLastPotential = CalcElectricPotential( m_vCells[ iChildIndex ]->m_iX, m_vCells[ iChildIndex ]->m_iY ) ;
						m_vCells[ iChildIndex ]->m_fPotential = m_vCells[ iChildIndex ]->m_fLastPotential ;
					}
				}
			}
		}
	}
}

bool	LightningGenerator::ProcessByCPU( ProcessType eType )
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

		if ( E_PT_ONE_STEP == eType )	// one step
		{
			bLoop = false ;
		}

//		int iIter = CalcElectricPotential() ;
		
		UpdateCandidate() ;

		Cell nextCell ;
		bool bRet = SelectCandidate( nextCell ) ;

		// ----------------------------------------------------------------
		// set next cell & update cells

		if ( bRet )
		{
			int iEndX, iEndY ;

			if ( IsNearToEndCell( nextCell.m_iX, nextCell.m_iY, iEndX, iEndY ) )
			{
				// end loop
				bLoop = false ;
				m_bProcessFinished = true ;

				AddNewLightningPath( nextCell ) ;

				// update candidate map
//				UpdateCandidateMap( nextCell ) ;


				// add final target position
				nextCell.m_iParentX = nextCell.m_iX ;
				nextCell.m_iParentY = nextCell.m_iY ;
				nextCell.m_iX = iEndX ;
				nextCell.m_iY = iEndY ;

				// update final target cell
				AddNewLightningPath( nextCell ) ;

				// update candidate map
//				UpdateCandidateMap( nextCell ) ;
			}
			else
			{
				AddNewLightningPath( nextCell ) ;

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

int		LightningGenerator::CalcElectricPotential()
{
	int iTotalCellNum = m_iGridSize * m_iGridSize ;
	if ( m_vCells.size() < iTotalCellNum )
	{
		std::cerr << std::endl ;
		std::cerr << "cell size error !! " << std::endl ;

		return 0 ;
	}

	// calculate electric field for only candidate cells
	int iCalc = 0 ;

	float fPotential ;
	float R = m_fH / 2.0f ;
	float r ;
	
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
			fPotential = 0 ;

			// -----------------------------------------------------------
			// for positive charges
//*/
			std::vector< Cell >::const_iterator pItr = m_vPositiveCells.begin() ;
			while ( pItr != m_vPositiveCells.end() )
			{
				r = Distance( pItr->m_iX * m_fH, pItr->m_iY * m_fH, pCell->m_iX * m_fH, pCell->m_iY * m_fH ) ;

//				fPotential += ( 1 - R / r ) ;
				fPotential += ( R / r ) * m_fWeight ;

				++pItr ;
			}
//*/

			// -----------------------------------------------------------
			// for negative charges
			std::vector< Cell >::const_iterator nItr = m_vNegativeCells.begin() ;
			while ( nItr != m_vNegativeCells.end() )
			{
				r = Distance( nItr->m_iX * m_fH, nItr->m_iY * m_fH, pCell->m_iX * m_fH, pCell->m_iY * m_fH ) ;

				fPotential += ( 1 - R / r ) ;

				++nItr ;
			}

			// -----------------------------------------------------------
			// for boundary negative charges
//*/
			std::vector< Cell >::const_iterator cItr = m_vBoundaryCells.begin() ;
			while ( cItr != m_vBoundaryCells.end() )
			{
				r = Distance( cItr->m_iX * m_fH, cItr->m_iY * m_fH, pCell->m_iX * m_fH, pCell->m_iY * m_fH ) ;

				fPotential += ( 1 - R / r ) ;

				++cItr ;
			}
//*/
			
			pCell->m_fLastPotential = fPotential ;
			pCell->m_fPotential = fPotential ;
		}

		++mapItr ;
	}

	return iCalc ;
}

float	LightningGenerator::CalcElectricPotential( int gx, int gy )
{
	float fPotential = 0 ;
	float R = m_fH / 2.0f ;
	float r ;

	// -----------------------------------------------------------
	// for positive charges
//*/
	std::vector< Cell >::const_iterator pItr = m_vPositiveCells.begin() ;
	while ( pItr != m_vPositiveCells.end() )
	{
		r = Distance( pItr->m_iX * m_fH, pItr->m_iY * m_fH, gx * m_fH, gy * m_fH ) ;

//		fPotential += ( 1 - R / r ) ;
		fPotential += ( R / r ) * m_fWeight ;

		++pItr ;
	}
//*/

	// -----------------------------------------------------------
	// for negative charges
	std::vector< Cell >::const_iterator nItr = m_vNegativeCells.begin() ;
	while ( nItr != m_vNegativeCells.end() )
	{
		r = Distance( nItr->m_iX * m_fH, nItr->m_iY * m_fH, gx * m_fH, gy * m_fH ) ;

		fPotential += ( 1 - R / r ) ;

		++nItr ;
	}

	// -----------------------------------------------------------
	// for boundary negative charges
//*/
	std::vector< Cell >::const_iterator cItr = m_vBoundaryCells.begin() ;
	while ( cItr != m_vBoundaryCells.end() )
	{
		r = Distance( cItr->m_iX * m_fH, cItr->m_iY * m_fH, gx * m_fH, gy * m_fH ) ;

		fPotential += ( 1 - R / r ) ;

		++cItr ;
	}
//*/
			
	return fPotential ;
}

void	LightningGenerator::UpdateCandidate()
{
	// add candidates
	m_vCandidates.clear() ;

	int iIndex ;

	int px, py ;
	int cx, cy ;
	int iAdjIndex ;

	float fMin = 99999 ;
	float fMax = -99999 ;

	std::vector< Cell >::const_iterator itr = m_vNegativeCells.begin() ;
	while ( itr != m_vNegativeCells.end() )
	{
		px = ( *itr ).m_iX ;
		py = ( *itr ).m_iY ;
		iIndex = py * m_iGridSize + px ;

		if ( m_vCells[ iIndex ] )
		{
			// check 8 direction
			for ( int dir = 0; dir < 8; ++dir )
			{
				cx = px + Cell::DIR_8_WAY_X_DIFF[ dir ] ;
				cy = py + Cell::DIR_8_WAY_Y_DIFF[ dir ] ;
				iAdjIndex = cy * m_iGridSize + cx ;

				if ( cx >= 0 && cx < m_iGridSize && cy >= 0 && cy < m_iGridSize && m_vCells[ iAdjIndex ] )
				{
					if ( E_CT_EMPTY == m_vCells[ iAdjIndex ]->m_eType )
					{
						if ( 0 != m_vCells[ iAdjIndex ]->m_fPotential )	// remove 0 potential cells
						{
							Cell candi ;
							candi.m_iX = cx ;
							candi.m_iY = cy ;
							candi.m_iParentX = px ;
							candi.m_iParentY = py ;
							candi.m_fProb = m_vCells[ iAdjIndex ]->m_fPotential ;

							m_vCandidates.push_back( candi ) ;

							if ( candi.m_fProb > fMax )
							{
								fMax = candi.m_fProb ;
							}
							else if ( candi.m_fProb < fMin )
							{
								fMin = candi.m_fProb ;
							}
						}
					}
				}
			}
		}

		++itr ;
	}

	// normalization
	std::vector< Cell >::iterator cItr = m_vCandidates.begin() ;
	while ( cItr != m_vCandidates.end() )
	{
		( *cItr ).m_fProb = ( ( *cItr ).m_fProb - fMin ) / ( fMax - fMin ) ;

		++cItr ;
	}
}

bool	LightningGenerator::SelectCandidate( Cell& outNextCell )
{
	outNextCell.m_iX = -1 ;
	outNextCell.m_iY = -1 ;

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

void	LightningGenerator::AddNewLightningPath( const Cell& newPath )
{
	int iIndex = newPath.m_iY * m_iGridSize + newPath.m_iX ;

	// update cell type
	SetCellType( iIndex, E_CT_START ) ;

	// add lightning node to tree
	m_tree.AddChild( newPath.m_iParentX, newPath.m_iParentY, newPath.m_iX, newPath.m_iY ) ;
}

float	LightningGenerator::Distance( float x1, float y1, float x2, float y2 ) const
{
	return sqrt( ( x2 - x1 ) * ( x2 - x1 ) + ( y2 - y1 ) * ( y2 - y1 ) ) ;
}

float	LightningGenerator::Distance2( float x1, float y1, float x2, float y2 ) const
{
	return ( ( x2 - x1 ) * ( x2 - x1 ) + ( y2 - y1 ) * ( y2 - y1 ) ) ;
}

void	LightningGenerator::ApplyIntensityAndThickness( float fBaseThickness, float fIntensityAttenuation )
{
	m_tree.ApplyIntensityAndThickness( fBaseThickness, fIntensityAttenuation ) ;
}

bool	LightningGenerator::IsNearToEndCell( int x, int y, int& outEndX, int& outEndY ) const
{
	bool bResult = false ;

	outEndX = -1 ;
	outEndY = -1 ;

	if ( 0 <= x && x < m_iGridSize && 0 <= y && y < m_iGridSize )
	{
		int cx, cy ;
		int iNearCellIndex ;

		std::vector< int > vOutEndX ;
		std::vector< int > vOutEndY ;

		// check 8 direction for 2D map
		for ( int dir = 0; dir < 8; ++dir )
		{
			cx = x + Cell::DIR_8_WAY_X_DIFF[ dir ] ;
			cy = y + Cell::DIR_8_WAY_Y_DIFF[ dir ] ;
			
			iNearCellIndex = cy * m_iGridSize + cx ;

			if ( cx >= 0 && cx < m_iGridSize && cy >= 0 && cy < m_iGridSize && m_vCells[ iNearCellIndex ] )
			{
				if ( E_CT_END == m_vCells[ iNearCellIndex ]->m_eType )
				{
					bResult = true ;

					// add candidates of final position
					vOutEndX.push_back( cx ) ;
					vOutEndY.push_back( cy ) ;
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
		}
	}

	return bResult ;
}

bool	LightningGenerator::ExistFile( const std::string& strPath ) const
{
	FILE* fp = fopen( strPath.c_str(), "r" ) ;
	if ( fp )
	{
		fclose( fp ) ;
		return true ;
	}

	return false ;
}

void	LightningGenerator::ClearLog()
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


bool	LightningGenerator::WriteValue( const std::string& strPath )
{
	std::cout << "Write cell values to file : " << strPath.c_str() << std::endl ;

	// write cell values to file
	std::ofstream out( strPath.c_str() ) ;
	if ( !out )
	{
		std::cerr << "Cannot open " << strPath.c_str() << std::endl ;
		return false ;
	}

	int iIndex ;

	// write 
	for ( int y = 0; y < m_iGridSize; ++y )
	{
		for ( int x = 0; x < m_iGridSize; ++x )
		{
			iIndex = y * m_iGridSize + x ;
			
			if ( m_vCells[ iIndex ] )
			{
				if ( E_CT_OBSTACLE == m_vCells[ iIndex ]->m_eType )
				{
					//out << float( E_CT_OBSTACLE ) ;
					out << 1.0f ;
				}
				else
				{
					// TODO : why negative value?
					if ( m_vCells[ iIndex ]->m_fPotential < 0 )
					{
						out << -m_vCells[ iIndex ]->m_fPotential ;
					}
					else
					{
						out << m_vCells[ iIndex ]->m_fPotential ;
					}
				}
				
				if ( x != m_iGridSize - 1 )
				{
					out << "," ;
				}
			}
		}

		out << std::endl ;
	}
	out << std::endl ;
	
	out.close() ;

	std::cout << "Finish writing cell values to file !!" << std::endl ;

	return true ;
}

bool	LightningGenerator::WriteMap( const std::string& strPath )
{
	std::cout << "Write map file : " << strPath.c_str() << std::endl ;

	// write map file
	std::ofstream out( strPath.c_str() ) ;
	if ( !out )
	{
		std::cerr << "Cannot open " << strPath.c_str() << std::endl ;
		return false ;
	}

	// blank line
	out << std::endl ;

	// version
	out << "VERSION: " << "1.0" << std::endl ;

	// blank line
	out << std::endl ;

	// grid size
	out << "GRID_SIZE: " << m_iGridSize << std::endl ;

	// clustered grid size
	out << "CLUSTERED_GRID_SIZE: " << m_iClusterGridSize << std::endl ;

	// blank line
	out << std::endl ;

	// default X
	out << "DEFAULT_X: " << "0.5" << std::endl ;

	// blank line
	out << std::endl ;

	// map data
	out << "MAP_START:" << std::endl ;

	// ruler (for tenth digit)
	out << "#\t" ;
	int iDecimalCount = m_iGridSize / 10 ;
	for ( int i = 0; i < iDecimalCount; ++i )
	{
		for ( int j = 0; j < 10; ++j )
		{
			out << "\t" ;
		}

		out << i + 1 ;
	}
	out << std::endl ;

	// ruler (for units digit)
	out << "#" ;
	for ( int i = 0; i < m_iGridSize; ++i )
	{
		out << "\t" << i % 10 ;
	}
	out << std::endl ;
	out << std::endl ;

	// map row data
	int iCellIndex ;
	for ( int i = 0; i < m_iGridSize; ++i )
	{
		out << "M:\t" ;

		for ( int j = 0; j < m_iGridSize; ++j )
		{
			iCellIndex = i * m_iGridSize + j ;
			if ( m_vCells[ iCellIndex ] )
			{
				switch ( m_vCells[ iCellIndex ]->m_eType )
				{
					case E_CT_EMPTY :		out << "0\t" ;	break ;
					case E_CT_START :		out << "1\t" ;	break ;
					case E_CT_END :			out << "2\t" ;	break ;
					case E_CT_OBSTACLE :	out << "3\t" ;	break ;
				}
			}
		}

		if ( i < 10 )			out << "#   " << i << std::endl ;
		else if ( i < 100 )		out << "#  " << i << std::endl ;
		else					out << "# " << i << std::endl ;
	}

	out << "MAP_END:" << std::endl ;

	std::cout << "Finish writing map file !!" << std::endl ;

	return true ;
}


void	GetNodeSegments( const LightningTreeNode* pNode, std::vector< Cell >& vSegments )
{
	if ( pNode && pNode->m_pParent )
	{
		Cell seg ;
		
		seg.m_iParentX = pNode->m_pParent->m_iX ;
		seg.m_iParentY = pNode->m_pParent->m_iY ;
		seg.m_iX = pNode->m_iX ;
		seg.m_iY = pNode->m_iY ;

		vSegments.push_back( seg ) ;

		int iChildCount = pNode->m_vChildren.size() ;
		for ( int i = 0; i < iChildCount; ++i )
		{
			GetNodeSegments( pNode->m_vChildren[ i ], vSegments ) ;
		}
	}
}

float	LightningGenerator::CalculateFractalDimension()
{
	// assume that total grid map size is 200 x 200 pixel

	// extract nodes from tree
	std::vector< Cell > vSegments ;

	const LightningTreeNode* pNode = m_tree.GetRoot() ;
	if ( pNode )
	{
		int iChildCount = pNode->m_vChildren.size() ;
		for ( int i = 0; i < iChildCount; ++i )
		{
			GetNodeSegments( pNode->m_vChildren[ i ], vSegments ) ;
		}
	}

	std::map< int, int > mapCells ;
	std::map< int, int >::iterator itr ;

	Cell seg ;
	int iIndex ;

	// calculate the number of cells that occupied by segments
	// actually, it is same to the number of nodes in lightning tree
	int iSegCount = vSegments.size() ;
	
	for ( int i = 0; i < iSegCount; ++i )
	{
		seg = vSegments[ i ] ;

		// start point
		iIndex = seg.m_iParentY * m_iGridSize + seg.m_iParentX ;
		itr = mapCells.find( iIndex ) ;
		if ( itr == mapCells.end() )
		{
			mapCells.insert( std::map< int, int >::value_type( iIndex, 1 ) ) ;
		}

		// end point
		iIndex = seg.m_iY * m_iGridSize + seg.m_iX ;
		itr = mapCells.find( iIndex ) ;
		if ( itr == mapCells.end() )
		{
			mapCells.insert( std::map< int, int >::value_type( iIndex, 1 ) ) ;
		}
	}

	int iBaseScaleCellCount = mapCells.size() ;

	// scale segment position
	for ( int i = 0; i < iSegCount; ++i )
	{
		vSegments[ i ].m_iParentX *= 2 ;
		vSegments[ i ].m_iParentY *= 2 ;

		vSegments[ i ].m_iX *= 2 ;
		vSegments[ i ].m_iY *= 2 ;
	}

	// calculate the number of cells that occupied by segments on 2 time scaled grid map
	int iScaledGridSize = m_iGridSize * 2 ;

	for ( int i = 0; i < iSegCount; ++i )
	{
		seg = vSegments[ i ] ;

		// start point
		iIndex = seg.m_iParentY * iScaledGridSize + seg.m_iParentX ;
		itr = mapCells.find( iIndex ) ;
		if ( itr == mapCells.end() )
		{
			mapCells.insert( std::map< int, int >::value_type( iIndex, 1 ) ) ;
		}

		// end point
		iIndex = seg.m_iY * iScaledGridSize + seg.m_iX ;
		itr = mapCells.find( iIndex ) ;
		if ( itr == mapCells.end() )
		{
			mapCells.insert( std::map< int, int >::value_type( iIndex, 1 ) ) ;
		}

		// middle point cell
		int iDiffX = seg.m_iParentX - seg.m_iX ;
		int iDiffY = seg.m_iParentY - seg.m_iY ;

		// iDirIndex start from 12 clock with clock wise direction
		int iDirIndex ;

		if ( iDiffX < 0 )
		{
			if ( iDiffY < 0 )		iDirIndex = 7 ;
			else if ( iDiffY > 0 )	iDirIndex = 5 ;
			else					iDirIndex = 6 ;
		}
		else if ( iDiffX > 0 )
		{
			if ( iDiffY < 0 )		iDirIndex = 1 ;
			else if ( iDiffY > 0 )	iDirIndex = 3 ;
			else					iDirIndex = 2 ;
		}
		else
		{
			if ( iDiffY < 0 )		iDirIndex = 0 ;
			else if ( iDiffY > 0 )	iDirIndex = 4 ;
		}

		int cx = seg.m_iParentX + Cell::DIR_8_WAY_X_DIFF[ iDirIndex ] ;
		int cy = seg.m_iParentY + Cell::DIR_8_WAY_Y_DIFF[ iDirIndex ] ;

		iIndex = cy * iScaledGridSize + cx ;
		itr = mapCells.find( iIndex ) ;
		if ( itr == mapCells.end() )
		{
			mapCells.insert( std::map< int, int >::value_type( iIndex, 1 ) ) ;
		}
	}

	int iScaleCellCount = mapCells.size() ;

	// calculate fractal dimension
	// d = log( new created count ) / log( Scale )
	float N = (float)iScaleCellCount / (float)iBaseScaleCellCount ;
	float S = 2.0f ;
	float D = log( N ) / log( S ) ;

	return D ;
}

void	LightningGenerator::PrintValue()
{
	std::string strMsg ;
	float fValue ;

	std::ofstream log ;
	if ( m_bLogEnabled && false == m_strLogFile.empty() )
	{
		log.open( m_strLogFile.c_str(), ios::app ) ;

		log.setf( ios::fixed ) ;
		log.precision( 3 ) ;
	}

	strMsg = " -- potential values " ;
	
	std::cout << std::endl ;
	std::cout << strMsg << std::endl ;

	if ( log )
	{
		log << std::endl ;
		log << strMsg << std::endl ;
	}

	int iIndex = 0 ;

	std::cout.setf( ios::fixed ) ;
	std::cout.precision( 3 ) ;

	for ( int i = 0; i < m_iGridSize; ++i )
	{
		for ( int j = 0; j < m_iGridSize; ++j )
		{
			iIndex = i * m_iGridSize + j ;

			if ( m_vCells[ iIndex ] )
			{
				fValue = m_vCells[ iIndex ]->m_fPotential ;

				// TODO : for debugging
				if ( E_CT_START == m_vCells[ iIndex ]->m_eType ) fValue = 0 ;

				std::cout << "  " << fValue << "," ;

				if ( log )
				{
					log << "  " << fValue << "," ;
				}
			}
		}

		std::cout << std::endl ;

		if ( log )
		{
			log << std::endl ;
		}
	}

	if ( log )
	{
		log.close() ;
	}
}



void	LightningGenerator::MakeAllCellsToCandidates()
{
	m_mapCandidates.clear() ;

	int iIndex ;

	for ( int i = 0; i < m_iGridSize; ++i )
	{
		for ( int j = 0; j < m_iGridSize; ++j )
		{
			iIndex = i * m_iGridSize + j ;

			if ( m_vCells[ iIndex ] && E_CT_EMPTY == m_vCells[ iIndex ]->m_eType )
			{
				m_mapCandidates.insert( std::map< int, Cell* >::value_type( iIndex, m_vCells[ iIndex ] ) ) ;
			}
		}
	}
}

void	LightningGenerator::Normalize( float fMax )
{
	int iIndex ;

	// find current max value
	float fCurMax = 0 ;

	for ( int i = 0; i < m_iGridSize; ++i )
	{
		for ( int j = 0; j < m_iGridSize; ++j )
		{
			iIndex = i * m_iGridSize + j ;

			if ( m_vCells[ iIndex ] && E_CT_EMPTY == m_vCells[ iIndex ]->m_eType )
			{
				if ( m_vCells[ iIndex ]->m_fPotential > fCurMax )
				{
					fCurMax = m_vCells[ iIndex ]->m_fPotential ;
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
			iIndex = i * m_iGridSize + j ;

			if ( m_vCells[ iIndex ] && E_CT_EMPTY == m_vCells[ iIndex ]->m_eType )
			{
				m_vCells[ iIndex ]->m_fPotential *= fRatio ;
			}
		}
	}
}
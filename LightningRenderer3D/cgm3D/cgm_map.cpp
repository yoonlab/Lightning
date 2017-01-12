

#include "cgm_map.h"

#include <iostream>
#include <ios>
#include <fstream>
#include <sstream>

#include <ctime>

#include <map>

using namespace Cgm3D ;
using namespace std ;


// =========================================================================================================
// CellMap

CellMap::CellMap()
	: m_iGridSize( 0 ), m_iEta( DEFAULT_ETA ), m_iProcessIndex( 0 ), m_bProcessFinished( false ), m_strLogFile( "" ), m_bLogEnabled( false )
{
	m_vCells.clear() ;
	
	m_vBoundaryCells.clear() ;
	m_vNegativeCells.clear() ;
	m_vPositiveCells.clear() ;
	
	m_vStartCells.clear() ;
	m_vEndCells.clear() ;

	m_vCandidates.clear() ;

	m_fBaseThickness = 1.0f ;
	m_fIntensityAttenuation = 0.7f ;

	// random number generator
	std::random_device	rd ;
	m_randGen.seed( rd() ) ;

	srand( static_cast< unsigned >( time( 0 ) ) ) ;
}

CellMap::CellMap( int iEta )
	: m_iGridSize( 0 ), m_iEta( iEta ), m_iProcessIndex( 0 ), m_bProcessFinished( false ), m_strLogFile( "" ), m_bLogEnabled( false )
{
	m_vCells.clear() ;
	
	m_vBoundaryCells.clear() ;
	m_vNegativeCells.clear() ;
	m_vPositiveCells.clear() ;
	
	m_vStartCells.clear() ;
	m_vEndCells.clear() ;

	m_vCandidates.clear() ;

	// random number generator
	std::random_device	rd ;
	m_randGen.seed( rd() ) ;

	m_fBaseThickness = 1.0f ;
	m_fIntensityAttenuation = 0.7f ;

	srand( static_cast< unsigned >( time( 0 ) ) ) ;
}

CellMap::~CellMap()
{
	Clear() ;
}

void	CellMap::Clear()
{
	std::vector< Cell* >::iterator itr = m_vCells.begin() ;
	while ( itr != m_vCells.end() )
	{
		SAFE_DELETE( *itr ) ;
		++itr ;
	}

	m_vCells.clear() ;
	
	m_vBoundaryCells.clear() ;
	m_vNegativeCells.clear() ;
	m_vPositiveCells.clear() ;
	
	m_vStartCells.clear() ;
	m_vEndCells.clear() ;

	m_vCandidates.clear() ;
	m_tree.Clear() ;

	m_bProcessFinished = false ;
}

bool	CellMap::Load( int iGridSize, int sx, int sy, int sz, int ex, int ey, int ez )
{
	std::cout << "Load map (Grid size : " << iGridSize << ") !!" << std::endl ;

	Clear() ;

	m_iGridSize = iGridSize ;

	bool bUsePositiveGround = false ;
	if ( -1 == ex && -1 == ey && -1 == ez )
	{
		bUsePositiveGround = true ;
	}

	// create whole cells
	Cell* pCell ;
	SimpleCell simpleCell ;

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

					simpleCell.m_iX = x ;
					simpleCell.m_iY = y ;
					simpleCell.m_iZ = z ;

					if ( bUsePositiveGround && m_iGridSize - 1 == y )
					{
						pCell->SetCellType( E_CT_END ) ;

						m_vEndCells.push_back( simpleCell ) ;
						m_vPositiveCells.push_back( simpleCell ) ;
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

		simpleCell.m_iX = sx ;
		simpleCell.m_iY = sy ;
		simpleCell.m_iZ = sz ;

		m_vStartCells.push_back( simpleCell ) ;
		m_vNegativeCells.push_back( simpleCell ) ;

		// initialize lightning tree
		InitTree() ;
	}

	// set goal position if it does not use default setting
	if ( false == bUsePositiveGround )
	{
		int iIndex = ez * ( m_iGridSize * m_iGridSize ) + ey * m_iGridSize + ex ;
		if ( m_vCells[ iIndex ] )
		{
			m_vCells[ iIndex ]->SetCellType( E_CT_END ) ;

			simpleCell.m_iX = ex ;
			simpleCell.m_iY = ey ;
			simpleCell.m_iZ = ez ;

			m_vEndCells.push_back( simpleCell ) ;
			m_vPositiveCells.push_back( simpleCell ) ;
		}
	}

	// initialize lightning tree
	InitTree() ;

	std::cout << "Finish generating map !!" << std::endl ;
	
	return true ;
}

CellType	CellMap::GetCellType( int iIndex ) const
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

void	CellMap::SetCellType( int iIndex, CellType eType )
{
	if ( 0 <= iIndex && iIndex < (int)m_vCells.size() )
	{
		if ( m_vCells[ iIndex ] )
		{
			CellType eOldType = m_vCells[ iIndex ]->m_eType ;

			m_vCells[ iIndex ]->SetCellType( eType ) ;

			if ( E_CT_EMPTY != eOldType )
			{
				// remove cell from each type cell list

				switch ( eOldType )
				{
					case E_CT_START :
					{
						std::vector< SimpleCell >::iterator itr = m_vNegativeCells.begin() ;
						while ( itr != m_vNegativeCells.end() )
						{
							if ( (*itr).m_iX == m_vCells[ iIndex ]->m_iX && (*itr).m_iY == m_vCells[ iIndex ]->m_iY && (*itr).m_iZ == m_vCells[ iIndex ]->m_iZ )
							{
								m_vNegativeCells.erase( itr ) ;

								break ;
							}

							++itr ;
						}
					}
					break ;

					case E_CT_END :
					{
						std::vector< SimpleCell >::iterator itr = m_vPositiveCells.begin() ;
						while ( itr != m_vPositiveCells.end() )
						{
							if ( (*itr).m_iX == m_vCells[ iIndex ]->m_iX && (*itr).m_iY == m_vCells[ iIndex ]->m_iY && (*itr).m_iZ == m_vCells[ iIndex ]->m_iZ )
							{
								m_vPositiveCells.erase( itr ) ;

								break ;
							}

							++itr ;
						}
					}
					break ;

					case E_CT_OBSTACLE :
					{
						std::vector< SimpleCell >::iterator itr = m_vBoundaryCells.begin() ;
						while ( itr != m_vBoundaryCells.end() )
						{
							if ( (*itr).m_iX == m_vCells[ iIndex ]->m_iX && (*itr).m_iY == m_vCells[ iIndex ]->m_iY && (*itr).m_iZ == m_vCells[ iIndex ]->m_iZ )
							{
								m_vBoundaryCells.erase( itr ) ;

								break ;
							}

							++itr ;
						}
					}
					break ;
				}
			}

			if ( E_CT_EMPTY != eType )
			{
				// add cell to each type cell list
				SimpleCell cell ;
				cell.m_iX = m_vCells[ iIndex ]->m_iX ;
				cell.m_iY = m_vCells[ iIndex ]->m_iY ;
				cell.m_iZ = m_vCells[ iIndex ]->m_iZ ;
				
				switch ( eType )
				{
					case E_CT_START :		m_vNegativeCells.push_back( cell ) ;		break ;
					case E_CT_END :			m_vPositiveCells.push_back( cell ) ;		break ;
					case E_CT_OBSTACLE :	m_vBoundaryCells.push_back( cell ) ;		break ;
				}
			}
		}
	}
}

const SimpleCell*	CellMap::GetStartCell( int iIndex ) const
{
	if ( 0 <= iIndex && iIndex < (int)m_vStartCells.size() )
	{
		return &m_vStartCells[ iIndex ] ;
	}

	return NULL ;
}

const SimpleCell*	CellMap::GetEndCell( int iIndex ) const
{
	if ( 0 <= iIndex && iIndex < (int)m_vEndCells.size() )
	{
		return &m_vEndCells[ iIndex ] ;
	}

	return NULL ;
}

void	CellMap::InitTree()
{
	SimpleCell nextCell ;
	bool bRoot = true ;

	std::vector< SimpleCell >::iterator itr = m_vStartCells.begin() ;
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

	m_nextCell = nextCell ;
}

void	CellMap::UpdateCandidate()
{
	// add next cell to negative cells
	//m_vNegativeCells.push_back( m_nextCell ) ;	// it is operated by SetCellType() function call.

	// add candidates
	m_vCandidates.clear() ;

	int iIndex ;

	int px, py, pz ;
	int cx, cy, cz ;
	int iAdjIndex ;
	float fPotentialSum = 0.0f ;

	std::vector< SimpleCell >::const_iterator itr = m_vNegativeCells.begin() ;
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
				cx = px + SimpleCell::DIR_WAY_X_DIFF[ dir ] ;
				cy = py + SimpleCell::DIR_WAY_Y_DIFF[ dir ] ;
				cz = pz + SimpleCell::DIR_WAY_Z_DIFF[ dir ] ;
				iAdjIndex = cz * ( m_iGridSize * m_iGridSize ) + cy * m_iGridSize + cx ;

				if ( cx >= 0 && cx < m_iGridSize && cy >= 0 && cy < m_iGridSize && cz >= 0 && cz < m_iGridSize && m_vCells[ iAdjIndex ] )
				{
/*/
					if ( E_CT_EMPTY == m_vCells[ iAdjIndex ]->m_eType || E_CT_END == m_vCells[ iAdjIndex ]->m_eType )
					{
						SimpleCell candi ;
						candi.m_iX = cx ;
						candi.m_iY = cy ;
						candi.m_iZ = cz ;
						candi.m_iParentX = px ;
						candi.m_iParentY = py ;
						candi.m_iParentZ = pz ;
						candi.m_fProb = m_vCells[ iAdjIndex ]->m_fPotential ;

						m_vCandidates.push_back( candi ) ;

						fPotentialSum += candi.m_fProb ;
					}
//*/
//*/
					if ( E_CT_EMPTY == m_vCells[ iAdjIndex ]->m_eType )
					{
						SimpleCell candi ;
						candi.m_iX = cx ;
						candi.m_iY = cy ;
						candi.m_iZ = cz ;
						candi.m_iParentX = px ;
						candi.m_iParentY = py ;
						candi.m_iParentZ = pz ;
						candi.m_fProb = m_vCells[ iAdjIndex ]->m_fPotential ;

						m_vCandidates.push_back( candi ) ;

						fPotentialSum += candi.m_fProb ;
					}
					else if ( E_CT_END == m_vCells[ iAdjIndex ]->m_eType )
					{
						m_vCandidates.clear() ;

						SimpleCell candi ;
						candi.m_iX = cx ;
						candi.m_iY = cy ;
						candi.m_iZ = cz ;
						candi.m_iParentX = px ;
						candi.m_iParentY = py ;
						candi.m_iParentZ = pz ;
						candi.m_fProb = m_vCells[ iAdjIndex ]->m_fPotential ;

						m_vCandidates.push_back( candi ) ;

						fPotentialSum += candi.m_fProb ;

						break ;
					}
//*/
				}
			}
		}

		++itr ;
	}

/*/	// it will be done by std::discret_distribution
	// calculate probability for sub-candidates
	if ( fPotentialSum != 0.0f )
	{
		std::vector< SimpleCell >::iterator subItr = m_vCandidates.begin() ;
		while ( subItr != m_vCandidates.end() )
		{
			( *subItr ).m_fProb /= fPotentialSum ;
			++subItr ;
		}
	}
//*/
}

bool	CellMap::IsNearEndCell( int iIndex ) const
{
	if ( 0 <= iIndex && iIndex < m_iGridSize * m_iGridSize * m_iGridSize )
	{
		int x, y, z, cx, cy, cz ;
		int iXYRemain ;
		int iNearCellIndex ;

		if ( m_vCells[ iIndex ] )
		{
			z = iIndex / ( m_iGridSize * m_iGridSize ) ;
			iXYRemain = iIndex % ( m_iGridSize * m_iGridSize ) ;
			x = iXYRemain % m_iGridSize ;
			y = iXYRemain / m_iGridSize ;

			// check 26 direction
			for ( int dir = 0; dir < 8; ++dir )
			{
				cx = x + SimpleCell::DIR_WAY_X_DIFF[ dir ] ;
				cy = y + SimpleCell::DIR_WAY_Y_DIFF[ dir ] ;
				cz = z + SimpleCell::DIR_WAY_Y_DIFF[ dir ] ;
				iNearCellIndex = cz * ( m_iGridSize * m_iGridSize ) + cy * m_iGridSize + cx ;

				if ( cx >= 0 && cx < m_iGridSize && cy >= 0 && cy < m_iGridSize && cz >= 0 && cz < m_iGridSize && m_vCells[ iNearCellIndex ] )
				{
					if ( E_CT_END == m_vCells[ iNearCellIndex ]->m_eType )
					{
						return true ;
					}
				}
			}
		}
	}

	return false ;
}

bool	CellMap::IsNearEndCell( int x, int y, int z, int tx, int ty, int tz ) const
{
	if ( abs( tx - x ) <= 1 && abs( ty - y ) <= 1 && abs( tz - z ) <= 1 )
	{
		return true ;
	}

	return false ;
}


void	CellMap::PrintValue( ValueType eType )
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

	switch ( eType )
	{
		case E_VT_X :		strMsg = " -- x values " ;					break ;
		case E_VT_A :		strMsg = " -- A values " ;					break ;
		case E_VT_B :		strMsg = " -- b values " ;					break ;
		case E_VT_R :		strMsg = " -- r values " ;					break ;
		
		case E_VT_X_DIFF :	strMsg = " -- X derivative values " ;						break ;
		case E_VT_Y_DIFF :	strMsg = " -- Y derivative values " ;						break ;
		case E_VT_2_DIFF :	strMsg = " -- 2nd derivative values (normalized) " ;		break ;
		
		default	:			strMsg = " invalid values !! " ;			break ;
	}

	std::cout << std::endl ;
	std::cout << strMsg << std::endl ;

	if ( log )
	{
		log << std::endl ;
		log << strMsg << std::endl ;
	}

	if ( E_VT_X <= eType && eType < MAX_VALUE_TYPE && (int)m_vCells.size() >= m_iGridSize * m_iGridSize )
	{
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
					switch ( eType )
					{
						case E_VT_X :		fValue = m_vCells[ iIndex ]->m_fPotential ;	break ;
						case E_VT_A :		fValue = m_vCells[ iIndex ]->m_fStencil ;	break ;
						case E_VT_B :		fValue = m_vCells[ iIndex ]->m_fB ;			break ;
						case E_VT_R :		fValue = m_vCells[ iIndex ]->m_fR ;			break ;
						
						case E_VT_X_DIFF :	fValue = m_vCells[ iIndex ]->m_fXDiff ;		break ;
						case E_VT_Y_DIFF :	fValue = m_vCells[ iIndex ]->m_fYDiff ;		break ;
						case E_VT_2_DIFF :	fValue = m_vCells[ iIndex ]->m_f2Diff ;		break ;
					}

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
	}

	if ( log )
	{
		log.close() ;
	}
}

void	CellMap::ClearLog()
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



void	GetNodeSegments( const LightningTreeNode* pNode, std::vector< SimpleCell >& vSegments )
{
	if ( pNode && pNode->m_pParent )
	{
		SimpleCell seg ;
		
		seg.m_iParentX = pNode->m_pParent->m_iX ;
		seg.m_iParentY = pNode->m_pParent->m_iY ;
		seg.m_iParentZ = pNode->m_pParent->m_iZ ;
		seg.m_iX = pNode->m_iX ;
		seg.m_iY = pNode->m_iY ;
		seg.m_iZ = pNode->m_iZ ;

		vSegments.push_back( seg ) ;

		int iChildCount = pNode->m_vChildren.size() ;
		for ( int i = 0; i < iChildCount; ++i )
		{
			GetNodeSegments( pNode->m_vChildren[ i ], vSegments ) ;
		}
	}
}


// =========================================================================================================
// CGMMap

CGMMap::CGMMap()
	: m_iMaxIteration( MAX_ITERATION )
{
}

CGMMap::CGMMap( int iMaxIteration, int iEta )
	: CellMap( iEta ), m_iMaxIteration( iMaxIteration )
{
}

CGMMap::~CGMMap()
{
}

bool	CGMMap::Process( ProcessType eType, bool bShowTime, bool bShowValue )
{
	if ( m_bProcessFinished )
	{
		return false ;
	}

	bool bLoop = true ;

	clock_t timeStartTotal, timeEndTotal ;
	clock_t timeStart, timeEnd ;
	double timeEllapsed ;

	timeStartTotal = clock() ;
	
	while ( bLoop )
	{
		++m_iProcessIndex ;

		if ( E_PT_ONE_STEP == eType )	// one step
		{
			bLoop = false ;
		}

		// ----------------------------------------------------------------
		// solve CGM
		timeStart = clock() ;
		
		int iIter = m_solver.Solve( m_vCells, m_iGridSize, m_iMaxIteration ) ;
		
		timeEnd = clock() ;
		timeEllapsed = double( timeEnd - timeStart ) / CLOCKS_PER_SEC ;

/*/
		if ( bShowTime )
		{
			std::cout.setf( ios::fixed ) ;
			std::cout.precision( 2 ) ;

			std::cout << std::endl ;
			std::cout << " -- (" << m_iProcessIndex << ") CGM : iter = " << iIter << " (time: " << timeEllapsed << ") " << std::endl ;
		}
//*/

		if ( bShowValue )
		{
			PrintValue( E_VT_X ) ;
		}

		// ----------------------------------------------------------------
		// update negative cell & get candidates
		UpdateCandidate() ;

		// ----------------------------------------------------------------
		// select next cell from candidates
		std::vector< float > vProb ;
		std::vector< SimpleCell >::iterator itr = m_vCandidates.begin() ;
		while ( itr != m_vCandidates.end() )
		{
			vProb.push_back( pow( fabs( ( *itr ).m_fProb ), m_iEta ) ) ;	// apply eta
			++itr ;
		}

// -------------------------------------------------------------------------------
//		// for VS 2015
//		//std::discrete_distribution<> d( vProb.begin(), vProb.end() ) ;
// -------------------------------------------------------------------------------
//		// for VS 2013
		std::size_t i( 0 ) ;
		std::discrete_distribution<> d( vProb.size(), 0, 1, [&vProb, &i](double)
		{
			auto w = vProb[ i ] ;
			++i ;
			return w ;
		} ) ;
// -------------------------------------------------------------------------------

		int iSelectedIndex = d( m_randGen ) ;
/*/		
		if ( bShowTime )
		{
			std::cout << std::endl ;
			std::cout << " -- selected index : " << iSelectedIndex << " in " << m_vCandidates.size() << std::endl ;
		}
//*/

		// ----------------------------------------------------------------
		// set next cell & update cells
		m_nextCell = m_vCandidates[ iSelectedIndex ] ;

		int iIndex = m_nextCell.m_iZ * ( m_iGridSize * m_iGridSize ) + m_nextCell.m_iY * m_iGridSize + m_nextCell.m_iX ;
		CellType eType = GetCellType( iIndex ) ;
		if ( E_CT_END == eType )
		{
			// end loop
			bLoop = false ;
			m_bProcessFinished = true ;
		}
		else
		{
			// update cell
			SetCellType( iIndex, E_CT_START ) ;
		}
		
		// ----------------------------------------------------------------
		// add lightning node to tree
		m_tree.AddChild( m_nextCell.m_iParentX, m_nextCell.m_iParentY, m_nextCell.m_iParentZ, m_nextCell.m_iX, m_nextCell.m_iY, m_nextCell.m_iZ ) ;

		// add next cell to negative cells
		//m_vNegativeCells.push_back( m_nextCell ) ;	// it is operated by SetCellType() function call
	}

	timeEndTotal = clock() ;
	timeEllapsed = double( timeEndTotal - timeStartTotal ) / CLOCKS_PER_SEC ;

	if ( bShowTime )
	{
		std::cout.setf( ios::fixed ) ;
		std::cout.precision( 2 ) ;

		std::cout << std::endl ;
		std::cout << " -- Total time : " << timeEllapsed << std::endl ;
	}

	return true ;
}



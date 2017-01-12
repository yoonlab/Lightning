

#include "cgm_map.h"
#include "GL/freeglut.h"

#include <iostream>
#include <ios>
#include <fstream>
#include <sstream>

#include <ctime>

#include <map>

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
	m_vObstacleCells.clear() ;

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
	m_vObstacleCells.clear() ;

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
	m_vObstacleCells.clear() ;

	m_vCandidates.clear() ;
	m_tree.Clear() ;

	m_bProcessFinished = false ;
}

bool	CellMap::Load( const std::string& strPath )
{
	std::cout << "Load map file : " << strPath.c_str() << std::endl ;

	Clear() ;

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
	SimpleCell simpleCell ;
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
		else if ( strLine.substr( 0, 10 ) == "DEFAULT_X:" )
		{
			std::istringstream pos( strLine.substr( 10 ) ) ;
			pos >> fDefaultX ;
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

					simpleCell.m_iX = iCellX ;
					simpleCell.m_iY = iCellY ;

					pCell = new Cell( iCellX, iCellY, E_CT_EMPTY, fDefaultX ) ;
					if ( pCell )
					{
						m_vCells.push_back( pCell ) ;

						switch ( iCellType )
						{
							case E_CT_START :
							{
								pCell->SetCellType( E_CT_START ) ;

								m_vNegativeCells.push_back( simpleCell ) ;
								m_vStartCells.push_back( simpleCell ) ;
							}
							break ;

							case E_CT_END :
							{
								pCell->SetCellType( E_CT_END ) ;

								m_vPositiveCells.push_back( simpleCell ) ;
								m_vEndCells.push_back( simpleCell ) ;
							}
							break ;

							case E_CT_OBSTACLE :
							{
								pCell->SetCellType( E_CT_OBSTACLE ) ;

								m_vBoundaryCells.push_back( simpleCell ) ;	// consider obstacle as a boundary
								m_vObstacleCells.push_back( simpleCell ) ;
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

	std::cout << "Finish loading map file !!" << std::endl ;

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
							if ( (*itr).m_iX == m_vCells[ iIndex ]->m_iX && (*itr).m_iY == m_vCells[ iIndex ]->m_iY )
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
							if ( (*itr).m_iX == m_vCells[ iIndex ]->m_iX && (*itr).m_iY == m_vCells[ iIndex ]->m_iY )
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
							if ( (*itr).m_iX == m_vCells[ iIndex ]->m_iX && (*itr).m_iY == m_vCells[ iIndex ]->m_iY )
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

const SimpleCell*	CellMap::GetObstacleCell( int iIndex ) const
{
	if ( 0 <= iIndex && iIndex < (int)m_vObstacleCells.size() )
	{
		return &m_vObstacleCells[ iIndex ] ;
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

	m_nextCell = nextCell ;
}

void	CellMap::UpdateCandidate()
{
	// add next cell to negative cells
	//m_vNegativeCells.push_back( m_nextCell ) ;	// it is operated by SetCellType() function call.

	// add candidates
	m_vCandidates.clear() ;

	int iIndex ;

	int px, py ;
	int cx, cy ;
	int iAdjIndex ;
	float fPotentialSum = 0.0f ;

	std::vector< SimpleCell >::const_iterator itr = m_vNegativeCells.begin() ;
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
				cx = px + SimpleCell::DIR_8_WAY_X_DIFF[ dir ] ;
				cy = py + SimpleCell::DIR_8_WAY_Y_DIFF[ dir ] ;
				iAdjIndex = cy * m_iGridSize + cx ;

				if ( cx >= 0 && cx < m_iGridSize && cy >= 0 && cy < m_iGridSize && m_vCells[ iAdjIndex ] )
				{
					if ( E_CT_EMPTY == m_vCells[ iAdjIndex ]->m_eType || E_CT_END == m_vCells[ iAdjIndex ]->m_eType )
					{
						SimpleCell candi ;
						candi.m_iX = cx ;
						candi.m_iY = cy ;
						candi.m_iParentX = px ;
						candi.m_iParentY = py ;
						candi.m_fProb = m_vCells[ iAdjIndex ]->m_fPotential ;

						m_vCandidates.push_back( candi ) ;

						fPotentialSum += candi.m_fProb ;
					}
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
	if ( 0 <= iIndex && iIndex < m_iGridSize * m_iGridSize )
	{
		int x, y, cx, cy ;
		int iNearCellIndex ;

		if ( m_vCells[ iIndex ] )
		{
			x = iIndex % m_iGridSize ;
			y = iIndex / m_iGridSize ;

			// check 8 direction
			for ( int dir = 0; dir < 8; ++dir )
			{
				cx = x + SimpleCell::DIR_8_WAY_X_DIFF[ dir ] ;
				cy = y + SimpleCell::DIR_8_WAY_Y_DIFF[ dir ] ;
				iNearCellIndex = cy * m_iGridSize + cx ;

				if ( cx >= 0 && cx < m_iGridSize && cy >= 0 && cy < m_iGridSize && m_vCells[ iNearCellIndex ] )
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

bool	CellMap::IsNearEndCell( int x, int y, int tx, int ty ) const
{
	if ( abs( tx - x ) <= 1 && abs( ty - y ) <= 1 )
	{
		return true ;
	}

	return false ;
}

bool	CellMap::IsNearEndCell( int x, int y, int tx, int ty, int iRadius ) const
{
	if ( abs( tx - x ) <= iRadius && abs( ty - y ) <= iRadius )
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


bool	CellMap::WriteValue( const std::string& strPath )
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
/*/
	// normalize
	float fMin ;
	float fMax ;
	float bFirst = true ;

	for ( int y = 0; y < m_iGridSize; ++y )
	{
		for ( int x = 0; x < m_iGridSize; ++x )
		{
			iIndex = y * m_iGridSize + x ;
			
			if ( m_vCells[ iIndex ] )
			{
				if ( bFirst )
				{
					fMin = m_vCells[ iIndex ]->m_fPotential ;
					fMax = fMin ;
					bFirst = false ;
				}
				else if ( fMin > m_vCells[ iIndex ]->m_fPotential )
				{
					fMin = m_vCells[ iIndex ]->m_fPotential ;
				}
				else if ( fMax < m_vCells[ iIndex ]->m_fPotential )
				{
					fMax = m_vCells[ iIndex ]->m_fPotential ;
				}
			}
		}
	}

	if ( fMax > 1 || fMin < 0 )
	{
		if ( fMax != fMin )
		{
			for ( int y = 0; y < m_iGridSize; ++y )
			{
				for ( int x = 0; x < m_iGridSize; ++x )
				{
					iIndex = y * m_iGridSize + x ;
			
					if ( m_vCells[ iIndex ] && E_CT_EMPTY == m_vCells[ iIndex ]->m_eType )
					{
						if ( fMax < 0 )	m_vCells[ iIndex ]->m_fPotential -= fMax ;
						if ( fMin < 0 )	m_vCells[ iIndex ]->m_fPotential -= fMin ;

						m_vCells[ iIndex ]->m_fPotential /= ( fMax - fMin ) ;
					}
				}
			}
		}
	}
//*/

/*/
	// write x
	for ( int y = 0; y < m_iGridSize; ++y )
	{
		for ( int x = 0; x < m_iGridSize; ++x )
		{
			out << x ;
			
			if ( y != m_iGridSize - 1 || x != m_iGridSize - 1 )
			{
				out << "," ;
			}
		}
	}
	out << std::endl ;

	
	// write y
	for ( int y = 0; y < m_iGridSize; ++y )
	{
		for ( int x = 0; x < m_iGridSize; ++x )
		{
			out << y ;
			
			if ( y != m_iGridSize - 1 || x != m_iGridSize - 1 )
			{
				out << "," ;
			}
		}
	}
	out << std::endl ;
//*/
	
	// write p
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

bool	CellMap::WriteMap( const std::string& strPath )
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


void	GetNodeSegments( const LightningTreeNode* pNode, std::vector< SimpleCell >& vSegments )
{
	if ( pNode && pNode->m_pParent )
	{
		SimpleCell seg ;
		
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

float	CellMap::CalculateFractalDimension()
{
	// assume that total grid map size is 200 x 200 pixel

	// extract nodes from tree
	std::vector< SimpleCell > vSegments ;

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

	SimpleCell seg ;
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

		int cx = seg.m_iParentX + SimpleCell::DIR_8_WAY_X_DIFF[ iDirIndex ] ;
		int cy = seg.m_iParentY + SimpleCell::DIR_8_WAY_Y_DIFF[ iDirIndex ] ;

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


// =========================================================================================================
// CGMMap

CGMMap::CGMMap()
	: m_iMaxIteration( MAX_ITERATION )
{
	m_iTimeProcesStart = 0 ;
	m_iTimeIndex = 0 ;
	m_vTimeCheckSteps.clear() ;
	m_vTimePerSteps.clear() ;
}

CGMMap::CGMMap( int iMaxIteration, int iEta )
	: CellMap( iEta ), m_iMaxIteration( iMaxIteration )
{
	m_iTimeProcesStart = 0 ;
	m_iTimeIndex = 0 ;
	m_vTimeCheckSteps.clear() ;
	m_vTimePerSteps.clear() ;
}

CGMMap::~CGMMap()
{
}

bool	CGMMap::Load( const std::string& strPath )
{
	bool bRet = CellMap::Load( strPath ) ;
	if ( bRet )
	{
		//m_vObstacleCells.clear() ;	// do not consider boundary & obstacle cells	// use it for rendering
	}

	return bRet ;
}

bool	CGMMap::Load( int iGridSize, int sx, int sy, int ex, int ey )
{
	std::cout << "Load map (Grid size : " << iGridSize << ") !!" << std::endl ;

	Clear() ;

	m_iGridSize = iGridSize ;

	bool bUsePositiveGround = false ;
	if ( -1 == ex && -1 == ey )
	{
		bUsePositiveGround = true ;
	}

	// create whole cells
	Cell* pCell ;
	SimpleCell simpleCell ;

	for ( int y = 0; y < m_iGridSize; ++y )
	{
		for ( int x = 0; x < m_iGridSize; ++x )
		{
			pCell = new Cell( x, y, E_CT_EMPTY, 0 ) ;
			if ( pCell )
			{
				m_vCells.push_back( pCell ) ;

				simpleCell.m_iX = x ;
				simpleCell.m_iY = y ;

				if ( bUsePositiveGround && m_iGridSize - 1 == y )
				{
					pCell->SetCellType( E_CT_END ) ;

					m_vEndCells.push_back( simpleCell ) ;
					m_vPositiveCells.push_back( simpleCell ) ;
				}
			}
		}
	}

	// set start position
	int iIndex = sy * m_iGridSize + sx ;
	if ( m_vCells[ iIndex ] )
	{
		m_vCells[ iIndex ]->SetCellType( E_CT_START ) ;

		simpleCell.m_iX = sx ;
		simpleCell.m_iY = sy ;

		m_vStartCells.push_back( simpleCell ) ;
		m_vNegativeCells.push_back( simpleCell ) ;

		// initialize lightning tree
		InitTree() ;
	}

	// set goal position if it does not use default setting
	if ( false == bUsePositiveGround )
	{
		int iIndex = ey * m_iGridSize + ex ;
		if ( m_vCells[ iIndex ] )
		{
			m_vCells[ iIndex ]->SetCellType( E_CT_END ) ;

			simpleCell.m_iX = ex ;
			simpleCell.m_iY = ey ;

			m_vEndCells.push_back( simpleCell ) ;
			m_vPositiveCells.push_back( simpleCell ) ;
		}
	}

	// initialize lightning tree
	InitTree() ;

	std::cout << "Finish generating map !!" << std::endl ;
	
	return true ;
}

bool	CGMMap::Process( ProcessType eType, bool bShowTime, bool bShowValue )
{
	m_iTimeProcesStart = glutGet( GLUT_ELAPSED_TIME ) ;

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

		int iIndex = m_nextCell.m_iY * m_iGridSize + m_nextCell.m_iX ;
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
		m_tree.AddChild( m_nextCell.m_iParentX, m_nextCell.m_iParentY, m_nextCell.m_iX, m_nextCell.m_iY ) ;

		// add next cell to negative cells
		//m_vNegativeCells.push_back( m_nextCell ) ;	// it is operated by SetCellType() function call


		// time check per steps
		if ( !m_vTimeCheckSteps.empty() && m_iTimeIndex < m_vTimeCheckSteps.size() && m_iProcessIndex == m_vTimeCheckSteps[ m_iTimeIndex ] )
		{
			m_vTimePerSteps.push_back( glutGet( GLUT_ELAPSED_TIME ) ) ;

			++m_iTimeIndex ;
		}
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

	//return m_bProcessFinished ;
	return true ;
}


void	CGMMap::SetTimeCheckSteps( const std::vector< int >& vSteps )
{
	m_vTimeCheckSteps.clear() ;
	m_vTimePerSteps.clear() ;

	m_iTimeIndex = 0 ;
	m_iTimeProcesStart = 0 ;

	m_vTimeCheckSteps = vSteps ;
}

const std::vector< int > CGMMap::GetTimesPerSteps() const
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



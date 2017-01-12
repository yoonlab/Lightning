

#include "lightning_tree.h"
#include "base.h"

using namespace Lightning ;
using namespace std ;


LightningTree::LightningTree()
	: m_pRoot( NULL )
{
	m_vNodes.clear() ;
}

LightningTree::~LightningTree()
{
	Clear() ;
}


void	LightningTree::Clear()
{
	std::vector< LightningTreeNode* >::iterator itr = m_vNodes.begin() ;
	while ( itr != m_vNodes.end() )
	{
		SAFE_DELETE( *itr ) ;
		++itr ;
	}

	m_pRoot = NULL ;
	m_vNodes.clear() ;
}


bool	LightningTree::SetRoot( LightningTreeNode* pRoot )
{
	Clear() ;

	if ( pRoot )
	{
		m_pRoot = pRoot ;
		m_pRoot->m_pParent = NULL ;

		m_vNodes.push_back( m_pRoot ) ;

		return true ;
	}

	return false ;
}


void	LightningTree::AddChild( int iParentX, int iParentY, int iChildX, int iChildY )
{
	if ( m_pRoot )
	{
		LightningTreeNode* pParent = NULL ;
		std::vector< LightningTreeNode* >::iterator itr = m_vNodes.begin() ;
		while ( itr != m_vNodes.end() )
		{
			pParent = ( *itr ) ;

			// find parent
			if ( pParent && pParent->m_iX == iParentX && pParent->m_iY == iParentY )
			{
				// add child
				LightningTreeNode* pChild = new LightningTreeNode() ;
				if ( pChild )
				{
					pChild->m_iX = iChildX ;
					pChild->m_iY = iChildY ;
					pChild->m_pParent = pParent ;

					pParent->AddChild( pChild ) ;

					m_vNodes.push_back( pChild ) ;

					break ;
				}
			}

			++itr ;
		}
	}
}

void	LightningTree::ApplyIntensityAndThickness( float fBaseThickness, float fIntensityAttenuation )
{
	int iSize = (int)m_vNodes.size() ;
	if ( iSize > 1 && m_pRoot )
	{
		int iMainChannelLength = 0 ;

		// set root node values
		m_pRoot->m_bMainChannel = true ;
		m_pRoot->m_fThickness = fBaseThickness ;
		m_pRoot->m_fIntensity = 1.0f ;

		// find last node (it is end node)
		LightningTreeNode* pNode = m_vNodes[ iSize - 1 ] ;
		if ( pNode )
		{
			// find main channel nodes
			while ( pNode != m_pRoot )
			{
				pNode->m_bMainChannel = true ;
				pNode->m_fThickness = fBaseThickness ;
				pNode->m_fIntensity = 1.0f ;

				pNode = pNode->m_pParent ;

				++iMainChannelLength ;
			}

			// attenuate thickness & intensity
			AttenuateIntensityAndThickness( m_pRoot, fIntensityAttenuation ) ;
		}
	}
}

void	LightningTree::AttenuateIntensityAndThickness( LightningTreeNode* pNode, float fAttenuation )
{
	if ( pNode )
	{
		if ( false == pNode->m_bMainChannel && pNode->m_pParent )
		{
			pNode->m_fIntensity = pNode->m_pParent->m_fIntensity * fAttenuation ;
			
			if ( pNode->m_pParent->m_fThickness > 1.0f )
			{
				pNode->m_fThickness = pNode->m_pParent->m_fThickness / 2.0f ;
			}
		}

		std::vector< LightningTreeNode* >::iterator itr = pNode->m_vChildren.begin() ;
		while ( itr != pNode->m_vChildren.end() )
		{
			AttenuateIntensityAndThickness( *itr, fAttenuation ) ;

			++itr ;
		}
	}
}
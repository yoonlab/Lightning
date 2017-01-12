

#ifndef	__LIGHTNING_TREE_3D_H__
#define	__LIGHTNING_TREE_3D_H__

#include <vector>

namespace Lightning3D
{


class LightningTreeNode
{
public :

	// constructor & destructor
	LightningTreeNode()
		: m_iX( 0 ), m_iY( 0 ), m_iZ( 0 )
		, m_fXJittered( 0 ), m_fYJittered( 0 ), m_fZJittered( 0 ), m_bJittered( false )
		, m_fIntensity( 1.0f ), m_fThickness( 1.0f ), m_bMainChannel( false ), m_pParent( NULL )
		, m_bTarget( false )
	{
		m_vChildren.clear() ;
	}

	virtual ~LightningTreeNode()
	{
	}

	void	AddChild( LightningTreeNode* pChild )
	{
		if ( pChild )
		{
			pChild->m_pParent = this ;
			m_vChildren.push_back( pChild ) ;
		}
	}

public :

	int									m_iX ;
	int									m_iY ;
	int									m_iZ ;

	float								m_fXJittered ;
	float								m_fYJittered ;
	float								m_fZJittered ;
	bool								m_bJittered ;

	float								m_fIntensity ;
	float								m_fThickness ;
	bool								m_bMainChannel ;

	LightningTreeNode*					m_pParent ;
	std::vector< LightningTreeNode* >	m_vChildren ;

	bool								m_bTarget ;

} ;	// LightningTreeNode



class LightningTree
{
public :

	// constructor & destructor
	LightningTree() ;
	virtual ~LightningTree() ;

	// clear all nodes
	void	Clear() ;


	// root
	bool	SetRoot( LightningTreeNode* pRoot ) ;

	// add child node
	void	AddChild( int iParentX, int iParentY, int iParentZ, int iChildX, int iChildY, int iChildZ, bool bTarget = false ) ;

	// apply intensity & thickness of node
	void	ApplyIntensityAndThickness( float fBaseThickness, float fIntensityAttenuation ) ;
	void	ApplyIntensityAndThicknessForMultipleTarget( float fBaseThickness, float fIntensityAttenuation, int iTargetCount ) ;

	// getter
	const LightningTreeNode*					GetRoot() const		{ return m_pRoot ;	}
	//const std::vector< LightningTreeNode* >&	GetNodeList() const	{ return m_vNodes ;	}
	std::vector< LightningTreeNode* >&			GetNodeList()		{ return m_vNodes ;	}

protected :

	void	AttenuateIntensityAndThickness( LightningTreeNode* pNode, float fAttenuation ) ;

private :

	LightningTreeNode*					m_pRoot ;	// tree structure
	std::vector< LightningTreeNode* >	m_vNodes ;	// node list


} ;	// LightningTree


}	// namespace of Lightning

#endif	// __LIGHTNING_TREE_3D_H__

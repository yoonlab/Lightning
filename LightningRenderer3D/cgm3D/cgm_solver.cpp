

#include "cgm_solver.h"
#include "../LightningGenerator3D/base.h"

#include <iostream>

using namespace Cgm3D ;
using namespace std ;


const float	CGMSolver::EPSILON = 0.0001f ;


CGMSolver::CGMSolver()
{
}

CGMSolver::~CGMSolver()
{
}

int		CGMSolver::Solve( std::vector< Cell* >& vCells, int iGridSize, int iMaxIteration )
{
	int iTotalCellNum = iGridSize * iGridSize * iGridSize ;
	if ( vCells.size() < iTotalCellNum )
	{
		std::cerr << std::endl ;
		std::cerr << "cell size error !! " << std::endl ;

		return 0 ;
	}

	// calculate stencil(for A) & boundary(b)
	CalcStencilNBoundary( vCells, iGridSize ) ;

	// calculate r0
	// r = b - Ax
	CalcResidual( vCells, iGridSize ) ;


	// initialize p0 by r0
	// calculate delta (delta = rT * r = r ^ 2)
	std::vector< float > p ;
	float fDelta = 0.0f ;

	for ( int i = 0; i < iTotalCellNum; ++i )
	{
		if ( vCells[ i ] )
		{
			p.push_back( vCells[ i ]->m_fR ) ;
			fDelta += p[ i ] * p[ i ] ;
		}
	}
		
	float fNeighborSum = 0.0f ;
	int iNeighborX ;
	int iNeighborY ;
	int iNeighborZ ;
	int iNeighborIndex ;

	std::vector< float > q ;
	q.assign( iTotalCellNum, 0.0f ) ;

	int k = 0 ;
	float fMaxR = EPSILON * 2 ;
	
	while ( k < iMaxIteration && fMaxR > EPSILON )
	{
		float fDenominator = 0.0f ;

		// calculate q (temporary variable to optimize)
		// q = A * p
		for ( int i = 0; i < iTotalCellNum; ++i )
		{
			if ( vCells[ i ] )
			{
				fNeighborSum = 0.0f ;

				for ( int dir = E_ND_UP; dir < MAX_NEIGHBOR_DIRECTION; ++dir )
				{
					iNeighborX = vCells[ i ]->m_iX + Cell::DIR_X_DIFF[ dir ] ;
					iNeighborY = vCells[ i ]->m_iY + Cell::DIR_Y_DIFF[ dir ] ;
					iNeighborZ = vCells[ i ]->m_iZ + Cell::DIR_Z_DIFF[ dir ] ;
					iNeighborIndex = iNeighborZ * ( iGridSize * iGridSize ) + iNeighborY * iGridSize + iNeighborX ;

					if ( iNeighborIndex >= 0 && iNeighborIndex < iTotalCellNum && vCells[ iNeighborIndex ] )
					{
						fNeighborSum += p[ iNeighborIndex ] * vCells[ i ]->m_fNeighborStencil[ dir ] ;
					}
				}

				q[ i ] = -fNeighborSum + vCells[ i ]->m_fStencil * p[ i ] ;

				fDenominator += p[ i ] * q[ i ] ;	// denominator = pT * A * p  for calculating alpha
			}
		}

		// calculate alpha
		// alpha = rT * r / pT * A * p
		//       = delta / pT * A * p
		//       = delta / pT * q
		float fAlpha = 0.0f ;
		if ( fDenominator != 0.0f )
		{
			fAlpha = fDelta / fDenominator ;
		}

		float fDeltaOld = fDelta ;
		fDelta = 0.0f ;

		fMaxR = 0.0f ;

		// x(k+1) = x(k) + alpha * p
		// 
		// r(k+1) = r(k) - alpha * A * p
		//        = r(k) - alpha * q
		for ( int i = 0; i < iTotalCellNum; ++i )
		{
			if ( vCells[ i ] )
			{
				// x(k+1)
				vCells[ i ]->m_fPotential += fAlpha * p[ i ] ;
				
				if ( E_CT_START == vCells[ i ]->m_eType || E_CT_OBSTACLE == vCells[ i ]->m_eType )
				{
					vCells[ i ]->m_fPotential = 0.0f ;
				}
				else if ( E_CT_END == vCells[ i ]->m_eType )
				{
					vCells[ i ]->m_fPotential = 1.0f ;
				}

				// r(k+1)
				vCells[ i ]->m_fR -= fAlpha * q[ i ] ;

				fDelta += vCells[ i ]->m_fR * vCells[ i ]->m_fR ;
				
				if ( fabs( vCells[ i ]->m_fR ) > fMaxR )
				{
					fMaxR = fabs( vCells[ i ]->m_fR ) ;
				}
			}
		}

		// calculate beta
		// beta = rT(k+1) * r(k+1) / rT(k) * r(k)
		float fBeta = 0.0f ;
		if ( fDeltaOld != 0.0f )
		{
			fBeta = fDelta / fDeltaOld ;
		}

		// update delta(p)
		// p(k+1) = r(k+1) + beta * p(k)
		for ( int i = 0; i < iTotalCellNum; ++i )
		{
			if ( vCells[ i ] )
			{
				p[ i ] = vCells[ i ]->m_fR + fBeta * p[ i ] ;
			}
		}
		
		++k ;
	}
	
	return k ;
}


void	CGMSolver::CalcStencilNBoundary( std::vector< Cell* >& vCells, int iGridSize )
{
	if ( vCells.size() < iGridSize * iGridSize * iGridSize )
	{
		std::cerr << std::endl ;
		std::cerr << "cell size error !! " << std::endl ;

		return ;
	}

	int iIndex = 0 ;
	
	int iNeighborX ;
	int iNeighborY ;
	int iNeighborZ ;
	int iNeighborIndex ;

	float fNeighborSum ;
	float fBoundarySum ;

	float fInvDx = 1.0f / MAX_NEIGHBOR_DIRECTION ;

	for ( int z = 0; z < iGridSize; ++z )
	{
		for ( int y = 0; y < iGridSize; ++y )
		{
			for ( int x = 0; x < iGridSize; ++x )
			{
				iIndex = z * ( iGridSize * iGridSize ) + y * iGridSize + x ;

				if ( vCells[ iIndex ] )
				{
					fNeighborSum = 0.0f ;
					fBoundarySum = 0.0f ;

					for ( int dir = E_ND_UP; dir < MAX_NEIGHBOR_DIRECTION; ++dir )
					{
						iNeighborX = x + Cell::DIR_X_DIFF[ dir ] ;
						iNeighborY = y + Cell::DIR_Y_DIFF[ dir ] ;
						iNeighborZ = z + Cell::DIR_Z_DIFF[ dir ] ;
						iNeighborIndex = iNeighborZ * ( iGridSize * iGridSize ) + iNeighborY * iGridSize + iNeighborX ;

						vCells[ iIndex ]->m_fNeighborStencil[ dir ] = 0.0f ;

						fNeighborSum += fInvDx ;

						if ( iNeighborX < 0 || iNeighborY < 0 || iNeighborZ < 0 )
						{
							// left & top boundary
							fBoundarySum += 0.0f * fInvDx ;
						}
						else if ( iNeighborX >= iGridSize || iNeighborY >= iGridSize || iNeighborZ >= iGridSize )
						{
							// right & bottom boundary
							fBoundarySum += 0.0f * fInvDx ;
						}
						else if ( vCells[ iNeighborIndex ] )
						{
							if ( E_CT_START == vCells[ iNeighborIndex ]->m_eType || E_CT_OBSTACLE == vCells[ iNeighborIndex ]->m_eType )
							{
								// start & obstacle boundary
								fBoundarySum += 0.0f * fInvDx ;
							}
							else if ( E_CT_END == vCells[ iNeighborIndex ]->m_eType )
							{
								// end boundary
								fBoundarySum += 1.0f * fInvDx ;
							}
							else if ( E_CT_EMPTY == vCells[ iNeighborIndex ]->m_eType )
							{
								// empty cell
								vCells[ iIndex ]->m_fNeighborStencil[ dir ] = fInvDx ;
							}
						}
					}

					vCells[ iIndex ]->m_fStencil = fNeighborSum ;	// for A
					vCells[ iIndex ]->m_fB = fBoundarySum ;			// b
				}
			}
		}
	}
}

float	CGMSolver::CalcResidual( std::vector< Cell* >& vCells, int iGridSize )
{
	float fMaxResidual = 0.0f ;

	int iTotalCellNum = iGridSize * iGridSize * iGridSize ;
	if ( vCells.size() < iTotalCellNum )
	{
		std::cerr << std::endl ;
		std::cerr << "cell size error !! " << std::endl ;

		return fMaxResidual ;
	}

	float fNeighborSum = 0.0f ;

	int iNeighborX ;
	int iNeighborY ;
	int iNeighborZ ;
	int iNeighborIndex ;

	for ( int i = 0; i < iTotalCellNum; ++i )
	{
		if ( vCells[ i ] )
		{
			fNeighborSum = 0.0f ;

			for ( int dir = E_ND_UP; dir < MAX_NEIGHBOR_DIRECTION; ++dir )
			{
				iNeighborX = vCells[ i ]->m_iX + Cell::DIR_X_DIFF[ dir ] ;
				iNeighborY = vCells[ i ]->m_iY + Cell::DIR_Y_DIFF[ dir ] ;
				iNeighborZ = vCells[ i ]->m_iZ + Cell::DIR_Z_DIFF[ dir ] ;
				iNeighborIndex = iNeighborZ * ( iGridSize * iGridSize ) + iNeighborY * iGridSize + iNeighborX ;

				if ( iNeighborIndex >= 0 && iNeighborIndex < iTotalCellNum && vCells[ iNeighborIndex ] )
				{
					fNeighborSum += vCells[ iNeighborIndex ]->m_fPotential * vCells[ i ]->m_fNeighborStencil[ dir ] ;
				}
			}

			vCells[ i ]->m_fR = vCells[ i ]->m_fB - ( -fNeighborSum + vCells[ i ]->m_fPotential * vCells[ i ]->m_fStencil ) ;

			if ( fabs( vCells[ i ]->m_fR ) > fMaxResidual )
			{
				fMaxResidual = fabs( vCells[ i ]->m_fR ) ;
			}
		}
	}

	return fMaxResidual ;
}

float	CGMSolver::CalcAlpha( std::vector< Cell* >& vCells, const std::vector< float >& p, int iGridSize )
{
	float alpha = 0.0 ;

	int iTotalCellNum = iGridSize * iGridSize * iGridSize ;
	if ( vCells.size() < iTotalCellNum || p.size() < iTotalCellNum )
	{
		std::cerr << std::endl ;
		std::cerr << "cell size error !! " << std::endl ;

		return alpha ;
	}

	float numerator = 0.0f ;
	float denominator = 0.0f ;

	float fNeighborSum = 0.0f ;

	int iNeighborX ;
	int iNeighborY ;
	int iNeighborZ ;
	int iNeighborIndex ;

	std::vector< float > q ;	// q = A * p
	q.reserve( iTotalCellNum ) ;
		
	for ( int i = 0; i < iTotalCellNum; ++i )
	{
		if ( vCells[ i ] )
		{
			numerator += vCells[ i ]->m_fR * vCells[ i ]->m_fR ;	// numerator = rT * r

			fNeighborSum = 0.0f ;

			for ( int dir = E_ND_UP; dir < MAX_NEIGHBOR_DIRECTION; ++dir )
			{
				iNeighborX = vCells[ i ]->m_iX + Cell::DIR_X_DIFF[ dir ] ;
				iNeighborY = vCells[ i ]->m_iY + Cell::DIR_Y_DIFF[ dir ] ;
				iNeighborZ = vCells[ i ]->m_iZ + Cell::DIR_Z_DIFF[ dir ] ;
				iNeighborIndex = iNeighborZ * ( iGridSize * iGridSize ) + iNeighborY * iGridSize + iNeighborX ;

				if ( iNeighborIndex >= 0 && iNeighborIndex < iTotalCellNum && vCells[ iNeighborIndex ] )
				{
					fNeighborSum += p[ iNeighborIndex ] * vCells[ i ]->m_fNeighborStencil[ dir ] ;
				}
			}

			q.push_back( -fNeighborSum + vCells[ i ]->m_fStencil * p[ i ] ) ;

			denominator += p[ i ] * q[ i ] ;	// denominator = pT * A * p
		}
	}

	if ( denominator != 0.0f )
	{
		alpha = numerator / denominator ;	// alpha = rT * r / pT * A * p
	}

	return alpha ;
}

float	CGMSolver::CalcBeta( std::vector< Cell* >& vCells, const std::vector< float >& new_r, int iGridSize )
{
	float beta = 0.0 ;

	int iTotalCellNum = iGridSize * iGridSize * iGridSize ;
	if ( vCells.size() < iTotalCellNum || new_r.size() < iTotalCellNum )
	{
		std::cerr << std::endl ;
		std::cerr << "cell size error !! " << std::endl ;

		return beta ;
	}

	float numerator = 0.0f ;
	float denominator = 0.0f ;

	for ( int i = 0; i < iTotalCellNum; ++i )
	{
		if ( vCells[ i ] )
		{
			numerator += new_r[ i ] * new_r[ i ] ;
			denominator += vCells[ i ]->m_fR * vCells[ i ]->m_fR ;
		}
	}

	if ( denominator != 0.0f )
	{
		beta = numerator / denominator ;
	}

	return beta ;
}


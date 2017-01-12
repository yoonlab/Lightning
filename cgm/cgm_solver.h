

#ifndef	__CGM_SOLVER_H__
#define __CGM_SOLVER_H__

#include "cell.h"
#include <vector>
#include <string>


class CGMSolver
{
	static const float	EPSILON ;

public :

	// constructor & destructor
	CGMSolver() ;
	virtual ~CGMSolver() ;

	// Solve
	int		Solve( std::vector< Cell* >& vCells, int iGridSize, int iMaxIteration ) ;
	int		Solve_Old( std::vector< Cell* >& vCells, int iGridSize, int iMaxIteration ) ;

protected :

	// compute values
	void	CalcStencilNBoundary( std::vector< Cell* >& vCells, int iGridSize ) ;
	float	CalcResidual( std::vector< Cell* >& vCells, int iGridSize ) ;

	// for Solve_Old function
	float	CalcAlpha( std::vector< Cell* >& vCells, const std::vector< float >& p, int iGridSize ) ;
	float	CalcBeta( std::vector< Cell* >& vCells, const std::vector< float >& new_r, int iGridSize ) ;

private :
	
} ;	// CGMSolver


#endif	// __CGM_SOLVER_H__

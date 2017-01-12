
#include <iostream>
#include <random>
#include <sstream>

#include "lightning_generator.h"

using namespace std ;
using namespace Lightning ;


int main()
{
	// extract data for heat map visualization
	std::string strMap = "./res/paper_7.map" ;
	std::string strOurPath = "./res/paper_7_our.txt" ;

	LightningGenerator gen ;
	gen.SetEta( 2 ) ;
	gen.SetPowOfR( 1 ) ;

	gen.Load( strMap ) ;
	gen.MakeAllCellsToCandidates() ;
	gen.Process( Lightning::E_PT_ONE_STEP ) ;
	gen.Normalize( 0.5 ) ;
	gen.WriteValue( strOurPath ) ;
	

	// finish
	std::cout << std::endl ;
	std::cout << "press enter key !!" << std::endl ;

	std::cin.get() ;

	return 0 ;
}

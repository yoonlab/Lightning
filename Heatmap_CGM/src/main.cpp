
#include <iostream>
#include <random>
#include <sstream>

#include "cgm_map.h"

using namespace std ;


int main()
{
	// extract data for heat map visualization
	std::string strMap = "./res/paper_7.map" ;
	std::string strCgmPath = "./res/paper_7_cgm.txt" ;

	CGMMap cgmMap ;
	cgmMap.SetEta( 2 ) ;

	cgmMap.Load( strMap ) ;
	cgmMap.Process( E_PT_ONE_STEP ) ;
	cgmMap.WriteValue( strCgmPath ) ;

	

	// finish
	std::cout << std::endl ;
	std::cout << "press enter key !!" << std::endl ;

	std::cin.get() ;

	return 0 ;
}

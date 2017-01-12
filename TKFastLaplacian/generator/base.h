

#ifndef	__BASE_DEFINE_AND_MACROS_H__
#define	__BASE_DEFINE_AND_MACROS_H__


#ifndef	PURE
#define PURE	= 0
#endif


#ifndef SAFE_DELETE
#define SAFE_DELETE( p )		{ if ( (p) ) { delete (p) ; (p) = NULL ; } }
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY( p )	{ if ( (p) ) { delete [] (p) ; (p) = NULL ; } }
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE( p )		{ if ( (p) ) { (p)->Release() ; (p) = NULL ; } }
#endif

#endif	// __BASE_DEFINE_AND_MACROS_H__

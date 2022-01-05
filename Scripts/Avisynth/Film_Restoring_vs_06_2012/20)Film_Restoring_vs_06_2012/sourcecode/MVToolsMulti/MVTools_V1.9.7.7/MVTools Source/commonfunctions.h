#ifndef __COMMON_F__
#define __COMMON_F__

/* returns the biggest integer x such as 2^x <= i */
inline static int ilog2(int i)
{
	int result = 0;
	while ( i > 1 ) { i >>= 1; ++result; }
	return result;
}

/* computes 2^i */ 
inline static int iexp2(int i)
{
    return (1<<i);
/*
	int result = 1;
	while ( i > 0 ) { result *= 2; --i; }
	return result;
*/
}

#endif
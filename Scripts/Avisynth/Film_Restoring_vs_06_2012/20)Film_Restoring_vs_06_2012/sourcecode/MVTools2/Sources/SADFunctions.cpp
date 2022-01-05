#include "SADFunctions.h"
/*
A change in interface can be covered like this at least until release

unsigned int Sad16x16_wrap(const BYTE *pSrc, int nSrcPitch, const BYTE *pRef, int nRefPitch){
	return Sad16x16_iSSE(pSrc,nSrcPitch,pRef,nRefPitch);
}

#define MK_CPPWRAP(blkx, blky) unsigned int  Sad##blkx##x##blky##_wrap(const BYTE *pSrc, int nSrcPitch, const BYTE *pRef, int nRefPitch){return Sad##blkx##x##blky##_iSSE(pSrc, nSrcPitch, pRef, nRefPitch);}
//MK_CPPWRAP(16,16);
MK_CPPWRAP(16,8);
MK_CPPWRAP(16,2);
MK_CPPWRAP(8,16);
MK_CPPWRAP(8,8);
MK_CPPWRAP(8,4);
MK_CPPWRAP(8,2);
MK_CPPWRAP(8,1);
MK_CPPWRAP(4,8);
MK_CPPWRAP(4,4);
MK_CPPWRAP(4,2);
MK_CPPWRAP(2,4);
MK_CPPWRAP(2,2);
#undef MK_CPPWRAP
*/


// MVTools
// 2004 Manao
// Copyright(c)2006-2009 A.G.Balakhnin aka Fizick - true motion, overlap, YUY2, pelclip, divide, super
// General classe for motion based filters

// See legal notice in Copying.txt for more information

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .

#include "MVAnalyse.h"
#include <stdio.h>
#include <math.h>
#include "dctfftw.h"
#include "dctint.h"
#include "cpu.h"


//extern MVCore mvCore;

MVAnalyse::MVAnalyse(PClip _child, int _blksizex, int _blksizey, int lv, int st, int stp, int _pelSearch,
                     bool isb, int lambda, bool chroma, int df,
                     int _lsad, int _plevel, bool _global, int _pnew, int _pzero, int _pglobal, int _overlapx, int _overlapy,
					 const char* _outfilename, int _dctmode, int _divide,
					 int _sadx264, int _badSAD, int _badrange, bool _isse, IScriptEnvironment* env) :
GenericVideoFilter(_child)
{

	if (!vi.IsYV12() && !vi.IsYUY2())
		env->ThrowError("MAnalyse: Clip must be YV12 or YUY2");

	// get parameters of super clip - v2.0
	SuperParams64Bits params;
    memcpy(&params, &child->GetVideoInfo().num_audio_samples, 8);
    int nHeight = params.nHeight;
    int nSuperHPad = params.nHPad;
    int nSuperVPad = params.nVPad;
    int nSuperPel = params.nPel;
    int nSuperModeYUV = params.nModeYUV;
    int nSuperLevels = params.nLevels;

    if (nHeight <=0 || nSuperHPad<0 || nSuperHPad>=vi.width/2 || nSuperVPad<0
    || nSuperPel<1 || nSuperPel>4 || nSuperModeYUV< 0 || nSuperModeYUV>YUVPLANES || nSuperLevels<1)
		env->ThrowError("MAnalyse: wrong super clip (pseudoaudio) parameters");

	analysisData.nWidth = vi.width - nSuperHPad*2;

	analysisData.nHeight = nHeight;

	analysisData.pixelType = vi.pixel_type;
	analysisData.yRatioUV = (vi.IsYV12()) ? 2 : 1;
	analysisData.xRatioUV = 2; // for YV12 and YUY2, really do not used and assumed to 2

//		env->ThrowError("MVAnalyse: %d, %d, %d, %d, %d",nPrepHPad, nPrepVPad, nPrepPel, nPrepModeYUV, nPrepLevels );
    pSrcGOF = new MVGroupOfFrames(nSuperLevels, analysisData.nWidth, analysisData.nHeight, nSuperPel, nSuperHPad, nSuperVPad, nSuperModeYUV, _isse, analysisData.yRatioUV);
    pRefGOF = new MVGroupOfFrames(nSuperLevels, analysisData.nWidth, analysisData.nHeight, nSuperPel, nSuperHPad, nSuperVPad, nSuperModeYUV, _isse, analysisData.yRatioUV);
    nSuperWidth = child->GetVideoInfo().width;
    nSuperHeight = child->GetVideoInfo().height;


	analysisData.nBlkSizeX = _blksizex;
	analysisData.nBlkSizeY = _blksizey;
	if (( analysisData.nBlkSizeX != 4 || analysisData.nBlkSizeY != 4) &&
        ( analysisData.nBlkSizeX != 8 || analysisData.nBlkSizeY != 4) &&
        ( analysisData.nBlkSizeX != 8 || analysisData.nBlkSizeY != 8 ) &&
        ( analysisData.nBlkSizeX != 16 || analysisData.nBlkSizeY != 2 ) &&
        ( analysisData.nBlkSizeX != 16 || analysisData.nBlkSizeY != 8 ) &&
        ( analysisData.nBlkSizeX != 16 || analysisData.nBlkSizeY != 16 ) &&
        ( analysisData.nBlkSizeX != 32 || analysisData.nBlkSizeY != 16))
		env->ThrowError("MAnalyse: Block's size must be 4x4, 8x4, 8x8, 16x2, 16x8, 16x16, 32x16");


   analysisData.nPel = nSuperPel;
	if (( analysisData.nPel != 1 ) && ( analysisData.nPel != 2 ) && ( analysisData.nPel != 4 ))
		env->ThrowError("MAnalyse: pel has to be 1 or 2 or 4");

   analysisData.nDeltaFrame = df;
//   if ( analysisData.nDeltaFrame < 1 )
//      analysisData.nDeltaFrame = 1;

   if (_overlapx<0 || _overlapx >= _blksizex || _overlapy<0 || _overlapy >= _blksizey)
		env->ThrowError("MAnalyse: overlap must be less than block size");

   if (_overlapx%2 || (_overlapy%2 >0 && vi.IsYV12()))
		env->ThrowError("MAnalyse: overlap must be more even");

	if (_divide != 0 && (_blksizex < 8 && _blksizey < 8) )
		env->ThrowError("MAnalyse: Block sizes must be 8 or more for divide mode");
   if (_divide != 0 && (_overlapx%4 || (_overlapy%4 >0 && vi.IsYV12() ) || (_overlapy%2 >0 && vi.IsYUY2() )  ))
		env->ThrowError("MAnalyse: overlap must be more even for divide mode");

    divideExtra = _divide;

    headerSize = max(4 + sizeof(analysisData), 256); // include itself, but usually equal to 256 :-)

	analysisData.nOverlapX = _overlapx;
	analysisData.nOverlapY = _overlapy;

	int nBlkX = (analysisData.nWidth - analysisData.nOverlapX) / (analysisData.nBlkSizeX - analysisData.nOverlapX);

	int nBlkY = (analysisData.nHeight - analysisData.nOverlapY) / (analysisData.nBlkSizeY - analysisData.nOverlapY);

	analysisData.nBlkX = nBlkX;
	analysisData.nBlkY = nBlkY;

	analysisData.nLvCount = ilog2(	((nBlkX) > (nBlkY)) ? (nBlkY) : (nBlkX) ) - lv;
	if (analysisData.nLvCount > nSuperLevels) analysisData.nLvCount = nSuperLevels; // v2.0
	analysisData.nLvCount = ( analysisData.nLvCount < 1 ) ? 1 : analysisData.nLvCount;

	analysisData.isBackward = isb;


   nLambda = lambda;
   lsad = _lsad*(_blksizex*_blksizey)/64;
   pnew = _pnew;
   plevel = _plevel;
   global = _global;
   pglobal = _pglobal;
   pzero = _pzero;
   badSAD = _badSAD*(_blksizex*_blksizey)/64;
   badrange = _badrange;

   if (_dctmode == 0)
   {
		hinstFFTW3 = NULL;
		DCTc = 0;
   }
   else
   {
	   if (_isse && (_blksizex == 8) && _blksizey ==8 )
			DCTc = new DCTINT(_blksizex, _blksizey, _dctmode);
	   else
	   {
			hinstFFTW3 = LoadLibrary("fftw3.dll"); // delayed loading
			if (hinstFFTW3==NULL) env->ThrowError("MAnalyse: Can not load FFTW3.DLL !");
			DCTc = new DCTFFTW(_blksizex, _blksizey, hinstFFTW3, _dctmode); // check order x,y
	   }
	}

	switch ( st )
	{
	case 0 :
		searchType = ONETIME;
		nSearchParam = ( stp < 1 ) ? 1 : stp;
		break;
	case 1 :
		searchType = NSTEP;
		nSearchParam = ( stp < 0 ) ? 0 : stp;
		break;
	case 3 :
		searchType = EXHAUSTIVE;
		nSearchParam = ( stp < 1 ) ? 1 : stp;
		break;
	case 4 :
		searchType = HEX2SEARCH;
		nSearchParam = ( stp < 1 ) ? 1 : stp;
		break;
	case 5 :
		searchType = UMHSEARCH;
		nSearchParam = ( stp < 1 ) ? 1 : stp; // really min is 4
		break;
	case 2 :
	default :
		searchType = LOGARITHMIC;
		nSearchParam = ( stp < 1 ) ? 1 : stp;
	}

    nPelSearch = ( _pelSearch <= 0) ?  analysisData.nPel : _pelSearch; // not below value of 0 at finest level


   analysisData.nFlags = 0;
   analysisData.nFlags |= _isse ? MOTION_USE_ISSE : 0;
   analysisData.nFlags |= analysisData.isBackward ? MOTION_IS_BACKWARD : 0;
   analysisData.nFlags |= chroma ? MOTION_USE_CHROMA_MOTION : 0;
/*
#define CPU_CACHELINE_32   0x00001000
#define CPU_CACHELINE_64   0x00002000
#define CPU_MMX            0x00004000
#define CPU_MMXEXT         0x00008000
#define CPU_SSE            0x00010000
#define CPU_SSE2           0x00020000
#define CPU_SSE2_IS_SLOW   0x00040000
#define CPU_SSE2_IS_FAST   0x00080000
#define CPU_SSE3           0x00100000
#define CPU_SSSE3          0x00200000
#define CPU_PHADD_IS_FAST  0x00400000
#define CPU_SSE4           0x00800000
//force MVAnalyse to use a different function for SAD / SADCHROMA (debug)
#define MOTION_USE_SSD     0x01000000
#define MOTION_USE_SATD    0x02000000
*/
	if (_sadx264 == 0)
	{
		analysisData.nFlags |= cpu_detect();
	}
	else
	{
		if ((_sadx264 > 0)&&(_sadx264 <= 12))
		{
			//force specific function
			analysisData.nFlags |= CPU_MMXEXT;
			analysisData.nFlags |= (_sadx264 == 2) ? CPU_CACHELINE_32 : 0;
			analysisData.nFlags |= ((_sadx264 == 3)||(_sadx264 == 5)||(_sadx264 == 7)) ? CPU_CACHELINE_64 : 0;
			analysisData.nFlags |= ((_sadx264 == 4)||(_sadx264 == 5)||(_sadx264 ==10)) ? CPU_SSE2_IS_FAST : 0;
			analysisData.nFlags |= (_sadx264 == 6) ? CPU_SSE3 : 0;
			analysisData.nFlags |= ((_sadx264 == 7)||(_sadx264 >=11)) ? CPU_SSSE3 : 0;
			//beta (debug)
			analysisData.nFlags |= (_sadx264 == 8) ? MOTION_USE_SSD : 0;
			analysisData.nFlags |= ((_sadx264 >= 9)&&(_sadx264 <= 12)) ? MOTION_USE_SATD : 0;
			analysisData.nFlags |= (_sadx264 ==12) ? CPU_PHADD_IS_FAST : 0;
		}
	}

/*	analysisData.usePelClip = false;
   if (pelclip && (analysisData.nPel >= 2))
   {
		if (pelclip->GetVideoInfo().width != vi.width*analysisData.nPel || pelclip->GetVideoInfo().height != vi.height*analysisData.nPel)
			env->ThrowError("MVAnalyse: pelclip frame size must be Pel of source!");
		else
			analysisData.usePelClip = true;
   }
*/

    nModeYUV = chroma ? YUVPLANES : YPLANE;
    if ((nModeYUV & nSuperModeYUV) != nModeYUV)
			env->ThrowError("MAnalyse: super clip does not contain needed color data");

	vectorFields = new GroupOfPlanes(analysisData.nWidth, analysisData.nHeight, analysisData.nBlkSizeX, analysisData.nBlkSizeY,
		analysisData.nLvCount, analysisData.nPel, analysisData.nFlags,
		analysisData.nOverlapX, analysisData.nOverlapY, analysisData.nBlkX, analysisData.nBlkY, analysisData.yRatioUV, divideExtra);

//   analysisData.nIdx = _idx;

   analysisData.nMagicKey = MOTION_MAGIC_KEY;

//   analysisData.pmvCore = &mvCore;

   analysisData.nHPadding = nSuperHPad; //v2.0
   analysisData.nVPadding = nSuperVPad;

//   mvCore.AddFrames(analysisData.nIdx, analysisData.nDeltaFrame*2+1, analysisData.nLvCount,
//	   analysisData.nWidth, analysisData.nHeight, analysisData.nPel, analysisData.nHPadding,
//	   analysisData.nVPadding, YUVPLANES, _isse, analysisData.yRatioUV);


//   if ( (analysisData.pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 )
//   {
//		SrcPlanes =  new YUY2Planes(nSuperWidth, nSuperHeight);
//		RefPlanes =  new YUY2Planes(nSuperWidth, nSuperHeight);
//   }

	analysisData.nVersion = MVANALYSIS_DATA_VERSION; // MVAnalysisData and outfile format version: last update v1.8.1
	//DebugPrintf(" MVAnalyseData size= %d",sizeof(analysisData));

    outfilename = _outfilename;
	if (lstrlen(outfilename) > 0) {
		outfile = fopen(outfilename,"wb");
		if (outfile == NULL)
			env->ThrowError("MAnalyse: out file can not be created!");
		else
		{
			fwrite( &analysisData, sizeof(analysisData), 1, outfile );
			outfilebuf = new short[nBlkX*nBlkY*4]; // short vx, short vy, int SAD = 4 words = 8 bytes per block
		}
	 }
	 else {
		 outfile = NULL;
		 outfilebuf = NULL;
	 }

//	analysisData.sharp = _sharp; // pel2 interpolation type

    // vector steam packed in
	vi.height = 1;
	vi.width = headerSize/sizeof(int) + vectorFields->GetArraySize(); //v1.8.1
	vi.pixel_type = VideoInfo::CS_BGR32;
    vi.audio_samples_per_second = 0; //v1.8.1

	if (divideExtra) { //v1.8.1
        memcpy(&analysisDataDivided, &analysisData, sizeof(analysisData));
        analysisDataDivided.nBlkX = analysisData.nBlkX * 2;
        analysisDataDivided.nBlkY = analysisData.nBlkY * 2;
        analysisDataDivided.nBlkSizeX = analysisData.nBlkSizeX / 2;
        analysisDataDivided.nBlkSizeY = analysisData.nBlkSizeY / 2;
        analysisDataDivided.nOverlapX = analysisData.nOverlapX / 2;
        analysisDataDivided.nOverlapY = analysisData.nOverlapY / 2;
        analysisDataDivided.nLvCount = analysisData.nLvCount + 1;
        vi.nchannels = reinterpret_cast<int>(&analysisDataDivided);
//        analysisDataDivided.pmvCore = &mvCore;
	}
	else
	{

   //  we'll transmit to the processing filters a handle
   // on the analyzing filter itself ( it's own pointer ), in order
   // to activate the right parameters.
        vi.nchannels = reinterpret_cast<int>(&analysisData);
	}



}

MVAnalyse::~MVAnalyse()
{
//   if ( (analysisData.pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 )
//   {
//	delete SrcPlanes;
//	delete RefPlanes;
//   }
	if (vectorFields)
		delete vectorFields;
	if (outfile != NULL) {
		fclose(outfile);
		delete [] outfilebuf;
	}
	if (DCTc)
		delete DCTc;
	if (hinstFFTW3 != NULL)
		FreeLibrary(hinstFFTW3);

    delete pSrcGOF;
    delete pRefGOF;
}

PVideoFrame __stdcall MVAnalyse::GetFrame(int n, IScriptEnvironment* env)
{
	const unsigned char *pSrcY, *pSrcU, *pSrcV;
	const unsigned char *pRefY, *pRefU, *pRefV;
//	const unsigned char *pSrc2xY, *pSrc2xU, *pSrc2xV;
//	const unsigned char *pRef2xY, *pRef2xU, *pRef2xV;
	unsigned char *pDst;
	int nSrcPitchY, nSrcPitchUV;
	int nRefPitchY, nRefPitchUV;
//	int nSrc2xPitchY, nSrc2xPitchUV;
//	int nRef2xPitchY, nRef2xPitchUV;

    int minframe, maxframe, nref;

    if (analysisData.nDeltaFrame > 0)
    {
        minframe = ( analysisData.isBackward ) ? 0 : analysisData.nDeltaFrame;
        maxframe = ( analysisData.isBackward ) ? vi.num_frames - analysisData.nDeltaFrame : vi.num_frames;
    int offset = ( analysisData.isBackward ) ? analysisData.nDeltaFrame : -analysisData.nDeltaFrame;
        nref = n + offset;
    }
    else // special static mode
    {
        nref = - analysisData.nDeltaFrame; // positive fixed frame number
        minframe = 0;
        maxframe = vi.num_frames;
    }

//	DebugPrintf("MVAnalyse: Get src frame %d",n);
	PVideoFrame src = child->GetFrame(n, env); // v2.0
	bool paritysrc = child->GetParity(n);
//	PVideoFrame src2x;
//	if (analysisData.usePelClip)
//		src2x = pelclip->GetFrame(n, env);

      PROFILE_START(MOTION_PROFILE_YUY2CONVERT);
	if ( (analysisData.pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 )
	{
/*		pSrcY = SrcPlanes->GetPtr();
		pSrcU = SrcPlanes->GetPtrU();
		pSrcV = SrcPlanes->GetPtrV();
		nSrcPitchY  = SrcPlanes->GetPitch();
		nSrcPitchUV  = SrcPlanes->GetPitchUV();
		YUY2ToPlanes(src->GetReadPtr(), src->GetPitch(), nSuperWidth, nSuperHeight,
			pSrcY, nSrcPitchY, pSrcU, pSrcV, nSrcPitchUV, isse);
*/
        // planer data packed to interleaved format (same as interleved2planar by kassandro) - v2.0.0.5
		pSrcY = src->GetReadPtr();
		pSrcU = pSrcY + src->GetRowSize()/2;
		pSrcV = pSrcU + src->GetRowSize()/4;
		nSrcPitchY  = src->GetPitch();
		nSrcPitchUV  = nSrcPitchY;
	}
	else
	{
		pSrcY = src->GetReadPtr(PLANAR_Y);
	    pSrcU = src->GetReadPtr(PLANAR_U);
	    pSrcV = src->GetReadPtr(PLANAR_V);
		nSrcPitchY = src->GetPitch(PLANAR_Y);
	    nSrcPitchUV = src->GetPitch(PLANAR_U);
	}
      PROFILE_STOP(MOTION_PROFILE_YUY2CONVERT);

	PVideoFrame	dst;

	dst = env->NewVideoFrame(vi);
	pDst = dst->GetWritePtr();

    // write analysis parameters as a header to frame

    memcpy(pDst, &headerSize, sizeof(int));

    if (divideExtra)
        memcpy(pDst+sizeof(int), &analysisDataDivided, sizeof(analysisData));
    else
        memcpy(pDst+sizeof(int), &analysisData, sizeof(analysisData));

    pDst += headerSize;

//    DebugPrintf("MVAnalyse: Get ref frame %d",n + offset);
//        DebugPrintf("MVAnalyse frame %i backward=%i", n, analysisData.isBackward);


	if (( n < maxframe ) && ( n >= minframe ))
	{
		PVideoFrame ref = child->GetFrame(nref, env);
		bool parityref = child->GetParity(nref);

		int fieldShift = 0;
		if (vi.IsFieldBased() && analysisData.nPel > 1 && (analysisData.nDeltaFrame %2 != 0))
		{
				fieldShift = (paritysrc && !parityref) ? analysisData.nPel/2 : ( (parityref && !paritysrc) ? -(analysisData.nPel/2) : 0);
				// vertical shift of fields for fieldbased video at finest level pel2
		}
      PROFILE_START(MOTION_PROFILE_YUY2CONVERT);
		if ( (analysisData.pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 )
		{
/*			pRefY = RefPlanes->GetPtr();
			pRefU = RefPlanes->GetPtrU();
			pRefV = RefPlanes->GetPtrV();
			nRefPitchY  = RefPlanes->GetPitch();
			nRefPitchUV  = RefPlanes->GetPitchUV();
			YUY2ToPlanes(ref->GetReadPtr(), ref->GetPitch(), nSuperWidth, nSuperHeight,
				pRefY, nRefPitchY, pRefU, pRefV, nRefPitchUV, isse);
*/
        // planer data packed to interleaved format (same as interleved2planar by kassandro) - v2.0.0.5
		pRefY = ref->GetReadPtr();
		pRefU = pRefY + ref->GetRowSize()/2;
		pRefV = pRefU + ref->GetRowSize()/4;
		nRefPitchY  = ref->GetPitch();
		nRefPitchUV  = nRefPitchY;
		}
		else
		{
		  pRefY = ref->GetReadPtr(PLANAR_Y);
		  pRefU = ref->GetReadPtr(PLANAR_U);
		  pRefV = ref->GetReadPtr(PLANAR_V);
		  nRefPitchY = ref->GetPitch(PLANAR_Y);
		  nRefPitchUV = ref->GetPitch(PLANAR_U);
		}
      PROFILE_STOP(MOTION_PROFILE_YUY2CONVERT);

/*
      MVFrames *pFrames = mvCore.GetFrames(analysisData.nIdx);

      PROFILE_START(MOTION_PROFILE_INTERPOLATION);
      PMVGroupOfFrames pSrcGOF = pFrames->GetFrame(n);
      pSrcGOF->SetPlane(pSrcY, nSrcPitchY, YPLANE);
      pSrcGOF->SetPlane(pSrcU, nSrcPitchUV, UPLANE);
      pSrcGOF->SetPlane(pSrcV, nSrcPitchUV, VPLANE);
      pSrcGOF->Reduce(rfilter);
      pSrcGOF->Pad(YUVPLANES);

		  pSrcGOF->Refine(YUVPLANES, analysisData.sharp);


      PMVGroupOfFrames pRefGOF = pFrames->GetFrame(n + offset);
      pRefGOF->SetPlane(pRefY, nRefPitchY, YPLANE);
      pRefGOF->SetPlane(pRefU, nRefPitchUV, UPLANE);
      pRefGOF->SetPlane(pRefV, nRefPitchUV, VPLANE);
      pRefGOF->Reduce(rfilter);
      pRefGOF->Pad(YUVPLANES);

	      pRefGOF->Refine(YUVPLANES, analysisData.sharp);

      PROFILE_STOP(MOTION_PROFILE_INTERPOLATION);
*/
      pSrcGOF->Update(nModeYUV, (BYTE*)pSrcY, nSrcPitchY, (BYTE*)pSrcU, nSrcPitchUV, (BYTE*)pSrcV, nSrcPitchUV); // v2.0
      pRefGOF->Update(nModeYUV, (BYTE*)pRefY, nRefPitchY, (BYTE*)pRefU, nRefPitchUV, (BYTE*)pRefV, nRefPitchUV); // v2.0

	  if (outfile != NULL) {
			fwrite( &n, sizeof( int ), 1, outfile );// write frame number
	  }

      vectorFields->SearchMVs(pSrcGOF, pRefGOF, searchType, nSearchParam, nPelSearch, nLambda, lsad, pnew, plevel, global,
		  analysisData.nFlags, reinterpret_cast<int*>(pDst), outfilebuf, fieldShift, DCTc, pzero, pglobal, badSAD, badrange);

    if (divideExtra) {
      // make extra level with divided sublocks with median (not estimated) motion
      vectorFields->ExtraDivide(reinterpret_cast<int*>(pDst), analysisData.nFlags);
    }

//      PROFILE_CUMULATE();
		if (outfile != NULL)
			fwrite( outfilebuf, sizeof(short)* analysisData.nBlkX*analysisData.nBlkY*4, 1, outfile );

	}
	else
	{
		vectorFields->WriteDefaultToArray(reinterpret_cast<int*>(pDst));
	}

	return dst;
}

// MVTools
// 2004 Manao
// Copyright(c)2006 A.G.Balakhnin aka Fizick - true motion, overlap, YUY2, pelclip, divide
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
#include "dctfftw.h"
#include "dctint.h"
#include "cpu.h"

#include <stdio.h>
#include <math.h>
#include <algorithm>

extern MVCore mvCore;

MVAnalyse::MVAnalyse(PClip _child, int _blksizex, int _blksizey, int pel, int lv, int st, int stp, 
                     int _pelSearch, bool isb, int lambda, bool chroma, int df, int _lsad, int _pnew,
                     int _plevel, bool _global, int _overlapx, int _overlapy, const char* _outfilename,
                     int _sharp, PClip _pelclip, int _dctmode, int _divide, int _idx, int _sadx264,
                     bool _mmx, bool _isse, IScriptEnvironment* env) :
    GenericVideoFilter(_child),
    //analysisData(),
    outfilename(_outfilename), pelclip(_pelclip)
{
    analysisData.nWidth = vi.width;

    analysisData.nHeight = vi.height;

    analysisData.nBlkSizeX = _blksizex;
    analysisData.nBlkSizeY = _blksizey;
    if (( analysisData.nBlkSizeX != 4 || analysisData.nBlkSizeY != 4) &&
        ( analysisData.nBlkSizeX != 8 || analysisData.nBlkSizeY != 4) &&
        ( analysisData.nBlkSizeX != 8 || analysisData.nBlkSizeY != 8 ) &&
        ( analysisData.nBlkSizeX != 16 || analysisData.nBlkSizeY != 2 ) &&
        ( analysisData.nBlkSizeX != 16 || analysisData.nBlkSizeY != 8 ) &&
        ( analysisData.nBlkSizeX != 16 || analysisData.nBlkSizeY != 16 ) &&
        ( analysisData.nBlkSizeX != 32 || analysisData.nBlkSizeY != 16))
        env->ThrowError("MVAnalyse: Block's size must be 4x4, 8x4, 8x8, 16x2, 16x8, 16x16, 32x16");

    if (!vi.IsYV12() && !vi.IsYUY2())
        env->ThrowError("MVAnalyse: Clip must be YV12 or YUY2");

    analysisData.nPel = pel;
    if (( analysisData.nPel != 1 ) && ( analysisData.nPel != 2 ) && ( analysisData.nPel != 4 ))
        env->ThrowError("MVAnalyse: pel has to be 1 or 2 or 4");

    analysisData.nDeltaFrame = df;
    if ( analysisData.nDeltaFrame < 1 )
        analysisData.nDeltaFrame = 1;
    //   if ( analysisData.nDeltaFrame > 8 )
    //      analysisData.nDeltaFrame = 8;

    if (_overlapx<0 || _overlapx >= _blksizex || _overlapy<0 || _overlapy >= _blksizey)
        env->ThrowError("MVAnalyse: overlap must be less than block size");

    if (_overlapx%2 || (_overlapy%2 >0 && vi.IsYV12()))
        env->ThrowError("MVAnalyse: overlap must be more even");

    if (_divide != 0 && (_blksizex < 8 && _blksizey < 8) )
        env->ThrowError("MVAnalyse: Block sizes must be 8 or more for divide mode");
    if (_divide != 0 && (_overlapx%4 || (_overlapy%4 >0 && vi.IsYV12() ) || (_overlapy%2 >0 && vi.IsYUY2() )  ))
        env->ThrowError("MVAnalyse: overlap must be more even for divide mode");

    divideExtra = _divide;

    headerSize = std::max<int>(4 + sizeof(analysisData), 256); // include itself, but usually equal to 256 :-)

    analysisData.nOverlapX = _overlapx;
    analysisData.nOverlapY = _overlapy;

    int nBlkX = (analysisData.nWidth - analysisData.nOverlapX) / (analysisData.nBlkSizeX - analysisData.nOverlapX);
    //	if (analysisData.nWidth > (analysisData.nBlkSize - analysisData.nOverlap) * nBlkX )
    //		nBlkX += 1;

    int nBlkY = (analysisData.nHeight - analysisData.nOverlapY) / (analysisData.nBlkSizeY - analysisData.nOverlapY);
    //	if (analysisData.nHeight > (analysisData.nBlkSize - analysisData.nOverlap) * nBlkY )
    //		nBlkY += 1;

    analysisData.nBlkX = nBlkX;
    analysisData.nBlkY = nBlkY;
    int nWidth_B = nBlkX*(analysisData.nBlkSizeX - analysisData.nOverlapX) + analysisData.nOverlapX;
    int nHeight_B = nBlkY*(analysisData.nBlkSizeY - analysisData.nOverlapY) + analysisData.nOverlapY;

    analysisData.nLvCount = ilog2(	((nBlkX) > (nBlkY)) ? (nBlkY) : (nBlkX) ) - lv;
    analysisData.nLvCount = ( analysisData.nLvCount < 1 ) ? 1 : analysisData.nLvCount;

    analysisData.isBackward = isb;


    nLambda = lambda;
    lsad = _lsad;
    pnew = _pnew;
    plevel = _plevel;
    global = _global;

    mmx = _mmx && ( env->GetCPUFlags() & CPUF_MMX );

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
            if (hinstFFTW3==NULL) env->ThrowError("MVAnalyse: Can not load FFTW3.DLL !");
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
    case 2 :
    default :
        searchType = LOGARITHMIC;
        nSearchParam = ( stp < 1 ) ? 1 : stp;
    }

    nPelSearch = ( _pelSearch < pel) ? pel : _pelSearch; // not below value of pel at finest level

    analysisData.pixelType = vi.pixel_type;
    analysisData.yRatioUV = (vi.IsYV12()) ? 2 : 1;
    analysisData.xRatioUV = 2; // for YV12 and YUY2, really do not used and assumed to 2

    analysisData.nFlags = 0;
    analysisData.nFlags |= _isse ? MOTION_USE_ISSE : 0;
    analysisData.nFlags |= _mmx ? MOTION_USE_MMX : 0;
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

    analysisData.usePelClip = false;
    if (pelclip && (analysisData.nPel >= 2))
    {
        if (pelclip->GetVideoInfo().width != vi.width*analysisData.nPel || pelclip->GetVideoInfo().height != vi.height*analysisData.nPel)
            env->ThrowError("MVAnalyse: pelclip frame size must be Pel of source!");
        else
            analysisData.usePelClip = true;
    }

    vectorFields = new GroupOfPlanes(analysisData.nWidth, analysisData.nHeight, analysisData.nBlkSizeX, analysisData.nBlkSizeY,
                                     analysisData.nLvCount, analysisData.nPel, analysisData.nFlags,
                                     analysisData.nOverlapX, analysisData.nOverlapY, analysisData.nBlkX, analysisData.nBlkY, 
                                     analysisData.yRatioUV, divideExtra);

    analysisData.nIdx = _idx;

    analysisData.nMagicKey = MOTION_MAGIC_KEY;

    analysisData.pmvCore = &mvCore;

    analysisData.nHPadding = analysisData.nBlkSizeX; //v1.8.1
    analysisData.nVPadding = analysisData.nBlkSizeY;
    mvCore.AddFrames(analysisData.nIdx, analysisData.nDeltaFrame*2+1, analysisData.nLvCount,
					 analysisData.nWidth, analysisData.nHeight, analysisData.nPel, analysisData.nHPadding,
					 analysisData.nVPadding, YUVPLANES, _isse, analysisData.yRatioUV);

    analysisData.nVersion = MVANALYSIS_DATA_VERSION; // MVAnalysisData and outfile format version: last update v1.8.1
    //DebugPrintf(" MVAnalyseData size= %d",sizeof(analysisData));

    if (lstrlen(outfilename) > 0) {
        fopen_s(&outfile, outfilename,"wb");
        if (outfile == NULL)
            env->ThrowError("MVAnalyse: out file can not be created!");
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

    analysisData.sharp = _sharp; // pel2 interpolation type

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
        analysisDataDivided.pmvCore = &mvCore;
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
    if (vectorFields) delete vectorFields;
    if (outfile != NULL) {
        fclose(outfile);
        if (outfilebuf) delete [] outfilebuf;
    }
    if (DCTc) delete DCTc;
    if (hinstFFTW3 != NULL) FreeLibrary(hinstFFTW3);
}

PVideoFrame __stdcall MVAnalyse::GetFrame(int n, IScriptEnvironment* env)
{
    // get source frame
    PMVGroupOfFrames pSrcGOF = mvCore.GetFrame(analysisData.nIdx, n);
    if (!pSrcGOF->IsProcessed()) {
        PVideoFrame src, src2x;
        src = child->GetFrame(n, env);
        pSrcGOF->SetParity(child->GetParity(n));
        if (analysisData.usePelClip) 
	        src2x = pelclip->GetFrame(n, env);
        ProcessFrameIntoGroupOfFrames(&pSrcGOF, &src, &src2x, analysisData.sharp, analysisData.pixelType,
                                      analysisData.nHeight, analysisData.nWidth, analysisData.nPel, isse);
    }

    int offset = ( analysisData.isBackward ) ? analysisData.nDeltaFrame : -analysisData.nDeltaFrame;

    // get reference frames
    PMVGroupOfFrames pRefGOF;
    int FrameNum=n+offset;
    if (FrameNum>=0 && FrameNum<vi.num_frames) {
        pRefGOF = mvCore.GetFrame(analysisData.nIdx, FrameNum);
        if (!pRefGOF->IsProcessed()) {
            PVideoFrame ref, ref2x;
            ref = child->GetFrame(FrameNum, env);
            pRefGOF->SetParity(child->GetParity(FrameNum));
            if (analysisData.usePelClip)
                ref2x= pelclip->GetFrame(FrameNum, env);

            ProcessFrameIntoGroupOfFrames(&pRefGOF, &ref, &ref2x, analysisData.sharp, analysisData.pixelType,
                                          analysisData.nHeight, analysisData.nWidth, analysisData.nPel, isse);
        }
    }

    // get destination frame
    PVideoFrame dst = env->NewVideoFrame(vi);
    unsigned char *pDst = dst->GetWritePtr();

    // write analysis parameters as a header to frame

    memcpy(pDst, &headerSize, sizeof(int));

    if (divideExtra)
        memcpy(pDst+sizeof(int), &analysisDataDivided, sizeof(analysisData));
    else
        memcpy(pDst+sizeof(int), &analysisData, sizeof(analysisData));

    pDst += headerSize;

    //    DebugPrintf("MVAnalyse: Get ref frame %d",n + offset);
    int minframe = ( analysisData.isBackward ) ? 0 : analysisData.nDeltaFrame;
    int maxframe = ( analysisData.isBackward ) ? vi.num_frames - analysisData.nDeltaFrame : vi.num_frames;
    if (( n < maxframe ) && ( n >= minframe ))
    {
        int fieldShift = 0;
        if (vi.IsFieldBased() && analysisData.nPel > 1 && (analysisData.nDeltaFrame %2 != 0))
        {
            fieldShift = (pSrcGOF->Parity() && !pRefGOF->Parity() ) ? 
                         analysisData.nPel/2 : ((pRefGOF->Parity()  && !pSrcGOF->Parity() ) ? -(analysisData.nPel/2) : 0);
            // vertical shift of fields for fieldbased video at finest level pel2
        }

        if (outfile != NULL) {
            fwrite( &n, sizeof( int ), 1, outfile );// write frame number
        }

        vectorFields->SearchMVs(pSrcGOF, pRefGOF, searchType, nSearchParam, nPelSearch, nLambda, lsad, pnew, plevel, global,
								analysisData.nFlags, reinterpret_cast<int*>(pDst), outfilebuf, fieldShift, DCTc);

        if (divideExtra) {
            // make extra level with divided sublocks with median (not estimated) motion
            vectorFields->ExtraDivide(reinterpret_cast<int*>(pDst), analysisData.nFlags);
        }

        PROFILE_CUMULATE();
        if (outfile != NULL)
            fwrite( outfilebuf, sizeof(short)* analysisData.nBlkX*analysisData.nBlkY*4, 1, outfile );

    }
    else
    {
        vectorFields->WriteDefaultToArray(reinterpret_cast<int*>(pDst));
    }

    return dst;
}

void MVAnalyse::ProcessFrameIntoGroupOfFrames(PMVGroupOfFrames *srcGOF, PVideoFrame *src, PVideoFrame *src2x, 
                                              int const Sharp, int const pixelType, int const nHeight, int const nWidth, 
                                              int const nPel, bool const isse)
{
    (*srcGOF)->ConvertVideoFrame(src, src2x, pixelType, nWidth, nHeight, nPel, isse);

    PROFILE_START(MOTION_PROFILE_INTERPOLATION);
    (*srcGOF)->Reduce();
    (*srcGOF)->Pad(YUVPLANES);

    if (*src2x) {
        MVFrame *srcFrames = (*srcGOF)->GetFrame(0);
        MVPlane *srcPlaneY = srcFrames->GetPlane(YPLANE);
        srcPlaneY->RefineExt((*srcGOF)->GetVF2xYPtr(), (*srcGOF)->GetVF2xYPitch());
        MVPlane *srcPlaneU = srcFrames->GetPlane(UPLANE);
        srcPlaneU->RefineExt((*srcGOF)->GetVF2xUPtr(), (*srcGOF)->GetVF2xUPitch());
        MVPlane *srcPlaneV = srcFrames->GetPlane(VPLANE);
        srcPlaneV->RefineExt((*srcGOF)->GetVF2xVPtr(), (*srcGOF)->GetVF2xVPitch());
    }
    else
        (*srcGOF)->Refine(YUVPLANES, Sharp);

    PROFILE_STOP(MOTION_PROFILE_INTERPOLATION);

    // set processed
    (*srcGOF)->SetProcessed();
}

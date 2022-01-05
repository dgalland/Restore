// Author: Manao
// Copyright(c)2006 A.G.Balakhnin aka Fizick - bicubic, wiener
// See legal notice in Copying.txt for more information
//
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

#include "MVInterface.h"
#include "Padding.h"
#include "CopyCode.h"
#include "Interpolation.h"

using namespace ThreadFunctions;

/******************************************************************************
*                                                                             *
*  MVPlane : manages a single plane, allowing padding and refinin             *
*                                                                             *
******************************************************************************/

MVPlane::MVPlane(int _nWidth, int _nHeight, int _nPel, int _nHPad, int _nVPad, bool _isse)
{
    nWidth = _nWidth;
    nHeight = _nHeight;
    nPel = _nPel;
    nHPadding = _nHPad;
    nVPadding = _nVPad;
    isse = _isse;
    nHPaddingPel = _nHPad * nPel;
    nVPaddingPel = _nVPad * nPel;

    nExtendedWidth = nWidth + 2 * nHPadding;
    nExtendedHeight = nHeight + 2 * nVPadding;

    // round pitches to be mod 64
    unsigned int PitchAlignment=64;
    nPitch = (nExtendedWidth + (PitchAlignment-1)) & (~(PitchAlignment-1)); //ensure that each row gets aligned (ie. row length in memory is on a border)
    nOffsetPadding = nPitch * nVPadding + nHPadding;

    // round plane sizes to be mod 128
    unsigned int PlaneSizeAlignment=128;
    unsigned int PlaneSize=(nPitch * nExtendedHeight + (PlaneSizeAlignment-1)) & (~(PlaneSizeAlignment-1));

    // alocate planes
    unsigned int numberOfPlanes = nPel * nPel;
    pPlaneBase = (Uint8*) _aligned_malloc(numberOfPlanes * PlaneSize, PlaneSizeAlignment); // create all planes aligned to 128
    if (pPlaneBase) {
        // store offsets
        pPlane = new Uint8 *[nPel * nPel]; // store offsets
        for(unsigned int PlaneNum=0; PlaneNum<numberOfPlanes; ++PlaneNum) {
            pPlane[PlaneNum]=pPlaneBase+PlaneSize*PlaneNum;
        }
    }
    else {
        // no memory 
        throw "Out of memroy MVFrame.cpp";
    }

    // reset the state
    ResetState();
}

MVPlane::~MVPlane()
{
    if (pPlaneBase) {
        if (pPlane)     delete [] pPlane;
        _aligned_free(pPlaneBase);;
    }
}

void MVPlane::ChangePlane(const Uint8 *pNewPlane, int nNewPitch)
{
    CriticalSectionAuto AutoCS(cs);
    if ( !isFilled )
        PlaneCopy(pPlane[0] + nOffsetPadding, nPitch, pNewPlane, nNewPitch, nWidth, nHeight, isse);
    isFilled = true;
}

void MVPlane::Pad()
{
    CriticalSectionAuto AutoCS(cs);
    if ( !isPadded )
        Padding::PadReferenceFrame(pPlane[0], nPitch, nHPadding, nVPadding, nWidth, nHeight);
    isPadded = true;
}

void MVPlane::Refine(int sharp)
{
    CriticalSectionAuto AutoCS(cs);
    if (( nPel == 2 ) && ( !isRefined )) {
        if (sharp == 0) // bilinear
        {
            if (isse) {
                HorizontalBilin_iSSE(pPlane[1], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                VerticalBilin_iSSE(pPlane[2], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                DiagonalBilin_iSSE(pPlane[3], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
            }
            else {
                HorizontalBilin(pPlane[1], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                VerticalBilin(pPlane[2], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                DiagonalBilin(pPlane[3], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
            }
        }
        else if(sharp==1) // bicubic
        {
            if (isse) {
                HorizontalBicubic_iSSE(pPlane[1], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                VerticalBicubic_iSSE(pPlane[2], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                HorizontalBicubic_iSSE(pPlane[3], pPlane[2], nPitch, nPitch, nExtendedWidth, nExtendedHeight); // faster from ready-made horizontal
            }
            else {
                HorizontalBicubic(pPlane[1], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                VerticalBicubic(pPlane[2], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                HorizontalBicubic(pPlane[3], pPlane[2], nPitch, nPitch, nExtendedWidth, nExtendedHeight); // faster from ready-made horizontal
                //         DiagonalBicubic(pPlane[3], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
            }
        }
        else // Wiener
        {
            if (isse) {
                HorizontalWiener_iSSE(pPlane[1], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                VerticalWiener_iSSE(pPlane[2], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                HorizontalWiener_iSSE(pPlane[3], pPlane[2], nPitch, nPitch, nExtendedWidth, nExtendedHeight);// faster from ready-made horizontal
            }
            else {
                HorizontalWiener(pPlane[1], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                VerticalWiener(pPlane[2], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                HorizontalWiener(pPlane[3], pPlane[2], nPitch, nPitch, nExtendedWidth, nExtendedHeight); // faster from ready-made horizontal
                //         DiagonalWiener(pPlane[3], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
            }
        }
    }
    else if (( nPel == 4 ) && ( !isRefined )) // firstly pel2 interpolation
    {
        if (sharp == 0) // bilinear
        {
            if (isse) {
                HorizontalBilin_iSSE(pPlane[2], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                VerticalBilin_iSSE(pPlane[8], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                DiagonalBilin_iSSE(pPlane[10], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
            }
            else {
                HorizontalBilin(pPlane[2], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                VerticalBilin(pPlane[8], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                DiagonalBilin(pPlane[10], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
            }
        }
        else if(sharp==1) // bicubic
        {
            if (isse) {
                HorizontalBicubic_iSSE(pPlane[2], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                VerticalBicubic_iSSE(pPlane[8], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                HorizontalBicubic_iSSE(pPlane[10], pPlane[8], nPitch, nPitch, nExtendedWidth, nExtendedHeight); // faster from ready-made horizontal
            }
            else {
                HorizontalBicubic(pPlane[2], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                VerticalBicubic(pPlane[8], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                HorizontalBicubic(pPlane[10], pPlane[8], nPitch, nPitch, nExtendedWidth, nExtendedHeight); // faster from ready-made horizontal
                //         DiagonalBicubic(pPlane[3], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
            }
        }
        else // Wiener
        {
            if (isse) {
                HorizontalWiener_iSSE(pPlane[2], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                VerticalWiener_iSSE(pPlane[8], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                HorizontalWiener_iSSE(pPlane[10], pPlane[8], nPitch, nPitch, nExtendedWidth, nExtendedHeight);// faster from ready-made horizontal
            }
            else {
                HorizontalWiener(pPlane[2], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                VerticalWiener(pPlane[8], pPlane[0], nPitch, nPitch, nExtendedWidth, nExtendedHeight);
                HorizontalWiener(pPlane[10], pPlane[8], nPitch, nPitch, nExtendedWidth, nExtendedHeight); // faster from ready-made horizontal
            }
        }
        // now interpolate intermediate
        Average2(pPlane[1], pPlane[0], pPlane[2], nPitch, nExtendedWidth, nExtendedHeight);
        Average2(pPlane[9], pPlane[8], pPlane[10], nPitch, nExtendedWidth, nExtendedHeight);
        Average2(pPlane[4], pPlane[0], pPlane[8], nPitch, nExtendedWidth, nExtendedHeight);
        Average2(pPlane[6], pPlane[2], pPlane[10], nPitch, nExtendedWidth, nExtendedHeight);
        Average2(pPlane[5], pPlane[4], pPlane[6], nPitch, nExtendedWidth, nExtendedHeight);

        Average2(pPlane[3], pPlane[0] + 1, pPlane[2], nPitch, nExtendedWidth-1, nExtendedHeight);
        Average2(pPlane[11], pPlane[8] + 1, pPlane[10], nPitch, nExtendedWidth-1, nExtendedHeight);
        Average2(pPlane[12], pPlane[0] + nPitch, pPlane[8], nPitch, nExtendedWidth, nExtendedHeight-1);
        Average2(pPlane[14], pPlane[2] + nPitch, pPlane[10], nPitch, nExtendedWidth, nExtendedHeight-1);
        Average2(pPlane[13], pPlane[12], pPlane[14], nPitch, nExtendedWidth, nExtendedHeight);
        Average2(pPlane[7], pPlane[4] + 1, pPlane[6], nPitch, nExtendedWidth-1, nExtendedHeight);
        Average2(pPlane[15], pPlane[12] + 1, pPlane[14], nPitch, nExtendedWidth-1, nExtendedHeight);

    }
    isRefined = true;
}

void MVPlane::RefineExt(const Uint8 *pSrc2x, int nSrc2xPitch) // copy from external upsized clip
{
    CriticalSectionAuto AutoCS(cs);
    if (( nPel == 2 ) && ( !isRefined )) {
        Uint8* pp1 = pPlane[1] + nPitch*nVPadding + nHPadding;
        Uint8* pp2 = pPlane[2] + nPitch*nVPadding + nHPadding;
        Uint8* pp3 = pPlane[3] + nPitch*nVPadding + nHPadding;

        int nPitch1=nPitch-nWidth;
        for (int h=0; h<nHeight; ++h) // assembler optimization?
        {
            for (int w=0; w<nWidth; ++w) {
                int wx2=w<<1;
                int w3=wx2+nSrc2xPitch;
                *(pp1++) = pSrc2x[++wx2];
                *(pp2++) = pSrc2x[w3];
                *(pp3++) = pSrc2x[++w3];
            }
            pp1 += nPitch1;
            pp2 += nPitch1;
            pp3 += nPitch1;
            pSrc2x += nSrc2xPitch<<1;
        }
        Padding::PadReferenceFrame(pPlane[1], nPitch, nHPadding, nVPadding, nWidth, nHeight);
        Padding::PadReferenceFrame(pPlane[2], nPitch, nHPadding, nVPadding, nWidth, nHeight);
        Padding::PadReferenceFrame(pPlane[3], nPitch, nHPadding, nVPadding, nWidth, nHeight);
        isPadded = true;
    }
    else if (( nPel == 4 ) && ( !isRefined ))
    {
        Uint8* pp1 = pPlane[1] + nPitch*nVPadding + nHPadding;
        Uint8* pp2 = pPlane[2] + nPitch*nVPadding + nHPadding;
        Uint8* pp3 = pPlane[3] + nPitch*nVPadding + nHPadding;
        Uint8* pp4 = pPlane[4] + nPitch*nVPadding + nHPadding;
        Uint8* pp5 = pPlane[5] + nPitch*nVPadding + nHPadding;
        Uint8* pp6 = pPlane[6] + nPitch*nVPadding + nHPadding;
        Uint8* pp7 = pPlane[7] + nPitch*nVPadding + nHPadding;
        Uint8* pp8 = pPlane[8] + nPitch*nVPadding + nHPadding;
        Uint8* pp9 = pPlane[9] + nPitch*nVPadding + nHPadding;
        Uint8* pp10 = pPlane[10] + nPitch*nVPadding + nHPadding;
        Uint8* pp11 = pPlane[11] + nPitch*nVPadding + nHPadding;
        Uint8* pp12 = pPlane[12] + nPitch*nVPadding + nHPadding;
        Uint8* pp13 = pPlane[13] + nPitch*nVPadding + nHPadding;
        Uint8* pp14 = pPlane[14] + nPitch*nVPadding + nHPadding;
        Uint8* pp15 = pPlane[15] + nPitch*nVPadding + nHPadding;

        int nPitch1=nPitch-nWidth;
        for (int h=0; h<nHeight; ++h) // assembler optimization?
        {
            for (int w=0; w<nWidth; ++w) {
                int wx4=w<<2;
                int w5=wx4 + nSrc2xPitch;
                int w6=w5 + nSrc2xPitch;
                int w7=w6 + nSrc2xPitch;
                *(pp1++)  = pSrc2x[++wx4];
                *(pp2++)  = pSrc2x[++wx4];
                *(pp3++)  = pSrc2x[++wx4];
                *(pp4++)  = pSrc2x[w5++];
                *(pp5++)  = pSrc2x[w5++];
                *(pp6++)  = pSrc2x[w5++];
                *(pp7++)  = pSrc2x[w5];
                *(pp8++)  = pSrc2x[w6++];
                *(pp9++)  = pSrc2x[w6++];
                *(pp10++) = pSrc2x[w6++];
                *(pp11++) = pSrc2x[w6];
                *(pp12++) = pSrc2x[w7++];
                *(pp13++) = pSrc2x[w7++];
                *(pp14++) = pSrc2x[w7++];
                *(pp15++) = pSrc2x[w7];
            }
            pp1  += nPitch1;
            pp2  += nPitch1;
            pp3  += nPitch1;
            pp4  += nPitch1;
            pp5  += nPitch1;
            pp6  += nPitch1;
            pp7  += nPitch1;
            pp8  += nPitch1;
            pp9  += nPitch1;
            pp10 += nPitch1;
            pp11 += nPitch1;
            pp12 += nPitch1;
            pp13 += nPitch1;
            pp14 += nPitch1;
            pp15 += nPitch1;
            pSrc2x += nSrc2xPitch<<2;
        }
        Padding::PadReferenceFrame(pPlane[1], nPitch, nHPadding, nVPadding, nWidth, nHeight);
        Padding::PadReferenceFrame(pPlane[2], nPitch, nHPadding, nVPadding, nWidth, nHeight);
        Padding::PadReferenceFrame(pPlane[3], nPitch, nHPadding, nVPadding, nWidth, nHeight);
        Padding::PadReferenceFrame(pPlane[4], nPitch, nHPadding, nVPadding, nWidth, nHeight);
        Padding::PadReferenceFrame(pPlane[5], nPitch, nHPadding, nVPadding, nWidth, nHeight);
        Padding::PadReferenceFrame(pPlane[6], nPitch, nHPadding, nVPadding, nWidth, nHeight);
        Padding::PadReferenceFrame(pPlane[7], nPitch, nHPadding, nVPadding, nWidth, nHeight);
        Padding::PadReferenceFrame(pPlane[8], nPitch, nHPadding, nVPadding, nWidth, nHeight);
        Padding::PadReferenceFrame(pPlane[9], nPitch, nHPadding, nVPadding, nWidth, nHeight);
        Padding::PadReferenceFrame(pPlane[10], nPitch, nHPadding, nVPadding, nWidth, nHeight);
        Padding::PadReferenceFrame(pPlane[11], nPitch, nHPadding, nVPadding, nWidth, nHeight);
        Padding::PadReferenceFrame(pPlane[12], nPitch, nHPadding, nVPadding, nWidth, nHeight);
        Padding::PadReferenceFrame(pPlane[13], nPitch, nHPadding, nVPadding, nWidth, nHeight);
        Padding::PadReferenceFrame(pPlane[14], nPitch, nHPadding, nVPadding, nWidth, nHeight);
        Padding::PadReferenceFrame(pPlane[15], nPitch, nHPadding, nVPadding, nWidth, nHeight);
        isPadded = true;
    }
    isRefined = true;
}

void MVPlane::ReduceTo(MVPlane *pReducedPlane)
{
    CriticalSectionAuto AutoCS(cs);
    if ( !pReducedPlane->isFilled ) {
        if (isse) {
            RB2F_iSSE(pReducedPlane->pPlane[0] + pReducedPlane->nOffsetPadding, pPlane[0] + nOffsetPadding,
                      pReducedPlane->nPitch, nPitch, pReducedPlane->nWidth, pReducedPlane->nHeight);
        }
        else {
            RB2F_C(pReducedPlane->pPlane[0] + pReducedPlane->nOffsetPadding, pPlane[0] + nOffsetPadding,
                   pReducedPlane->nPitch, nPitch, pReducedPlane->nWidth, pReducedPlane->nHeight);
        }
    }
    pReducedPlane->isFilled = true;
}

void MVPlane::WritePlane(FILE *pFile)
{
    for ( int i = 0; i < nHeight; ++i )
        fwrite(pPlane[0] + i * nPitch + nOffsetPadding, 1, nWidth, pFile);
}

/******************************************************************************
*                                                                             *
*  MVFrame : a MVFrame is a threesome of MVPlane, some undefined, some        *
*  defined, according to the nMode value                                      *
*                                                                             *
******************************************************************************/

MVFrame::MVFrame(int nWidth, int nHeight, int nPel, int nHPad, int nVPad, int _nMode, bool _isse, int _yRatioUV)
{
    nMode = _nMode;
    isse = _isse;
    yRatioUV = _yRatioUV;

    if ( nMode & YPLANE )
        pYPlane = new MVPlane(nWidth, nHeight, nPel, nHPad, nVPad, isse);
    else
        pYPlane = 0;

    if ( nMode & UPLANE )
        pUPlane = new MVPlane(nWidth / 2, nHeight / yRatioUV, nPel, nHPad / 2, nVPad / yRatioUV, isse);
    else
        pUPlane = 0;

    if ( nMode & VPLANE )
        pVPlane = new MVPlane(nWidth / 2, nHeight / yRatioUV, nPel, nHPad / 2, nVPad / yRatioUV, isse);
    else
        pVPlane = 0;
}

MVFrame::~MVFrame()
{
    if ( nMode & YPLANE )
        delete pYPlane;

    if ( nMode & UPLANE )
        delete pUPlane;

    if ( nMode & VPLANE )
        delete pVPlane;
}

void MVFrame::ChangePlane(const unsigned char *pNewPlane, int nNewPitch, MVPlaneSet _nMode)
{
    if ( _nMode & nMode & YPLANE )
        pYPlane->ChangePlane(pNewPlane, nNewPitch);

    if ( _nMode & nMode & UPLANE )
        pUPlane->ChangePlane(pNewPlane, nNewPitch);

    if ( _nMode & nMode & VPLANE )
        pVPlane->ChangePlane(pNewPlane, nNewPitch);
}

void MVFrame::Refine(MVPlaneSet _nMode, int sharp)
{
    if (nMode & YPLANE & _nMode)
        pYPlane->Refine(sharp);

    if (nMode & UPLANE & _nMode)
        pUPlane->Refine(sharp);

    if (nMode & VPLANE & _nMode)
        pVPlane->Refine(sharp);
}

void MVFrame::Pad(MVPlaneSet _nMode)
{
    if (nMode & YPLANE & _nMode)
        pYPlane->Pad();

    if (nMode & UPLANE & _nMode)
        pUPlane->Pad();

    if (nMode & VPLANE & _nMode)
        pVPlane->Pad();
}

void MVFrame::ResetState()
{
    if ( nMode & YPLANE )
        pYPlane->ResetState();

    if ( nMode & UPLANE )
        pUPlane->ResetState();

    if ( nMode & VPLANE )
        pVPlane->ResetState();
}

void MVFrame::WriteFrame(FILE *pFile)
{
    if ( nMode & YPLANE )
        pYPlane->WritePlane(pFile);

    if ( nMode & UPLANE )
        pUPlane->WritePlane(pFile);

    if ( nMode & VPLANE )
        pVPlane->WritePlane(pFile);
}

void MVFrame::ReduceTo(MVFrame *pFrame)
{
    if ( nMode & YPLANE )
        pYPlane->ReduceTo(pFrame->GetPlane(YPLANE));

    if ( nMode & UPLANE )
        pUPlane->ReduceTo(pFrame->GetPlane(UPLANE));

    if ( nMode & VPLANE )
        pVPlane->ReduceTo(pFrame->GetPlane(VPLANE));
}

/******************************************************************************
*                                                                             *
*  MVGroupOfFrames : manage a hierachal frame structure                       *
*                                                                             *
******************************************************************************/

MVGroupOfFrames::MVGroupOfFrames(int _nLevelCount, int _nWidth, int _nHeight, int _nPel, int _nHPad, int _nVPad, int _nMode, 
                                 bool _isse, int _nyRatioUV) :
                 nLevelCount(_nLevelCount), nWidth(_nWidth), nHeight(_nHeight), nPel(_nPel), nHPad(_nHPad), nVPad(_nVPad),
                 nMode(_nMode), isse(_isse), nyRatioUV(_nyRatioUV)
{
    nRefCount = 0;
    bIsProcessed = false;
    bParity = false;
    pYUY2Planes=pYUY2Planes2x=0;

    pFrames = new MVFrame *[nLevelCount];
    if (pFrames) {
        pFrames[0] = new MVFrame(nWidth, nHeight, nPel, nHPad, nVPad, nMode, isse, nyRatioUV);
        for ( int i = 1; i < nLevelCount; ++i ) {
            nWidth /= 2;
            nHeight /= 2;
            pFrames[i] = new MVFrame(nWidth, nHeight, 1, nHPad, nVPad, nMode, isse, nyRatioUV);
        }
    }
}

MVGroupOfFrames::~MVGroupOfFrames()
{
    if (pFrames) {
        for ( int i = 0; i < nLevelCount; ++i )
            if (pFrames[i]) delete pFrames[i];
    
        delete [] pFrames;
    }

    if (pYUY2Planes)   delete pYUY2Planes;
    if (pYUY2Planes2x) delete pYUY2Planes2x;
}

MVFrame *MVGroupOfFrames::GetFrame(int nLevel)
{
    if (( nLevel < 0 ) || ( nLevel >= nLevelCount )) return 0;
    return pFrames[nLevel];
}

void MVGroupOfFrames::SetPlane(const Uint8 *pNewSrc, int nNewPitch, MVPlaneSet nMode)
{
    pFrames[0]->ChangePlane(pNewSrc, nNewPitch, nMode);
}

void MVGroupOfFrames::Refine(MVPlaneSet nMode, int sharp)
{
    pFrames[0]->Refine(nMode, sharp);
}

void MVGroupOfFrames::Pad(MVPlaneSet nMode)
{
    pFrames[0]->Pad(nMode);
}

void MVGroupOfFrames::Reduce()
{
    for (int i = 0; i < nLevelCount - 1; ++i ) {
        pFrames[i]->ReduceTo(pFrames[i+1]);
        pFrames[i+1]->Pad(YUVPLANES);
    }
}

void MVGroupOfFrames::ResetState()
{
    bIsProcessed = false;

    for ( int i = 0; i < nLevelCount; ++i )
        pFrames[i]->ResetState();
}

void MVGroupOfFrames::ConvertVideoFrame(PVideoFrame *pVF, PVideoFrame *pVF2x, int PixelType, 
                                        unsigned int const nWidth, unsigned int const nHeight, 
                                        unsigned int const nPel, bool const isse)
{
     PROFILE_START(MOTION_PROFILE_YUY2CONVERT);
    // convert src YUY2
    if (pYUY2Planes) {
        // check if same dimension
        if (nWidth!=pYUY2Planes->GetWidth() || nHeight!=pYUY2Planes->GetHeight()) {
            // not same size so delete and create new size
            delete pYUY2Planes;
            pYUY2Planes=new YUY2Planes(nWidth, nHeight, PixelType, isse);
        }
    }
    else {
        // no planes yet so create
        pYUY2Planes=new YUY2Planes(nWidth, nHeight, PixelType, isse);
    }
    // fill the planes
    pYUY2Planes->ConvertVideoFrameToPlanes(pVF, VFPtrs, VFPitches);

    if (*pVF2x) {
        // convert src2x YUY2
        int TotalWidth=nWidth*nPel, TotalHeight=nHeight*nPel;
        if (pYUY2Planes2x) {
            // check if same dimension
            if (TotalWidth!=pYUY2Planes2x->GetWidth() || TotalHeight!=pYUY2Planes2x->GetHeight()) {
                // not same size so delete and create new size
                delete pYUY2Planes2x;
                pYUY2Planes2x=new YUY2Planes(TotalWidth, TotalHeight, PixelType, isse);
            }
        }
        else {
            // no planes yet so create
            pYUY2Planes2x=new YUY2Planes(TotalWidth, TotalHeight, PixelType, isse);
        }
        // fill the planes
        pYUY2Planes2x->ConvertVideoFrameToPlanes(pVF2x, VF2xPtrs, VF2xPitches);
    }

    PROFILE_STOP(MOTION_PROFILE_YUY2CONVERT);

    // set planes
    SetPlane(VFPtrs[0], VFPitches[0], YPLANE);
    SetPlane(VFPtrs[1], VFPitches[1], UPLANE);
    SetPlane(VFPtrs[2], VFPitches[2], VPLANE);
}

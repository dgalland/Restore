// DCT calculation with fftw (real)
// Copyright(c)2006 A.G.Balakhnin aka Fizick
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

#include "dctfftw.h"
#include "stdio.h"

#include <algorithm>

DCTFFTW::DCTFFTW(int _sizex, int _sizey, HINSTANCE hinstFFTW3, int _dctmode) 

{
    fftwf_free_addr = (fftwf_free_proc) GetProcAddress(hinstFFTW3, "fftwf_free"); 
    fftwf_malloc_addr = (fftwf_malloc_proc)GetProcAddress(hinstFFTW3, "fftwf_malloc"); 
    fftwf_destroy_plan_addr = (fftwf_destroy_plan_proc) GetProcAddress(hinstFFTW3, "fftwf_destroy_plan");
    fftwf_plan_r2r_2d_addr = (fftwf_plan_r2r_2d_proc) GetProcAddress(hinstFFTW3, "fftwf_plan_r2r_2d");
    fftwf_execute_r2r_addr = (fftwf_execute_r2r_proc) GetProcAddress(hinstFFTW3, "fftwf_execute_r2r");

    sizex = _sizex;
    sizey = _sizey;
    dctmode = _dctmode;

    int size2d = sizey*sizex;

    int cursize = 1;
    dctshift = 0;
    while (cursize < size2d) {
        ++dctshift;
        cursize = (cursize<<1);
    }

    dctshift0 = dctshift + 2;

    fSrc = (float *)fftwf_malloc_addr(sizeof(float) * size2d );
    fSrcDCT = (float *)fftwf_malloc_addr(sizeof(float) * size2d );

    dctplan = fftwf_plan_r2r_2d_addr(sizey, sizex, fSrc, fSrcDCT, FFTW_REDFT10, FFTW_REDFT10, FFTW_ESTIMATE); // direct fft 
}

DCTFFTW::~DCTFFTW()
{
    fftwf_destroy_plan_addr(dctplan);
    fftwf_free_addr(fSrc);
    fftwf_free_addr(fSrcDCT);
}

//  put source data to real array for FFT
void DCTFFTW::Bytes2Float (const unsigned char * srcp, int src_pitch, float * realdata)
{
    int src_pitch1=src_pitch-sizex;
    for (int j = 0; j < sizey; ++j) { 
        for (int i = 0; i < sizex; ++i) {
            *(realdata++) = *(srcp++);
        }
        srcp += src_pitch1;
    }
}

//  put source data to real array for FFT
void DCTFFTW::Float2Bytes (unsigned char * dstp, int dst_pitch, float * realdata)
{
    int dstpitch1 = dst_pitch-sizex;
    int integ;
    float f = *(realdata++)*0.5f; // to be compatible with integer DCTINT8
    _asm fld f; 
    _asm fistp integ; // fast conversion
    *(dstp++) = static_cast<unsigned char>(std::min<int>(255, std::max<int>(0, (integ>>dctshift0) + 128))); // DC
    for (int i = 1; i < sizex; ++i) {
        f = *(realdata++)*0.707f; // to be compatible with integer DCTINT8
        _asm fld f;
        _asm fistp integ;
        *(dstp++) = static_cast<unsigned char>(std::min<int>(255, std::max<int>(0, (integ>>dctshift) + 128)));
    }
    dstp += dstpitch1;
    for (int j = 1; j < sizey; ++j) { 
        for (int i = 0; i < sizex; ++i) {
            f = *(realdata++)*0.707f; // to be compatible with integer DCTINT8
            _asm fld f;
            _asm fistp integ;
            *(dstp++) = static_cast<unsigned char>(std::min<int>(255, std::max<int>(0, (integ>>dctshift) + 128)));
        }
        dstp += dstpitch1;
    }
}


void DCTFFTW::DCTBytes2D(const unsigned char *srcp, int src_pitch, unsigned char *dctp, int dct_pitch)
{
    _asm emms;
    Bytes2Float (srcp, src_pitch, fSrc);
    fftwf_execute_r2r_addr(dctplan, fSrc, fSrcDCT);
    Float2Bytes (dctp, dct_pitch, fSrcDCT);
}

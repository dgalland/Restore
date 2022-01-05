/*
    DePanEstimate plugin for Avisynth 2.5 - global motion estimation
	Version 1.9.2 March 25, 2007
    (DePanEstimate function)
	Copyright(c) 2004-2007, A.G. Balakhnin aka Fizick
	bag@hotmail.ru

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

	uses FFTW library (http://www.fftw.org)
	as Windows binary DLL compiled with gcc under MinGW by Alessio Massaro,
	which support for threads and have AMD K7 (3dNow!) in addition to SSE/SSE2.
	ftp://ftp.fftw.org/pub/fftw/fftw3win32mingw.zip

    DePan plugin at first stage calculates global motion in frames (by phase-shift method),
	and at second stage shift frame images for global motion compensation


	Parameters of DePanEstimate:
		clip - input clip
		range - number of previous (and also next) frames (fields) near requsted frame to estimate motion
		trust - frames correlation at scene change
		winx  - number of columns of fft window (must be power of 2). (width)
		winy  - number of rows of fft window (must be power of 2).    (height)
		dxmax - limit of x shift
		dymax - limit of y shift
		zoommax - maximum zoom factor ( if =1, zoom is not estimated)
		improve - improve zoom estimation by iteration - disabled in v1.6
		stab -  calculated trust decreasing for large shifts
		pixaspect - pixel aspect
		info  - show motion info on frame
		log - output log file with motion data
		debug - output data for debugview utility
		show - show correlation sufrace
		fftw - use fftw external DLL library (dummy parameter since v1.9)
		extlog - output extended log file with motion and trust data

	The DePanEstimate function output is special service clip with coded motion data in frames.


*/

#include "windows.h"
#include "avisynth.h"
#include "math.h"
#include "stdio.h"


#include "depanio.h"
#include "info.h"
#include "estimate_fftw.h"


// constructor
DePanEstimate_fftw::DePanEstimate_fftw(PClip _child, int _range, float _trust, int _winx, int _winy, int _dxmax, int _dymax, float _zoommax, int _improve, float _stab, float _pixaspect, int _info, const char * _logfilename, int _debug, int _show, const char * _extlogfilename, IScriptEnvironment* env) :
	GenericVideoFilter(_child), range(_range), trust_limit(_trust), winx(_winx), winy(_winy), dxmax(_dxmax), dymax(_dymax), zoommax(_zoommax), improve(_improve), stab(_stab), pixaspect(_pixaspect), info(_info), logfilename(_logfilename), debug(_debug), show(_show), extlogfilename(_extlogfilename){

	if (!vi.IsYV12() && !vi.IsYUY2())	env->ThrowError("DePanEstimate: input to filter must be in YV12 or YUY2!");

	fieldbased = vi.IsFieldBased();  // after separatefields
	TFF = vi.IsTFF(); //

	// check and set fft window x size
	if (winx > vi.width)	env->ThrowError("DePanEstimate: WINX must not be greater than WIDTH !");
	int i;
	int wx;
	if (winx == 0){
		winx = vi.width;
		// find max fft window size (power of 2)
		wx = 1;
		for (i=0; i < 13; i++) {
			if (wx*2 <=  winx) wx = wx*2;
		}
		winx=wx;
	}
//	else {
//		// find max fft window size (power of 2)
//		wx = 1;
//		for (i=0; i < 13; i++) {
//			if (wx*2 <=  winx) wx = wx*2;
//		}
//		if (wx != winx)	env->ThrowError("DePanEstimate: WINX must be power of 2 !");
//	}

	if (zoommax != 1.0) winx=winx/2;  // devide window x by 2 part (left and right)

	// check and set fft window y size
	if (winy > vi.height) env->ThrowError("DePanEstimate: WINY must not be greater than Field Height !");
	int wy;
	if (winy == 0) {
		winy = vi.height; // start value
		// find max fft window size (power of 2)
		 wy = 1;
		for (i=0; i < 13; i++) {
			if (wy*2 <=  winy) wy = wy*2;
		}
		winy = wy;
	}
//	else {
//		// find max fft window size (power of 2)
//		 wy = 1;
//		for (i=0; i < 13; i++) {
//			if (wy*2 <=  winy) wy = wy*2;
//		}
//		if (wy != winy)	env->ThrowError("DePanEstimate: WINY must be power of 2 !");
//	}

	// max dx shift must be less winx/2
	if (dxmax < 0) dxmax = winx/4;  // default, permit max=0 in v1.8.2
	if (dymax < 0) dymax = winy/4;  // default, permit max=0 in v1.8.2
	if (dxmax >= winx/2) env->ThrowError("DePanEstimate: DXMAX must be less WINX/2 !");
	if (dymax >= winy/2) env->ThrowError("DePanEstimate: DYMAX must be less WINY/2 !");


	hinstLib = LoadLibrary("fftw3.dll"); // added in v 1.2 for delayed loading
	if (hinstLib != NULL)
	{
		fftwf_free_addr = (fftwf_free_proc) GetProcAddress(hinstLib, "fftwf_free");
		fftwf_malloc_addr = (fftwf_malloc_proc)GetProcAddress(hinstLib, "fftwf_malloc");
		fftwf_plan_dft_r2c_2d_addr = (fftwf_plan_dft_r2c_2d_proc) GetProcAddress(hinstLib, "fftwf_plan_dft_r2c_2d");
		fftwf_plan_dft_c2r_2d_addr = (fftwf_plan_dft_c2r_2d_proc) GetProcAddress(hinstLib, "fftwf_plan_dft_c2r_2d");
		fftwf_destroy_plan_addr = (fftwf_destroy_plan_proc) GetProcAddress(hinstLib, "fftwf_destroy_plan");
		fftwf_execute_dft_r2c_addr = (fftwf_execute_dft_r2c_proc) GetProcAddress(hinstLib, "fftwf_execute_dft_r2c");
		fftwf_execute_dft_c2r_addr = (fftwf_execute_dft_c2r_proc) GetProcAddress(hinstLib, "fftwf_execute_dft_c2r");
	}
	if (hinstLib==NULL || fftwf_free_addr==NULL || fftwf_malloc_addr==NULL || fftwf_plan_dft_r2c_2d_addr==NULL ||
		fftwf_plan_dft_c2r_2d_addr==NULL || fftwf_destroy_plan_addr==NULL || fftwf_execute_dft_r2c_addr==NULL || fftwf_execute_dft_c2r_addr==NULL)
			env->ThrowError("DePanEstimate: Can not load FFTW3.DLL !");

	// set frames capacity of fft cache
	fftcachecapacity = range*2 + 4;// modified in version 0.6e to correct for range=0

	fftcachelist = (int *)malloc(fftcachecapacity*sizeof(int));
	if (fftcachelist == NULL) env->ThrowError("DepanEstimate: Allocation Failure!\n");
	fftcachelist2 = (int *)malloc(fftcachecapacity*sizeof(int));
	if (fftcachelist2 == NULL) env->ThrowError("DepanEstimate: Allocation Failure!\n");

	fftcachelistcomp = (int *)malloc(fftcachecapacity*sizeof(int));
	if (fftcachelistcomp == NULL) env->ThrowError("DepanEstimate: Allocation Failure!\n");
	fftcachelistcomp2 = (int *)malloc(fftcachecapacity*sizeof(int));
	if (fftcachelistcomp2 == NULL) env->ThrowError("DepanEstimate: Allocation Failure!\n");

	// init (clear) cached frame numbers and fft matrices
	for (i=0; i<fftcachecapacity; i++) {
		fftcachelist[i] = -1;
		fftcachelist2[i] = -1;  // right window if zoom
		fftcachelistcomp[i] = -1;
		fftcachelistcomp2[i] = -1;  // right window if zoom
	}

//	winsize = winx*winy;
	int winxpadded =(winx/2 + 1)*2;
	int fftsize = winy*winxpadded/2; //complex

	// memory for cached fft
	// fftw version
	fftcache = (fftwf_complex **)fftwf_malloc_addr(fftcachecapacity*sizeof(int)); // array of pointers
	if (fftcache == NULL) env->ThrowError("DepanEstimate: FFTW Allocation Failure!\n");
	fftcache2 = (fftwf_complex **)fftwf_malloc_addr(fftcachecapacity*sizeof(int));
	fftcachecomp = (fftwf_complex **)fftwf_malloc_addr(fftcachecapacity*sizeof(int));
	fftcachecomp2 = (fftwf_complex **)fftwf_malloc_addr(fftcachecapacity*sizeof(int));
	for (i=0; i<fftcachecapacity; i++) {
		fftcache[i] = (fftwf_complex *)fftwf_malloc_addr(sizeof(fftwf_complex) * fftsize );
		fftcachecomp[i] = (fftwf_complex *)fftwf_malloc_addr(sizeof(fftwf_complex) * fftsize );
		if (zoommax != 1){
			fftcache2[i] = (fftwf_complex *)fftwf_malloc_addr(sizeof(fftwf_complex) * fftsize );  // right window if zoom
			fftcachecomp2[i] = (fftwf_complex *)fftwf_malloc_addr(sizeof(fftwf_complex) * fftsize ); // right window if zoom
		}
	}


	// memory for correlation matrice
	correl = (fftwf_complex *)fftwf_malloc_addr(sizeof(fftwf_complex) * fftsize);//alloc_2d_float(winy, winx, env);
	if (zoommax != 1){
		correl2 = (fftwf_complex *)fftwf_malloc_addr(sizeof(fftwf_complex) * fftsize);
	}

	realcorrel = (float *) correl; // for inplace transform
	if (zoommax != 1){
		realcorrel2 = (float *) correl2; // for inplace transform
	}
	// create FFTW plan
	// change from FFTW_MEASURE to FFTW_ESTIMATE for more short init, without speed change (for  power-2 windows) in v 1.1.1
	plan = fftwf_plan_dft_r2c_2d_addr(winy, winx, realcorrel, correl, FFTW_ESTIMATE); // direct fft
	planinv = fftwf_plan_dft_c2r_2d_addr(winy, winx, correl, realcorrel, FFTW_ESTIMATE); // inverse fft

	motionx = (float *)malloc(vi.num_frames*sizeof(float));
	if (motionx == NULL) env->ThrowError("DepanEstimate: Allocation Failure!\n");
	motiony = (float *)malloc(vi.num_frames*sizeof(float));
	if (motiony == NULL) env->ThrowError("DepanEstimate: Allocation Failure!\n");
	motionzoom = (float *)malloc(vi.num_frames*sizeof(float));
	if (motionzoom == NULL) env->ThrowError("DepanEstimate: Allocation Failure!\n");
	trust =   (float *)malloc(vi.num_frames*sizeof(float));
	if (trust == NULL) env->ThrowError("DepanEstimate: Allocation Failure!\n");

	// set motion value for initial frame as 0 (interpreted as scene change)
	motionx[0] = 0;
	motiony[0] = 0;
	motionzoom[0] = 1.0;
	for (i=1; i < vi.num_frames; i++) {
		motionx [i] = MOTIONUNKNOWN;  //set motion as unknown for all frames beside 0
	}

	// set frame cache
	child->SetCacheHints(CACHE_RANGE,2*range+1); // v1.7

//	logfilename = "DePan.log";
	if (logfilename != "") {
		logfile = fopen(logfilename,"wt");
		if (logfile == NULL)	env->ThrowError("DePanEstimate: Log file can not be created!");
	}

	if (extlogfilename != "") {
		extlogfile = fopen(extlogfilename,"wt");
		if (extlogfile == NULL)	env->ThrowError("DePanEstimate: ExtLog file can not be created!");
	}

	if (info ==0 && show==0) {  // if image is not used for look, crop it to size of depan data
		// number of bytes for writing of all depan data for frames from ndest-range to ndest+range
		vi.width = depan_data_bytes(range*2 +1);
		vi.height = 4;  // safe value. depan need in 1 only, but 1 is not possible for YV12
	}


}

//****************************************************************************
// This is where any actual destructor code used goes
DePanEstimate_fftw::~DePanEstimate_fftw() {
  // This is where you can deallocate any memory you might have used.

	if (logfilename != "" && logfile != NULL) {
		fclose(logfile);
	}

	if (extlogfilename != "" && extlogfile != NULL) {
		fclose(extlogfile);
	}

	free(fftcachelist);
	free(fftcachelist2);
	free(fftcachelistcomp);
	free(fftcachelistcomp2);

	fftwf_destroy_plan_addr(plan);
	fftwf_destroy_plan_addr(planinv);

	for (int i=0; i<fftcachecapacity; i++) {
		fftwf_free_addr(fftcache[i]);
		fftwf_free_addr(fftcachecomp[i]);
		if (zoommax != 1){
			fftwf_free_addr(fftcache2[i]);
			fftwf_free_addr(fftcachecomp2[i]);
		}
	}
	fftwf_free_addr(fftcache);
	fftwf_free_addr(fftcachecomp);
	fftwf_free_addr(correl);
	if (zoommax != 1){
		fftwf_free_addr(fftcache2);
		fftwf_free_addr(fftcachecomp2);
		fftwf_free_addr(correl2);
	}
	free(motionx);
	free(motiony);
	free(motionzoom);
	free(trust);

	if (hinstLib != NULL)
		FreeLibrary(hinstLib);
}


//****************************************************************************
//
//  put source data to real array for FFT
//
void DePanEstimate_fftw::frame_data2d (const BYTE * srcp0, int height, int src_width, int pitch, float * realdata, int winx, int winy, int winleft, bool isYUY2)
{
   int i, j, h0;
	const BYTE *srcp = srcp0;
	int winxpadded =(winx/2 + 1)*2;

	h0 = (height - winy)/2;  // top of fft window
	if (isYUY2)
	{
		srcp += pitch*h0 +winleft*2;		// offset of window data
		for (j = 0; j < winy; j++) {
			for (i = 0; i < winx; i+=1) {
				realdata[i] = srcp[i*2]; // real part
			}
			srcp += pitch;
			realdata += winxpadded;
		}
	}
	else
	{ // YV12
		srcp += pitch*h0 +winleft;		// offset of window data
		for (j = 0; j < winy; j++) {
			for (i = 0; i < winx; i+=2) {
				realdata[i] = srcp[i]; // real part
				realdata[i+1] = srcp[i+1]; // real part
			}
			srcp += pitch;
			realdata += winxpadded;
		}
	}


}



//****************************************************************************
//
//
void mult_conj_data2d_nosse (fftwf_complex *fftnext, fftwf_complex *fftsrc, fftwf_complex *mult, int winx, int winy)
{
	// multiply complex conj. *next to src
	// (hermit)
//	int i,j;
	int nx = winx/2 + 1; //padded, odd
//	int winxpadded = nx*2;
//	int w = winxpadded/2;

//	int jw =0;
	int total = winy*nx; // even
	for (int k = 0; k < total; k+=2) { //paired for speed
		mult[k][0]  = fftnext[k][0] * fftsrc[k][0] + fftnext[k][1] * fftsrc[k][1];  // real part
		mult[k][1] =  fftnext[k][0] * fftsrc[k][1] - fftnext[k][1] * fftsrc[k][0] ; // imagine part
		mult[k+1][0]  = fftnext[k+1][0] * fftsrc[k+1][0] + fftnext[k+1][1] * fftsrc[k+1][1];  // real part
		mult[k+1][1] =  fftnext[k+1][0] * fftsrc[k+1][1] - fftnext[k+1][1] * fftsrc[k+1][0] ; // imagine part
	}
}

//****************************************************************************
//
//
void DePanEstimate_fftw::mult_conj_data2d (fftwf_complex *fftnext, fftwf_complex *fftsrc, fftwf_complex *mult, int winx, int winy)
{
	// SSE version - but i can not get any speed improving, - v.1.8
	// multiply complex conj. *next to src
	// (hermit)
//	int i,j;
	int nx = winx/2 + 1; //padded, odd
//	int winxpadded = nx*2;
//	int w = winxpadded/2;

//	int jw =0;
	int totalbytes = winy*nx*8; // even
	_asm
	{
		mov esi, fftsrc;
		mov edi, mult;
		mov edx, fftnext;
		mov ecx, totalbytes;
		mov eax, 0;
align 16
nextpair:
		movaps xmm0, [esi+eax]; //low: resrc | imsrc | re src1 | imsrc1
		movaps xmm1, [edx+eax]; //low: renext | imnext | renext1 | imnext1
		movaps xmm2, xmm0; // copy src
		mulps xmm2, xmm1; //low: resrc*renext | insrc*imnext | resrc1*renext1 | imsrc1*imnext1
		movaps xmm3, xmm2; // copy
		shufps xmm3, xmm3, (1 + 0 + 48 + 128);// swap re & im
		addps xmm3, xmm2; //low: resrc*renext + insrc*imnext |same | resrc1*renext1 + imsrc1*imnext1 |same
		//so, real parts of mult are ready
		shufps xmm1, xmm1, (1 + 0 + 48 + 128);// swap re & im //low: imnext | renext | imnext1 | renext1
		mulps xmm0, xmm1; //low: resrc*imnext | imsrc*renext | resrc1*imnext1 | imsrc*renext1
		movaps xmm1, xmm0; // copy
		shufps xmm1, xmm1, (1 + 0 + 48 + 128);// swap re & im
		subps xmm1, xmm0; //low: imsrc*renext - resrc*imnext | resrc*imnext-imsrc*renext | ...same for 1
		//so, image part of mult are ready too
		movaps xmm0, xmm3; //copy re
		unpcklps xmm0, xmm1;// resrc*renext + insrc*imnext | imsrc*renext - resrc*imnext | resrc*renext + insrc*imnext | resrc*imnext-imsrc*renext
		movaps xmm2, xmm3; //copy re
		unpckhps xmm2, xmm1;// resrc1*renext1 + insrc1*imnext1 | imsrc1*renext1 - resrc1*imnext1 | resrc1*renext1 + insrc1*imnext1 | resrc1*imnext1-imsrc1*renext1
		movlhps xmm0, xmm2; //copy 1 to high part
		movaps [edi+eax], xmm0; // write mult

		add eax, 16;
		cmp eax, ecx;
		jl nextpair;
		emms;

	//for (int k = 0; k < total; k+=2) {
	//	mult[k][0]  = fftnext[k][0] * fftsrc[k][0] + fftnext[k][1] * fftsrc[k][1];  // real part
	//	mult[k][1] =  fftnext[k][0] * fftsrc[k][1] - fftnext[k][1] * fftsrc[k][0] ; // imagine part
	//	mult[k+1][0]  = fftnext[k+1][0] * fftsrc[k+1][0] + fftnext[k+1][1] * fftsrc[k+1][1];  // real part
	//	mult[k+1][1] =  fftnext[k+1][0] * fftsrc[k+1][1] - fftnext[k+1][1] * fftsrc[k+1][0] ; // imagine part
	//}
	}
}


//****************************************************************************
////
//
void DePanEstimate_fftw::get_motion_vector (float *correl, int winx, int winy, float trust_limit, int dxmax, int dymax,
		float stab, int nframe, int fieldbased, int TFF, float pixaspect, float *fdx, float *fdy, float *trust, int debug)
{
	float correlmax, cur, correlmean;
	float f1,f2;
	float xadd =0;
	float yadd =0;
	int i, j;
	int dx, dy;
	int imax =0,jmax=0;
	int imaxm1, imaxp1, jmaxm1,jmaxp1;
	int count;
	int isnframeodd;

	int winxpadded =(winx/2 + 1)*2;
	float * correlp; //pointer


//	char debugbuf[100];

	// find global max on real part of correlation surface
	// new version: search only at 4 corners with ranges dxmax, dymax
	correlmax = correl[0];
	correlmean = 0;
	count = 0;
	correlp = correl;
	for (j = 0; j <= dymax; j++) {   // top
		for (i = 0; i <= dxmax; i++) {  //left
			cur = correlp[i]; // real part
			correlmean += cur;
			count += 1;
			if (correlmax < cur) {
				correlmax = cur;
				imax = i;
				jmax = j;
			}
		}
		for (i = winx-dxmax; i < winx; i++) {  //right
			cur = correlp[i]; // real part
			correlmean += cur;
			count += 1;
			if (correlmax < cur) {
				correlmax = cur;
				imax = i;
				jmax = j;
			}
		}
		correlp += winxpadded;
	}
	correlp = correl +(winy-dymax)*winxpadded;
	for (j = winy-dymax; j < winy; j++) {   // bottom
		for (i = 0; i <= dxmax; i++) {  //left
			cur = correlp[i]; // real part
			correlmean += cur;
			count += 1;
			if (correlmax < cur) {
				correlmax = cur;
				imax = i;
				jmax = j;
			}
		}
		for (i = winx-dxmax; i < winx; i++) {  //right
			cur = correlp[i]; // real part
			correlmean += cur;
			count += 1;
			if (correlmax < cur) {
				correlmax = cur;
				imax = i;
				jmax = j;
			}
		}
		correlp += winxpadded;
	}

	correlmean = correlmean/count; // mean value

	correlmax = correlmax/(winx*winy); // normalize value
	correlmean = correlmean/(winx*winy); // normalize value

	*trust = (correlmax-correlmean)*100.0f/(correlmax + 0.1f); // +0.1 for safe divide


	if (imax*2 < winx) {
		dx = imax;
	}
	else { // get correct shift values on periodic surface (adjusted borders)
		dx = (imax - winx);
	}

	if (jmax*2 < winy) {
		dy = jmax;
	}
	else { // get correct shift values on periodic surface (adjusted borders)
		dy = (jmax - winy);
	}

	// some trust decreasing for large shifts

	*trust *= (dxmax+1)/(dxmax+1 + stab*abs(dx))*(dymax+1)/(dymax+1 + stab*abs(dy)); //v1.8.2

	if (*trust < trust_limit)  { 	// reject if relative diffference correlmax from correlmean is small
//		 probably due to scene change
		dx = 0; // set value to pure 0, what will be interpreted as bad mark (scene change)
		dy = 0;
		xadd =0;
		yadd =0;
		*fdx = 0;
		*fdy = 0;

	}
	else {
//		normal, no scene change
		// get more precise float dx, dy by interpolation
		// get i, j, of left and right of max
		if ( imax+1 < winx ) 	imaxp1 = imax+1; // plus 1
		else imaxp1 = imax+1-winx;  // over period

		if ( imax-1 >= 0 ) 	imaxm1 = imax-1; // minus 1
		else 	imaxm1 = imax-1 + winx;

		if ( jmax+1 < winy ) jmaxp1 = jmax+1;
		else 	jmaxp1 = jmax+1-winy;

		if ( jmax-1 >= 0 ) 	jmaxm1 = jmax-1;
		else 	jmaxm1 = jmax-1 + winy;

		// first and second differential
		f1 = (correl[jmax*winxpadded+imaxp1] - correl[jmax*winxpadded+imaxm1])/2;
		f2 =  correl[jmax*winxpadded+imaxp1] + correl[jmax*winxpadded+imaxm1] - correl[jmax*winxpadded+imax]*2;

		if (f2 == 0) xadd = 0;
		else {
			xadd = -f1/f2;
			if (xadd > 1.0)	xadd = 1.0;
			else if (xadd < -1.0)	xadd = -1.0;
		}

		if (fabs(dx + xadd) > dxmax ) xadd = 0;

		f1 = (correl[jmaxp1*winxpadded+imax] - correl[jmaxm1*winxpadded+imax])/2;
		f2 = correl[jmaxp1*winxpadded+imax] + (correl[jmaxm1*winxpadded+imax]) - correl[jmax*winxpadded+imax]*2;

		if (f2 == 0) yadd = 0;
		else {
			yadd = -f1/f2;
			if (yadd > 1.0)	yadd = 1.0; // limit addition for stability
			else if (yadd < -1.0)	yadd = -1.0;
		}

		if (fabs(dy + yadd) > dymax ) yadd = 0;

		isnframeodd = nframe%2;  // =0 for even,    =1 for odd

		if (fieldbased != 0) { // correct line shift for fields
			// correct unneeded fields matching
			{if ( TFF != 0) yadd += 0.5f - isnframeodd; // TFF
			else yadd += - 0.5f + isnframeodd; } // BFF (or undefined?)
			// scale dy for fieldbased frame by factor 2
			yadd = yadd *2;
			dy = dy * 2;
		}


		*fdx = (float)dx + xadd;
		*fdy = (float)dy + yadd;

		*fdy = (*fdy)/pixaspect;

		if (fabs(*fdx) < 0.01f)  // if it is accidentally very small, reset it to small, but non-zero value ,
			*fdx = (2*rand()-RAND_MAX) > 0 ? 0.011f : -0.011f; // to differ from pure 0, which be interpreted as bad value mark (scene change)

//		if (fabs(*fdy) < 0.01f) *fdy = 0.011f; // disabled in 0.9.1 (only dx used)


	}


//	if (debug != 0) { // debug mode
		// output data for debugview utility
//		sprintf(debugbuf,"DePanEstimate: correl %e %e %e %e \n", correl[254][imax], correl[255][imax], correl[0][imax], correl[1][imax]);
//		OutputDebugString(debugbuf);
//		sprintf(debugbuf,"DePanEstimate: fr=%d imax=%d jmax=%d dx=%7.2f dy=%7.2f trust=%7.2f\n", nframe, imax, jmax, *fdx, *fdy, *trust);
//		OutputDebugString(debugbuf);
//	}

}



//****************************************************************************
//	clear un-needed cache
void DePanEstimate_fftw::clear_unnecessary_cache (int *fftcachelist, int fftcachecapacity, int ndest, int range)
{
	int i;
	for (i=0; i<fftcachecapacity; i++ ){
		if (fftcachelist[i]>ndest + range || fftcachelist[i]<ndest-range-1)		fftcachelist[i] = -1;
	}
}


//****************************************************************************
// check if fft of this frame is in cache, get number of it
int DePanEstimate_fftw::get_cache_number (int * fftcachelist, int fftcachecapacity, int ndest)
{
	int i;
	int found = -1;
	for (i=0; i<fftcachecapacity; i++) {
		if (fftcachelist[i] == ndest) {
			found = i ;
		}
	}
	return found;
}

//****************************************************************************
//
//
int DePanEstimate_fftw::get_free_cache_number(int * fftcachelist, int fftcachecapacity)
{
	int i;
	int found = -1;
	for (i=0; i<fftcachecapacity; i++) {
		if (fftcachelist[i] == -1) {
			found = i ;
		}
	}
	return found;
}

//****************************************************************************
//
fftwf_complex *  DePanEstimate_fftw::get_plane_fft(const BYTE * srcp,  int src_height, int src_width, int src_pitch,
		int nsrc, int *fftcachelist, int fftcachecapacity, fftwf_complex **fftcache, int winx, int winy, int winleft, fftwf_plan plan, bool isYUY2)
{		// get forward fft of src frame plane
	int ncs;
	float * realdata;
	fftwf_complex * fftsrc;
//	char debugbuf[64];

//	const BYTE * srcp = src->GetReadPtr();
//	const int src_width = src->GetRowSize();
//	const int src_height = src->GetHeight();
//	const int src_pitch = src->GetPitch();

	// get free cache number
	ncs = get_free_cache_number(fftcachelist, fftcachecapacity);
	if (ncs < 0) { // if no free cache number
		return NULL; // than return error
	}
	else {
		// get address of fft matrice in cache
		fftsrc = fftcache[ncs];
		realdata = (float *) fftsrc;
		// make forward fft of src frame
		// prepare 2d data for fft
		frame_data2d (srcp, src_height, src_width, src_pitch, realdata, winx, winy, winleft, isYUY2);
		// make forward fft of data
//		rdft2d(winy, winx, 1, fftsrc, NULL, fftip, fftwork);
		fftwf_execute_dft_r2c_addr(plan, realdata, fftsrc);
		// now data is fft
		// reserve cache with this number
		fftcachelist[ncs] = nsrc;

		return fftsrc;  // return pointer to fft of frame plane
	}
}

// ***********************************************************************
void  DePanEstimate_fftw::showcorrelation(float *correl, int winx, int winy, BYTE *dstp, int dst_pitch, int winleft, int wintop, int isYUY2)
{
	float correlmax, correlmin, cur, norm;
	int i,j;
	int winxpadded =(winx/2 + 1)*2;

	float * correlp; //pointer

	// find max and min
	correlmax = correl[0];
	correlmin = correl[0];
	correlp = correl;
	for (j=0; j<winy; j++) {
		for (i=0; i<winx; i++) {
			cur = correlp[i];
			if (correlmax < cur) {
				correlmax = cur;
			}
			if (correlmin > cur) {
				correlmin = cur;
			}
		}
		correlp += winxpadded;
	}

	// normalize to BYTE
	norm = 255/(correlmax-correlmin);

	dstp += wintop*dst_pitch; // go to first line of window

	if (isYUY2)
	{
		dstp += winleft*2; // go to first point of window
		correlp = correl;
		for (j=0; j<winy; j++) {
			for (i=0; i<winx; i++) {
				dstp [i<<1] = int( (correlp[i]-correlmin)*norm ); // real part
				dstp [(i<<1)+1] = 128; // U or V
			}
			dstp += dst_pitch;
			correlp += winxpadded;
		}
	}
	else
	{ // YV12
		dstp += winleft;
		correlp = correl;
		for (j=0; j<winy; j++) {
			for (i=0; i<winx; i++) {
				dstp [i] = int( (correlp[i]-correlmin)*norm ); // real part
			}
			dstp += dst_pitch;
			correlp += winxpadded;
		}
	}


}


//
// ****************************************************************************
//
PVideoFrame __stdcall DePanEstimate_fftw::GetFrame(int ndest, IScriptEnvironment* env) {
	fftwf_complex *fftcur, * fftprev; // chanded in v.1.0
	fftwf_complex *fftcur2, * fftprev2; // right for zoom
//	char debugbuf[96]; // moved to constructor in v.0.9.1
	PVideoFrame cur;
	PVideoFrame prev;
	int ncur;
	int ncs;
	const float rotation = 0; //always 0 in current version
	float zoom = 1;     //always 1 in current version
	float dx1, dy1, trust1;
	float dx2, dy2, trust2;
	int winleft, winleft2;
	int i;
	int nfields;

	const BYTE * prevp;
	const BYTE * curp;

	float motionrotdummy =0; // fictive, =0

	int prev_pitch, cur_pitch; //v1.6

// ---------------------------------------------------------------------------
	// Phase-shift algorithm to calculate global motion
	// Get motion info from the Y Plane

	//	clear some places in fft cache, un-needed for current frame range calculation  (all besides ndest-range-1 to ndest+range+1)
//	clear_unnecessary_cache(fftcachelist, fftcachecapacity, ndest, range);
//	clear_unnecessary_cache(fftcachelist2, fftcachecapacity, ndest, range);
	int clearrange = range+1;
	for (i=0; i<fftcachecapacity; i++ ){
		if (fftcachelist[i] > ndest+clearrange || fftcachelist[i] < ndest-clearrange)	fftcachelist[i] = -1; // free
	}
	for (i=0; i<fftcachecapacity; i++ ){
		if (fftcachelist2[i] > ndest+clearrange || fftcachelist2[i] < ndest-clearrange) fftcachelist2[i] = -1; //free
	}

	for (i=0; i<fftcachecapacity; i++ ){
		if (fftcachelistcomp[i] > ndest+clearrange || fftcachelistcomp[i] < ndest-clearrange)	fftcachelistcomp[i] = -1; // free
	}
	for (i=0; i<fftcachecapacity; i++ ){
		if (fftcachelistcomp2[i] > ndest+clearrange || fftcachelistcomp2[i] < ndest-clearrange) fftcachelistcomp2[i] = -1; //free
	}

	PVideoFrame src = child->GetFrame(ndest, env);
	const int src_width = src->GetRowSize();
	const int src_height = src->GetHeight();
   // Request frame 'ndest' from the child (source) clip.

	// create output frame with crypted motion data info
	PVideoFrame dst = env->NewVideoFrame(vi);

// ---------------------------------------------------------------------------
	bool isYUY2 = vi.IsYUY2(); // v.1.1
	int width = (isYUY2) ? src_width/2 : src_width; //vi.width; // v 1.1, bug fixed in v.1.1.5 (we change vi.width !)

	// Process Y Luma Plane
	const BYTE * srcp = src->GetReadPtr();
	const int src_pitch = src->GetPitch();
	BYTE * dstp = dst->GetWritePtr();
	const int dst_pitch = dst->GetPitch();
	const int dst_width = dst->GetRowSize();
	const int dst_height = dst->GetHeight();


	float xcenter = width/2.0f;  // center of source frame // v.1.1
	float ycenter = src_height/2.0f;

	// correction for fieldbased

	if (fieldbased != 0) 	nfields = 2;
	else nfields = 1;




	// calculate motion data
	for (ncur = ndest-range-1; ncur <= ndest+range+1; ncur++) {  // extended range by 1 for scene detection
		if (ncur >0 && ncur < vi.num_frames) {
			if (motionx[ncur] == MOTIONUNKNOWN || (ncur==ndest && show != 0) ) {  // modified to always show correlation
				// calculate dx, dy now

				if (zoommax == 1) { // NO ZOOM

					winleft = (width - winx)/2;   // left of fft window //v1.1
					// check if fft of cur frame is in cache
					ncs = get_cache_number (fftcachelist, fftcachecapacity, ncur);
					if (ncs != -1) { // found in cache
						// get address of fft matrice in cache
						fftcur = fftcache[ncs];  // central
					}
					else {  // not found in cache
						if (ncur == ndest) cur = src;
						else cur = child->GetFrame(ncur, env); // get current frame
						curp = cur->GetReadPtr();
						cur_pitch = cur->GetPitch();//v1.6

						// get forward fft of src frame from cache or calculation
						fftcur = get_plane_fft(curp, src_height, src_width, cur_pitch, ncur, fftcachelist, fftcachecapacity, fftcache, winx, winy, winleft, plan, isYUY2);
						if (debug != 0) { // debug mode
							// output data for debugview utility
							sprintf(debugbuf,"DePanEstimate: process ncur=%d fftcur\n", ncur);
							OutputDebugString(debugbuf);
						}
					}

					// check if fft of prev frame is in cache
					ncs = get_cache_number (fftcachelist, fftcachecapacity, ncur-1);
					if (ncs != -1) { // found in cache
						// get address of fft matrice in cache
						fftprev = fftcache[ncs];
					}
					else {  // not found in cache
						if (ncur-1 == ndest) prev = src;
						else prev = child->GetFrame(ncur-1, env); //get previous frame
						prevp = prev->GetReadPtr();
						prev_pitch = prev->GetPitch();//v1.6

						// get forward fft of prev frame from cache or calculation
						fftprev = get_plane_fft(prevp, src_height, src_width, prev_pitch, ncur-1, fftcachelist, fftcachecapacity, fftcache, winx, winy, winleft, plan, isYUY2); //v1.6
						if (debug != 0) { // debug mode
							// output data for debugview utility
							sprintf(debugbuf,"DePanEstimate: process ncur-1=%d fftprev\n", ncur-1);
							OutputDebugString(debugbuf);
						}
					}

					// prepare correlation data = mult fftsrc* by fftprev
					mult_conj_data2d (fftcur, fftprev, correl, winx, winy);
					// make inverse fft of prepared correl data
					fftwf_execute_dft_c2r_addr(planinv, correl, realcorrel); // added in v.1.0
					// now correl is is true correlation surface
					// find global motion vector as maximum on correlation sufrace
					// save vector to motion table
					get_motion_vector (realcorrel, winx, winy, trust_limit, dxmax, dymax, stab,  ncur, fieldbased, TFF, pixaspect,&motionx[ncur], &motiony[ncur],  &trust[ncur], debug);
					motionzoom[ncur] =1;  //no zoom
					if (show != 0 && ncur==ndest) {	// show correlation sufrace
						// copy full luma plane to fill borders
						env->BitBlt(dstp, dst_pitch, srcp, src_pitch, dst_width, dst_height);
						// show corr
						showcorrelation(realcorrel,winx, winy, dstp, dst_pitch, winleft,(dst_height-winy)/2, isYUY2);
					}
				}
				else { // ZOOM, calculate 2 data sets (left and right)

					winleft = width/4 - winx/2;   // left edge of left (1) fft window // v.1.1
					winleft2 = width/2 + width/4 - winx/2;   // left edge of right (2)fft window //v1.1

					// check if fft of cur frame is in cache
					ncs = get_cache_number (fftcachelist, fftcachecapacity, ncur);
					if (ncs != -1) { // found in cache
						// get address of fft matrice in cache
						fftcur = fftcache[ncs];  //  left
						fftcur2 = fftcache2[ncs]; // right
					}
					else {  // not found in cache
						if (ncur == ndest) cur = src;
						else cur = child->GetFrame(ncur, env); // get current frame
						curp = cur->GetReadPtr();
						cur_pitch = cur->GetPitch(); //v1.6

						// get forward fft of src frame from cache or calculation
						fftcur = get_plane_fft(curp,src_height, src_width, cur_pitch,  ncur, fftcachelist, fftcachecapacity, fftcache, winx, winy, winleft, plan, isYUY2);//v1.6
						fftcur2 = get_plane_fft(curp, src_height, src_width, cur_pitch, ncur, fftcachelist2, fftcachecapacity, fftcache2, winx, winy, winleft2, plan, isYUY2);//v1.6
					}

					// check if fft of prev frame is in cache
					ncs = get_cache_number (fftcachelist, fftcachecapacity, ncur-1);
					if (ncs != -1) { // found in cache
						// get address of fft matrice in cache
						fftprev = fftcache[ncs];  // left
						fftprev2 = fftcache2[ncs]; // right
					}
					else {  // not found in cache
						if (ncur-1 == ndest) prev = src;
						else prev = child->GetFrame(ncur-1, env); //get previous frame
						prevp = prev->GetReadPtr();
						prev_pitch = prev->GetPitch();//v1.6

						// get forward fft of prev frame from cache or calculation
						fftprev = get_plane_fft(prevp, src_height, src_width, prev_pitch, ncur-1, fftcachelist, fftcachecapacity, fftcache, winx, winy, winleft,  plan, isYUY2);//v1.6
						fftprev2 = get_plane_fft(prevp, src_height, src_width, prev_pitch, ncur-1, fftcachelist2, fftcachecapacity, fftcache2, winx, winy, winleft2,  plan, isYUY2);//v1.6
					}

					// do estimation for left and right windows

					// left window
					// prepare correlation data = mult fftsrc* by fftprev
					mult_conj_data2d (fftcur, fftprev, correl, winx, winy);
					// make inverse fft of prepared correl data
//					rdft2d(winy, winx, -1, correl, NULL, fftip, fftwork);
					fftwf_execute_dft_c2r_addr(planinv, correl, realcorrel); // added in v.1.0
					// now correl is is true correlation surface
					// find global motion vector as maximum on correlation sufrace
					// save vector to motion table
					// use  trust_limit decreased by factor 1.5, if will be improve pass later
//					get_motion_vector (realcorrel, winx, winy, trust_limit/(1.0f+0.5f*improve), dxmax, dymax, stab,  ncur, fieldbased, TFF, pixaspect, &dx1, &dy1, &trust1, debug );
					get_motion_vector (realcorrel, winx, winy, trust_limit, dxmax, dymax, stab,  ncur, fieldbased, TFF, pixaspect, &dx1, &dy1, &trust1, debug );//v1.6

					// right window
					// prepare correlation data = mult fftsrc* by fftprev
					mult_conj_data2d (fftcur2, fftprev2, correl2, winx, winy);
					// make inverse fft of prepared correl data
//					rdft2d(winy, winx, -1, correl2, NULL, fftip, fftwork);
					fftwf_execute_dft_c2r_addr(planinv, correl2, realcorrel2); // added in v.1.0
					// now correl is is true correlation surface
					// find global motion vector as maximum on correlation sufrace
					// save vector to motion table
//					get_motion_vector (realcorrel2, winx, winy, trust_limit/(1.0f+0.5f*improve), dxmax, dymax, stab, ncur, fieldbased, TFF, pixaspect, &dx2, &dy2,  &trust2,debug);
					get_motion_vector (realcorrel2, winx, winy, trust_limit, dxmax, dymax, stab, ncur, fieldbased, TFF, pixaspect, &dx2, &dy2,  &trust2,debug); //v1.6

					// now we have 2 motion data sets for left and right windows
					// estimate zoom factor
					zoom = 1 + (dx2 - dx1)/(winleft2-winleft);
					if (debug != 0) { // debug mode
						// output data for debugview utility
						sprintf(debugbuf,"DePanEstimate: n=%d dx=%7.2f %7.2f dy=%7.2f %7.2f trust=%5.1f %5.1f zoom=%7.5f\n", ncur, dx1, dx2, dy1, dy2, trust1, trust2, zoom);
						OutputDebugString(debugbuf);
					}
					if ( (dx1 != 0) && (dx2 != 0) && ( fabs(zoom-1) < (zoommax-1) ) ) { // if motion data and zoom good
							motionx[ncur] = (dx1 + dx2)/2;
							motiony[ncur] = (dy1 + dy2)/2;
							motionzoom[ncur] = zoom ;
							trust[ncur] = min(trust1, trust2);
					}
					else { // bad zoom,
							motionx[ncur] = 0;
							motiony[ncur] = 0;
							motionzoom[ncur] = 1;
							trust[ncur] = min(trust1, trust2);
					}

//					if (improve != 0) / did not never really work, disabled in v1.6
//					{	//  second improve estimation to make zoom motion estimation more precise
//						// make compensation of prev frame luma to current position with first estimation motion data
//					}

					if (show != 0 && ncur==ndest) {	// show correlation sufrace
						env->BitBlt(dstp, dst_pitch, srcp, src_pitch, dst_width, dst_height);
						showcorrelation(realcorrel,winx, winy, dstp, dst_pitch, winleft,(dst_height-winy)/2, isYUY2);//v1.6
						showcorrelation(realcorrel2,winx, winy, dstp, dst_pitch, winleft2,(dst_height-winy)/2, isYUY2);//v1.6
					}
				}
			}

		}
		else if (ncur==0 && ndest==0 && show!=0) { // simply copy luma plane as no correlation for frame 0
			env->BitBlt(dstp, dst_pitch, srcp, src_pitch, dst_width, dst_height);
		}

	}


	// check scenechanges in range, as sharp decreasing of trust
	for (ncur = ndest-range; ncur <= ndest+range; ncur++) {
		if (ncur-1>=0 && ncur < vi.num_frames && trust[ncur]<trust_limit*2  && trust[ncur]<0.5*trust[ncur-1]  ) {
			// very sharp decrease of not very big trust, probably due to scenechange
			motionx[ncur] = 0;
			motiony[ncur] = 0;
			motionzoom[ncur] = 1;
		}
		if (ncur>=0 && ncur+1 < vi.num_frames && trust[ncur]<trust_limit*2  && trust[ncur]<0.5*trust[ncur+1] ) {
			// very sharp decrease of not very big trust, probably due to scenechange
			motionx[ncur] = 0;
			motiony[ncur] = 0;
			motionzoom[ncur] = 1;
		}
	}

	if (debug != 0) { // debug mode
		// output summary data for debugview utility
		sprintf(debugbuf,"DePanEst.result: %d dx=%7.2f dy=%7.2f zoom=%7.5f trust=%7.2f\n", ncur, motionx[ndest], motiony[ndest], motionzoom[ndest], trust[ndest]);
		OutputDebugString(debugbuf);
		}


	// So, now we got all needed global motion info



	int xmsg = (fieldbased !=0) ? (width/4 - 15 + (ndest%2)*15) : width/4 - 8; // x-position of odd fields message //v1.6
	int ymsg = dst_height/40 - 4;  // y position
	if (info) { // show text info on frame
		if( show == 0) {  // luma plane was not copied by show, then copy it now
			env->BitBlt(dstp, dst_pitch, srcp, src_pitch, dst_width, dst_height);
		}
		sprintf(messagebuf," DePanEstimate");
		DrawString(dst,xmsg,ymsg,messagebuf, isYUY2);//v1.6
		if (motionx[ndest] == 0) {
			sprintf(messagebuf," SCENE CHANGE!");
			DrawString(dst,xmsg,ymsg+1,messagebuf, isYUY2);
		}
		sprintf(messagebuf," frame=%7d", ndest);
		DrawString(dst,xmsg,ymsg+2,messagebuf, isYUY2);
		sprintf(messagebuf," dx   =%7.2f", motionx[ndest]);
		DrawString(dst,xmsg,ymsg+3,messagebuf, isYUY2);
		sprintf(messagebuf," dy   =%7.2f", motiony[ndest]);
		DrawString(dst,xmsg,ymsg+4,messagebuf, isYUY2);
		sprintf(messagebuf," zoom =%7.5f", motionzoom[ndest]);
		DrawString(dst,xmsg,ymsg+5,messagebuf, isYUY2);
		sprintf(messagebuf," trust=%7.2f", trust[ndest]);
		DrawString(dst,xmsg,ymsg+6,messagebuf, isYUY2);
	}

	int nfirst = ndest - range;
	if (nfirst <0) nfirst = 0;
	int nlast = ndest + range;
	if (nlast > vi.num_frames -1) nlast = vi.num_frames -1;

	// write motion data for frames for nfirst to nlast frames to framebuffer of ndest frame
	// not write if show correlation surface
	if (show == 0) write_depan_data(dstp, nfirst,nlast,motionx,motiony,motionzoom);

	if ( (logfilename != "") && logfile != NULL ) {  // write log file if name correct
		// write frame number, dx, dy, rotation and zoom in Deshaker log format
		write_deshakerlog(logfile, vi.IsFieldBased(), vi.IsTFF(), ndest, motionx, motiony, motionzoom);

	}
	if ( (extlogfilename != "") && extlogfile != NULL ) {  // write log file if name correct
		// write frame number, dx, dy, rotation and zoom in Deshaker log format
		write_extlog(extlogfile, vi.IsFieldBased(), vi.IsTFF(), ndest, motionx, motiony, motionzoom, trust);
	}

	// end of Y plane Code

// ---------------------------------------------------------------------------

	if (!isYUY2)
	{ // YV12
		// Process U plane
		srcp = src->GetReadPtr(PLANAR_U);
		dstp = dst->GetWritePtr(PLANAR_U);

		const int dst_pitchUV = dst->GetPitch(PLANAR_U);	// The pitch,height and width information
		const int src_pitchUV = src->GetPitch(PLANAR_U);	// is guaranted to be the same for both
		const int dst_widthUV = dst->GetRowSize(PLANAR_U);	// the U and V planes so we only the U
		const int dst_heightUV = dst->GetHeight(PLANAR_U);	//plane values and use them for V as well
		// copy plane
		if (info != 0 || show != 0) env->BitBlt(dstp, dst_pitchUV, srcp, src_pitchUV, dst_widthUV, dst_heightUV);

		// end of U plane Code

	// ---------------------------------------------------------------------------
		// Process V plane
		srcp = src->GetReadPtr(PLANAR_V);
		dstp = dst->GetWritePtr(PLANAR_V);
		// copy plane
		if (info != 0 || show != 0) env->BitBlt(dstp, dst_pitchUV, srcp, src_pitchUV, dst_widthUV, dst_heightUV);

		// end of V plane code
	}

	return dst;
}


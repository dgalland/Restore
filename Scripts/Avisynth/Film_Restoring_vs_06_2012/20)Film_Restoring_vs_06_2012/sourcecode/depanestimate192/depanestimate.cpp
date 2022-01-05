/*
    DePanEstimate plugin for Avisynth 2.5 - global motion estimation
	Version 1.9.2, March 25, 2007
	(plugin interface)
	Copyright(c)2004-2007, A.G. Balakhnin aka Fizick
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

	Old version used FFT2D code by Takuya OOURA (email: ooura@kurims.kyoto-u.ac.jp)

	Since version 1.9, plugin use only FFTW library (http://www.fftw.org)
	as Windows binary DLL compiled with gcc under MinGW by Alessio Massaro,
	which support for threads and have AMD K7 (3dNow!) in addition to SSE/SSE2.
	ftp://ftp.fftw.org/pub/fftw/fftw3win32mingw.zip


	DePanEstimate - estimates global motion (pan) in frames (by phase-shift method),
		and put it to special service clip

	Functions of DePan plugin at second stage can shifts frame images for global motion compensation


  v1.9 - Remove DePanEstimate function from DePan.dll to separate plugin depanestimate.dll

	Parameters of DePanEstimate:
		clip - input clip
		range - number of previous (and also next) frames (fields) near requsted frame to estimate motion
		trust - frames correlation at scene change
		winx  - number of columns of fft window (must be power of 2). (width)
		winy  - number of rows of fft window (must be power of 2).    (height)
		dxmax - limit of x shift
		dymax - limit of y shift
		zoommax - maximum zoom factor ( if =1, zoom is not estimated)
		improve - improve zoom estimation by iteration
		stab -  calculated trust decreasing for large shifts
		pixaspect - pixel aspect
		info  - show motion info on frame
		log - output log file with motion data
		debug - output data for debugview utility
		show - show correlation sufrace
		fftw - use fftw external DLL library (dummy parameter since v1.9)
		extlog - output extended log file with motion and trust data

*/

#include "windows.h"
#include "avisynth.h"
#include "estimate_fftw.h"


AVSValue __cdecl Create_DePanEstimate(AVSValue args, void* user_data, IScriptEnvironment* env) {

		return new DePanEstimate_fftw(args[0].AsClip(), // the 0th parameter is the source clip
		 args[1].AsInt(0),		//  parameter - range.
		 (float)args[2].AsFloat(4.0),	//  parameter - trust.
		 args[3].AsInt(0),		//  parameter - the WINX value of FFT window.
		 args[4].AsInt(0),		//  parameter - the WINY value of FFT window.
		 args[5].AsInt(-1),		//  parameter - dxmax.
		 args[6].AsInt(-1),		//  parameter - dymax.
		 (float)args[7].AsFloat(1.0),	//  parameter - zoommax.
		 args[8].AsBool(false),//  parameter - improve (do not used anymore)
		 (float)args[9].AsFloat(1.0),	//  parameter - stab.
		 (float)args[10].AsFloat(1.0),	//  parameter - pixaspect.
		 args[11].AsBool(false),//  parameter - info.
		 args[12].AsString(""),	//  parameter - log.
		 args[13].AsBool(false),	//  parameter - debug.
		 args[14].AsBool(false),	//  parameter - show.
		 args[15].AsString(""),	//  parameter - extlog.
		 env);
}




extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env) {
    env->AddFunction("DePanEstimate", "c[range]i[trust]f[winx]i[winy]i[dxmax]i[dymax]i[zoommax]f[improve]b[stab]f[pixaspect]f[info]b[log]s[debug]b[show]b[extlog]s[fftw]b", Create_DePanEstimate, 0);

    return "`DePanEstimate' DePanEstimate plugin";
}


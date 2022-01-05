// Set of tools based on Motion Compensation

// Author: Manao
// Copyright(c)2006 A.G.Balakhnin aka Fizick - some modifications, additions (see releases history in provided documentation)
// See legal notice in Copying.txt for more information

// Avisynth: www.avisynth.org

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

#define WIN32_LEAN_AND_MEAN

//#include "AviSynth.h"
#include "MVInterface.h"

// Use MVClip / MVFilter helpers
#include "MVMask.h"
//#include "MVDenoise2.h"
#include "MVShow.h"
//#include "MVSpeed.h"
#include "MVCompensate.h"
#include "MVSCDetection.h"
//#include "MVIncrease.h"

#include "MVDepan.h" // added by Fizick
//#include "MVInter.h"
#include "MVFlow.h"
#include "MVFlowInter.h"
#include "MVFlowFps.h"
#include "MVFlowBlur.h"
//#include "MVFlowFps2.h"
#include "MVDegrain1.h"
#include "MVDegrain2.h"
#include "MVDegrain3.h"
#include "MVBlockFps.h"
//#include "MVCheckPel.h"

// Work in progress
//#include "MVBidouille.h"

// Special filter, not a good example
//#include "MVChangeCompensate.h"

// Analysing filter
#include "MVAnalyse.h"
#include "MVRecalculate.h"
#include "MVSuper.h"

// Test & helpers filters
//#include "TestFilter.h"
#include "Padding.h"
//#include "QDeQuant.h"
//#include "Deblock.h"
//#include "YUVSource.h"
//#include "Corrector.h"


#include "MVFinest.h"

//MVCore mvCore;



AVSValue __cdecl Create_Padding(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new Padding(
      args[0].AsClip(),
      args[1].AsInt(8),
		args[2].AsInt(8),
		args[3].AsBool(false), // planar
		env);
}
/*
AVSValue __cdecl Create_QDenoise(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new QDeQuant(
		args[0].AsClip(),
		args[1].AsInt(25),
      args[2].AsInt(0),
      args[3].AsInt(6),
		args[4].AsBool(true),
      args[5].AsBool(true),
		env);
}

AVSValue __cdecl Create_Deblock(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new Deblock(
		args[0].AsClip(),
      args[1].AsInt(25),
      args[2].AsInt(0),
      args[3].AsInt(0),
		args[4].AsBool(true),
      args[5].AsBool(true),
    	env);
}

AVSValue __cdecl Create_Corrector(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new Corrector(
		args[0].AsClip(),
      args[1],
      args[2].AsInt(0),
      args[3].AsInt(10),
		env);
}

AVSValue __cdecl Create_MVSpeed(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new MVSpeed(
		args[0].AsClip(),
		args[1].AsClip(),
      args[2].AsBool(true),
      args[3].AsBool(true),
      args[4].AsBool(true),
      args[5].AsBool(true),
      args[6].AsBool(true),
      args[7].AsBool(true),
      args[8].AsInt(MV_DEFAULT_SCD1),
      args[9].AsInt(MV_DEFAULT_SCD2),
      args[10].AsBool(true),
      args[11].AsBool(true),
		env);
}

AVSValue __cdecl Create_MVBidouille(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new MVBidouille(
		args[0].AsClip(),
		args[1].AsClip(),
      args[2].AsClip(),
      args[3].AsInt(mvCore.GetNextIdx()),
      args[4].AsInt(0),
      args[5].AsInt(MV_DEFAULT_SCD1),
      args[6].AsInt(MV_DEFAULT_SCD2),
      args[7].AsBool(true),
      args[8].AsBool(true),
		env);
}

*/
/*
AVSValue __cdecl Create_TestFilter(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new TestFilter(
		args[0].AsClip(),
		env);
}

AVSValue __cdecl Create_YUVSource(AVSValue args, void *user_data, IScriptEnvironment *env)
{
   return new YUVSource(
      args[0].AsString(""),
      args[1].AsInt(720),
      args[2].AsInt(576),
      env);
}

AVSValue __cdecl Create_MVInter(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	int time256 = int(args[3].AsFloat(50.0)*256.0/100.0);
	return new MVInter(
		args[0].AsClip(),
		args[1].AsClip(),
		args[2].AsClip(),
      time256,
      args[4].AsInt(0), // mode
      args[5].AsInt(0), // thres
      args[6].AsInt(mvCore.GetNextIdx()),
      args[7].AsInt(MV_DEFAULT_SCD1),
      args[8].AsInt(MV_DEFAULT_SCD2),
      args[9].AsBool(true),
      args[10].AsBool(true),
		env);
}
*/

/*

AVSValue __cdecl Create_MVChangeCompensate(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new MVChangeCompensate(
		args[0].AsClip(),
		args[1].AsClip(),
		args[2].AsBool(true), //sse
		env);
}

AVSValue __cdecl Create_MVDenoise2(AVSValue args, void* user_data, IScriptEnvironment* env)
{
   int thT = 10;
	int sadT = 200;
	int thMV = 30;

   int nMode = 0;
   nMode |= args[2].AsBool(true) ? YPLANE : 0;
   nMode |= args[3].AsBool(true) ? UPLANE : 0;
   nMode |= args[4].AsBool(true) ? VPLANE : 0;

	return new MVDenoise2(
		args[0].AsClip(),
      args[1],
      nMode,
      args[5].AsInt(thT),
		args[6].AsInt(sadT),
		args[7].AsInt(thMV),
      args[8].AsInt(MV_DEFAULT_SCD1),
      args[9].AsInt(MV_DEFAULT_SCD2),
      args[10].AsBool(true),
		env);
}
*/

AVSValue __cdecl Create_MVShow(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	int sc = 1;
	int sil = 0;
	int tol = 20000;
	return new MVShow(
		args[0].AsClip(), // super
		args[1].AsClip(), // vec
		args[2].AsInt(sc),
		args[3].AsInt(sil),
		args[4].AsInt(tol),
		args[5].AsBool(false), // showsad
		args[6].AsInt(-1), // number
      args[7].AsInt(MV_DEFAULT_SCD1),
      args[8].AsInt(MV_DEFAULT_SCD2),
      args[9].AsBool(true),
      args[10].AsBool(false), // planar
		env);
}

AVSValue __cdecl Create_MVCompensate(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new MVCompensate(
		args[0].AsClip(), // source
		args[1].AsClip(), // super
		args[2].AsClip(), // vec
      args[3].AsBool(true),
      args[4].AsFloat(0), //recursion
      args[5].AsInt(10000), //thSAD
      args[6].AsBool(false), // fields
     args[7].AsInt(MV_DEFAULT_SCD1),
      args[8].AsInt(MV_DEFAULT_SCD2),
      args[9].AsBool(true),
      args[10].AsBool(false), //planar
		env);
}
/*
AVSValue __cdecl Create_MVIncrease(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new MVIncrease(
		args[0].AsClip(),
		args[1].AsClip(),
      args[2].AsInt(2),                   // vertical
      args[3].AsInt(1),                   // horizontal
      args[4].AsBool(true),               // sc detection
      args[5].AsInt(mvCore.GetNextIdx()), // frame idx
      args[6].AsInt(MV_DEFAULT_SCD1),
      args[7].AsInt(MV_DEFAULT_SCD2),
      args[8].AsBool(true),
		env);
}
*/

AVSValue __cdecl Create_MVSCDetection(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new MVSCDetection(
		args[0].AsClip(),
		args[1].AsClip(),
      args[2].AsInt(255),
      args[3].AsInt(MV_DEFAULT_SCD1),
      args[4].AsInt(MV_DEFAULT_SCD2),
      args[5].AsBool(true),
		env);
}

AVSValue __cdecl Create_MVAnalyse(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	int blksize = 	args[1].AsInt(8);             // block size horizontal
    int blksizeV = args[2].AsInt(blksize);      // block size vertical

	int lambda;
	int lsad;
	int pnew;
	int plevel;
	bool global;
	int overlap = args[18].AsInt(0);

	bool truemotion = args[11].AsBool(true); // preset added in v0.9.13
	if (truemotion)
	{
		lambda = args[8].AsInt(1000*blksize*blksizeV/64);
		lsad = args[12].AsInt(1200);
		pnew = args[15].AsInt(50); // relative to 256 in v1.5.8
		plevel = args[13].AsInt(1);
		global = args[14].AsBool(true);
	}
	else // old versions 0.9.9.1 compatibility mode
	{
		lambda = args[8].AsInt(0);
		lsad = args[12].AsInt(400);
		pnew = args[15].AsInt(0);
		plevel = args[13].AsInt(0);
		global = args[14].AsBool(false);
	}


   return new MVAnalyse(
		args[0].AsClip(), // super
		blksize,
		blksizeV, // v.1.7
		args[3].AsInt(1),             // levels skip
		args[4].AsInt(4),   // search type
		args[5].AsInt(2),             // search parameter
		args[6].AsInt(0),             // search parameter at finest level
		args[7].AsBool(false),        // is backward
		lambda,             // lambda
      args[9].AsBool(true),        // chroma = true since v1.0.1
		args[10].AsInt(1),             // delta frame
		lsad,          // lsad - SAD limit for lambda using - added by Fizick (was internal fixed to 400 since v0.9.7)
		plevel,        // plevel - exponent for lambda scaling on level size  - added by Fizick
		global,  // use global motion predictor - added by Fizick
		pnew,          // pnew - cost penalty for new candidate vector - added by Fizick
		args[16].AsInt(pnew), // pzero - v1.10.3
		args[17].AsInt(0), // pglobal
		overlap, // overlap horizontal
		args[19].AsInt(overlap), // overlap vertical - v1.7
		args[20].AsString(""), // outfile - v1.2.6
		args[21].AsInt(0), // dct
		args[22].AsInt(0), // divide
		args[23].AsInt(0), // sadx264
		args[24].AsInt(10000), // badSAD
		args[25].AsInt(24), // badrange
		args[26].AsBool(true),        // isse
		env);
}

AVSValue __cdecl Create_MVMask(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	double ml = args[2].AsFloat(100);
	if (ml<=0)
	   env->ThrowError("MVMask: ML must be > 0.0");
	double gamma = 1;
   int Ysc = 0;
	return new MVMask(
		args[0].AsClip(), // source
		args[1].AsClip(), // vec
		ml,
		args[3].AsFloat(gamma),
		args[4].AsInt(0), // kind - replaced by Fizick
      args[5].AsInt(Ysc),
      args[6].AsInt(MV_DEFAULT_SCD1),
      args[7].AsInt(MV_DEFAULT_SCD2),
      args[8].AsBool(true),
      args[9].AsBool(false),
		env);
}

AVSValue __cdecl Create_MVDepan(AVSValue args, void* user_data, IScriptEnvironment* env) // Added by Fizick
{
	return new MVDepan(
		args[0].AsClip(),
		args[1].AsClip(),
		args[2].AsBool(true), // zoom
		args[3].AsBool(true), // rot
		(float)args[4].AsFloat(1), // pixaspect
		(float)args[5].AsFloat(15), // error
		args[6].AsBool(false), // info
		args[7].AsString(""), // log
      (float)args[8].AsFloat(10), // wrong difference
      (float)args[9].AsFloat(0.05f), // zeroMV weight
		args[10].AsInt(0), // range
      args[11].AsInt(MV_DEFAULT_SCD1),
      args[12].AsInt(MV_DEFAULT_SCD2),
      args[13].AsBool(true),
		env);
}


AVSValue __cdecl Create_MVFlow(AVSValue args, void* user_data, IScriptEnvironment* env)
{
   double time = args[3].AsFloat(100.0);
	if (time<0 || time>100)
	   env->ThrowError("MVFlow: Time must be from 0 to 100 percent.");
	int time256 = int(time*256.0/100.0);
	return new MVFlow(
		args[0].AsClip(), // source
		args[1].AsClip(), // super
		args[2].AsClip(), // vec
      time256,
      args[4].AsInt(0), // fetch or shift
      args[5].AsBool(false), // fields
      args[6].AsInt(MV_DEFAULT_SCD1),
      args[7].AsInt(MV_DEFAULT_SCD2),
      args[8].AsBool(true),
      args[9].AsBool(false), // planar
		env);
}

AVSValue __cdecl Create_MVFlowInter(AVSValue args, void* user_data, IScriptEnvironment* env)
{
   double time = args[4].AsFloat(50.0);
	if (time<0 || time>100)
	   env->ThrowError("MVFlowInter: Time must be from 0 to 100 percent.");
	int time256 = int(time*256.0/100.0);
	double ml = args[5].AsFloat(100);
	if (ml<=0)
	   env->ThrowError("MVFlowInter: ML must be > 0.0");
	return new MVFlowInter(
		args[0].AsClip(), // source
		args[1].AsClip(), // finest
		args[2].AsClip(), // mvbw
		args[3].AsClip(), // mvfw
      time256,
      ml, // ml
      args[6].AsBool(true), // blend
      args[7].AsInt(MV_DEFAULT_SCD1),
      args[8].AsInt(MV_DEFAULT_SCD2),
      args[9].AsBool(true), // isse
      args[10].AsBool(false), // planar
		env);
}

AVSValue __cdecl Create_MVFlowFps(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	double ml = args[7].AsFloat(100);
	if (ml<=0)
	   env->ThrowError("MVFlowFps: ML must be > 0.0");
	return new MVFlowFps(
		args[0].AsClip(), // source
		args[1].AsClip(), // finest
		args[2].AsClip(), // mvbw
		args[3].AsClip(), // mvfw
      args[4].AsInt(25), // num
      args[5].AsInt(1), // den
      args[6].AsInt(2), // maskmode - v1.8.1
      ml, // ml
      args[8].AsBool(true), // blend
      args[9].AsInt(MV_DEFAULT_SCD1),
      args[10].AsInt(MV_DEFAULT_SCD2),
      args[11].AsBool(true), // isse
      args[12].AsBool(false), // planar
		env);
}

AVSValue __cdecl Create_MVFlowBlur(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	double time = args[4].AsFloat(50.0);
	if (time<0 || time>200)
	   env->ThrowError("MVFlowBlur: Blur time must be from 0 to 200 percent.");
	int blur256 = int(time*256.0/200.0);
	return new MVFlowBlur(
		args[0].AsClip(), // source
		args[1].AsClip(), // finest
		args[2].AsClip(), // mvbw
		args[3].AsClip(), // mvfw
      blur256,
      args[5].AsInt(1), // prec
      args[6].AsInt(MV_DEFAULT_SCD1),
      args[7].AsInt(MV_DEFAULT_SCD2),
      args[8].AsBool(true), // isse
      args[9].AsBool(false), // planar
		env);
}
/*
AVSValue __cdecl Create_MVFlowFps2(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	if (!args[1].Defined() || !args[2].Defined() || !args[3].Defined() || !args[4].Defined())
	   env->ThrowError("MVFlowFps2: must be source and 4 vector clips");
	double ml = args[8].AsFloat(100);
	if (ml<=0)
	   env->ThrowError("MVFlowFps2: ML must be > 0.0");
	return new MVFlowFps2(
		args[0].AsClip(),
		args[1].AsClip(),
		args[2].AsClip(),
		args[3].AsClip(),
		args[4].AsClip(),
      args[5].AsInt(25), // num
      args[6].AsInt(1), // den
      args[7].AsInt(2), // maskmode - v1.8.3
      ml, // ml
	 (args[9].Defined() ? args[9].AsClip() : 0), // pelclip
      args[10].AsInt(mvCore.GetNextIdx()),
      args[11].AsInt(mvCore.GetNextIdx()),
      args[12].AsInt(MV_DEFAULT_SCD1),
      args[13].AsInt(MV_DEFAULT_SCD2),
      args[14].AsBool(true),
		env);
}
*/
AVSValue __cdecl Create_MVDegrain1(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	int plane = args[6].AsInt(4);
	int YUVplanes;

	switch (plane)
	{
		case 0:
			YUVplanes = 1;
			break;
		case 1:
			YUVplanes = 2;
			break;
		case 2:
			YUVplanes = 4;
			break;
		case 3:
			YUVplanes = 6;
			break;
		case 4:
		default:
			YUVplanes = 7;
	}

	int thSAD = args[4].AsInt(400);

	return new MVDegrain1(
		args[0].AsClip(), // source
		args[1].AsClip(), // super clip
		args[2].AsClip(), // mvbw
		args[3].AsClip(), // mvfw
		thSAD, // thSAD
		args[5].AsInt(thSAD), // thSADC
		YUVplanes, // YUV planes
		args[7].AsInt(255), // limit
      args[8].AsInt(MV_DEFAULT_SCD1),
      args[9].AsInt(MV_DEFAULT_SCD2),
      args[10].AsBool(true),
      args[11].AsBool(false), // planar
		env);
}

AVSValue __cdecl Create_MVDegrain2(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	int plane = args[8].AsInt(4);
	int YUVplanes;

	switch (plane)
	{
		case 0:
			YUVplanes = 1;
			break;
		case 1:
			YUVplanes = 2;
			break;
		case 2:
			YUVplanes = 4;
			break;
		case 3:
			YUVplanes = 6;
			break;
		case 4:
		default:
			YUVplanes = 7;
	}

	int thSAD = args[6].AsInt(400);

	return new MVDegrain2(
		args[0].AsClip(), // source
		args[1].AsClip(), // super
		args[2].AsClip(), // mvbw
		args[3].AsClip(), // mvfw
		args[4].AsClip(), // mvbw2
		args[5].AsClip(), // mvfw2
		thSAD, // thSAD
		args[7].AsInt(thSAD), // thSAD
		YUVplanes, // YUV planes
		args[9].AsInt(255), // limit
      args[10].AsInt(MV_DEFAULT_SCD1),
      args[11].AsInt(MV_DEFAULT_SCD2),
      args[12].AsBool(true),
      args[13].AsBool(false),
		env);
}

AVSValue __cdecl Create_MVDegrain3(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	int plane = args[10].AsInt(4);
	int YUVplanes;

	switch (plane)
	{
		case 0:
			YUVplanes = 1;
			break;
		case 1:
			YUVplanes = 2;
			break;
		case 2:
			YUVplanes = 4;
			break;
		case 3:
			YUVplanes = 6;
			break;
		case 4:
		default:
			YUVplanes = 7;
	}

	int thSAD = args[8].AsInt(400);

	return new MVDegrain3(
		args[0].AsClip(), // source
		args[1].AsClip(), // super
		args[2].AsClip(), // mvbw
		args[3].AsClip(), // mvfw
		args[4].AsClip(), // mvbw2
		args[5].AsClip(), // mvfw2
		args[6].AsClip(), // mvbw3
		args[7].AsClip(), // mvfw3
		thSAD, // thSAD
		args[9].AsInt(thSAD), // thSAD
		YUVplanes, // YUV planes
		args[11].AsInt(255), // limit
      args[12].AsInt(MV_DEFAULT_SCD1),
      args[13].AsInt(MV_DEFAULT_SCD2),
      args[14].AsBool(true),
      args[15].AsBool(false),
		env);
}

AVSValue __cdecl Create_MVRecalculate(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	int blksize = 	args[4].AsInt(8);             // block size horizontal
    int blksizeV = args[5].AsInt(blksize);      // block size vertical

	int lambda;
	int pnew;

	int overlap = args[12].AsInt(0);

	bool truemotion = args[10].AsBool(true); // preset added in v0.9.13
	if (truemotion)
	{
		lambda = args[8].AsInt(1000*blksize*blksizeV/64);
		pnew = args[11].AsInt(50); // relative to 256 in v1.5.8
	}
	else // old versions 0.9.9.1 compatibility mode
	{
		lambda = args[8].AsInt(0);
		pnew = args[11].AsInt(0);
	}
   return new MVRecalculate(
		args[0].AsClip(), // super
		args[1].AsClip(), // vectors
        args[2].AsInt(200), // thSAD
        args[3].AsInt(1), // smooth
		blksize,
		blksizeV, // v.1.7
		args[6].AsInt(4),   // search type
		args[7].AsInt(2),             // search parameter
		lambda,             // lambda
      args[9].AsBool(true),        // chroma = true since v1.0.1
		pnew,          // pnew - cost penalty for new candidate vector - added by Fizick
		overlap, // overlap horizontal
		args[13].AsInt(overlap), // overlap vertical - v1.7
		args[14].AsString(""), // outfile - v1.2.6
		args[15].AsInt(0), // dct
		args[16].AsInt(0), // divide
		args[17].AsInt(0), // sadx264
		args[18].AsBool(true),        // isse
		env);
}

AVSValue __cdecl Create_MVBlockFps(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new MVBlockFps(
		args[0].AsClip(), // src
		args[1].AsClip(), // super
		args[2].AsClip(), // bw
		args[3].AsClip(), // fw
      args[4].AsInt(25), // num
      args[5].AsInt(1), // den
      args[6].AsInt(0), // mode
      args[7].AsInt(0), // thres
      args[8].AsBool(true), // blend
      args[9].AsInt(MV_DEFAULT_SCD1),
      args[10].AsInt(MV_DEFAULT_SCD2),
      args[11].AsBool(true), // isse
      args[12].AsBool(false), // planar
		env);
}
/*
AVSValue __cdecl Create_MVCheckPel(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new MVCheckPel(
		args[0].AsClip(),
		args[1].AsClip(),
	  (args[2].Defined() ? args[2].AsClip() : 0), // pelclip
      args[3].AsInt(mvCore.GetNextIdx()),
      args[4].AsInt(MV_DEFAULT_SCD1),
      args[5].AsInt(MV_DEFAULT_SCD2),
      args[6].AsBool(true),
		env);
}

*/


AVSValue __cdecl Create_MVSuper(AVSValue args, void* user_data, IScriptEnvironment* env)
{
   return new MVSuper(
		args[0].AsClip(), // source
		args[1].AsInt(8),             // hpad
		args[2].AsInt(8),             // vpad
		args[3].AsInt(2),             // pel
		args[4].AsInt(0),             // levels
        args[5].AsBool(true),        // chroma
		args[6].AsInt(2), // sharp
		args[7].AsInt(2), // rfilter
		(args[8].Defined() ? args[8].AsClip() : 0), // pelclip
		args[9].AsBool(true),        // isse
		args[10].AsBool(false),        // planar
		env);
}

AVSValue __cdecl Create_MVFinest(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new MVFinest(
      args[0].AsClip(), // super
      args[1].AsInt(true), // isse
		env);
}


extern "C" __declspec(dllexport) const char* __stdcall
AvisynthPluginInit2(IScriptEnvironment* env) {

//	env->AddFunction("MVDenoise", "cc+[Y]b[U]b[V]b[thT]i[thSAD]i[thMV]i[thSCD1]i[thSCD2]i[isse]b", Create_MVDenoise2, 0);
	env->AddFunction("MShow", "cc[scale]i[sil]i[tol]i[showsad]b[number]i[thSCD1]i[thSCD2]i[isse]b[planar]b", Create_MVShow, 0);
	//env->AddFunction("MVSpeed", "c[vectors]c[srcluma]b[refluma]b[var]b[compy]b[compu]b[compv]b[thSCD1]i[thSCD2]i[mmx]b[isse]b", Create_MVSpeed, 0);
	env->AddFunction("MAnalyse", "c[blksize]i[blksizeV]i[level]i[search]i[searchparam]i[pelsearch]i[isb]b[lambda]i[chroma]b[delta]i[truemotion]b[lsad]i[plevel]i[global]b[pnew]i[pzero]i[pglobal]i[overlap]i[overlapV]i[outfile]s[dct]i[divide]i[sadx264]i[badSAD]i[badrange]i[isse]b", Create_MVAnalyse, 0);
	env->AddFunction("MMask", "cc[ml]f[gamma]f[kind]i[Ysc]i[thSCD1]i[thSCD2]i[isse]b[planar]b", Create_MVMask, 0);
   //env->AddFunction("MVBidouille", "c[mvbw]c[mvfw]c[idx]i[boo]i[thSCD1]i[thSCD2]i[mmx]b[isse]b", Create_MVBidouille, 0);
	env->AddFunction("MCompensate", "ccc[scbehavior]b[recursion]f[thSAD]i[fields]b[thSCD1]i[thSCD2]i[isse]b[planar]b", Create_MVCompensate, 0);
//   env->AddFunction("MVIncrease", "c[vectors]c[vertical]i[horizontal]i[scbehavior]b[idx]i[thSCD1]i[thSCD2]i[isse]b", Create_MVIncrease, 0);
//   env->AddFunction("MVChangeCompensate", "cc[isse]b", Create_MVChangeCompensate, 0);
   env->AddFunction("MSCDetection", "cc[Yth]i[thSCD1]i[thSCD2]i[isse]b", Create_MVSCDetection, 0);
//	env->AddFunction("TestFilter","c", Create_TestFilter, 0);
//	env->AddFunction("Padding","c[hPad]i[vPad]i[planar]b", Create_Padding, 0);
//   env->AddFunction("YUVSource","[file]s[width]i[height]i", Create_YUVSource, 0);
//   env->AddFunction("Corrector", "cc+[mode]i[th]i", Create_Corrector, 0);
//   env->AddFunction("Deblock", "c[quant]i[aOffset]i[bOffset]i[mmx]b[isse]b", Create_Deblock, 0);
	env->AddFunction("MDepan", "cc[zoom]b[rot]b[pixaspect]f[error]f[info]b[log]s[wrong]f[zerow]f[range]i[thSCD1]i[thSCD2]i[isse]b", Create_MVDepan, 0);
//	env->AddFunction("MVInter", "c[mvbw]c[mvfw]c[time]f[mode]i[thres]i[idx]i[thSCD1]i[thSCD2]i[mmx]b[isse]b", Create_MVInter, 0);
	env->AddFunction("MFlow", "ccc[time]f[mode]i[fields]b[thSCD1]i[thSCD2]i[isse]b[planar]b[planar]b", Create_MVFlow, 0);
	env->AddFunction("MFlowInter", "cccc[time]f[ml]f[pelclip]c[idx]i[blend]b[thSCD1]i[thSCD2]i[isse]b[planar]b", Create_MVFlowInter, 0);
	env->AddFunction("MFlowFps", "cccc[num]i[den]i[mask]i[ml]f[blend]b[thSCD1]i[thSCD2]i[isse]b[planar]b", Create_MVFlowFps, 0);
	env->AddFunction("MFlowBlur", "cccc[blur]f[prec]i[thSCD1]i[thSCD2]i[isse]b[planar]b", Create_MVFlowBlur, 0);
//	env->AddFunction("MVFlowFps2", "c[mvbw]c[mvfw]c[mvbw2]c[mvfw2]c[num]i[den]i[mask]i[ml]f[pelclip]c[idx]i[idx2]i[thSCD1]i[thSCD2]i[isse]b", Create_MVFlowFps2, 0);
	env->AddFunction("MDegrain1", "cccc[thSAD]i[thSADC]i[plane]i[limit]i[thSCD1]i[thSCD2]i[isse]b[planar]b", Create_MVDegrain1, 0);
	env->AddFunction("MDegrain2", "cccccc[thSAD]i[thSADC]i[plane]i[limit]i[thSCD1]i[thSCD2]i[isse]b[planar]b", Create_MVDegrain2, 0);
	env->AddFunction("MDegrain3", "cccccccc[thSAD]i[thSADC]i[plane]i[limit]i[thSCD1]i[thSCD2]i[isse]b[planar]b", Create_MVDegrain3, 0);
	env->AddFunction("MRecalculate", "cc[thsad]i[smooth]i[blksize]i[blksizeV]i[search]i[searchparam]i[lambda]i[chroma]b[truemotion]b[pnew]i[overlap]i[overlapV]i[outfile]s[dct]i[divide]i[sadx264]i[isse]b", Create_MVRecalculate, 0);
	env->AddFunction("MBlockFps", "cccc[num]i[den]i[mode]i[thres]i[blend]b[thSCD1]i[thSCD2]i[isse]b[planar]b", Create_MVBlockFps, 0);
//	env->AddFunction("MVCheckPel", "c[vectors]c[pelclip]c[idx]i[thSCD1]i[thSCD2]i[isse]b", Create_MVCheckPel, 0);
	env->AddFunction("MSuper", "c[hpad]i[vpad]i[pel]i[levels]i[chroma]b[sharp]i[rfilter]i[pelclip]c[isse]b[planar]b", Create_MVSuper, 0);
//	env->AddFunction("MVFinest", "c[isse]b", Create_MVFinest, 0);
	return("MVTools : set of tools based on a motion estimation engine");
}

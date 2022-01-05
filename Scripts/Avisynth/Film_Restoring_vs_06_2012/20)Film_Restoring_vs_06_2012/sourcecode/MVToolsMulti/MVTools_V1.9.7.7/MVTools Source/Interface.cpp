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

#include "MVInterface.h"
#include "Padding.h"
#include "Critical Section.hpp"

// Use MVClip / MVFilter helpers
#include "MVMask.h"
#include "MVShow.h"
#include "MVCompensate.h"
#include "MVSCDetection.h"
#include "MVIncrease.h"

#include "MVDepan.h" // added by Fizick
#include "MVFlow.h"
#include "MVFlowInter.h"
#include "MVFlowFps.h"
#include "MVFlowBlur.h"
#include "MVFlowFps2.h"
#include "MVDegrain1.h"
#include "MVDegrain2.h"
#include "MVDegrain3.h"
#include "MVDegrainMulti.h"
#include "MVMultiExtract.h"

// Work in progress

// Special filter, not a good example
#include "MVChangeCompensate.h"

// Analysing filter
#include "MVAnalyse.h"
#include "MVAnalyseMulti.h"

// Test & helpers filters

MVCore mvCore;

AVSValue __cdecl Create_MVChangeCompensate(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    return new MVChangeCompensate(args[0].AsClip(),
                                  args[1].AsClip(),
                                  args[2].AsBool(true), //sse
                                  env);
}

AVSValue __cdecl Create_MVShow(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    int sc = 1;
    int sil = 0;
    int tol = 20000;

    return new MVShow(args[0].AsClip(),
                      args[1].AsClip(),
                      args[2].AsInt(sc),
                      args[3].AsInt(sil),
                      args[4].AsInt(tol),
                      args[5].AsBool(false),
                      args[6].AsInt(MV_DEFAULT_SCD1),
                      args[7].AsInt(MV_DEFAULT_SCD2),
                      args[8].AsBool(true),
                      args[9].AsBool(true),
                      env);
}

AVSValue __cdecl Create_MVCompensate(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    int const thSAD=10000;

    return new MVCompensate(args[0].AsClip(),
                            args[1].AsClip(),
                            args[2].AsBool(true),
                            args[3].AsInt(1), //mode
                            args[4].AsInt(thSAD), //thSAD
                            args[5].AsInt(thSAD), //thSADC
                            args[6].AsBool(false), // fields
                            (args[7].Defined() ? args[7].AsClip() : 0), // pelclip
                            args[8].AsInt(mvCore.GetEmptyIndex()),
                            args[9].AsInt(MV_DEFAULT_SCD1),
                            args[10].AsInt(MV_DEFAULT_SCD2),
                            args[11].AsBool(true),
                            args[12].AsBool(true),
                            env);
}

AVSValue __cdecl Create_MVIncrease(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    return new MVIncrease(args[0].AsClip(),
                          args[1].AsClip(),
                          args[2].AsInt(2),                   // vertical
                          args[3].AsInt(1),                   // horizontal
                          args[4].AsBool(true),               // sc detection
                          args[5].AsInt(mvCore.GetEmptyIndex()), // frame idx
                          args[6].AsInt(MV_DEFAULT_SCD1),
                          args[7].AsInt(MV_DEFAULT_SCD2),
                          args[8].AsBool(true),
                          args[9].AsBool(true),
                          env);
}


AVSValue __cdecl Create_MVSCDetection(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    return new MVSCDetection(args[0].AsClip(),
                             args[1].AsClip(),
                             args[2].AsInt(255),
                             args[3].AsInt(MV_DEFAULT_SCD1),
                             args[4].AsInt(MV_DEFAULT_SCD2),
                             args[5].AsBool(true),
                             args[6].AsBool(true),
                             env);
}

AVSValue __cdecl Create_MVAnalyse(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    int blksize = 	args[1].AsInt(8);             // block size horizontal
    int blksizeV = args[2].AsInt(blksize);      // block size vertical

    int pel = args[3].AsInt(2); // pel
    int lambda;
    int lsad;
    int pnew;
    int plevel;
    bool global;

    int overlap = args[17].AsInt(0);

    bool truemotion = args[12].AsBool(true); // preset added in v0.9.13
    if (truemotion)
    {
        lambda = args[9].AsInt(1000*blksize*blksizeV/64);
        lsad = args[13].AsInt(MV_DEFAULT_SCD1*4*blksize*blksizeV/64);
        pnew = args[14].AsInt(50); // relative to 256 in v1.5.8
        plevel = args[15].AsInt(1);
        global = args[16].AsBool(true);
    }
    else // old versions 0.9.9.1 compatibility mode
    {
        lambda = args[9].AsInt(0);
        lsad = args[13].AsInt(MV_DEFAULT_SCD1*4/3*blksize*blksizeV/64);
        pnew = args[14].AsInt(0);
        plevel = args[15].AsInt(0);
        global = args[16].AsBool(false);
    }


    return new MVAnalyse(args[0].AsClip(),
                         blksize,
                         blksizeV, // v.1.7
                         args[3].AsInt(2),             // pel
                         args[4].AsInt(0),             // levels skip
                         args[5].AsInt(LOGARITHMIC),   // search type
                         args[6].AsInt(2),             // search parameter
                         args[7].AsInt(pel),             // search parameter at finest level
                         args[8].AsBool(false),        // is backward
                         lambda,             // lambda
                         args[10].AsBool(true),        // chroma = true since v1.0.1
                         args[11].AsInt(1),             // delta frame
                         lsad,          // lsad - SAD limit for lambda using - added by Fizick (was internal fixed to 400 since v0.9.7)
                         pnew,          // pnew - cost penalty for new candidate vector - added by Fizick
                         plevel,        // plevel - exponent for lambda scaling on level size  - added by Fizick
                         global,  // use global motion predictor - added by Fizick
                         overlap, // overlap horizontal
                         args[18].AsInt(overlap), // overlap vertical - v1.7
                         args[19].AsString(""), // outfile - v1.2.6
                         args[20].AsInt(2), // sharp - v1.3
                         (args[21].Defined() ? args[21].AsClip() : 0), // pelclip - v1.6, renamed to pelclip in v1.8.2
                         args[22].AsInt(0), // dct
                         args[23].AsInt(0), // divide
                         args[24].AsInt(mvCore.GetEmptyIndex()),   // index of the frames for this analysis
                         args[25].AsInt(0), // sadx264
                         args[26].AsBool(true),        // mmx
                         args[27].AsBool(true),        // isse
                         env);
}

AVSValue __cdecl Create_MVAnalyseMulti(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    int blksize = 	args[1].AsInt(8);             // block size horizontal
    int blksizeV = args[2].AsInt(blksize);        // block size vertical

    int pel = args[3].AsInt(2); // pel
    int lambda;
    int lsad;
    int pnew;
    int plevel;
    bool global;

    int overlap = args[16].AsInt(0);

    bool truemotion = args[11].AsBool(true); // preset added in v0.9.13
    if (truemotion)
    {
        lambda = args[9].AsInt(1000*blksize*blksizeV/64);
        lsad = args[12].AsInt(MV_DEFAULT_SCD1*4*blksize*blksizeV/64);
        pnew = args[13].AsInt(50); // relative to 256 in v1.5.8
        plevel = args[14].AsInt(1);
        global = args[15].AsBool(true);
    }
    else // old versions 0.9.9.1 compatibility mode
    {
        lambda = args[9].AsInt(0);
        lsad = args[12].AsInt(MV_DEFAULT_SCD1*4/3*blksize*blksizeV/64);
        pnew = args[13].AsInt(0);
        plevel = args[14].AsInt(0);
        global = args[15].AsBool(false);
    }

	// get the system information
    SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);

    return new MVAnalyseMulti(args[0].AsClip(),
                              blksize,
                              blksizeV, // v.1.7
                              args[3].AsInt(2),             // pel
                              args[4].AsInt(0),             // levels skip
                              args[5].AsInt(LOGARITHMIC),   // search type
                              args[6].AsInt(2),             // search parameter
                              args[7].AsInt(pel),             // search parameter at finest level
                              args[8].AsInt(1),              // reference frames
                              lambda,             // lambda
                              args[10].AsBool(true),        // chroma = true since v1.0.1
                              lsad,          // lsad - SAD limit for lambda using - added by Fizick (was internal fixed to 400 since v0.9.7)
                              pnew,          // pnew - cost penalty for new candidate vector - added by Fizick
                              plevel,        // plevel - exponent for lambda scaling on level size  - added by Fizick
                              global,  // use global motion predictor - added by Fizick
                              overlap, // overlap horizontal
                              args[17].AsInt(overlap), // overlap vertical - v1.7
                              args[18].AsString(""), // outfile - v1.2.6
                              args[19].AsInt(2), // sharp - v1.3
                              (args[20].Defined() ? args[20].AsClip() : 0), // pelclip - v1.6, renamed to pelclip in v1.8.2
                              args[21].AsInt(0), // dct
                              args[22].AsInt(0), // divide
                              args[23].AsInt(mvCore.GetEmptyIndex()),   // index of the frames for this analysis
                              args[24].AsInt(0), // sadx264
                              args[25].AsBool(true),        // mmx
                              args[26].AsBool(true),        // isse
                              args[27].AsInt(1),            // deltamult
                              args[28].AsInt(SystemInfo.dwNumberOfProcessors),           // max number of threads
                              args[29].AsInt(1),            // prefetch
                              env);
}


AVSValue __cdecl Create_MVMask(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    double ml = args[2].AsFloat(100);
    if (ml<=0)
        env->ThrowError("MVMask: ML must be > 0.0");
    double gamma = 1;
    int Ysc = 0;
    return new MVMask(args[0].AsClip(),
                      args[1].AsClip(),
                      ml,
                      args[3].AsFloat(gamma),
                      args[4].AsInt(0), // kind - replaced by Fizick
                      args[5].AsInt(Ysc),
                      args[6].AsInt(MV_DEFAULT_SCD1),
                      args[7].AsInt(MV_DEFAULT_SCD2),
                      args[8].AsBool(true),
                      args[9].AsBool(true),
                      env);
}

AVSValue __cdecl Create_MVDepan(AVSValue args, void* user_data, IScriptEnvironment* env) // Added by Fizick
{
    return new MVDepan(args[0].AsClip(),
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
                       args[14].AsBool(true),
                       env);
}

AVSValue __cdecl Create_MVFlow(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    double time = args[2].AsFloat(100.0);
    if (time<0 || time>100)
        env->ThrowError("MVFlow: Time must be from 0 to 100 percent.");
    int time256 = int(time*256.0/100.0);

    return new MVFlow(args[0].AsClip(),
                      args[1].AsClip(),
                      time256,
                      args[3].AsInt(0), // fetch or shift
                      args[4].AsBool(false), // fields
                      (args[5].Defined() ? args[5].AsClip() : 0), // pelclip
                      args[6].AsInt(mvCore.GetEmptyIndex()),
                      args[7].AsInt(MV_DEFAULT_SCD1),
                      args[8].AsInt(MV_DEFAULT_SCD2),
                      args[9].AsBool(true),
                      args[10].AsBool(true),
                      env);
}

AVSValue __cdecl Create_MVFlowInter(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    double time = args[3].AsFloat(50.0);
    if (time<0 || time>100)
        env->ThrowError("MVFlowInter: Time must be from 0 to 100 percent.");
    int time256 = int(time*256.0/100.0);
    double ml = args[4].AsFloat(100);
    if (ml<=0)
        env->ThrowError("MVFlowInter: ML must be > 0.0");
    double thSAD = args[5].AsFloat(200);
    if (thSAD<=0)
        env->ThrowError("MVFlowInter: thSAD must be > 0.0");

    return new MVFlowInter(args[0].AsClip(),
                           args[1].AsClip(),
                           args[2].AsClip(),
                           time256,
                           ml, // ml
                           thSAD, // thSAD
                           thSAD, // thSADC
                           (args[7].Defined() ? args[7].AsClip() : 0), // pelclip
                           args[8].AsInt(mvCore.GetEmptyIndex()),
                           args[9].AsInt(MV_DEFAULT_SCD1),
                           args[10].AsInt(MV_DEFAULT_SCD2),
                           args[11].AsBool(true),
                           args[12].AsBool(true),
                           env);
}

AVSValue __cdecl Create_MVFlowBlur(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    double time = args[3].AsFloat(50.0);
    if (time<0 || time>200)
        env->ThrowError("MVFlowBlur: Blur time must be from 0 to 200 percent.");
    int blur256 = int(time*256.0/200.0);

    return new MVFlowBlur(args[0].AsClip(),
                          args[1].AsClip(),
                          args[2].AsClip(),
                          blur256,
                          args[4].AsInt(1), // prec
                          (args[5].Defined() ? args[5].AsClip() : 0), // pelclip
                          args[6].AsInt(mvCore.GetEmptyIndex()),
                          args[7].AsInt(MV_DEFAULT_SCD1),
                          args[8].AsInt(MV_DEFAULT_SCD2),
                          args[9].AsBool(true),
                          args[10].AsBool(true),
                          env);
}

AVSValue __cdecl Create_MVFlowFps(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    double ml = args[6].AsFloat(100);
    if (ml<=0)
     env->ThrowError("MVFlowFps: ML must be > 0.0");
    double thSAD = args[7].AsFloat(200);
    if (thSAD<=0)
        env->ThrowError("MVFlowInter: thSAD must be > 0.0");

    return new MVFlowFps(args[0].AsClip(),
                         args[1].AsClip(),
                         args[2].AsClip(),
                         args[3].AsInt(25), // num
                         args[4].AsInt(1), // den
                         args[5].AsInt(2), // maskmode - v1.8.1
                         ml, // ml
                         thSAD, // thSAD
                         thSAD, // thSADC
                         (args[9].Defined() ? args[9].AsClip() : 0), // pelclip
                         args[10].AsInt(mvCore.GetEmptyIndex()),
                         args[11].AsInt(MV_DEFAULT_SCD1),
                         args[12].AsInt(MV_DEFAULT_SCD2),
                         args[13].AsBool(true),
                         args[14].AsBool(true),
                         env);
}

AVSValue __cdecl Create_MVFlowFps2(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    if (!args[1].Defined() || !args[2].Defined() || !args[3].Defined() || !args[4].Defined())
        env->ThrowError("MVFlowFps2: must be source and 4 vector clips");
    double ml = args[8].AsFloat(100);
    if (ml<=0)
        env->ThrowError("MVFlowFps2: ML must be > 0.0");
    double thSAD = args[9].AsFloat(200);
    if (thSAD<=0)
        env->ThrowError("MVFlowInter: thSAD must be > 0.0");

    return new MVFlowFps2(args[0].AsClip(),
                          args[1].AsClip(),
                          args[2].AsClip(),
                          args[3].AsClip(),
                          args[4].AsClip(),
                          args[5].AsInt(25), // num
                          args[6].AsInt(1), // den
                          args[7].AsInt(2), // maskmode - v1.8.3
                          ml, // ml
                          thSAD, // thSAD
                          thSAD, // thSADC
                          (args[11].Defined() ? args[11].AsClip() : 0), // pelclip
                          args[12].AsInt(mvCore.GetEmptyIndex()),
                          args[13].AsInt(mvCore.GetEmptyIndex()),
                          args[14].AsInt(MV_DEFAULT_SCD1),
                          args[15].AsInt(MV_DEFAULT_SCD2),
                          args[16].AsBool(true),
                          args[17].AsBool(true),
                          env);
}

AVSValue __cdecl Create_MVDegrain1(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    int plane = args[5].AsInt(4);
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

    int thSAD = args[3].AsInt(400);

	// get the system information
    SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);

    return new MVDegrain1(args[0].AsClip(),
                          args[1].AsClip(), // mvbw
                          args[2].AsClip(), // mvfw
                          thSAD, // thSAD
                          args[4].AsInt(thSAD), // thSADC
                          YUVplanes, // YUV planes
                          args[6].AsInt(255), // limit
                          (args[7].Defined() ? args[7].AsClip() : 0), // pelclip
                          args[8].AsInt(mvCore.GetEmptyIndex()),
                          args[9].AsInt(MV_DEFAULT_SCD1),
                          args[10].AsInt(MV_DEFAULT_SCD2),
                          args[11].AsBool(true),
                          args[12].AsBool(true),
                          args[13].AsInt(SystemInfo.dwNumberOfProcessors), // max threads
                          args[14].AsInt(1),  // prefetch
                          args[15].AsInt(0),  // SadMode
                          env);
}

AVSValue __cdecl Create_MVDegrain2(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    int plane = args[7].AsInt(4);
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

    int thSAD = args[5].AsInt(400);

	// get the system information
    SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);

    return new MVDegrain2(args[0].AsClip(),
                          args[1].AsClip(), // mvbw
                          args[2].AsClip(), // mvfw
                          args[3].AsClip(), // mvbw2
                          args[4].AsClip(), // mvfw2
                          thSAD, // thSAD
                          args[6].AsInt(thSAD), // thSAD
                          YUVplanes, // YUV planes
                          args[8].AsInt(255), // limit
                          (args[9].Defined() ? args[9].AsClip() : 0), // pelclip
                          args[10].AsInt(mvCore.GetEmptyIndex()),
                          args[11].AsInt(MV_DEFAULT_SCD1),
                          args[12].AsInt(MV_DEFAULT_SCD2),
                          args[13].AsBool(true),
                          args[14].AsBool(true),
                          args[15].AsInt(SystemInfo.dwNumberOfProcessors), // max threads
                          args[16].AsInt(1),  // prefetch
                          args[17].AsInt(0),  // SadMode
                          env);
}

AVSValue __cdecl Create_MVDegrain3(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    int plane = args[9].AsInt(4);
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

    int thSAD = args[7].AsInt(400);

	// get the system information
    SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);

    return new MVDegrain3(args[0].AsClip(),
                          args[1].AsClip(), // mvbw
                          args[2].AsClip(), // mvfw
                          args[3].AsClip(), // mvbw2
                          args[4].AsClip(), // mvfw2
                          args[5].AsClip(), // mvbw3
                          args[6].AsClip(), // mvfw3
                          thSAD, // thSAD
                          args[8].AsInt(thSAD), // thSAD
                          YUVplanes, // YUV planes
                          args[10].AsInt(255), // limit
                          (args[11].Defined() ? args[11].AsClip() : 0), // pelclip
                          args[12].AsInt(mvCore.GetEmptyIndex()),
                          args[13].AsInt(MV_DEFAULT_SCD1),
                          args[14].AsInt(MV_DEFAULT_SCD2),
                          args[15].AsBool(true),
                          args[16].AsBool(true),
                          args[17].AsInt(SystemInfo.dwNumberOfProcessors), // max threads
                          args[18].AsInt(1),  // prefetch
                          args[19].AsInt(0),  // SadMode
                          env);
}

AVSValue __cdecl Create_MVDegrainMulti(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    int plane = args[5].AsInt(4);
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

    int thSAD = args[3].AsInt(400);

	// get the system information
    SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);

    return new MVDegrainMulti(args[0].AsClip(),
                              args[1].AsClip(), // mvMulti
                              args[2].AsInt(32),  // refframes
                              thSAD, // thSAD
                              args[4].AsInt(thSAD), // thSADC
                              YUVplanes, // YUV planes
                              args[6].AsInt(255), // limit
                              (args[7].Defined() ? args[7].AsClip() : 0), // pelclip
                              args[8].AsInt(mvCore.GetEmptyIndex()),
                              args[9].AsInt(MV_DEFAULT_SCD1),
                              args[10].AsInt(MV_DEFAULT_SCD2),
                              args[11].AsBool(true),
                              args[12].AsBool(true),
                              args[13].AsInt(SystemInfo.dwNumberOfProcessors), // max threads
                              args[14].AsInt(1),  // prefetch
                              args[15].AsInt(0),  // SadMode
                              env);
}

AVSValue __cdecl Create_MVMultiExtract(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    return new MVMultiExtract(args[0].AsClip(),
                              args[1],  // array of indexes
                              env);
}

extern "C" __declspec(dllexport) const char* __stdcall
AvisynthPluginInit2(IScriptEnvironment* env)
{
    env->AddFunction("MVShow",             "c[vectors]c[scale]i[sil]i[tol]i[showsad]b[thSCD1]i[thSCD2]i[mmx]b[isse]b", Create_MVShow, 0);
    env->AddFunction("MVAnalyse",          "c[blksize]i[blksizeV]i[pel]i[level]i[search]i[searchparam]i[pelsearch]i[isb]b[lambda]i[chroma]b[delta]i[truemotion]b[lsad]i[pnew]i[plevel]i[global]b[overlap]i[overlapV]i[outfile]s[sharp]i[pelclip]c[dct]i[divide]i[idx]i[sadx264]i[mmx]b[isse]b", Create_MVAnalyse, 0);
    env->AddFunction("MVAnalyseMulti",     "c[blksize]i[blksizeV]i[pel]i[level]i[search]i[searchparam]i[pelsearch]i[refframes]i[lambda]i[chroma]b[truemotion]b[lsad]i[pnew]i[plevel]i[global]b[overlap]i[overlapV]i[outfile]s[sharp]i[pelclip]c[dct]i[divide]i[idx]i[sadx264]i[mmx]b[isse]b[deltamult]i[Threads]i[PreFetch]i", Create_MVAnalyseMulti, 0);
    env->AddFunction("MVMask",             "c[vectors]c[ml]f[gamma]f[kind]i[Ysc]i[thSCD1]i[thSCD2]i[mmx]b[isse]b", Create_MVMask, 0);
    env->AddFunction("MVCompensate",       "c[vectors]c[scbehavior]b[mode]i[thSAD]i[thSADC]i[fields]b[pelclip]c[idx]i[thSCD1]i[thSCD2]i[mmx]b[isse]b", Create_MVCompensate, 0);
    env->AddFunction("MVIncrease",         "c[vectors]c[vertical]i[horizontal]i[scbehavior]b[idx]i[thSCD1]i[thSCD2]i[mmx]b[isse]b", Create_MVIncrease, 0);
    env->AddFunction("MVChangeCompensate", "cc[isse]b", Create_MVChangeCompensate, 0);
    env->AddFunction("MVSCDetection",      "c[vectors]c[Yth]i[thSCD1]i[thSCD2]i[mmx]b[isse]b", Create_MVSCDetection, 0);
    env->AddFunction("MVDepan",             "c[vectors]c[zoom]b[rot]b[pixaspect]f[error]f[info]b[log]s[wrong]f[zerow]f[range]i[thSCD1]i[thSCD2]i[mmx]b[isse]b", Create_MVDepan, 0);
    env->AddFunction("MVFlow",              "c[vectors]c[time]f[mode]i[fields]b[pelclip]c[idx]i[thSCD1]i[thSCD2]i[mmx]b[isse]b", Create_MVFlow, 0);
    env->AddFunction("MVFlowInter",         "c[mvbw]c[mvfw]c[time]f[ml]f[thSAD]f[thSADC]f[pelclip]c[idx]i[thSCD1]i[thSCD2]i[mmx]b[isse]b", Create_MVFlowInter, 0);
    env->AddFunction("MVFlowBlur",          "c[mvbw]c[mvfw]c[blur]f[prec]i[pelclip]c[idx]i[thSCD1]i[thSCD2]i[mmx]b[isse]b", Create_MVFlowBlur, 0);
    env->AddFunction("MVFlowFps",           "c[mvbw]c[mvfw]c[num]i[den]i[mask]i[ml]f[thSAD]f[thSADC]f[pelclip]c[idx]i[thSCD1]i[thSCD2]i[mmx]b[isse]b", Create_MVFlowFps, 0);
    env->AddFunction("MVFlowFps2",          "c[mvbw]c[mvfw]c[mvbw2]c[mvfw2]c[num]i[den]i[mask]i[ml]f[thSAD]f[thSADC]f[pelclip]c[idx]i[idx2]i[thSCD1]i[thSCD2]i[mmx]b[isse]b", Create_MVFlowFps2, 0);
    env->AddFunction("MVDegrain1",          "c[mvbw]c[mvfw]c[thSAD]i[thSADC]i[plane]i[limit]i[pelclip]c[idx]i[thSCD1]i[thSCD2]i[mmx]b[isse]b[Threads]i[PreFetch]i[SadMode]i", Create_MVDegrain1, 0);
    env->AddFunction("MVDegrain2",          "c[mvbw]c[mvfw]c[mvbw2]c[mvfw2]c[thSAD]i[thSADC]i[plane]i[limit]i[pelclip]c[idx]i[thSCD1]i[thSCD2]i[mmx]b[isse]b[Threads]i[PreFetch]i[SadMode]i", Create_MVDegrain2, 0);
    env->AddFunction("MVDegrain3",          "c[mvbw]c[mvfw]c[mvbw2]c[mvfw2]c[mvbw3]c[mvfw3]c[thSAD]i[thSADC]i[plane]i[limit]i[pelclip]c[idx]i[thSCD1]i[thSCD2]i[mmx]b[isse]b[Threads]i[PreFetch]i[SadMode]i", Create_MVDegrain3, 0);
    env->AddFunction("MVDegrainMulti",      "c[mvMulti]c[refframes]i[thSAD]i[thSADC]i[plane]i[limit]i[pelclip]c[idx]i[thSCD1]i[thSCD2]i[mmx]b[isse]b[Threads]i[PreFetch]i[SadMode]i", Create_MVDegrainMulti, 0);
    env->AddFunction("MVMultiExtract",      "ci+", Create_MVMultiExtract, 0);
    return("MVTools : set of tools based on a motion estimation engine");
}

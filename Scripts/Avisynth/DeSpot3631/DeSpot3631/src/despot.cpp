/*
    DeSpot - Conditional Temporal Despotting Filter for Avisynth 2.5

	Main file

	Version 3.5.3 October 23, 2008.



Copyright (C)2003-2008 Alexander G. Balakhnin aka Fizick.
bag@hotmail.ru         http://bag.hotmail.ru
under the GNU General Public Licence version 2.

This plugin is based on  Conditional Temporal Median Filter (c-plugin) for Avisynth 2.5
Version 0.93, September 27, 2003
Copyright (C) 2003 Kevin Atkinson (kevin.med@atkinson.dhs.org) under the GNU GPL version 2.
http://kevin.atkinson.dhs.org/temporal_median/

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
*/

//  I replace all names from "border" to "segment" in v.3.0 (Fizick)

//#include <assert.h>

#include "windows.h"
#include "avs_headers\avisynth.h"

#include <malloc.h> //  3.2

#include <string.h>
#include <cmath>
//#include <stdlib.h>

#include "despot.hpp"

//#include "stdio.h" // for debug

/* disabled in v3.5.2
struct Log {
  template <typename T>
  Log & operator<<(const T &) {return *this;}
} clog;
#define endl 0
*/

struct WorkingData {
  int num;
  BYTE * noise;
  BYTE * motion;
  Segment * segments;
  void init(size_t);
  WorkingData() : num(-1), noise(0), motion(0), segments(0) {}
  ~WorkingData();
};

struct Frame {
  int num;
  WorkingData * data;
  PVideoFrame frame;
  const BYTE * y; // Y plane of frame
  int pitch; // pitch of Y plane
  BYTE * noise;
  BYTE * motion;
  Segments segments;
  Frame() : num(-1), data(0), frame(0), noise(0), motion(0) {}
  ~Frame();
};

struct Buffer
{
  PClip childclip;
  WorkingData data_cache[2];
  Frame frame_cache[4];
  BYTE * motion; // summary motion - added in v.3.0
  int num_frames;

  Frame * get_frame(int n, IScriptEnvironment* env)
    {if (n < 0 || n >= num_frames) return 0;
     Frame * f = frame_cache + n % 4;
     if (f->num != n) ready_frame(n, f, env);
     return f;}

  void alloc_noise(Frame * f)
    {if (!f->data) ready_data(f);
     f->noise = f->data->noise;}

  void alloc_motion(Frame * f)
    {if (!f->data) ready_data(f);
     f->motion = f->data->motion;}

  void alloc_segments(Frame * f)
    {if (!f->data) ready_data(f);
     f->segments.data = f->data->segments;
     f->segments.clear();}

  REGPARM void ready_frame(int n, Frame * f, IScriptEnvironment* env);  // change  f to n in v.1.1  for VC
  REGPARM void ready_data(Frame * f);
  REGPARM void free_frame(Frame * f);
  REGPARM void free_data(WorkingData *d);
};





// definition
class Filter : public GenericVideoFilter {
  // SimpleSample defines the name of your filter class.
  // This name is only used internally, and does not affect the name of your filter or similar.
  // This filter extends GenericVideoFilter, which incorporates basic functionality.
  // All functions present in the filter must also be present here.

	PClip extmask; // v3.5

	Parms Params;
	bool planarYUY2; // v3.6
	Buffer buf;
	FILE *	outfile_ptr;

//	char debugbuf[96];


//	REGPARM void find_noise(int fn, IScriptEnvironment* env);
	REGPARM void find_segments(int fn, IScriptEnvironment* env); // added in v.3.0 in place of find_noise
//	REGPARM void find_motion_simple(int fn, IScriptEnvironment* env);
	REGPARM void find_motion_prev_cur(int fn, IScriptEnvironment* env); // renamed simple in v.3.0
	REGPARM void find_motion_prev_next(int fn, IScriptEnvironment* env); // added in v.3.0

	void	print_segments (int fn, IScriptEnvironment* env);

public:
  // This defines that these functions are present in your class.
  // These functions must be that same as those actually implemented.
  // Since the functions are "public" they are accessible to other classes.
  // Otherwise they can only be called from functions within the class itself.

	Filter (PClip _child, PClip _extmask, Parms _Params, bool _planarYUY2, IScriptEnvironment* env); //v3.5
  // This is the constructor. It does not return any value, and is always used,
  //  when an instance of the class is created.
  // Since there is no code in this, this is the definition.

  ~Filter();
  // The is the destructor definition. This is called when the filter is destroyed.


	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  // This is the function that AviSynth calls to get a given frame.
  // So when this functions gets called, the filter is supposed to return frame n.
};




//Here is the acutal constructor code used
//Filter::Filter(PClip _child, PClip _WindowVideo, int _SquareSize,  IScriptEnvironment* env) :
//	GenericVideoFilter(_child), WindowVideo(_WindowVideo), SquareSize(_SquareSize){
Filter::Filter(PClip _child, PClip _extmask, Parms _Params, bool _planarYUY2, IScriptEnvironment* env) :
GenericVideoFilter(_child), extmask(_extmask), Params(_Params), outfile_ptr (0) { //v3.5

  // This is the implementation of the constructor.
  // The child clip (source clip) is inherited by the GenericVideoFilter,
  //  where the following variables gets defined:
  //   PClip child;   // Contains the source clip.
  //   VideoInfo vi;  // Contains videoinfo on the source clip.

    planarYUY2 = _planarYUY2;

	if (!vi.IsYV12() && !vi.IsYUY2())
        env->ThrowError("DeSpot: input to filter must be in YV12 or YUY2 planar!");

    if (vi.IsYUY2() && !planarYUY2)
        env->ThrowError("DeSpot: YUY2 planar is supported with option: planar=true");

	buf.childclip = child;
	buf.num_frames = vi.num_frames;
	Params.height = vi.height;
	Params.width  = vi.width;
	Params.pitch = ((vi.width+7)/8)*8; // v.3.2
//	if (Params.width % 8 != 0) env->ThrowError("DeSpot: Video Width Must be a multiple of 8"); // only for SSE -  disabled
	if (!(env->GetCPUFlags() & CPUF_INTEGER_SSE) ) env->ThrowError("DeSpot: CPU must have Integer SSE support!"); // v.3.2
	Params.size   = Params.height * Params.pitch; // v.3.2

	buf.data_cache[0].init(Params.size);
	buf.data_cache[1].init(Params.size);
//	buf.motion = new BYTE[Params.size]; // added in v.3.0
	buf.motion =  (BYTE *)malloc(Params.size); // v.3.0,  changed in v.3.2
//	_child->SetCacheHints(CACHE_RANGE,3); // added in v.3.3.2, changed from 2 to 3 in v3.5

	if (! Params.outfilename.empty ())
	{
		const double diag = sqrt (double (vi.width) * vi.width + double (vi.height) * vi.height);
		const int    thickness = int (floor (diag / 860 + 0.75));
		outfile_ptr = fopen (Params.outfilename.c_str (), "w");
		fprintf (outfile_ptr,
"[Script Info]\n"
"Title: DeSpot automatically generated file\n"
"ScriptType: v4.00+\n"
"WrapStyle: 0\n"
"PlayResX: %d\n"
"PlayResY: %d\n"
"ScaledBorderAndShadow: yes\n"
"Video Aspect Ratio: 0\n"
"Video Zoom: 8\n"
"Video Position: 0\n"
"\n"
"[V4+ Styles]\n"
"Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n"
"Style: Mask,Arial,20,&H00FFFFFF,&H000000FF,&H00000000,&H00000000,0,0,0,0,100,100,0,0,1,0,0,7,0,0,0,1\n"
"Style: Outline,Arial,20,&HFFFFFFFF,&H000000FF,&H00FF00FF,&H00000000,0,0,0,0,100,100,0,0,1,%d,0,7,0,0,0,1\n"
"\n"
"[Events]\n"
"Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n"
"Comment: 0,0:00:00.00,0:00:00.00,Mask,,0,0,0,,--- Templates for manual editing ---\n"
"Dialogue: 0,0:00:00.00,0:00:00.00,Mask,,0,0,0,,{\\pos(100,100)\\fscx100\\fscy100\\p1}m -4 -4 l 4 -4 4 4 -4 4{\\p0}\n"
"Dialogue: 0,0:00:00.00,0:00:00.00,Mask,,0,0,0,,{\\pos(100,100)\\fscx100\\fscy100\\p1}m -6 -6 l 6 -6 6 6 -6 6{\\p0}\n"
"Dialogue: 0,0:00:00.00,0:00:00.00,Mask,,0,0,0,,{\\pos(100,100)\\fscx100\\fscy100\\p1}m -8 -8 l 8 -8 8 8 -8 8{\\p0}\n"
"Comment: 0,0:00:00.00,0:00:00.00,Mask,,0,0,0,,--- Masks ---\n",
			vi.width,
			vi.height,
			thickness
		);
	}
}

// This is where any actual destructor code used goes
Filter::~Filter() {
  // This is where you can deallocate any memory you might have used.
	// Probably we must free data cache here.
//	buf.data_cache[0].~WorkingData(); // disable in v3.5.2
//	buf.data_cache[1].~WorkingData();
	free(buf.motion); // added in v.3.0, changed in v.3.2
	if (outfile_ptr != 0)
	{
		fclose (outfile_ptr);
		outfile_ptr = 0;
	}
}








/////////////////////////////////////////////////////////////////////
//
// Filter
//

/*
void Filter::find_noise(int fn, IScriptEnvironment* env )
{
  clog << "Find Noise: " << fn << endl;
  Frame * p = buf.get_frame(fn-1, env);
  Frame * c = buf.get_frame(fn, env);
  buf.alloc_noise(c);
  buf.alloc_segments(c);
  Frame * n = buf.get_frame(fn+1, env);
  if (p && n) {
    ::find_outliers(p->y, c->y, n->y, c->noise, Params);
    ::find_sizes(c->noise, c->segments, Params);
	noise_dilate(c->noise, Params); // added in v.1.3
  }
  ::mark_noise(c->segments, c->noise, Params);
}
*/
void Filter::find_segments(int fn, IScriptEnvironment* env ) // added in v.3.0
{
//  clog << "Find Segments: " << fn << endl;
  Frame * p = buf.get_frame(fn-1, env);
  Frame * c = buf.get_frame(fn, env);
  buf.alloc_noise(c);
  buf.alloc_segments(c);
  Frame * n = buf.get_frame(fn+1, env);
  if (p && n) {
    ::find_outliers(p->y, p->pitch, c->y, c->pitch, n->y, n->pitch, c->noise, Params);
    ::find_sizes(c->noise, c->segments, Params);
  }
//sprintf(debugbuf,"DeSpot: reject fn+1 %d n %d\n", fn+1, n);
//OutputDebugString(debugbuf);
}

void Filter::find_motion_prev_cur(int fn, IScriptEnvironment* env) // - changed in v.3.0 inplace of simple
{
//  clog << "Find Motion: " << fn << endl;
  Frame * p = buf.get_frame(fn-1, env);
  Frame * c = buf.get_frame(fn, env);
  buf.alloc_motion(c);
   if (p && c) {
	 ::find_motion(p->y, p->pitch, c->y, c->pitch, c->motion, Params);
   }

}
/*
void Filter::find_motion_simple(int fn, IScriptEnvironment* env)
{
  clog << "Find Motion Simple: " << fn << endl;
  Frame * p = buf.get_frame(fn-1, env);
  Frame * c = buf.get_frame(fn, env);
  buf.alloc_motion(c);
  ::find_motion(p->y, c->y, c->motion, Params);
//  ::motion_denoise(c->motion, Params);
}
*/
void Filter::find_motion_prev_next(int fn, IScriptEnvironment* env) // added in 2.1
{
//  clog << "Find Motion p-n: " << fn << endl;
  Frame * p = buf.get_frame(fn-1, env);
  Frame * c = buf.get_frame(fn, env);
  Frame * n = buf.get_frame(fn+1, env);
  buf.alloc_motion(c);
  ::find_motion(p->y, p->pitch, n->y, n->pitch, c->motion, Params);
//  ::motion_denoise(c->motion, Params);
}
void	Filter::print_segments (int fn, IScriptEnvironment* env)
{
  const int r = (Params.mc_flag) ? 3 : 1;
  const int fnd = fn / r;
  if (fn == fnd * r + ((r - 1 ) >> 1))
  {
	  Frame * c = buf.get_frame(fn, env);
	  const Segments & segs = c->segments;

	  const int w = c->frame->GetRowSize ();
	  const int h = c->frame->GetHeight ();

	  const double fps = double (vi.fps_numerator) / double (vi.fps_denominator * r);

	  ::print_segments (segs, outfile_ptr, fnd, fps, w, h, Params.spotmax1, Params.spotmax2);
  }
}



Exec * exec_median[3] = { cond_median, map_outliers, map_outliers    };
Exec * exec_pixel[3] = { remove_outliers, mark_outliers, map_outliers };

// ********************************************************************************

PVideoFrame __stdcall Filter::GetFrame(int fn, IScriptEnvironment* env )
{
//  clog << "Exec: " << fn << endl;
  Frame * p = buf.get_frame(fn-1, env);
  Frame * c = buf.get_frame(fn, env);
  Frame * n = buf.get_frame(fn+1, env);

  if (p && n)
  {
		PVideoFrame fout = env->NewVideoFrame(vi);
		//MutableRow fout_y;
		//fout_y.data = fout->GetWritePtr();
		BYTE * fout_y = fout->GetWritePtr();
		int opitch = fout->GetPitch();
//		clog << "Render " << "Frame " << ": " << fn << endl;

 		if (Params.median)
		{ // simple median mode (old)
			if (!c->motion)
			{
				find_motion_prev_cur(fn, env);
				motion_denoise(c->motion, Params); // v.3.2
			}
			if (!n->motion)
			{
				find_motion_prev_cur(fn+1, env);
				motion_denoise(n->motion, Params); // v.3.2
			}
			motion_merge(c->motion, n->motion, buf.motion, Params); // added in v.3.0
			(*exec_median[Params.show])(p->y, p->pitch, c->y, c->pitch, c->noise, buf.motion, n->y, n->pitch, fout_y, opitch, Params);
		}
		else if (Params.motpn && Params.seg != 0)
		{ // motion prev-next,  segments mode
			// new mode - find motion prev to next - added in v.3.0
			if (!c->motion)
			{
				find_motion_prev_next(fn, env);
				motion_denoise(c->motion, Params);
				motion_scene(c->motion, Params); // added in v.3.3
				if (extmask) // added in v3.5
				{
					PVideoFrame fextmask = extmask->GetFrame(fn, env);
					add_external_mask(fextmask->GetReadPtr(), fextmask->GetPitch(), c->motion, Params.pitch, Params.width, Params.height); // fixed pitch in v3.5.3
				}
			}

			if (!c->noise)
			{
				find_segments(fn, env);
				reject_on_motion(c->segments, c->motion, Params);
			}

			if (Params.show == 0)
			{
				env->BitBlt(fout_y, opitch, c->y, c->pitch, Params.width, Params.height);
				remove_segments(c->segments, p->y, p->pitch, c->y, c->pitch, n->y, n->pitch, fout_y, opitch, c->noise, Params);// added in v.3.0
				if (Params.tsmooth>0)temporal_smooth(c->segments, p->y, p->pitch, n->y, n->pitch, fout_y, opitch, c->noise, c->motion, Params);// add condition in v.3.2
			}
			else if (Params.show == 1)
			{
//				buf.motion = c->motion; // was memory leak! - disabled in v3.5.2 - 3.6.0
				remove_segments(c->segments, p->y, p->pitch, c->y, c->pitch, n->y, n->pitch, fout_y, opitch, c->noise, Params);// added in v.3.0
//				mark_outliers(p->y, p->pitch, c->y,c->pitch,  c->noise, buf.motion, n->y, n->pitch, fout_y, opitch, Params);
				mark_outliers(p->y, p->pitch, c->y,c->pitch,  c->noise, c->motion, n->y, n->pitch, fout_y, opitch, Params);
			}
			else
			{ // =2
//				buf.motion = c->motion;
				remove_segments(c->segments, p->y, p->pitch, c->y, c->pitch, n->y, n->pitch, fout_y, opitch, c->noise, Params);// added in v.3.0
//				map_outliers(p->y, p->pitch,c->y, c->pitch,c->noise, buf.motion, n->y, n->pitch,fout_y, opitch,Params);
				map_outliers(p->y, p->pitch,c->y, c->pitch,c->noise, c->motion, n->y, n->pitch,fout_y, opitch,Params);
			}

			if (outfile_ptr != 0)
			{
				print_segments (fn, env);
			}
		}
		else if (Params.motpn && Params.seg == 0)
		{ // motion prev-next, no segments mode
			// new mode - find motion prev to next - added in v.3.0
			if (!c->motion)
			{
				find_motion_prev_next(fn, env);
				motion_denoise(c->motion, Params);
				motion_scene(c->motion, Params); // added in v.3.3
				if (extmask) // added in v3.5
				{
					PVideoFrame fextmask = extmask->GetFrame(fn, env);
					add_external_mask(fextmask->GetReadPtr(), fextmask->GetPitch(), c->motion, Params.pitch, Params.width, Params.height);
				}
			}

			if (!c->noise)
			{
				find_segments(fn, env);
				mark_noise(c->segments, c->noise, Params);
				noise_dilate(c->noise, Params); // added in v.1.3
			}
//			buf.motion = c->motion;
//			(*exec_pixel[Params.show])(p->y, p->pitch, c->y, c->pitch, c->noise, buf.motion, n->y, n->pitch, fout_y, opitch, Params);
			(*exec_pixel[Params.show])(p->y, p->pitch, c->y, c->pitch, c->noise, c->motion, n->y, n->pitch, fout_y, opitch, Params);
		}
		else if (Params.seg != 0)
		{ // motion prev-cur and cur-next (old mode), but segments mode
			if (!c->motion)
			{
				find_motion_prev_cur(fn, env);
				find_motion_prev_cur(fn-1, env);
				if (!p->noise)
				{
					find_segments(fn-1, env);
					mark_noise(p->segments, p->noise, Params);
				}
				if (!c->noise)
				{
					find_segments(fn, env);
					mark_noise(c->segments, c->noise, Params);
				}
				noise_to_one(p->noise, c->noise, c->motion, Params);
				motion_denoise(c->motion, Params);
				motion_scene(c->motion, Params); // added in v.3.3
			}

			if (!n->motion)
			{
				find_motion_prev_cur(fn+1, env);
				if (!c->noise)
				{
					find_segments(fn, env);
					mark_noise(c->segments, c->noise, Params);
				}
				if (!n->noise)
				{
					find_segments(fn+1, env);
					mark_noise(n->segments, n->noise, Params);
				}
				noise_to_one(c->noise, n->noise, n->motion, Params);
				motion_denoise(n->motion, Params);
				motion_scene(n->motion, Params); // added in v.3.3
			}
			motion_merge(c->motion, n->motion, buf.motion, Params); // added in v.3.0
				if (extmask) // added in v3.5
				{
					PVideoFrame fextmask = extmask->GetFrame(fn, env);
					add_external_mask(fextmask->GetReadPtr(), fextmask->GetPitch(), buf.motion, Params.pitch, Params.width, Params.height); // v3.6
				}
			reject_on_motion(c->segments, buf.motion, Params);
			if (Params.show == 0)
			{
				env->BitBlt(fout_y, opitch, c->y, c->pitch, Params.width, Params.height); // copy as base
				remove_segments(c->segments, p->y, p->pitch, c->y, c->pitch, n->y, n->pitch, fout_y, opitch, c->noise, Params);// added in v.3.0
				if (Params.tsmooth>0) temporal_smooth(c->segments, p->y, p->pitch, n->y, n->pitch, fout_y, opitch, c->noise, c->motion, Params);// must be added but was commented (bug)  in v.3.0, enabled in 3.2
			}
			else if (Params.show == 1)
			{
				remove_segments(c->segments, p->y, p->pitch, c->y, c->pitch, n->y, n->pitch, fout_y, opitch, c->noise, Params);// added in v.3.0
				mark_outliers(p->y, p->pitch, c->y, c->pitch, c->noise, buf.motion, n->y, n->pitch, fout_y, opitch, Params);
			}
			else
			{ // =2
				remove_segments(c->segments, p->y, p->pitch, c->y, c->pitch, n->y, n->pitch, fout_y, opitch, c->noise, Params);// added in v.3.0
				map_outliers(p->y, p->pitch, c->y, c->pitch, c->noise, buf.motion, n->y, n->pitch, fout_y, opitch, Params);
			}
		}
		else if (Params.seg == 0)
		{ // motion prev-cur and cur-next, no segments - old mode before v.3.0
			if (!c->motion)
			{
				find_motion_prev_cur(fn, env);
				find_motion_prev_cur(fn-1, env);
				if (!p->noise)
				{
					find_segments(fn-1, env);
					mark_noise(p->segments, p->noise, Params);
					noise_dilate(p->noise, Params); // added in v.1.3
				}
				if (!c->noise)
				{
					find_segments(fn, env);
					mark_noise(c->segments, c->noise, Params);
					noise_dilate(c->noise, Params); // added in v.1.3
				}
				noise_to_one(p->noise, c->noise, c->motion, Params);
				motion_denoise(c->motion, Params);
				motion_scene(c->motion, Params); // added in v.3.3
			}

			if (!n->motion)
			{
				find_motion_prev_cur(fn+1, env);
				if (!c->noise)
				{
					find_segments(fn, env);
					mark_noise(c->segments, c->noise, Params);
					noise_dilate(c->noise, Params); // added in v.1.3
				}
				if (!n->noise)
				{
					find_segments(fn+1, env);
					mark_noise(n->segments, n->noise, Params);
					noise_dilate(n->noise, Params); // added in v.1.3
				}
				noise_to_one(c->noise, n->noise, n->motion, Params);
				motion_denoise(n->motion, Params);
				motion_scene(n->motion, Params); // added in v.3.3
			}
			motion_merge(c->motion, n->motion, buf.motion, Params); // added in v.3.0
				if (extmask) // added in v3.5
				{
					PVideoFrame fextmask = extmask->GetFrame(fn, env);
					add_external_mask(fextmask->GetReadPtr(), fextmask->GetPitch(), buf.motion, Params.pitch, Params.width, Params.height);
				}
			(*exec_pixel[Params.show])(p->y, p->pitch, c->y, c->pitch, c->noise, buf.motion, n->y, n->pitch, fout_y, opitch, Params);
		}

		// now process color planes
		if (Params.show == S_MAP && !Params.show_chroma)
		{ //  grey map
		  if (vi.IsPlanar())
		  {
		  memset( fout->GetWritePtr(PLANAR_U), 0x80,
				 fout->GetPitch(PLANAR_U)*fout->GetHeight(PLANAR_U) );
		  memset(fout->GetWritePtr(PLANAR_V), 0x80,
				 fout->GetPitch(PLANAR_V)*fout->GetHeight(PLANAR_V));
		  }
		  else // pseudo-planar YUY2
		  {
		      BYTE *pU = fout->GetWritePtr() + Params.width;
		      for (int h=0; h<Params.height; h++)
		      {
                memset( pU, 0x80, fout->GetRowSize()-Params.width ); // U,V
                pU += fout->GetPitch();
		      }
		  }
		}
		else if (Params.show == S_MAP && Params.show_chroma)
		{// copy color planes
		  if (vi.IsPlanar())
		  {
			env->BitBlt(fout->GetWritePtr(PLANAR_U), fout->GetPitch(PLANAR_U),
						c->frame->GetReadPtr(PLANAR_U),	c->frame->GetPitch(PLANAR_U),
						c->frame->GetRowSize(PLANAR_U),	c->frame->GetHeight(PLANAR_U));
			env->BitBlt(fout->GetWritePtr(PLANAR_V), fout->GetPitch(PLANAR_V),
						c->frame->GetReadPtr(PLANAR_V),	c->frame->GetPitch(PLANAR_V),
						c->frame->GetRowSize(PLANAR_V),	c->frame->GetHeight(PLANAR_V));
		  }
		  else // pseudo-planar YUY2
		  {
			env->BitBlt(fout->GetWritePtr()+Params.width, fout->GetPitch(),
						c->frame->GetReadPtr()+Params.width,	c->frame->GetPitch(),
						c->frame->GetRowSize()-Params.width,	c->frame->GetHeight());
		  }
		}
		else if ((Params.show && S_MARK) && !Params.median) // median bug fixed in v.3.2
		{// copy color planes
		  if (vi.IsPlanar())
		  {
			env->BitBlt(fout->GetWritePtr(PLANAR_U), fout->GetPitch(PLANAR_U),
						c->frame->GetReadPtr(PLANAR_U),	c->frame->GetPitch(PLANAR_U),
						c->frame->GetRowSize(PLANAR_U),	c->frame->GetHeight(PLANAR_U));
			env->BitBlt(fout->GetWritePtr(PLANAR_V), fout->GetPitch(PLANAR_V),
						c->frame->GetReadPtr(PLANAR_V),	c->frame->GetPitch(PLANAR_V),
						c->frame->GetRowSize(PLANAR_V),	c->frame->GetHeight(PLANAR_V));
		  // change color of marked noise to pink - added in v.1.2
			mark_color_plane(c->noise,	fout->GetWritePtr(PLANAR_V), fout->GetPitch(PLANAR_V),
				fout->GetRowSize(PLANAR_V), fout->GetHeight(PLANAR_V), Params);// added in v.3.1
		  }
		  else // pseudo-planar YUY2
		  {
			env->BitBlt(fout->GetWritePtr()+Params.width, fout->GetPitch(),
						c->frame->GetReadPtr()+Params.width,	c->frame->GetPitch(),
						c->frame->GetRowSize()-Params.width,	c->frame->GetHeight());
		  // change color of marked noise to pink
			mark_color_plane(c->noise,	fout->GetWritePtr()+Params.width*3/2, fout->GetPitch(),
				(fout->GetRowSize()-Params.width)/2, fout->GetHeight(), Params);
		  }
		}
		else if (Params.color && !Params.median)// median bug fixed in v.3.2
		{	// normal mode with color
			// clean color YUV12 planes at places of luma spots (noise) - added in v.3.1
		  if (vi.IsPlanar())
		  {
			clean_color_plane(p->frame->GetReadPtr(PLANAR_U), p->frame->GetPitch(PLANAR_U),
				c->frame->GetReadPtr(PLANAR_U), c->frame->GetPitch(PLANAR_U),
				n->frame->GetReadPtr(PLANAR_U), n->frame->GetPitch(PLANAR_U),
				c->frame->GetRowSize(PLANAR_U), c->frame->GetHeight(PLANAR_U),
				c->noise, fout->GetWritePtr(PLANAR_U),fout->GetPitch(PLANAR_U), Params);
			clean_color_plane(p->frame->GetReadPtr(PLANAR_V),p->frame->GetPitch(PLANAR_V),
				c->frame->GetReadPtr(PLANAR_V), c->frame->GetPitch(PLANAR_V),
				n->frame->GetReadPtr(PLANAR_V), n->frame->GetPitch(PLANAR_V),
				c->frame->GetRowSize(PLANAR_V), c->frame->GetHeight(PLANAR_V),
				c->noise, fout->GetWritePtr(PLANAR_V), fout->GetPitch(PLANAR_V), Params);
		  }
		  else // pseudo-planar YUY2
		  {
			clean_color_plane(p->frame->GetReadPtr()+Params.width, p->frame->GetPitch(),
				c->frame->GetReadPtr()+Params.width, c->frame->GetPitch(),
				n->frame->GetReadPtr()+Params.width, n->frame->GetPitch(),
				c->frame->GetRowSize()/4, c->frame->GetHeight(),
				c->noise, fout->GetWritePtr()+Params.width,fout->GetPitch(), Params);
			clean_color_plane(p->frame->GetReadPtr()+Params.width*3/2, p->frame->GetPitch(),
				c->frame->GetReadPtr()+Params.width*3/2, c->frame->GetPitch(),
				n->frame->GetReadPtr()+Params.width*3/2, n->frame->GetPitch(),
				c->frame->GetRowSize()/4, c->frame->GetHeight(),
				c->noise, fout->GetWritePtr()+Params.width*3/2,fout->GetPitch(), Params);
		  }
		}
		else
		{ // mormal mode, copy color planes
		  if (vi.IsPlanar())
		  {
			env->BitBlt(fout->GetWritePtr(PLANAR_U), fout->GetPitch(PLANAR_U),
						c->frame->GetReadPtr(PLANAR_U),	c->frame->GetPitch(PLANAR_U),
						c->frame->GetRowSize(PLANAR_U),	c->frame->GetHeight(PLANAR_U));
			env->BitBlt(fout->GetWritePtr(PLANAR_V), fout->GetPitch(PLANAR_V),
						c->frame->GetReadPtr(PLANAR_V),	c->frame->GetPitch(PLANAR_V),
						c->frame->GetRowSize(PLANAR_V),	c->frame->GetHeight(PLANAR_V));
		  }
		  else // pseudo-planar YUY2
		  {
			env->BitBlt(fout->GetWritePtr()+Params.width, fout->GetPitch(),
						c->frame->GetReadPtr()+Params.width,	c->frame->GetPitch(),
						c->frame->GetRowSize()-Params.width,	c->frame->GetHeight());
		  }
		}

		return fout; // result
  }
  else
  { // first frame
		return c->frame; // nothing to do - no temporal info
  }
}


// macro for parameters defining
#define SET_INT(what, Min, Max) do {\
    i++;\
	tmp = args[i];\
    if (tmp.Defined()) {\
      int v = tmp.AsInt();\
      if (v < Min || v > Max)\
        env->ThrowError("DeSpot: "#what " must be from " #Min " to " #Max );\
      p.what = v;}} while (0)


// This is the function that created the filter, when the filter has been called.
// This can be used for simple parameter checking, so it is possible to create different filters,
// based on the arguments recieved.

AVSValue __cdecl Create_Despot(AVSValue args, void* user_data, IScriptEnvironment* env) {
	Parms p;
	AVSValue tmp;
	int i=0;
	// default parameters values are set in despot.hpp
	SET_INT(mthres,0,255);
	SET_INT(mwidth,0,32000);
	SET_INT(mheight,0,32000);
//	SET_INT(mratio,1,10); // added in v.2.1 and removed in 3.0
//	p.merode = 100/p.mratio; // default, added in v.2.1
	SET_INT(merode,0,100); // mp is changed to merode - relative percent of (MWidth+1)*(MHeight+1) in v.2.0
	i++;
	if (args[i].Defined() ) p.y_next =	 args[i].AsBool(false) ? 2 : 1;  //interlaced
	i++;
	p.median = args[i].AsBool(false); //median
	SET_INT(p1,0,255);
	SET_INT(p2,0,255);
	SET_INT(pwidth,0,32000);
	SET_INT(pheight,0,32000);
	i++;
	p.ranked=	 args[i].AsBool(true); //ranked
	SET_INT(sign,-2,2);
	SET_INT(maxpts,0,10000000);
	SET_INT(p1percent,0,100);
	SET_INT(dilate,0,1000);
	i++;
	p.fitluma=	 args[i].AsBool(false); //fitluma
	SET_INT(blur,0,4);
	SET_INT(tsmooth,0,255);
	SET_INT(show,0,2);
	SET_INT(mark_v,0,255);
	i++;
	p.show_chroma=	 args[i].AsBool(false); //show_chroma
	i++;
	p.motpn = args[i].AsBool(true); //motpn - changed to true in v3.5
	SET_INT(seg,0,2);
	i++;
	p.color = args[i].AsBool(false); //clean color - added in v.3.1
	SET_INT(mscene,0,100);
	SET_INT(minpts,0,10000000); // v.3.4
	i++;
	int iextmask=i;
	i++; // planar
	int planar=i;
	++ i;
	p.outfilename = args [i].Defined () ? args [i].AsString () : "";
	++ i;
	p.mc_flag = args [i].AsBool (false);
	p.spotmax1 = 12;
	SET_INT (spotmax1,1,10000000);
	p.spotmax2 = 20;
	SET_INT (spotmax2,1,10000000);

	return new Filter(args[0].AsClip(), args[iextmask].Defined () ? args[iextmask].AsClip() : 0, p,  args[planar].AsBool(false), env);
    // Calls the constructor with the arguments provied.
}


// change mp (absolute) to merode (relative percent) in v.2.0
#define common_parms  "c[mthres]i[mwidth]i[mheight]i[merode]i[interlaced]b[median]b"
#define denoise_parms "[p1]i[p2]i[pwidth]i[pheight]i[ranked]b[sign]i[maxpts]i[p1percent]i[dilate]i[fitluma]b[blur]i[tsmooth]i[show]i[mark_v]i[show_chroma]b[motpn]b[seg]i[color]b[mscene]i[minpts]i[extmask]c[planar]b[outfile]s[mc]b[spotmax1]i[spotmax2]i"

// The following function is the function that actually registers the filter in AviSynth
// It is called automatically, when the plugin is loaded to see which functions this filter contains.

const AVS_Linkage *AVS_linkage = 0;
extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, const AVS_Linkage* const vectors)
{
  AVS_linkage = vectors;
  env->AddFunction("DeSpot", common_parms denoise_parms, Create_Despot,0);
    // The AddFunction has the following paramters:
    // AddFunction(Filtername , Arguments, Function to call,0);

    // Arguments is a string that defines the types and optional names of the arguments for you filter.
    // c - Video Clip
    // i - Integer number
    // f - Float number
    // s - String
    // b - boolean


    return "`DeSpot' DeSpot plugin";
    // A freeform name of the plugin.
}



/////////////////////////////////////////////////////////////////////
//
// Frame
//

Frame::~Frame()
{
//  if (frame) avs_release_video_frame(frame);
}

//
// WorkingData
//

void WorkingData::init(size_t s)
{
  noise   = (BYTE *)malloc(s);
  motion  = (BYTE *)malloc(s);
  segments = (Segment *)malloc(segment_size * (s/2+1));
}

WorkingData::~WorkingData()
{
  if (noise)   free(noise);
  if (motion)  free(motion);
  if (segments) free(segments);
}

//
// Buffer
//

//void Buffer::ready_frame(int n, Frame * f)
void Buffer::ready_frame(int n, Frame * f, IScriptEnvironment* env)
{
  free_frame(f);
  f->num = n;
//  clog << "Getting Frame: " << n << "\n";
  f->frame = childclip->GetFrame(n, env);//avs_get_frame(child, n);
  f->y  = f->frame->GetReadPtr();//avs_get_read_ptr(f->frame);
  f->pitch = f->frame->GetPitch();//avs_get_pitch(f->frame);
}

void Buffer::ready_data(Frame * f)
{
  WorkingData * d = data_cache + f->num % 2;
  free_data(d);
  d->num = f->num;
  f->data = d;
}

void Buffer::free_frame(Frame * f)
{
  if (f->data) free_data(f->data);
//  clog << "Freeing Frame: " << f->num << "\n";
//  if (f->frame) avs_release_video_frame(f->frame);
  f->num = -1;
  f->frame = 0;
  f->y = 0;
}

void Buffer::free_data(WorkingData * d)
{
  Frame * f = frame_cache + d->num % 4;
  if (f->num == d->num) {
    f->data = 0;
    f->noise = 0;
    f->motion = 0;
    f->segments.data = 0;
    f->segments.clear();
  }
  d->num = -1;
}


/*
    DeSpot - Conditional Temporal Despotting Filter for Avisynth 2.5

    Include file

    Version 3.5  November 17, 2005.

Copyright (C)2003-2005 Alexander G. Balakhnin aka Fizick.
bag@hotmail.ru         http://bag.hotmail.ru
under the GNU General Public Licence version 2.

This plugin is based on Conditional Temporal Median Filter (c-plugin) for Avisynth 2.5
Version 0.93, September 27, 2003
Copyright (C) 2003 Kevin Atkinson (kevin.med@atkinson.dhs.org) under the GNU GPL version 2.
http://kevin.atkinson.dhs.org/temporal_median/

*/
#include	<string>
//#include "windows.h"
typedef unsigned char BYTE; // change all byte to BYTE  in v.1.2

// added in v.3.2 for profiling
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

// redefine to enable SSE min max
//#undef min
//#undef max

//static inline int min(int x, int y) {return x < y ? x : y;}
//static inline int max(int x, int y) {return x > y ? x : y;}
//static inline BYTE min(BYTE x, BYTE y) {return x < y ? x : y;}
//static inline BYTE max(BYTE x, BYTE y) {return x > y ? x : y;}


//#define REGPARM __attribute__ (( regparm(3) ))  // made fictive in v.1.1  for compatibility with VC:
#define REGPARM

//#include <stddef.h>

/*  removed in 3.2
struct MutableRow {
  BYTE * data;
  int pitch;
  MutableRow() : data(0) {}
  MutableRow(BYTE * d, int p) : data(d), pitch(p) {}
};

struct ConstRow {
  const BYTE * data;
  int pitch;
  ConstRow() : data(0) {}
  ConstRow(MutableRow o) : data(o.data), pitch(o.pitch) {}
  ConstRow(BYTE * d, int p) : data(d), pitch(p) {}
};
*/

#define M_SEARCH 0
#define M_MEDIAN 4

#define S_DENOISE 0
#define S_MARK    1
#define S_MAP     2

struct Parms {
  size_t size;
  int width, height; // changed to from size_t to int in v3.5
  int pitch; // v.3.2
  BYTE p1;
  BYTE p2;
  short pwidth;
  short pheight;
  int y_next; // =1 if not interlaced, =2 if interlaced
  BYTE mthres;
  int mwidth, mheight;
  int merode;// change mp from BYTE to int in v.1.3 ;
			//change mp to merode in v.2.0
  bool median; // changed from mode in v.3.0
  int show;
  BYTE mark_v;
  bool show_chroma;
  bool ranked; //Added in v. 1.2
  int sign;         //Added in v. 0.93a   -    may be negative
  int maxpts; // Added in v. 1.2
  int p1percent; // Added in v. 1.2, changed to int in v.2.0
  int dilate; // Added in v. 1.3
  bool fitluma;     //Added in v. 0.934
  BYTE blur;       //Added in v. 0.934
  int tsmooth;        //Added  in v. 1.0
//  BYTE mratio; //Added  in v. 2.1 and removed in 3.0
  bool motpn;	//Added in v.3.0
  int seg;	// Added in v.3.0
  bool color; // Added in v.3.1
  int mscene; // Added in v.3.3
  int minpts; // added in v.3.4

  std::string	outfilename;
  bool mc_flag;
  int spotmax1;
  int spotmax2;

  Parms()
    : p1(24), p2(12), pwidth(16), pheight(5), y_next(1), //changed y_next to 1 (progressive) in v.1.3
      mthres(16), mwidth(7), mheight(5),
//		mratio(3), // added in v.2.1 and removed in 3.0
	  merode(33), // change mp to merode in v.2.0
	  median(false), // v.3.0
	  show(0), // v.1.4
      mark_v(255), show_chroma(false), // {}
      ranked(true), sign(0),  maxpts(0), p1percent(10), dilate(1), fitluma(true), //Added by Fizick
	  blur(1), tsmooth(0), motpn(true), // changed motpn to true in v3.5
	  seg(2), color(false), mscene(40), minpts(0) {}   //Added by Fizick
};

struct Segment;
extern size_t segment_size;

struct Segments {
  Segment * data;
  Segment * end;
  Segments() : data(0), end(0) {}
  REGPARM void setup(size_t);
  void clear() {end = data;}
};

typedef void Exec(const BYTE * p, int Ppitch,
                  const BYTE * c, int Cpitch,
				  BYTE * c_noise, BYTE * c_motion,
                  const BYTE * n, int Npitch,
                  BYTE * o, int Opitch,
				  const Parms &);// BYTE * n_motion, - remove parameter in v.3.0


extern "C" {
  REGPARM void find_sizes(BYTE * noise, Segments & segs, const Parms &); // change parameters order in v.3.0

  REGPARM void mark_noise(Segments & segs, BYTE * noise, const Parms &);

  REGPARM void memzero(void *, size_t s);

  void find_motion(const BYTE * p,  int Ppitch, const BYTE * c, int Cpitch,
                   BYTE * motion, const Parms &);
  Exec cond_median;

  void motion_denoise(BYTE * moving, const Parms &); // 3.22

  void find_outliers(const BYTE * p,  int Ppitch, const BYTE * c, int Cpitch,
	  const BYTE * n, int Npitch,  BYTE * c_noise, const Parms &);

  void mark_motion(const BYTE * p, int Ppitch, BYTE * p_noise,
                   const BYTE * c, int Cpitch, BYTE * c_noise, BYTE * c_motion,
                   const Parms &);

  Exec remove_outliers;
  Exec mark_outliers;
  Exec map_outliers;

void noise_dilate(BYTE * noise, const Parms &); // added in v.1.3
void reject_on_motion(Segments & segs, BYTE * motion, const Parms & p); // added in v.3.0
void motion_merge(BYTE * c_motion, BYTE * n_motion, BYTE * m_motion,  const Parms & parms); // added in v.3.0
void noise_to_one(BYTE * p_noise,  BYTE * c_noise,  BYTE * c_motion, const Parms & parms);// added in v.3.0
void remove_segments(Segments & segs, const BYTE * P,  int Ppitch, const BYTE * C, int Cpitch, const BYTE * N,  int Npitch, BYTE * o,  int Opitch, BYTE *c_noise, const Parms & parms);// added in v.3.0
void temporal_smooth(Segments & segs, const BYTE * P, int Ppitch, const BYTE * N, int Npitch,  BYTE * o,  int Opitch, BYTE * c_noise, BYTE * c_motion, const Parms & parms);// added in v.3.0 , changed in 3.2
void clean_color_plane(const BYTE *p, int ppitch, const BYTE * c, int cpitch, const BYTE * n, int npitch,	int width, int height, BYTE * c_noise,	BYTE * o, int opitch, Parms & Params);// added in v.3.1
void mark_color_plane(BYTE * c_noise,	BYTE * oV, int opitchV, int widthUV, int heightUV, Parms & Params);// added in v.3.1
void motion_scene(BYTE * motion, const Parms & parms); // added in v.3.3
void add_external_mask(const BYTE * ext, int ext_pitch, BYTE * motion, int pitch, int width, int height); // added in v.3.5
void print_segments (const Segments & segs, FILE *f_ptr, int fn, double frate, int w, int h, int spotmax1, int spotmax2);

}




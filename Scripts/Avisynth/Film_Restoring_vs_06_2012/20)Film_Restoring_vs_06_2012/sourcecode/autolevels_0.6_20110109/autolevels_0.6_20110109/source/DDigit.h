/*
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

#ifndef __DDIGIT_H__
#define __DDIGIT_H__
// ---------------------------------------------------------------------------------

//  !!! Below lines configure DDIGIT, Edit as required !!!

#define DDIGIT_ENABLE_SUPPORT_PLANAR		// Enable PLANAR eg YV12 Support
#define DDIGIT_ENABLE_SUPPORT_YUY2			// Enable YUY2 Support
#define DDIGIT_ENABLE_SUPPORT_RGB			// Enable RGB24 & RGB32 Support

#define DDIGIT_INCLUDE_DRAWSTRING_FUNCTIONS	// Print Strings @ Pixel Coords
#define DDIGIT_INCLUDE_DRAWSTRING_VERTICAL	// Print Vertical Strings @ Pixel Coords
#define DDIGIT_INCLUDE_DRAWSTR_FUNCTIONS	// Print Strings @ Character Coords
#define DDIGIT_INCLUDE_DRAWSTR_VERTICAL		// Print Vertical Strings @ Character Coords

// ---------------------------------------------------------------------------------


#include <Windows.h>
#include "Avisynth.h"


// Use these in external string printing fn's just in case font changes.
#define DDIGIT_CHAR_WIDTH	10	// Font Character pixel width
#define DDIGIT_CHAR_HEIGHT	20	// Font Character pixel height
#define DDIGIT_CHARACTERS	224 // Characters in Font

extern const unsigned short DDigitFont[DDIGIT_CHARACTERS][DDIGIT_CHAR_HEIGHT];


#define	DDIGIT_DEFAULT		-1
#define	DDIGIT_HILITE		-2

// Number of CMAP colors and names fixed PERMANENT.
#define	DDIGIT_DARKGRAY			0
#define	DDIGIT_DODGERBLUE		1
#define	DDIGIT_ORANGERED		2
#define	DDIGIT_ORCHID			3
#define	DDIGIT_LIME				4
#define	DDIGIT_AQUAMARINE		5
#define	DDIGIT_YELLOW			6
#define	DDIGIT_WHITE			7
#define	DDIGIT_SILVER			8
#define	DDIGIT_CORNFLOWERBLUE	9
#define	DDIGIT_ORANGE			10
#define	DDIGIT_PLUM				11
#define	DDIGIT_CHARTREUSE		12
#define	DDIGIT_POWDERBLUE		13
#define	DDIGIT_GOLD				14
#define	DDIGIT_GAINSBORO		15


// Color index's used for defaults
#define DDIGIT_INDEX_TO_USE_AS_DEFAULT	DDIGIT_WHITE
#define DDIGIT_INDEX_TO_USE_AS_HILITE	DDIGIT_ORANGE


#define DDIGIT_CMAP_NELS	16	// FIXED 4E4

extern const unsigned long DDigit_Cmap_RGB[DDIGIT_CMAP_NELS];
extern const unsigned long DDigit_Cmap_YUV[DDIGIT_CMAP_NELS];


// Draw EXT ASCII string at [x,y] PIXEL or CHARACTER coords with color[16] selection
// of forground. pix==true = pixel coords: vert==true = Vertical (top down printing).

extern void __stdcall DDigitS(const VideoInfo &vi,PVideoFrame &dst,int x,int y,int color, \
	const bool pix,const bool vert,const char *s);


#ifdef DDIGIT_INCLUDE_DRAWSTRING_FUNCTIONS	
	// Draw Strings at pixel Coords
	extern void __stdcall DrawString(const VideoInfo &vi,PVideoFrame &dst,int x,int y,int color,const char *s);
	extern void __stdcall DrawString(const VideoInfo &vi,PVideoFrame &dst,int x,int y,const char *s);
	extern void __stdcall DrawString(const VideoInfo &vi,PVideoFrame &dst,int x,int y,bool hilite,const char *s);
#endif // DDIGIT_INCLUDE_DRAWSTRING_FUNCTIONS	

#ifdef DDIGIT_INCLUDE_DRAWSTRING_VERTICAL
	// Draw Vertical Strings at pixel Coords
	extern void __stdcall DrawStringV(const VideoInfo &vi,PVideoFrame &dst,int x,int y,int color,const char *s);
	extern void __stdcall DrawStringV(const VideoInfo &vi,PVideoFrame &dst,int x,int y,const char *s);
	extern void __stdcall DrawStringV(const VideoInfo &vi,PVideoFrame &dst,int x,int y,bool hilite,const char *s);
#endif // DDIGIT_INCLUDE_DRAWSTRING_VERTICAL

#ifdef DDIGIT_INCLUDE_DRAWSTR_FUNCTIONS
	// Draw Strings at Character Coords
	extern void __stdcall DrawStr(const VideoInfo &vi,PVideoFrame &dst, int x, int y, int color, const char *s);
	extern void __stdcall DrawStr(const VideoInfo &vi,PVideoFrame &dst, int x, int y, const char *s);
	extern void __stdcall DrawStr(const VideoInfo &vi,PVideoFrame &dst, int x, int y, bool hilite, const char *s);
#endif // DDIGIT_INCLUDE_DRAWSTR_FUNCTIONS

#ifdef DDIGIT_INCLUDE_DRAWSTR_VERTICAL
	// Draw Vertical Strings at Character Coords
	extern void __stdcall DrawStrV(const VideoInfo &vi,PVideoFrame &dst, int x, int y, int color, const char *s);
	extern void __stdcall DrawStrV(const VideoInfo &vi,PVideoFrame &dst, int x, int y, const char *s);
	extern void __stdcall DrawStrV(const VideoInfo &vi,PVideoFrame &dst, int x, int y, bool hilite, const char *s);			
#endif // DDIGIT_INCLUDE_DRAWSTR_VERTICAL


#endif //__DDIGIT_H__

#define LOGO		"RemoveGrain 1.0\n"
// An Avisynth plugin for removing grain from progressive video
//
// By Rainer Wittmann <gorw@gmx.de>
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
// To get a copy of the GNU General Public License write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .

//#define	MODIFYPLUGIN	1// creat Repair plugin instead of RemoveGrain, 0 = compatible with RemoveGrain
//#define	SHARPEN	1
//#define	BLUR 1
//#define	SSE2_TEST		// ISSE2 version that can be used side by side with the SSE version
//#define	DEBUG_NAME		// for debugging
//#define	ISSE	2		// P4, Athlon 64, Sempron 3100
//#define	ISSE	3		// Prescott P4	
//#define	CVERSION		// for debugging only
#define	ALIGNPITCH
#define	SMOOTH2

#define	DEFAULT_MODE	2
#define	DEFAULT_RGLIMIT	0

#define VC_EXTRALEAN 
#include <Windows.h>
#include <stdio.h>
#include <stdarg.h>

#include "avisynth.h"
#include "planar.h"

static	IScriptEnvironment	*AVSenvironment;

#ifdef	SSE2_TEST
#ifndef	ISSE
#define	ISSE	2
#endif
#ifndef	DEBUG_NAME
#define	DEBUG_NAME
#endif
#endif

#ifndef	ISSE
#define	ISSE	1
#endif

#if		ISSE > 1			
#define	CPUFLAGS		CPUF_SSE2
#else
#define	CPUFLAGS		CPUF_INTEGER_SSE
#endif

#ifdef	MODIFYPLUGIN
#define	MAXMODE			18
#elif defined(SHARPEN)
#define	MAXMODE			22
#define	MAXSTRENGTH		2
#define	DEFAULT_STRENGTH	1
#else
#define	MAXMODE			28
#endif

#if	defined(SHARPEN) && defined(MODIFYPLUGIN)
#error "SHARPEN cannot be combined with MODIFYPLUGIN"
#endif

#if	defined(BLUR) && defined(MODIFYPLUGIN)
#error "SHARPEN cannot be combined with MODIFYPLUGIN"
#endif

#if	1
void	debug_printf(const char *format, ...)
{
	char	buffer[200];
	va_list	args;
	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);
	OutputDebugString(buffer);	
}
#endif

#define	COMPARE_MASK	(~24)

static	void CompareVideoInfo(VideoInfo &vi1, const VideoInfo &vi2, const char *progname)
{	
	if( (vi1.width != vi2.width) || (vi1.height != vi2.height) || ( (vi1.pixel_type & COMPARE_MASK) != (vi2.pixel_type & COMPARE_MASK) ))
	{
#if	1
		debug_printf("widths = %u, %u, heights = %u, %u, color spaces = %X, %X\n"
						, vi1.width, vi2.width, vi1.height, vi2.height, vi1.pixel_type, vi2.pixel_type);
#endif
		AVSenvironment->ThrowError("%s: clips must be of equal type", progname);
	}
	if(vi1.num_frames > vi2.num_frames) vi1.num_frames = vi2.num_frames;
}

#ifdef	TESTCOMPARE
unsigned	testcompare(const BYTE *dp, int dpitch, const BYTE *pp, int ppitch, int width, int height)
{
	int i = height;
	--dp; --pp;
	unsigned	diffsum = 0;
	do
	{
		int j = width;
		do
		{
			int	diff = dp[j] - pp[j];
			if( diff < 0 ) diff = -diff;
			diffsum += diff;
		} while( --j );
		dp += dpitch;
		pp += ppitch;
	} while( --i );
	return	diffsum;
}

#define	xpitch	1
void	RemoveGrain(BYTE *dp, int dpitch, const BYTE *sp, int spitch, int width, int height, int threshold)
{
	int	sinc = - (width + 1) * xpitch;
	dpitch += sinc;
	sinc += spitch;

	do
	{
		dp[0] = sp[0];
		dp += xpitch; sp += xpitch;
		int	i = width;
		do
		{
			unsigned	sort1[8];
			int	leq = 0;
			int	geq = 0;
			unsigned	x = sp[0];
			if( (sort1[0] = (sp += xpitch)[0]) <= x )
			{
				if( sort1[0] == x ) ++geq;
				++leq;
			}
			if( (sort1[1] = (sp += spitch)[0]) <= x )
			{
				if( sort1[1] == x ) ++geq;
				++leq;
			}
			if( (sort1[2] = (sp -= xpitch)[0]) <= x )
			{
				if( sort1[2] == x ) ++geq;
				++leq;
			}
			if( (sort1[3] = (sp -= xpitch)[0]) <= x )
			{
				if( sort1[3] >= x ) ++geq;
				++leq;
			}
			if( (sort1[4] = (sp -= spitch)[0]) <= x )
			{
				if( sort1[4] >= x ) ++geq;
				++leq;
			}
			if( (sort1[5] = (sp -= spitch)[0]) <= x )
			{
				if( sort1[5] >= x ) ++geq;
				++leq;
			}
			if( (sort1[6] = (sp += xpitch)[0]) <= x )
			{
				if( sort1[6] >= x ) ++geq;
				++leq;
			}
			if( (sort1[7] = (sp += xpitch)[0]) <= x )
			{
				if( sort1[7] >= x ) ++geq;
				++leq;
			}

			if( ((geq += 8 - leq) < threshold) || (leq < threshold) )
			{ // do a merge sort of sort1[8] as fast as possible
				unsigned sort2[8];
				if( sort1[1] < sort1[0] ) 
				{
					sort2[0] = sort1[1];
					sort2[1] = sort1[0];
				}
				else
				{
					sort2[0] = sort1[0];
					sort2[1] = sort1[1];
				}
				if( sort1[3] < sort1[2] ) 
				{
					sort2[2] = sort1[3];
					sort2[3] = sort1[2];
				}
				else
				{
					sort2[2] = sort1[2];
					sort2[3] = sort1[3];
				}
				if( sort1[5] < sort1[4] ) 
				{
					sort2[4] = sort1[5];
					sort2[5] = sort1[4];
				}
				else
				{
					sort2[4] = sort1[4];
					sort2[5] = sort1[5];
				}
				if( sort1[7] < sort1[6] ) 
				{
					sort2[6] = sort1[7];
					sort2[7] = sort1[6];
				}
				else
				{
					sort2[6] = sort1[6];
					sort2[7] = sort1[7];
				}

				if( sort2[0] > sort2[2] )
				{
					sort1[0] = sort2[2];
					if( sort2[3] <= sort2[0] )
					{
						sort1[1] = sort2[3];
						sort1[2] = sort2[0];
						sort1[3] = sort2[1];
					}
					else
					{
						sort1[1] = sort2[0];
						if( sort2[1] < sort2[3] )
						{
							sort1[2] = sort2[1];
							sort1[3] = sort2[3];
						}
						else
						{
							sort1[2] = sort2[3];
							sort1[3] = sort2[1];
						}
					}
				}
				else
				{
					sort1[0] = sort2[0];
					if( sort2[1] <= sort2[2] )
					{
						sort1[1] = sort2[1];
						sort1[2] = sort2[2];
						sort1[3] = sort2[3];
					}
					else
					{
						sort1[1] = sort2[2];
						if( sort2[3] < sort2[1] )
						{
							sort1[2] = sort2[3];
							sort1[3] = sort2[1];
						}
						else
						{
							sort1[2] = sort2[1];
							sort1[3] = sort2[3];
						}
					}
				}
#if	0
				if( (sort1[0] > sort1[1]) || (sort1[1] > sort1[2]) || (sort1[2] > sort1[3]) )
					debug_printf("merge error: sort = %u, %u, %u, %u\n", sort1[0], sort1[1], sort1[2], sort1[3]);
#endif

				if( sort2[4] > sort2[6] )
				{
					sort1[4] = sort2[6];
					if( sort2[7] <= sort2[4] )
					{
						sort1[5] = sort2[7];
						sort1[6] = sort2[4];
						sort1[7] = sort2[5];
					}
					else
					{
						sort1[5] = sort2[4];
						if( sort2[5] < sort2[7] )
						{
							sort1[6] = sort2[5];
							sort1[7] = sort2[7];
						}
						else
						{
							sort1[6] = sort2[7];
							sort1[7] = sort2[5];
						}
					}
				}
				else
				{
					sort1[4] = sort2[4];
					if( sort2[5] <= sort2[6] )
					{
						sort1[5] = sort2[5];
						sort1[6] = sort2[6];
						sort1[7] = sort2[7];
					}
					else
					{
						sort1[5] = sort2[6];
						if( sort2[7] < sort2[5] )
						{
							sort1[6] = sort2[7];
							sort1[7] = sort2[5];
						}
						else
						{
							sort1[6] = sort2[5];
							sort1[7] = sort2[7];
						}
					}
				}
#if	0
				if( (sort1[4] > sort1[5]) || (sort1[5] > sort1[6]) || (sort1[6] > sort1[7]) )
					debug_printf("merge error: sort = %u, %u, %u, %u\n", sort1[4], sort1[5], sort1[6], sort1[7]);
#endif

				unsigned *s1 = sort1, *s2 = sort1 + 4, *t = sort2;
				*t++ = *s1	> *s2 ? *s2++ : *s1++;
				*t++ = *s1	> *s2 ? *s2++ : *s1++;
				*t++ = *s1	> *s2 ? *s2++ : *s1++;	
				if( sort1[3] > sort1[7] )
				{
					do
					{
						*t++ = *s1 > *s2 ? *s2++ : *s1++;
					} while( s2 != sort1 + 8 );
					do
					{
						*t++ = *s1++;
					} while( s1 != sort1 + 4 );
				}
				else
				{
					do
					{
						*t++ = *s1 > *s2 ? *s2++ : *s1++;
					} while( s1 != sort1 + 4 );
					do
					{
						*t++ = *s2++;
					} while( s2 != sort1 + 8 );
				}
				
#if	0
				if( (leq > 0) && (sort2[leq - 1] > x) ) debug_printf("leq = %u, x = %u, sort = %u,%u,%u,%u,%u,%u,%u,%u\n", leq, x, sort2[0], sort2[1], sort2[2], sort2[3], sort2[4], sort2[5], sort2[6], sort2[7]);
				if( (leq < 8) && (sort2[leq] <= x) ) debug_printf("leq = %u, x = %u, sort = %u,%u,%u,%u,%u,%u,%u,%u\n", leq, x, sort2[0], sort2[1], sort2[2], sort2[3], sort2[4], sort2[5], sort2[6], sort2[7]);
				if( (geq > 0) && (sort2[8 - geq] < x) ) debug_printf("geq = %u, x = %u, sort = %u,%u,%u,%u,%u,%u,%u,%u\n", geq, x, sort2[0], sort2[1], sort2[2], sort2[3], sort2[4], sort2[5], sort2[6], sort2[7]);
				if( (geq < 8) && (sort2[7 - geq] >= x) ) debug_printf("geq = %u, x = %u, sort = %u,%u,%u,%u,%u,%u,%u,%u\n", geq, x, sort2[0], sort2[1], sort2[2], sort2[3], sort2[4], sort2[5], sort2[6], sort2[7]);
#endif
				x = leq < threshold ? sort2[threshold - 1] : sort2[8 - threshold];
			}
			dp[0] = x;
			dp += xpitch;
			sp += spitch;
		} while( --i );
		dp[0] = sp[0];
		dp += dpitch; sp += sinc;
	} while( --height );
}
#undef	xpitch
#endif	// TESTCOMPARE

#if		ISSE > 1
#define	SSE_INCREMENT	16
#define	SSE_SHIFT		4
#define	SSE_MOVE		movdqu
#if		ISSE > 2
#define	SSE3_MOVE		lddqu
#else
#define	SSE3_MOVE		movdqu
#endif
#define	SSE_RMOVE		movdqa
#define	SSE0			xmm0
#define	SSE1			xmm1
#define	SSE2			xmm2
#define	SSE3			xmm3
#define	SSE4			xmm4
#define	SSE5			xmm5
#define	SSE6			xmm6
#define	SSE7			xmm7
#define	SSE_EMMS	
#else
#define	SSE_INCREMENT	8
#define	SSE_SHIFT		3
#define	SSE_MOVE		movq
#define	SSE3_MOVE		movq
#define	SSE_RMOVE		movq
#define	SSE0			mm0
#define	SSE1			mm1
#define	SSE2			mm2
#define	SSE3			mm3
#define	SSE4			mm4
#define	SSE5			mm5
#define	SSE6			mm6
#define	SSE7			mm7
#define	SSE_EMMS		__asm	emms
#endif	// ISSE

#if	defined(SHARPEN) || defined(BLUR)
#define	SHLUR
#endif

#if	BLUR == 1
#define	blur(center, min, max, reg1, reg2)\
__asm	SSE_RMOVE	reg2,				center			\
__asm	psubusb		max,				center			\
__asm	psubusb		reg2,				min				\
__asm	SSE_RMOVE	reg1,				max				\
__asm	SSE_RMOVE	min,				reg2			\
__asm	psubusb		max,				reg2			\
__asm	psubusb		min,				reg1			\
__asm	psrlw		max,				1				\
__asm	psrlw		min,				1				\
__asm	pminub		reg2,				max				\
__asm	pminub		reg1,				min				\
__asm	paddusb		center,				reg2			\
__asm	psubusb		center,				reg1
#elif	BLUR == 2
__asm	pminub		center,				max				\
__asm	pmaxub		center,				min				\
__asm	SSE_RMOVE	reg2,				center			\
__asm	psubusb		max,				center			\
__asm	psubusb		reg2,				min				\
__asm	SSE_RMOVE	reg1,				max				\
__asm	SSE_RMOVE	min,				reg2			\
__asm	psubusb		max,				reg2			\
__asm	psubusb		min,				reg1			\
__asm	psrlw		max,				1				\
__asm	psrlw		min,				1				\
__asm	pminub		reg2,				max				\
__asm	pminub		reg1,				min				\
__asm	paddusb		center,				reg2			\
__asm	psubusb		center,				reg1
#endif

#ifdef	SHARPEN

static	const __declspec(align(SSE_INCREMENT)) unsigned short rshift[3][SSE_INCREMENT / 2] =
{
	{
		0,0,0,0		
#if	SSE_INCREMENT == 16
		,0,0,0,0		
#endif
	},
	{
		1,0,0,0		
#if	SSE_INCREMENT == 16
		, 0,0,0,0			
#endif
	},
	{
		2,0,0,0		
#if	SSE_INCREMENT == 16
		, 0,0,0,0
#endif
	}
};

#define	SHIFT_MASK0	255
#define	SHIFT_MASK1	127
#define	SHIFT_MASK2	63
static	const __declspec(align(SSE_INCREMENT)) BYTE	shift_mask[3][SSE_INCREMENT] =
{
	{
		SHIFT_MASK0, SHIFT_MASK0, SHIFT_MASK0, SHIFT_MASK0, SHIFT_MASK0, SHIFT_MASK0, SHIFT_MASK0, SHIFT_MASK0		
#if	SSE_INCREMENT == 16
		, SHIFT_MASK0, SHIFT_MASK0, SHIFT_MASK0, SHIFT_MASK0, SHIFT_MASK0, SHIFT_MASK0, SHIFT_MASK0, SHIFT_MASK0
#endif
	},
	{
		SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1		
#if	SSE_INCREMENT == 16
		, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1
#endif
	},
	{
		SHIFT_MASK2, SHIFT_MASK2, SHIFT_MASK2, SHIFT_MASK2, SHIFT_MASK2, SHIFT_MASK2, SHIFT_MASK2, SHIFT_MASK2		
#if	SSE_INCREMENT == 16
		, SHIFT_MASK2, SHIFT_MASK2, SHIFT_MASK2, SHIFT_MASK2, SHIFT_MASK2, SHIFT_MASK2, SHIFT_MASK2, SHIFT_MASK2
#endif
	}
};

#if	SHARPEN == 1
// only sharpen
#define	sharpen(center, min, max, rshift, SHIFT_MASK1, reg1, reg2)\
__asm	SSE_RMOVE	reg2,				center			\
__asm	psubusb		max,				center			\
__asm	psubusb		reg2,				min				\
__asm	SSE_RMOVE	reg1,				max				\
__asm	SSE_RMOVE	min,				reg2			\
__asm	psubusb		max,				reg2			\
__asm	psubusb		min,				reg1			\
__asm	psrlw		reg2,				rshift			\
__asm	psrlw		reg1,				rshift			\
__asm	pand		reg2,				SHIFT_MASK1		\
__asm	pand		reg1,				SHIFT_MASK1		\
__asm	pminub		reg2,				max				\
__asm	pminub		reg1,				min				\
__asm	psubusb		center,				reg2			\
__asm	paddusb		center,				reg1

#elif	SHARPEN == 2
// clip and sharpen
#define	sharpen(center, min, max, rshift, SHIFT_MASK1, reg1, reg2)\
__asm	pminub		center,				max				\
__asm	pmaxub		center,				min				\
__asm	SSE_RMOVE	reg2,				center			\
__asm	psubusb		max,				center			\
__asm	psubusb		reg2,				min				\
__asm	SSE_RMOVE	reg1,				max				\
__asm	SSE_RMOVE	min,				reg2			\
__asm	psubusb		max,				reg2			\
__asm	psubusb		min,				reg1			\
__asm	psrlw		reg2,				rshift			\
__asm	psrlw		reg1,				rshift			\
__asm	pand		reg2,				SHIFT_MASK1		\
__asm	pand		reg1,				SHIFT_MASK1		\
__asm	pminub		reg2,				max				\
__asm	pminub		reg1,				min				\
__asm	psubusb		center,				reg2			\
__asm	paddusb		center,				reg1
#endif
#endif	// SHARPEN

#ifdef	BLUR
#define	sharpen(center, min, max, rshift, SHIFT_MASK1, reg1, reg2)	blur(center, min, max, reg1, reg2)
#endif

#ifdef	SHARPEN

void	do_nothing(BYTE *dp, int dpitch, const BYTE *sp, int spitch, int hblocks, int remainder, int incpitch, int height, int strength)
{
}

void	copy_plane(BYTE *dp, int dpitch, const BYTE *sp, int spitch, int hblocks, int remainder, int incpitch, int height, int strength)
{
	AVSenvironment->BitBlt(dp, dpitch, sp, spitch, hblocks * SSE_INCREMENT + 2 * (SSE_INCREMENT + 1) + remainder, height);
}

#else // SHARPEN

void	do_nothing(BYTE *dp, int dpitch, const BYTE *sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
}

void	copy_plane(BYTE *dp, int dpitch, const BYTE *sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
	AVSenvironment->BitBlt(dp, dpitch, sp, spitch, hblocks * SSE_INCREMENT + 2 * (SSE_INCREMENT + 1) + remainder, height);
}
#endif	// SHARPEN

#define	ins2(first, second, reg)						\
__asm	pmaxub		second,				reg				\
__asm	pminub		second,				first			\
__asm	pmaxub		first,				reg

#define	ins3(first, second, third, reg)					\
__asm	pmaxub		third,				reg				\
__asm	pminub		third,				second			\
		ins2(first, second, reg)

#define	ins4(first, second, third, fourth, reg)			\
__asm	pmaxub		fourth,				reg				\
__asm	pminub		fourth,				third			\
		ins3(first, second, third, reg)

#define	ins5(first, second, third, fourth, fifth, reg)	\
__asm	pmaxub		fifth,				reg				\
__asm	pminub		fifth,				fourth			\
		ins4(first, second, third, fourth, reg)

#define	ins6(first, second, third, fourth, fifth, sixth, reg)	\
__asm	pmaxub		sixth,				reg				\
__asm	pminub		sixth,				fifth			\
		ins5(first, second, third, fourth, fifth, reg)

#define	add2(first, second, reg)						\
__asm	SSE_RMOVE	second,				reg				\
__asm	pminub		second,				first			\
__asm	pmaxub		first,				reg	

#define	add3(first, second, third, reg)					\
__asm	SSE_RMOVE	third,				reg				\
__asm	pminub		third,				second			\
		ins2(first, second, reg)

#define	add4(first, second, third, fourth, reg)			\
__asm	SSE_RMOVE	fourth,				reg				\
__asm	pminub		fourth,				third			\
		ins3(first, second, third, reg)

#define	add5(first, second, third, fourth, fifth, reg)	\
__asm	SSE_RMOVE	fifth,				reg				\
__asm	pminub		fifth,				fourth			\
		ins4(first, second, third, fourth, reg)

#define	add6(first, second, third, fourth, fifth, sixth, reg)	\
__asm	SSE_RMOVE	sixth,				reg				\
__asm	pminub		sixth,				fifth			\
		ins5(first, second, third, fourth, fifth, reg)

#define	sub2(first, second, val)						\
__asm	pmaxub		second,				val				\
__asm	pminub		second,				first	

#define	sub3(first, second, third, reg)					\
__asm	pmaxub		third,				reg				\
__asm	pminub		third,				second			\
		sub2(first, second, reg)

#define	sub4(first, second, third, fourth, reg)			\
__asm	pmaxub		fourth,				reg				\
__asm	pminub		fourth,				third			\
		sub3(first, second, third, reg)

#define	sub5(first, second, third, fourth, fifth, reg)	\
__asm	pmaxub		fifth,				reg				\
__asm	pminub		fifth,				fourth			\
		sub4(first, second, third, fourth, reg)

#define	sub6(first, second, third, fourth, fifth, sixth, reg)	\
__asm	pmaxub		sixth,				reg				\
__asm	pminub		sixth,				fifth			\
		sub5(first, second, third, fourth, fifth, reg)

#define	minmax1(min, max, val)							\
__asm	pminub		min,				val				\
__asm	pmaxub		max,				val

#define	minmax2(max1, max2, min2, min1, reg)			\
__asm	pminub		min2,				reg				\
__asm	pmaxub		max2,				reg				\
__asm	pmaxub		min2,				min1			\
__asm	pminub		max2,				max1			\
__asm	pminub		min1,				reg				\
__asm	pmaxub		max1,				reg		


#define	minmax3(max1, max2, max3, min3, min2, min1, reg)\
__asm	pminub		min3,				reg				\
__asm	pmaxub		max3,				reg				\
__asm	pmaxub		min3,				min2			\
__asm	pminub		max3,				max2			\
		minmax2(max1, max2, min2, min1, reg)

#define	minmax2sub(max1, max2, min2, min1, val)			\
__asm	pminub		min2,				val				\
__asm	pmaxub		max2,				val				\
__asm	pmaxub		min2,				min1			\
__asm	pminub		max2,				max1

#define	minmax3sub(max1, max2, max3, min3, min2, min1, reg)\
__asm	pminub		min3,				reg				\
__asm	pmaxub		max3,				reg				\
__asm	pmaxub		min3,				min2			\
__asm	pminub		max3,				max2			\
		minmax2sub(max1, max2, min2, min1, reg)

#ifdef	SHARPEN
void	SSE_RemoveGrain4(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height, int strength)
#else
void	SSE_RemoveGrain4(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
#endif
{
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
#ifdef	SHARPEN
__asm	mov			spitch,				eax
__asm	mov			eax,				strength
#endif
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 1]
		add2(SSE0, SSE1, SSE7)
__asm	SSE3_MOVE	SSE6,				[esi + 2]
__asm	SSE3_MOVE	SSE5,				[esi + ebx]
		add3(SSE0, SSE1, SSE2, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 2]
		add4(SSE0, SSE1, SSE2, SSE3, SSE5)
__asm	movd		[edi],				SSE5
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx]				
		add5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE7)
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx + 1]				
		sub5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 2]				
		sub4(SSE1, SSE2, SSE3, SSE4, SSE5)
#if		(ISSE > 1) || defined(SHLUR)
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]
#endif
		sub3(SSE2, SSE3, SSE4, SSE7)
#ifdef	SHLUR
		sharpen(SSE5, SSE4, SSE3, rshift[eax], shift_mask[eax], SSE0, SSE1)
__asm	SSE_MOVE	[edi + 1],			SSE5
#else	// SHLUR
#if		ISSE > 1
__asm	pmaxub		SSE4,				SSE5
#else
__asm	pmaxub		SSE4,				[esi + ebx + 1]
#endif
__asm	pminub		SSE3,				SSE4
__asm	SSE_MOVE	[edi + 1],			SSE3
#endif	// SHLUR
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif // MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 1]
		add2(SSE0, SSE1, SSE7)
__asm	SSE3_MOVE	SSE6,				[esi + 2]
__asm	SSE3_MOVE	SSE5,				[esi + ebx]
		add3(SSE0, SSE1, SSE2, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 2]
		add4(SSE0, SSE1, SSE2, SSE3, SSE5)
#if		MODIFYPLUGIN == 1
__asm	SSE3_MOVE	SSE6,				[esi + ebx + 1]				
		add5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE7)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx]
		add6(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 1]
		sub6(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE7)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 2]
		sub5(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
#if		ISSE > 1	
__asm	SSE3_MOVE	SSE0,				[edi]
#endif
		sub4(SSE2, SSE3, SSE4, SSE5, SSE7)
#if		ISSE > 1
__asm	pmaxub		SSE5,				SSE0
#else
__asm	pmaxub		SSE5,				[edi]
#endif
__asm	add			esi,				SSE_INCREMENT
__asm	pminub		SSE3,				SSE5
__asm	SSE_MOVE	[edi],				SSE3
#else	// MODIFYPLUGIN == 1
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx]				
		add5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE7)
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx + 1]				
		sub5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 2]				
		sub4(SSE1, SSE2, SSE3, SSE4, SSE5)
#if		(ISSE > 1) || defined(SHLUR)
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE5,				[edi]
#else
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]
#endif
#endif
		sub3(SSE2, SSE3, SSE4, SSE7)
#ifdef	SHLUR
		sharpen(SSE5, SSE4, SSE3, rshift[eax], shift_mask[eax], SSE0, SSE1)
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE5
#else
#if		ISSE > 1
__asm	pmaxub		SSE4,				SSE5
#else
#ifdef	MODIFYPLUGIN
__asm	pmaxub		SSE4,				[edi]
#else	// ISSE > 1
__asm	pmaxub		SSE4,				[esi + ebx + 1]
#endif
#endif	// ISSE > 1
__asm	add			esi,				SSE_INCREMENT
__asm	pminub		SSE3,				SSE4
__asm	SSE_MOVE	[edi],				SSE3
#endif	// SHLUR
#endif	//MODIFYPLUGIN == 1
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 1]
		add2(SSE0, SSE1, SSE7)
__asm	SSE3_MOVE	SSE6,				[esi + 2]
__asm	SSE3_MOVE	SSE5,				[esi + ebx]
		add3(SSE0, SSE1, SSE2, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 2]
		add4(SSE0, SSE1, SSE2, SSE3, SSE5)
#ifndef		MODIFYPLUGIN
__asm	SSE_MOVE	[edi + 1],			SSE7
#endif
#if		MODIFYPLUGIN == 1
__asm	SSE3_MOVE	SSE6,				[esi + ebx + 1]				
		add5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE7)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx]
		add6(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 1]
		sub6(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE7)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 2]
		sub5(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
#if		ISSE > 1	
__asm	SSE3_MOVE	SSE0,				[edi]
#endif
		sub4(SSE2, SSE3, SSE4, SSE5, SSE7)
#if		ISSE > 1
__asm	pmaxub		SSE5,				SSE0
#else
__asm	pmaxub		SSE5,				[edi]
#endif
__asm	pminub		SSE3,				SSE5
__asm	add			esi,				eax
__asm	SSE_MOVE	[edi],				SSE3
#else	// MODIFYPLUGIN == 1
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx]				
		add5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE7)
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx + 1]				
		sub5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 2]				
		sub4(SSE1, SSE2, SSE3, SSE4, SSE5)
#if		(ISSE > 1) || defined(SHLUR)
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE5,				[edi]
#else
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]
#endif
#endif
		sub3(SSE2, SSE3, SSE4, SSE7)
#ifdef	SHLUR
		sharpen(SSE5, SSE4, SSE3, rshift[eax], shift_mask[eax], SSE0, SSE1)
#ifdef	SHARPEN
__asm	add			esi,				spitch
#else
__asm	add			esi,				eax
#endif
__asm	SSE_MOVE	[edi],				SSE5
#else
#if		ISSE > 1
__asm	pmaxub		SSE4,				SSE5
#else
#ifdef	MODIFYPLUGIN
__asm	pmaxub		SSE4,				[edi]
#else	// ISSE > 1
__asm	pmaxub		SSE4,				[esi + ebx + 1]
#endif
#endif	// ISSE > 1
__asm	pminub		SSE3,				SSE4
__asm	add			esi,				eax
__asm	SSE_MOVE	[edi],				SSE3
#endif	// SHLUR
#endif	//MODIFYPLUGIN == 1
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}

#ifdef	SHARPEN
void	SSE_RemoveGrain1(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height, int strength)
#else
void	SSE_RemoveGrain1(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
#endif
{
#ifdef	SHARPEN
__asm	mov			ebx,				strength
__asm	SSE_RMOVE	SSE2,				rshift[ebx]
__asm	SSE_RMOVE	SSE3,				shift_mask[ebx]
#endif
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE5,				[esi + 1]
__asm	SSE_RMOVE	SSE1,				SSE0
__asm	SSE3_MOVE	SSE4,				[esi + 2]
		minmax1(SSE0, SSE1, SSE5)
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx]
		minmax1(SSE0, SSE1, SSE4)
__asm	SSE3_MOVE	SSE4,				[esi + 2*ebx + 1]
		minmax1(SSE0, SSE1, SSE5)
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx + 2]
		minmax1(SSE0, SSE1, SSE4)
__asm	SSE3_MOVE	SSE4,				[esi + ebx]
		minmax1(SSE0, SSE1, SSE5)
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 2]
		minmax1(SSE0, SSE1, SSE4)
#if		(ISSE > 1) || defined(SHLUR)
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 1]
#endif
__asm	movd		[edi],				SSE4		// only for saving the first byte
		minmax1(SSE0, SSE1, SSE5)
#ifdef	SHLUR
		sharpen(SSE7, SSE0, SSE1, SSE2, SSE3, SSE5, SSE4)
__asm	SSE_MOVE	[edi + 1],			SSE7
#else
#if	ISSE > 1
__asm	pmaxub		SSE0,				SSE7
#else
__asm	pmaxub		SSE0,				[esi + ebx + 1]
#endif
__asm	pminub		SSE0,				SSE1
__asm	SSE_MOVE	[edi + 1],			SSE0
#endif	// SHLUR
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif	// MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE5,				[esi + 1]
__asm	SSE_RMOVE	SSE1,				SSE0
__asm	SSE3_MOVE	SSE4,				[esi + 2]
		minmax1(SSE0, SSE1, SSE5)
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx]
		minmax1(SSE0, SSE1, SSE4)
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx + 1]
		minmax1(SSE0, SSE1, SSE6)
__asm	SSE3_MOVE	SSE4,				[esi + 2*ebx + 2]
		minmax1(SSE0, SSE1, SSE5)
__asm	SSE3_MOVE	SSE6,				[esi + ebx]
		minmax1(SSE0, SSE1, SSE4)
#if	MODIFYPLUGIN == 1
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]
		minmax1(SSE0, SSE1, SSE6)
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 2]
		minmax1(SSE0, SSE1, SSE5)
#if		ISSE > 1
__asm	SSE3_MOVE	SSE7,				[edi]			
#endif
		minmax1(SSE0, SSE1, SSE4)
#else	// MODIFYPLUGIN
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 2]
		minmax1(SSE0, SSE1, SSE6)
#if		(ISSE > 1) || defined(SHLUR)
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 1]
#endif
		minmax1(SSE0, SSE1, SSE5)
#endif	// MODIFYPLUGIN
#ifdef	SHLUR
		sharpen(SSE7, SSE0, SSE1, SSE2, SSE3, SSE5, SSE4)
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE7
#else
#if		ISSE > 1
__asm	pmaxub		SSE0,					SSE7
#else
#ifdef	MODIFYPLUGIN
__asm	pmaxub	SSE0,					[edi]
#else
__asm	pmaxub		SSE0,				[esi + ebx + 1]
#endif
#endif	// ISSE > 1
__asm	pminub		SSE0,				SSE1
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE0
#endif	// SHLUR
__asm	add			edi,				SSE_INCREMENT
#if	((MODIFYPLUGIN == 1) && (ISSE > 1)) || defined(SHLUR)
__asm	dec			ecx
__asm	jnz			middle_loop
#else
__asm	loop		middle_loop
#endif
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE5,				[esi + 1]
__asm	SSE_RMOVE	SSE1,				SSE0
__asm	SSE3_MOVE	SSE4,				[esi + 2]
		minmax1(SSE0, SSE1, SSE5)
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx]
		minmax1(SSE0, SSE1, SSE4)
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx + 1]
		minmax1(SSE0, SSE1, SSE6)
__asm	SSE3_MOVE	SSE4,				[esi + 2*ebx + 2]
		minmax1(SSE0, SSE1, SSE5)
__asm	SSE3_MOVE	SSE6,				[esi + ebx]
		minmax1(SSE0, SSE1, SSE4)
#if	MODIFYPLUGIN == 1
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]
		minmax1(SSE0, SSE1, SSE6)
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 2]
		minmax1(SSE0, SSE1, SSE5)
#if		ISSE > 1
__asm	SSE3_MOVE	SSE7,				[edi]			
#endif
		minmax1(SSE0, SSE1, SSE4)
#else	// MODIFYPLUGIN == 1
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 2]
		minmax1(SSE0, SSE1, SSE6)
#if		(ISSE > 1) || defined(SHLUR)
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 1]
#endif
#ifndef	MODIFYPLUGIN
__asm	SSE_MOVE	[edi + 1],			SSE5		// only for saving the last byte
#endif
		minmax1(SSE0, SSE1, SSE5)
#endif	// MODIFYPLUGIN == 1
#ifdef	SHLUR
		sharpen(SSE7, SSE0, SSE1, SSE2, SSE3, SSE5, SSE4)
__asm	add			esi,				eax
__asm	SSE_MOVE	[edi],				SSE7
#else
#if		ISSE > 1
__asm	pmaxub		SSE0,				SSE7
#else
#ifdef	MODIFYPLUGIN
__asm	pmaxub		SSE0,				[edi]
#else
__asm	pmaxub		SSE0,				[esi + ebx + 1]
#endif
#endif	// ISSE > 1
__asm	pminub		SSE0,				SSE1
__asm	add			esi,				eax
__asm	SSE_MOVE	[edi],				SSE0
#endif	// SHLUR
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}

#ifdef	SHARPEN
void	SSE_RemoveGrain2(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height, int strength)
#else
void	SSE_RemoveGrain2(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
#endif
{
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
#ifdef	SHARPEN
__asm	mov			spitch,				eax
__asm	mov			eax,				strength
#endif
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 1]
		add2(SSE0, SSE1, SSE7)
__asm	SSE3_MOVE	SSE6,				[esi + 2]
__asm	SSE3_MOVE	SSE7,				[esi + ebx]
		add3(SSE0, SSE1, SSE2, SSE6)
__asm	movd		[edi],				SSE7
__asm	SSE3_MOVE	SSE6,				[esi + ebx + 2]
		add4(SSE0, SSE1, SSE2, SSE3, SSE7)
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx]
		minmax2(SSE0, SSE1, SSE2, SSE3, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 1]
		minmax2(SSE0, SSE1, SSE2, SSE3, SSE5)
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 2]
		minmax2(SSE0, SSE1, SSE2, SSE3, SSE7)
#if		(ISSE > 1) || defined(SHLUR)
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]				
#endif
		minmax2sub(SSE0, SSE1, SSE2, SSE3, SSE6)
#ifdef	SHLUR
		sharpen(SSE5, SSE2, SSE1, rshift[eax], shift_mask[eax], SSE6, SSE7)
__asm	SSE_MOVE	[edi + 1],			SSE5
#else
#if		ISSE > 1
__asm	pmaxub		SSE2,				SSE5
#else
__asm	pmaxub		SSE2,				[esi + ebx + 1]	
#endif
__asm	pminub		SSE1,				SSE2
__asm	SSE_MOVE	[edi + 1],			SSE1
#endif	// SHLUR
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif // MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 1]
		add2(SSE0, SSE1, SSE7)
__asm	SSE3_MOVE	SSE6,				[esi + 2]
__asm	SSE3_MOVE	SSE7,				[esi + ebx]
		add3(SSE0, SSE1, SSE2, SSE6)
#if	MODIFYPLUGIN == 1
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 1]
#else
__asm	SSE3_MOVE	SSE6,				[esi + ebx + 2]
#endif
		add4(SSE0, SSE1, SSE2, SSE3, SSE7)
#if	MODIFYPLUGIN == 1
__asm	SSE3_MOVE	SSE6,				[esi + ebx + 2]
		minmax2(SSE0, SSE1, SSE2, SSE3, SSE4)
#endif
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx]
		minmax2(SSE0, SSE1, SSE2, SSE3, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 1]
		minmax2(SSE0, SSE1, SSE2, SSE3, SSE5)
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 2]
		minmax2(SSE0, SSE1, SSE2, SSE3, SSE7)
#if		(ISSE > 1) || defined(SHLUR)
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE5,				[edi]
#else
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]	
#endif
#endif
		minmax2sub(SSE0, SSE1, SSE2, SSE3, SSE6)
#ifdef	SHLUR
		sharpen(SSE5, SSE2, SSE1, rshift[eax], shift_mask[eax], SSE6, SSE7)
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE5
#else
#if		ISSE > 1
__asm	pmaxub		SSE2,				SSE5
#else
#ifdef	MODIFYPLUGIN
__asm	pmaxub		SSE2,				[edi]
#else
__asm	pmaxub		SSE2,				[esi + ebx + 1]	
#endif
#endif	// ISSE > 1
__asm	pminub		SSE1,				SSE2
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE1
#endif	// SHLUR
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 1]
		add2(SSE0, SSE1, SSE7)
__asm	SSE3_MOVE	SSE6,				[esi + 2]
__asm	SSE3_MOVE	SSE7,				[esi + ebx]
		add3(SSE0, SSE1, SSE2, SSE6)
#if	MODIFYPLUGIN == 1
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 1]
#else
__asm	SSE3_MOVE	SSE6,				[esi + ebx + 2]
#endif
		add4(SSE0, SSE1, SSE2, SSE3, SSE7)
#if	MODIFYPLUGIN == 1
__asm	SSE3_MOVE	SSE6,				[esi + ebx + 2]
		minmax2(SSE0, SSE1, SSE2, SSE3, SSE4)
#endif
#ifndef	MODIFYPLUGIN
__asm	SSE_MOVE	[edi + 1],			SSE6
#endif
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx]
		minmax2(SSE0, SSE1, SSE2, SSE3, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 1]
		minmax2(SSE0, SSE1, SSE2, SSE3, SSE5)
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 2]
		minmax2(SSE0, SSE1, SSE2, SSE3, SSE7)
#if		(ISSE > 1) || defined(SHLUR)
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE5,				[edi]
#else
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]	
#endif
#endif
		minmax2sub(SSE0, SSE1, SSE2, SSE3, SSE6)
#ifdef	SHLUR
		sharpen(SSE5, SSE2, SSE1, rshift[eax], shift_mask[eax], SSE6, SSE7)
#ifdef	SHARPEN
__asm	add			esi,				spitch
#else
__asm	add			esi,				eax
#endif
__asm	SSE_MOVE	[edi],				SSE5
#else
#if		ISSE > 1
__asm	pmaxub		SSE2,				SSE5
#else
#ifdef	MODIFYPLUGIN
__asm	pmaxub		SSE2,				[edi]
#else
__asm	pmaxub		SSE2,				[esi + ebx + 1]	
#endif
#endif	// ISSE > 1
__asm	pminub		SSE1,				SSE2
__asm	add			esi,				eax
__asm	SSE_MOVE	[edi],				SSE1
#endif	// SHLUR
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}

#ifdef	SHARPEN
void	SSE_RemoveGrain3(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height, int strength)
#else
void	SSE_RemoveGrain3(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
#endif
{ 
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
#ifdef	SHARPEN
__asm	mov			spitch,				eax
__asm	mov			eax,				strength
#endif
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 1]
		add2(SSE0, SSE1, SSE7)
__asm	SSE3_MOVE	SSE6,				[esi + 2]
__asm	SSE3_MOVE	SSE5,				[esi + ebx]
		add3(SSE0, SSE1, SSE2, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 2]
		add4(SSE0, SSE1, SSE2, SSE3, SSE5)
__asm	movd		[edi],				SSE5
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx]				
		add5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE7)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 1]
		add6(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 2]
		minmax3sub(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE7)
#if		(ISSE > 1) || defined(SHLUR)
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 1]
#endif
		minmax2sub(SSE1, SSE2, SSE3, SSE4, SSE6)
#ifdef	SHLUR
		sharpen(SSE7, SSE3, SSE2, rshift[eax], shift_mask[eax], SSE0, SSE5)
__asm	SSE_MOVE	[edi + 1],			SSE7
#else
#if		ISSE > 1
__asm	pmaxub		SSE3,				SSE7
#else
__asm	pmaxub		SSE3,				[esi + ebx + 1]
#endif
__asm	pminub		SSE3,				SSE2
__asm	SSE_MOVE	[edi + 1],			SSE3
#endif	// SHLUR
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif // MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 1]
		add2(SSE0, SSE1, SSE7)
__asm	SSE3_MOVE	SSE6,				[esi + 2]
__asm	SSE3_MOVE	SSE5,				[esi + ebx]
		add3(SSE0, SSE1, SSE2, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 2]
		add4(SSE0, SSE1, SSE2, SSE3, SSE5)
#if		MODIFYPLUGIN == 1
__asm	SSE3_MOVE	SSE6,				[esi + ebx + 1]				
		add5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE7)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx]
		add6(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 1]
		minmax3(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE7)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 2]
		minmax3sub(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
#if		ISSE > 1
__asm	SSE3_MOVE	SSE6,				[edi]
#endif
		minmax2sub(SSE1, SSE2, SSE3, SSE4, SSE7)
#if		ISSE > 1
__asm	pmaxub		SSE3,				SSE6
#else
__asm	pmaxub		SSE3,				[edi]
#endif
__asm	pminub		SSE3,				SSE2
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE3
#else	// MODIFYPLUGIN == 1
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx]				
		add5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE7)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 1]
		add6(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 2]
		minmax3sub(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE7)
#if		(ISSE > 1) || defined(SHLUR)
#ifdef		MODIFYPLUGIN
__asm	SSE3_MOVE	SSE7,				[edi]
#else
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 1]
#endif
#endif
		minmax2sub(SSE1, SSE2, SSE3, SSE4, SSE6)
#ifdef	SHLUR
		sharpen(SSE7, SSE3, SSE2, rshift[eax], shift_mask[eax], SSE0, SSE5)
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE7
#else
#if		ISSE > 1
__asm	pmaxub		SSE3,				SSE7
#else
#ifdef		MODIFYPLUGIN
__asm	pmaxub		SSE3,				[edi]
#else
__asm	pmaxub		SSE3,				[esi + ebx + 1]
#endif
#endif	// ISSE > 1
__asm	pminub		SSE3,				SSE2
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE3
#endif	// SHLUR
#endif	// MODIFYPLUGIN == 1	
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 1]
		add2(SSE0, SSE1, SSE7)
__asm	SSE3_MOVE	SSE6,				[esi + 2]
__asm	SSE3_MOVE	SSE5,				[esi + ebx]
		add3(SSE0, SSE1, SSE2, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 2]
		add4(SSE0, SSE1, SSE2, SSE3, SSE5)
#if		MODIFYPLUGIN == 1
__asm	SSE3_MOVE	SSE6,				[esi + ebx + 1]				
		add5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE7)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx]
		add6(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 1]
		minmax3(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE7)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 2]
		minmax3sub(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
#if		ISSE > 1
__asm	SSE3_MOVE	SSE6,				[edi]
#endif
		minmax2sub(SSE1, SSE2, SSE3, SSE4, SSE7)
#if		ISSE > 1
__asm	pmaxub		SSE3,				SSE6
#else
__asm	pmaxub		SSE3,				[edi]
#endif
__asm	pminub		SSE3,				SSE2
__asm	add			esi,				eax
__asm	SSE_MOVE	[edi],				SSE3
#else	// MODIFYPLUGIN == 1
#ifndef	MODIFYPLUGIN
__asm	SSE_MOVE	[edi + 1],			SSE7
#endif
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx]				
		add5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE7)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 1]
		add6(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 2]
		minmax3sub(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE7)
#if		(ISSE > 1) || defined(SHLUR)
#ifdef		MODIFYPLUGIN
__asm	SSE3_MOVE	SSE7,				[edi]
#else
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 1]
#endif
#endif	// ISSE > 1
		minmax2sub(SSE1, SSE2, SSE3, SSE4, SSE6)
#ifdef	SHLUR
		sharpen(SSE7, SSE3, SSE2, rshift[eax], shift_mask[eax], SSE0, SSE5)
#ifdef	SHARPEN
__asm	add			esi,				spitch
#else
__asm	add			esi,				eax
#endif
__asm	SSE_MOVE	[edi],				SSE7
#else
#if		ISSE > 1
__asm	pmaxub		SSE3,				SSE7
#else
#ifdef		MODIFYPLUGIN
__asm	pmaxub		SSE3,				[edi]
#else
__asm	pmaxub		SSE3,				[esi + ebx + 1]
#endif
#endif	// ISSE > 1
__asm	pminub		SSE3,				SSE2
__asm	add			esi,				eax
__asm	SSE_MOVE	[edi],				SSE3
#endif	// SHLUR
#endif	// MODIFYPLUGIN == 1
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}


// if( weight2[i] <= weight1[i] ) { value1[i] = value2[i]; weight1[i] = weight2[i]; }
// value2 remains unchanged
// weight2 must be a SSE register, value1, value2, weight1 may very well be a memory variables
// but value1 and weight1 should be registers because they are used twice
#define	mergeweighted(value1, weight1, value2, weight2) \
__asm	pminub	weight1, weight2 \
__asm	pcmpeqb	weight2, weight1 \
__asm	psubusb	value1, weight2 \
__asm	pand	weight2, value2 \
__asm	por		value1, weight2 

#define	merge2weighted(val1, val2, weight1, val1b, val2b, weight2) \
__asm	pminub	weight1,	weight2		\
__asm	pcmpeqb	weight2,	weight1		\
__asm	psubusb	val1,		weight2		\
__asm	psubusb	val2,		weight2		\
__asm	pand	val1b,		weight2		\
__asm	pand	val2b,		weight2		\
__asm	por		val1,		val1b		\
__asm	por		val2,		val2b

#ifndef	SHLUR

#if	MODIFYPLUGIN > 0
#define		diagweight5(oldp, newp, weight, center, bound1, bound2, reg1, reg2)	\
__asm	SSE3_MOVE	newp,				bound1			\
__asm	SSE3_MOVE	reg1,				bound2			\
__asm	SSE_RMOVE	weight,				newp			\
__asm	SSE_RMOVE	reg2,				oldp			\
__asm	pmaxub		newp,				reg1			\
__asm	pminub		weight,				reg1			\
__asm	pmaxub		newp,				center			\
__asm	pminub		reg1,				center			\
__asm	psubusb		reg2,				newp			\
__asm	pminub		newp,				oldp			\
__asm	pmaxub		newp,				weight			\
__asm	psubusb		weight,				oldp			\
__asm	pmaxub		weight,				reg2
#else
// the values bound1 and bound2 are loaded into SSE registers
// then oldp is clipped with min(bound1, bound2) and max(bound1, bound2)
// finally weight = |oldp - newp|
// oldp is left unchanged
#define		diagweight5(oldp, newp, weight, center, bound1, bound2, reg1, reg2)	\
__asm	SSE3_MOVE	newp,				bound1			\
__asm	SSE3_MOVE	reg1,				bound2			\
__asm	SSE_RMOVE	weight,				newp			\
__asm	SSE_RMOVE	reg2,				oldp			\
__asm	pmaxub		newp,				reg1			\
__asm	pminub		weight,				reg1			\
__asm	psubusb		reg2,				newp			\
__asm	pminub		newp,				oldp			\
__asm	pmaxub		newp,				weight			\
__asm	psubusb		weight,				oldp			\
__asm	pmaxub		weight,				reg2
#endif

#ifdef	MODIFYPLUGIN
#define		diagweightw5(oldp, newp, weight, center, bound1, bound2, wmem, reg1, reg2)	diagweight5(oldp, newp, weight, center, bound1, bound2, reg1, reg2)
#else
// same as diagweight5, but in addition bound2 is written to wmem
#define		diagweightw5(oldp, newp, weight, center, bound1, bound2, wmem, reg1, reg2)	\
__asm	SSE3_MOVE	newp,				bound1			\
__asm	SSE3_MOVE	reg1,				bound2			\
__asm	SSE_RMOVE	weight,				newp			\
__asm	SSE_RMOVE	reg2,				oldp			\
__asm	pmaxub		newp,				reg1			\
__asm	pminub		weight,				reg1			\
__asm	psubusb		reg2,				newp			\
__asm	pminub		newp,				oldp			\
__asm	SSE_MOVE	wmem,				reg1			\
__asm	pmaxub		newp,				weight			\
__asm	psubusb		weight,				oldp			\
__asm	pmaxub		weight,				reg2
#endif	// MODIFYPLUGIN

void	diag5(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]		
		diagweight5(SSE0, SSE1, SSE2, SSE5, [esi], [esi + 2*ebx + 2], SSE6, SSE7)
		diagweight5(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx], [esi + 2], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweight5(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx + 1], [esi + 1], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweightw5(SSE0, SSE3, SSE4, SSE5, [esi + ebx + 2], [esi + ebx], [edi], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
__asm	SSE_MOVE	[edi + 1],			SSE1
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif	// MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE0,				[edi]
#if	MODIFYPLUGIN > 0
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]
#endif
#else
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]
#endif
		diagweight5(SSE0, SSE1, SSE2, SSE5, [esi], [esi + 2*ebx + 2], SSE6, SSE7)
		diagweight5(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx], [esi + 2], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweight5(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx + 1], [esi + 1], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweight5(SSE0, SSE3, SSE4, SSE5, [esi + ebx + 2], [esi + ebx], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE1
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE0,				[edi]
#if	MODIFYPLUGIN > 0
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]
#endif
#else
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]
#endif
		diagweight5(SSE0, SSE1, SSE2, SSE5, [esi], [esi + 2*ebx + 2], SSE6, SSE7)
		diagweight5(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx], [esi + 2], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweight5(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx + 1], [esi + 1], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweightw5(SSE0, SSE3, SSE4, SSE5, [esi + ebx], [esi + ebx + 2], [edi + 1], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
__asm	SSE_MOVE	[edi],				SSE1
__asm	add			esi,				eax
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}

#if	MODIFYPLUGIN > 0
#define		diagweight6(oldp, newp, weight, center, bound1, bound2, reg1, reg2)	\
__asm	SSE3_MOVE	newp,				bound1			\
__asm	SSE3_MOVE	reg1,				bound2			\
__asm	SSE_RMOVE	weight,				newp			\
__asm	SSE_RMOVE	reg2,				oldp			\
__asm	pmaxub		newp,				reg1			\
__asm	pminub		weight,				reg1			\
__asm	pmaxub		newp,				center			\
__asm	pminub		weight,				center			\
__asm	psubusb		reg2,				newp			\
__asm	SSE_RMOVE	reg1,				newp			\
__asm	pminub		newp,				oldp			\
__asm	psubusb		reg1,				weight			\
__asm	pmaxub		newp,				weight			\
__asm	psubusb		weight,				oldp			\
__asm	pmaxub		weight,				reg2			\
__asm	paddusb		weight,				weight			\
__asm	paddusb		weight,				reg1
#else
// the values bound1 and bound2 are loaded into SSE registers
// then oldp is clipped with min(bound1, bound2) and max(bound1, bound2)
// finally weight = 2*|oldp - newp| + |bound1 - bound2|
// oldp is left unchanged
#define		diagweight6(oldp, newp, weight, center, bound1, bound2, reg1, reg2)	\
__asm	SSE3_MOVE	newp,				bound1			\
__asm	SSE3_MOVE	reg1,				bound2			\
__asm	SSE_RMOVE	weight,				newp			\
__asm	SSE_RMOVE	reg2,				oldp			\
__asm	pmaxub		newp,				reg1			\
__asm	pminub		weight,				reg1			\
__asm	psubusb		reg2,				newp			\
__asm	SSE_RMOVE	reg1,				newp			\
__asm	pminub		newp,				oldp			\
__asm	psubusb		reg1,				weight			\
__asm	pmaxub		newp,				weight			\
__asm	psubusb		weight,				oldp			\
__asm	pmaxub		weight,				reg2			\
__asm	paddusb		weight,				weight			\
__asm	paddusb		weight,				reg1
#endif

#ifdef	MODIFYPLUGIN
#define		diagweightw6(oldp, newp, weight, center, bound1, bound2, wmem, reg1, reg2)	diagweight6(oldp, newp, weight, center, bound1, bound2, reg1, reg2)
#else
// same as diagweight6, but in addition bound2 is written to wmem
#define		diagweightw6(oldp, newp, weight, center, bound1, bound2, wmem, reg1, reg2)	\
__asm	SSE3_MOVE	newp,				bound1			\
__asm	SSE3_MOVE	reg1,				bound2			\
__asm	SSE_RMOVE	weight,				newp			\
__asm	SSE_RMOVE	reg2,				oldp			\
__asm	SSE_MOVE	wmem,				reg1			\
__asm	pmaxub		newp,				reg1			\
__asm	pminub		weight,				reg1			\
__asm	psubusb		reg2,				newp			\
__asm	SSE_RMOVE	reg1,				newp			\
__asm	pminub		newp,				oldp			\
__asm	psubusb		reg1,				weight			\
__asm	pmaxub		newp,				weight			\
__asm	psubusb		weight,				oldp			\
__asm	pmaxub		weight,				reg2			\
__asm	paddusb		weight,				weight			\
__asm	paddusb		weight,				reg1
#endif	// MODIFYPLUGIN

void	diag6(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]		
		diagweight6(SSE0, SSE1, SSE2, SSE5, [esi], [esi + 2*ebx + 2], SSE6, SSE7)
		diagweight6(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx], [esi + 2], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweight6(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx + 1], [esi + 1], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweightw6(SSE0, SSE3, SSE4, SSE5, [esi + ebx + 2], [esi + ebx], [edi], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
__asm	SSE_MOVE	[edi + 1],			SSE1
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif	// MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE0,				[edi]
#if	MODIFYPLUGIN > 0
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]
#endif
#else
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]
#endif
		diagweight6(SSE0, SSE1, SSE2, SSE5, [esi], [esi + 2*ebx + 2], SSE6, SSE7)
		diagweight6(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx], [esi + 2], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweight6(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx + 1], [esi + 1], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweight6(SSE0, SSE3, SSE4, SSE5, [esi + ebx + 2], [esi + ebx], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE1
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE0,				[edi]
#if	MODIFYPLUGIN > 0
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]
#endif
#else
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]
#endif
		diagweight6(SSE0, SSE1, SSE2, SSE5, [esi], [esi + 2*ebx + 2], SSE6, SSE7)
		diagweight6(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx], [esi + 2], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweight6(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx + 1], [esi + 1], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweightw6(SSE0, SSE3, SSE4, SSE5, [esi + ebx], [esi + ebx + 2], [edi + 1], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
__asm	SSE_MOVE	[edi],				SSE1
__asm	add			esi,				eax
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}

#if	MODIFYPLUGIN > 0
#define		diagweight7(oldp, newp, weight, center, bound1, bound2, reg1, reg2)	\
__asm	SSE3_MOVE	newp,				bound1			\
__asm	SSE3_MOVE	reg1,				bound2			\
__asm	SSE_RMOVE	weight,				newp			\
__asm	SSE_RMOVE	reg2,				oldp			\
__asm	pmaxub		newp,				reg1			\
__asm	pminub		weight,				reg1			\
__asm	pmaxub		newp,				center			\
__asm	pminub		weight,				center			\
__asm	psubusb		reg2,				newp			\
__asm	SSE_RMOVE	reg1,				newp			\
__asm	pminub		newp,				oldp			\
__asm	psubusb		reg1,				weight			\
__asm	pmaxub		newp,				weight			\
__asm	psubusb		weight,				oldp			\
__asm	pmaxub		weight,				reg2			\
__asm	paddusb		weight,				reg1
#else
// the values bound1 and bound2 are loaded into SSE registers
// then oldp is clipped with min(bound1, bound2) and max(bound1, bound2)
// finally weight = |oldp - newp| + |bound1 - bound2|
// oldp is left unchanged
#define		diagweight7(oldp, newp, weight, center, bound1, bound2, reg1, reg2)	\
__asm	SSE3_MOVE	newp,				bound1			\
__asm	SSE3_MOVE	reg1,				bound2			\
__asm	SSE_RMOVE	weight,				newp			\
__asm	SSE_RMOVE	reg2,				oldp			\
__asm	pmaxub		newp,				reg1			\
__asm	pminub		weight,				reg1			\
__asm	psubusb		reg2,				newp			\
__asm	SSE_RMOVE	reg1,				newp			\
__asm	pminub		newp,				oldp			\
__asm	psubusb		reg1,				weight			\
__asm	pmaxub		newp,				weight			\
__asm	psubusb		weight,				oldp			\
__asm	pmaxub		weight,				reg2			\
__asm	paddusb		weight,				reg1
#endif

#ifdef	MODIFYPLUGIN
#define		diagweightw7(oldp, newp, weight, center, bound1, bound2, wmem, reg1, reg2)	diagweight7(oldp, newp, weight, center, bound1, bound2, reg1, reg2)
#else
// same as diagweight7, but in addition bound2 is written to wmem
#define		diagweightw7(oldp, newp, weight, center, bound1, bound2, wmem, reg1, reg2)	\
__asm	SSE3_MOVE	newp,				bound1			\
__asm	SSE3_MOVE	reg1,				bound2			\
__asm	SSE_RMOVE	weight,				newp			\
__asm	SSE_RMOVE	reg2,				oldp			\
__asm	SSE_MOVE	wmem,				reg1			\
__asm	pmaxub		newp,				reg1			\
__asm	pminub		weight,				reg1			\
__asm	psubusb		reg2,				newp			\
__asm	SSE_RMOVE	reg1,				newp			\
__asm	pminub		newp,				oldp			\
__asm	psubusb		reg1,				weight			\
__asm	pmaxub		newp,				weight			\
__asm	psubusb		weight,				oldp			\
__asm	pmaxub		weight,				reg2			\
__asm	paddusb		weight,				reg1
#endif	// MODIFYPLUGIN

void	diag7(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]		
		diagweight7(SSE0, SSE1, SSE2, SSE5, [esi], [esi + 2*ebx + 2], SSE6, SSE7)
		diagweight7(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx], [esi + 2], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweight7(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx + 1], [esi + 1], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweightw7(SSE0, SSE3, SSE4, SSE5, [esi + ebx + 2], [esi + ebx], [edi], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
__asm	SSE_MOVE	[edi + 1],			SSE1
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif	// MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE0,				[edi]
#if	MODIFYPLUGIN > 0
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]
#endif
#else
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]
#endif
		diagweight7(SSE0, SSE1, SSE2, SSE5, [esi], [esi + 2*ebx + 2], SSE6, SSE7)
		diagweight7(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx], [esi + 2], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweight7(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx + 1], [esi + 1], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweight7(SSE0, SSE3, SSE4, SSE5, [esi + ebx + 2], [esi + ebx], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE1
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE0,				[edi]
#if	MODIFYPLUGIN > 0
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]
#endif
#else
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]
#endif
		diagweight7(SSE0, SSE1, SSE2, SSE5, [esi], [esi + 2*ebx + 2], SSE6, SSE7)
		diagweight7(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx], [esi + 2], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweight7(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx + 1], [esi + 1], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweightw7(SSE0, SSE3, SSE4, SSE5, [esi + ebx], [esi + ebx + 2], [edi + 1], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
__asm	SSE_MOVE	[edi],				SSE1
__asm	add			esi,				eax
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}

void	diag7b(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]		
		diagweight7(SSE0, SSE1, SSE2, SSE5, [esi], [esi + 2*ebx + 2], SSE6, SSE7)
		diagweight7(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx], [esi + 2], SSE6, SSE7)
__asm	movd		SSE6,				[esi + ebx]
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
__asm	movd		[edi],				SSE6
		diagweight7(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx + 1], [esi + 1], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
__asm	SSE_MOVE	[edi + 1],			SSE1
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif	// MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE0,				[edi]
#if	MODIFYPLUGIN > 0
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]
#endif
#else
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]
#endif
		diagweight7(SSE0, SSE1, SSE2, SSE5, [esi], [esi + 2*ebx + 2], SSE6, SSE7)
		diagweight7(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx], [esi + 2], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweight7(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx + 1], [esi + 1], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE1
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE0,				[edi]
#if	MODIFYPLUGIN > 0
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]
#endif
#else
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]
#endif
		diagweight7(SSE0, SSE1, SSE2, SSE5, [esi], [esi + 2*ebx + 2], SSE6, SSE7)
		diagweight7(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx], [esi + 2], SSE6, SSE7)
__asm	movd		SSE6,				[esi + ebx + 6]
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
__asm	movd		[edi + 5],			SSE6
		diagweight7(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx + 1], [esi + 1], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
__asm	SSE_MOVE	[edi],				SSE1
__asm	add			esi,				eax
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}

#if	MODIFYPLUGIN > 0
#define		diagweight8(oldp, newp, weight, center, bound1, bound2, reg1, reg2)	\
__asm	SSE3_MOVE	newp,				bound1			\
__asm	SSE3_MOVE	reg1,				bound2			\
__asm	SSE_RMOVE	weight,				newp			\
__asm	SSE_RMOVE	reg2,				oldp			\
__asm	pmaxub		newp,				reg1			\
__asm	pminub		weight,				reg1			\
__asm	pmaxub		newp,				center			\
__asm	pminub		weight,				center			\
__asm	psubusb		reg2,				newp			\
__asm	SSE_RMOVE	reg1,				newp			\
__asm	pminub		newp,				oldp			\
__asm	psubusb		reg1,				weight			\
__asm	pmaxub		newp,				weight			\
__asm	psubusb		weight,				oldp			\
__asm	paddusb		reg1,				reg1			\
__asm	pmaxub		weight,				reg2			\
__asm	paddusb		weight,				reg1
#else
// the values bound1 and bound2 are loaded into SSE registers
// then oldp is clipped with min(bound1, bound2) and max(bound1, bound2)
// finally weight = |oldp - newp| + 2*|bound1 - bound2|
// oldp is left unchanged
#define		diagweight8(oldp, newp, weight, center, bound1, bound2, reg1, reg2)	\
__asm	SSE3_MOVE	newp,				bound1			\
__asm	SSE3_MOVE	reg1,				bound2			\
__asm	SSE_RMOVE	weight,				newp			\
__asm	SSE_RMOVE	reg2,				oldp			\
__asm	pmaxub		newp,				reg1			\
__asm	pminub		weight,				reg1			\
__asm	psubusb		reg2,				newp			\
__asm	SSE_RMOVE	reg1,				newp			\
__asm	pminub		newp,				oldp			\
__asm	psubusb		reg1,				weight			\
__asm	pmaxub		newp,				weight			\
__asm	psubusb		weight,				oldp			\
__asm	paddusb		reg1,				reg1			\
__asm	pmaxub		weight,				reg2			\
__asm	paddusb		weight,				reg1
#endif

#ifdef	MODIFYPLUGIN
#define		diagweightw8(oldp, newp, weight, center, bound1, bound2, wmem, reg1, reg2)	diagweight8(oldp, newp, weight, center, bound1, bound2, reg1, reg2)
#else
// same as diagweight8, but in addition bound2 is written to wmem
#define		diagweightw8(oldp, newp, weight, center, bound1, bound2, wmem, reg1, reg2)	\
__asm	SSE3_MOVE	newp,				bound1			\
__asm	SSE3_MOVE	reg1,				bound2			\
__asm	SSE_RMOVE	weight,				newp			\
__asm	SSE_RMOVE	reg2,				oldp			\
__asm	SSE_MOVE	wmem,				reg1			\
__asm	pmaxub		newp,				reg1			\
__asm	pminub		weight,				reg1			\
__asm	psubusb		reg2,				newp			\
__asm	SSE_RMOVE	reg1,				newp			\
__asm	pminub		newp,				oldp			\
__asm	psubusb		reg1,				weight			\
__asm	pmaxub		newp,				weight			\
__asm	psubusb		weight,				oldp			\
__asm	paddusb		reg1,				reg1			\
__asm	pmaxub		weight,				reg2			\
__asm	paddusb		weight,				reg1
#endif	// MODIFYPLUGIN

void	diag8(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]		
		diagweight8(SSE0, SSE1, SSE2, SSE5, [esi], [esi + 2*ebx + 2], SSE6, SSE7)
		diagweight8(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx], [esi + 2], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweight8(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx + 1], [esi + 1], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweightw8(SSE0, SSE3, SSE4, SSE5, [esi + ebx + 2], [esi + ebx], [edi], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
__asm	SSE_MOVE	[edi + 1],			SSE1
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif	// MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE0,				[edi]
#if	MODIFYPLUGIN > 0
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]
#endif
#else
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]
#endif
		diagweight8(SSE0, SSE1, SSE2, SSE5, [esi], [esi + 2*ebx + 2], SSE6, SSE7)
		diagweight8(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx], [esi + 2], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweight8(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx + 1], [esi + 1], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweight8(SSE0, SSE3, SSE4, SSE5, [esi + ebx + 2], [esi + ebx], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE1
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE0,				[edi]
#if	MODIFYPLUGIN > 0
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]
#endif
#else
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]
#endif
		diagweight8(SSE0, SSE1, SSE2, SSE5, [esi], [esi + 2*ebx + 2], SSE6, SSE7)
		diagweight8(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx], [esi + 2], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweight8(SSE0, SSE3, SSE4, SSE5, [esi + 2*ebx + 1], [esi + 1], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
		diagweightw8(SSE0, SSE3, SSE4, SSE5, [esi + ebx], [esi + ebx + 2], [edi + 1], SSE6, SSE7)
		mergeweighted(SSE1, SSE2, SSE3, SSE4)
__asm	SSE_MOVE	[edi],				SSE1
__asm	add			esi,				eax
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}

#endif	// #ifndef	SHLUR

#if	MODIFYPLUGIN > 0
#define	get_min_weight(min, weight, center, mem1, mem2,reg)	\
__asm	SSE3_MOVE	min,				mem1		\
__asm	SSE3_MOVE	reg,				mem2		\
__asm	SSE_RMOVE	weight,				min			\
__asm	pminub		min,				center		\
__asm	pmaxub		weight,				center		\
__asm	pminub		min,				reg			\
__asm	pmaxub		weight,				reg			\
__asm	psubusb		weight,				min
#else
#define	get_min_weight(min, weight, center, mem1, mem2,reg)	\
__asm	SSE3_MOVE	min,				mem1		\
__asm	SSE3_MOVE	reg,				mem2		\
__asm	SSE_RMOVE	weight,				min			\
__asm	pminub		min,				reg			\
__asm	pmaxub		weight,				reg			\
__asm	psubusb		weight,				min
#endif

#ifdef	MODIFYPLUGIN
#define	get_min_weightw(min, weight, center, mem1, mem2, wmem, reg)	get_min_weight(min, weight, center, mem1, mem2,reg)
#else
#define	get_min_weightw(min, weight, center, mem1, mem2, wmem, reg)	\
__asm	SSE3_MOVE	min,				mem1		\
__asm	SSE3_MOVE	reg,				mem2		\
__asm	SSE_RMOVE	weight,				min			\
__asm	pminub		min,				reg			\
__asm	pmaxub		weight,				reg			\
__asm	SSE_MOVE	wmem,				reg			\
__asm	psubb		weight,				min
#endif	// MODIFYPLUGIN

#ifdef	SHARPEN
void	diag9(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height, int strength)
#else
void	diag9(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
#endif
{
#ifdef	SHARPEN
__asm	mov			ebx,				strength
__asm	SSE_RMOVE	SSE4,				rshift[ebx]
__asm	SSE_RMOVE	SSE6,				shift_mask[ebx]
#endif
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
__asm	align		16
__asm	column_loop:
		get_min_weight(SSE0, SSE1, SSE5, [esi], [esi + 2*ebx + 2], SSE7)
		get_min_weight(SSE2, SSE3, SSE5, [esi + 2*ebx], [esi + 2], SSE7)
		mergeweighted(SSE0, SSE1, SSE2, SSE3)
		get_min_weight(SSE2, SSE3, SSE5, [esi + 2*ebx + 1], [esi + 1], SSE7)
		mergeweighted(SSE0, SSE1, SSE2, SSE3)
		get_min_weightw(SSE2, SSE3, SSE5, [esi + ebx + 2], [esi + ebx], [edi], SSE7)
		mergeweighted(SSE0, SSE1, SSE2, SSE3)
#if		(ISSE > 1) || defined(SHLUR)
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 1]
#endif
__asm	paddusb		SSE1,				SSE0
#ifdef	SHLUR
		sharpen(SSE7, SSE0, SSE1, SSE4, SSE6, SSE2, SSE3)
__asm	SSE_MOVE	[edi + 1],			SSE7
#else
#if		ISSE > 1
__asm	pmaxub		SSE0,				SSE7			
#else
__asm	pmaxub		SSE0,				[esi + ebx + 1]
#endif
__asm	pminub		SSE0,				SSE1
__asm	SSE_MOVE	[edi + 1],			SSE0
#endif	// SHLUR
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif	// MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
#if	MODIFYPLUGIN > 0
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]
#endif
		get_min_weight(SSE0, SSE1, SSE5,[esi], [esi + 2*ebx + 2], SSE7)
		get_min_weight(SSE2, SSE3, SSE5, [esi + 2*ebx], [esi + 2], SSE7)
		mergeweighted(SSE0, SSE1, SSE2, SSE3)
		get_min_weight(SSE2, SSE3, SSE5, [esi + 2*ebx + 1], [esi + 1], SSE7)
		mergeweighted(SSE0, SSE1, SSE2, SSE3)
		get_min_weight(SSE2, SSE3, SSE5, [esi + ebx + 2], [esi + ebx], SSE7)
		mergeweighted(SSE0, SSE1, SSE2, SSE3)
#if		(ISSE > 1) || defined(SHLUR)
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE7,				[edi]
#else
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 1]
#endif
#endif	// (ISSE > 1) || defined(SHLUR)
__asm	paddusb		SSE1,				SSE0
#ifdef	SHLUR
		sharpen(SSE7, SSE0, SSE1, SSE4, SSE6, SSE2, SSE3)
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE7
#else
#if		ISSE > 1
__asm	pmaxub		SSE0,				SSE7			
#else
#ifdef	MODIFYPLUGIN
__asm	pmaxub		SSE0,				[edi]
#else
__asm	pmaxub		SSE0,				[esi + ebx + 1]
#endif
#endif	// ISSE > 1
__asm	pminub		SSE0,				SSE1
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE0
#endif	// SHLUR
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
#if	MODIFYPLUGIN > 0
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]
#endif
		get_min_weight(SSE0, SSE1, SSE5, [esi], [esi + 2*ebx + 2], SSE7)
		get_min_weight(SSE2, SSE3, SSE5, [esi + 2*ebx], [esi + 2], SSE7)
		mergeweighted(SSE0, SSE1, SSE2, SSE3)
		get_min_weight(SSE2, SSE3, SSE5, [esi + 2*ebx + 1], [esi + 1], SSE7)
		mergeweighted(SSE0, SSE1, SSE2, SSE3)
		get_min_weightw(SSE2, SSE3, SSE5, [esi + ebx], [esi + ebx + 2], [edi + 1], SSE7)
		mergeweighted(SSE0, SSE1, SSE2, SSE3)
#if		(ISSE > 1) || defined(SHLUR)
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE7,				[edi]
#else
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 1]
#endif
#endif	// (ISSE > 1) || defined(SHLUR)
__asm	paddusb		SSE1,				SSE0
#ifdef	SHLUR
		sharpen(SSE7, SSE0, SSE1, SSE4, SSE6, SSE2, SSE3)
__asm	add			esi,				eax
__asm	SSE_MOVE	[edi],				SSE7
#else
#if		ISSE > 1
__asm	pmaxub		SSE0,				SSE7			
#else
#ifdef	MODIFYPLUGIN
__asm	pmaxub		SSE0,				[edi]
#else
__asm	pmaxub		SSE0,				[esi + ebx + 1]
#endif
#endif	// ISSE > 1
__asm	pminub		SSE0,				SSE1
__asm	add			esi,				eax
__asm	SSE_MOVE	[edi],				SSE0
#endif	// SHLUR
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}

#define	get_val_weight(val, weight, mem, center, reg)		\
__asm	SSE3_MOVE	val,				mem			\
__asm	SSE_RMOVE	weight,				center		\
__asm	SSE_RMOVE	reg,				center		\
__asm	pmaxub		weight,				val			\
__asm	pminub		reg,				val			\
__asm	psubusb		weight,				reg

#ifdef	MODIFYPLUGIN
#define	get_val_weightw(val, weight, mem, center, wmem, reg)	get_val_weight(val, weight, mem, center, reg)
#else
#define	get_val_weightw1(val, weight, mem, center, wmem, reg)	\
__asm	SSE3_MOVE	val,				mem			\
__asm	SSE_RMOVE	weight,				center		\
__asm	SSE_RMOVE	reg,				center		\
__asm	pmaxub		weight,				val			\
__asm	pminub		reg,				val			\
__asm	movd		wmem,				val			\
__asm	psubusb		weight,				reg

#define	get_val_weightw(val, weight, mem, center, wmem, reg)	\
__asm	SSE3_MOVE	val,				mem			\
__asm	SSE_RMOVE	weight,				center		\
__asm	SSE_RMOVE	reg,				center		\
__asm	pmaxub		weight,				val			\
__asm	pminub		reg,				val			\
__asm	SSE_MOVE	wmem,				val			\
__asm	psubusb		weight,				reg
#endif	// MODIFYPLUGIN

void	SSE_RemoveGrain10(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE1,				[esi + ebx + 1]
		get_val_weightw1(SSE2, SSE3, [esi + ebx], SSE1, [edi], SSE7)
		get_val_weight(SSE4, SSE5, [esi + ebx + 2], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
		get_val_weight(SSE4, SSE5, [esi], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
		get_val_weight(SSE4, SSE5, [esi + 2], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
		get_val_weight(SSE4, SSE5, [esi + 1], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
		get_val_weight(SSE4, SSE5, [esi + 2*ebx], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
		get_val_weight(SSE4, SSE5, [esi + 2*ebx + 2], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
		get_val_weight(SSE4, SSE5, [esi + 2*ebx + 1], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
__asm	SSE_MOVE	SSE4,				SSE2
__asm	pminub		SSE1,				SSE2
__asm	pmaxub		SSE1,				SSE4
__asm	SSE_MOVE	[edi + 1],			SSE1
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif // MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE1,				[edi]
#else
__asm	SSE3_MOVE	SSE1,				[esi + ebx + 1]
#endif
		get_val_weight(SSE2, SSE3, [esi + ebx], SSE1, SSE7)
#if	MODIFYPLUGIN == 1
		get_val_weight(SSE4, SSE5, [esi + ebx + 1], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
#endif
		get_val_weightw(SSE4, SSE5, [esi + ebx + 2], SSE1, [edi + 1], SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
		get_val_weight(SSE4, SSE5, [esi], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
		get_val_weight(SSE4, SSE5, [esi + 2], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
		get_val_weight(SSE4, SSE5, [esi + 1], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
		get_val_weight(SSE4, SSE5, [esi + 2*ebx], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
		get_val_weight(SSE4, SSE5, [esi + 2*ebx + 2], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
		get_val_weight(SSE4, SSE5, [esi + 2*ebx + 1], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
__asm	SSE_MOVE	SSE4,				SSE2
__asm	pminub		SSE1,				SSE2
__asm	add			esi,				SSE_INCREMENT
__asm	pmaxub		SSE1,				SSE4
__asm	SSE_MOVE	[edi],				SSE1
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE1,				[edi]
#else
__asm	SSE3_MOVE	SSE1,				[esi + ebx + 1]
#endif
		get_val_weight(SSE2, SSE3, [esi + ebx], SSE1, SSE7)
#if	MODIFYPLUGIN == 1
		get_val_weight(SSE4, SSE5, [esi + ebx + 1], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
#endif
		get_val_weight(SSE4, SSE5, [esi + ebx + 2], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
		get_val_weight(SSE4, SSE5, [esi], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
		get_val_weight(SSE4, SSE5, [esi + 2], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
		get_val_weight(SSE4, SSE5, [esi + 1], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
		get_val_weight(SSE4, SSE5, [esi + 2*ebx], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
		get_val_weight(SSE4, SSE5, [esi + 2*ebx + 2], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
		get_val_weight(SSE4, SSE5, [esi + 2*ebx + 1], SSE1, SSE7)
		mergeweighted(SSE2, SSE3, SSE4, SSE5)
__asm	SSE_MOVE	SSE4,				SSE2
__asm	pminub		SSE1,				SSE2
__asm	pmaxub		SSE1,				SSE4
__asm	add			esi,				eax
__asm	SSE_MOVE	[edi],				SSE1
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}

#if	!(defined(MODIFYPLUGIN) || defined(SHLUR))
#define neighbourdiff(minus, plus, center1, center2, neighbour, nullreg)	\
__asm	SSE_RMOVE	center1,			center2		\
__asm	psubusb		center2,			neighbour	\
__asm	psubusb		neighbour,			center1		\
__asm	SSE_RMOVE	minus,				center2		\
__asm	SSE_RMOVE	plus,				neighbour	\
__asm	pcmpeqb		center2,			nullreg		\
__asm	pcmpeqb		neighbour,			nullreg		\
__asm	por			minus,				center2		\
__asm	pand		center2,			neighbour	\
__asm	por			plus,				neighbour	\
__asm	psubusb		minus,				center2		\
__asm	psubusb		plus,				center2

#define neighbourdiff_w(minus, plus, center1, center2, dest, neighbour, nullreg, mwrite)	\
__asm	SSE_RMOVE	center1,			center2		\
__asm	mwrite		dest,				neighbour	\
__asm	psubusb		center2,			neighbour	\
__asm	psubusb		neighbour,			center1		\
__asm	SSE_RMOVE	minus,				center2		\
__asm	SSE_RMOVE	plus,				neighbour	\
__asm	pcmpeqb		center2,			nullreg		\
__asm	pcmpeqb		neighbour,			nullreg		\
__asm	por			minus,				center2		\
__asm	pand		center2,			neighbour	\
__asm	por			plus,				neighbour	\
__asm	psubusb		minus,				center2		\
__asm	psubusb		plus,				center2

#define	SHIFT_MASK1	127

static	const __declspec(align(SSE_INCREMENT)) BYTE	shift_mask[SSE_INCREMENT] =
	{
		SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1		
#if	SSE_INCREMENT == 16
		, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1, SHIFT_MASK1
#endif
	};

#define	sharpen(center, minus, plus, reg1, reg2)\
__asm	SSE_RMOVE	reg1,				minus			\
__asm	SSE_RMOVE	reg2,				plus			\
__asm	psubusb		reg1,				plus			\
__asm	psubusb		reg2,				minus			\
__asm	psrlw		plus,				1				\
__asm	psrlw		minus,				1				\
__asm	pand		plus,				shift_mask		\
__asm	pand		minus,				shift_mask		\
__asm	pminub		plus,				reg1			\
__asm	pminub		minus,				reg2			\
__asm	paddusb		center,				plus			\
__asm	psubusb		center,				minus

void	nondestructivesharpen(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
__asm	mov			edx,				remainder
__asm	pxor		SSE0,				SSE0
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
__asm	lea			eax,				[ebx + eax + 1]	
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE1,				[esi + ebx + 1]
__asm	SSE3_MOVE	SSE3,				[esi + ebx]	
		neighbourdiff_w(SSE4, SSE5, SSE2, SSE1, [edi], SSE3, SSE0, movd)

__asm	SSE3_MOVE	SSE3,				[esi + ebx + 2]	
		neighbourdiff(SSE6, SSE7, SSE1, SSE2, SSE3, SSE0)
__asm	pminub		SSE4,				SSE6
__asm	pminub		SSE5,				SSE7

__asm	SSE3_MOVE	SSE3,				[esi]	
		neighbourdiff(SSE6, SSE7, SSE2, SSE1, SSE3, SSE0)
__asm	pminub		SSE4,				SSE6
__asm	pminub		SSE5,				SSE7

__asm	SSE3_MOVE	SSE3,				[esi + 1]	
		neighbourdiff(SSE6, SSE7, SSE1, SSE2, SSE3, SSE0)
__asm	pminub		SSE4,				SSE6
__asm	pminub		SSE5,				SSE7

__asm	SSE3_MOVE	SSE3,				[esi + 2]	
		neighbourdiff(SSE6, SSE7, SSE2, SSE1, SSE3, SSE0)
__asm	pminub		SSE4,				SSE6
__asm	pminub		SSE5,				SSE7

__asm	SSE3_MOVE	SSE3,				[esi + 2*ebx]	
		neighbourdiff(SSE6, SSE7, SSE1, SSE2, SSE3, SSE0)
__asm	pminub		SSE4,				SSE6
__asm	pminub		SSE5,				SSE7

__asm	SSE3_MOVE	SSE3,				[esi + 2*ebx + 1]	
		neighbourdiff(SSE6, SSE7, SSE2, SSE1, SSE3, SSE0)
__asm	pminub		SSE4,				SSE6
__asm	pminub		SSE5,				SSE7

__asm	SSE3_MOVE	SSE3,				[esi + 2*ebx + 2]	
		neighbourdiff(SSE6, SSE7, SSE1, SSE2, SSE3, SSE0)
__asm	pminub		SSE4,				SSE6
__asm	pminub		SSE5,				SSE7
		sharpen(SSE1, SSE4, SSE5, SSE6, SSE7)
__asm	SSE_MOVE	[edi + 1],			SSE1
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
__asm	align		16
__asm	middle_loop:
__asm	SSE3_MOVE	SSE1,				[esi + ebx + 1]
__asm	SSE3_MOVE	SSE3,				[esi + ebx]	
		neighbourdiff(SSE4, SSE5, SSE2, SSE1, SSE3, SSE0)

__asm	SSE3_MOVE	SSE3,				[esi + ebx + 2]	
		neighbourdiff(SSE6, SSE7, SSE1, SSE2, SSE3, SSE0)
__asm	pminub		SSE4,				SSE6
__asm	pminub		SSE5,				SSE7

__asm	SSE3_MOVE	SSE3,				[esi]	
		neighbourdiff(SSE6, SSE7, SSE2, SSE1, SSE3, SSE0)
__asm	pminub		SSE4,				SSE6
__asm	pminub		SSE5,				SSE7

__asm	SSE3_MOVE	SSE3,				[esi + 1]	
		neighbourdiff(SSE6, SSE7, SSE1, SSE2, SSE3, SSE0)
__asm	pminub		SSE4,				SSE6
__asm	pminub		SSE5,				SSE7

__asm	SSE3_MOVE	SSE3,				[esi + 2]	
		neighbourdiff(SSE6, SSE7, SSE2, SSE1, SSE3, SSE0)
__asm	pminub		SSE4,				SSE6
__asm	pminub		SSE5,				SSE7

__asm	SSE3_MOVE	SSE3,				[esi + 2*ebx]	
		neighbourdiff(SSE6, SSE7, SSE1, SSE2, SSE3, SSE0)
__asm	pminub		SSE4,				SSE6
__asm	pminub		SSE5,				SSE7

__asm	SSE3_MOVE	SSE3,				[esi + 2*ebx + 1]	
		neighbourdiff(SSE6, SSE7, SSE2, SSE1, SSE3, SSE0)
__asm	pminub		SSE4,				SSE6
__asm	pminub		SSE5,				SSE7

__asm	SSE3_MOVE	SSE3,				[esi + 2*ebx + 2]	
		neighbourdiff(SSE6, SSE7, SSE1, SSE2, SSE3, SSE0)
__asm	pminub		SSE4,				SSE6
__asm	pminub		SSE5,				SSE7
__asm	add			esi,				SSE_INCREMENT
		sharpen(SSE1, SSE4, SSE5, SSE6, SSE7)
__asm	SSE_MOVE	[edi],				SSE1
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
__asm	SSE3_MOVE	SSE1,				[esi + ebx + 1]
__asm	SSE3_MOVE	SSE3,				[esi + ebx]	
		neighbourdiff(SSE4, SSE5, SSE2, SSE1, SSE3, SSE0)

__asm	SSE3_MOVE	SSE3,				[esi + ebx + 2]	
		neighbourdiff_w(SSE6, SSE7, SSE1, SSE2, [edi + 1], SSE3, SSE0, SSE_MOVE)
__asm	pminub		SSE4,				SSE6
__asm	pminub		SSE5,				SSE7

__asm	SSE3_MOVE	SSE3,				[esi]	
		neighbourdiff(SSE6, SSE7, SSE2, SSE1, SSE3, SSE0)
__asm	pminub		SSE4,				SSE6
__asm	pminub		SSE5,				SSE7

__asm	SSE3_MOVE	SSE3,				[esi + 1]	
		neighbourdiff(SSE6, SSE7, SSE1, SSE2, SSE3, SSE0)
__asm	pminub		SSE4,				SSE6
__asm	pminub		SSE5,				SSE7

__asm	SSE3_MOVE	SSE3,				[esi + 2]	
		neighbourdiff(SSE6, SSE7, SSE2, SSE1, SSE3, SSE0)
__asm	pminub		SSE4,				SSE6
__asm	pminub		SSE5,				SSE7

__asm	SSE3_MOVE	SSE3,				[esi + 2*ebx]	
		neighbourdiff(SSE6, SSE7, SSE1, SSE2, SSE3, SSE0)
__asm	pminub		SSE4,				SSE6
__asm	pminub		SSE5,				SSE7

__asm	SSE3_MOVE	SSE3,				[esi + 2*ebx + 1]	
		neighbourdiff(SSE6, SSE7, SSE2, SSE1, SSE3, SSE0)
__asm	pminub		SSE4,				SSE6
__asm	pminub		SSE5,				SSE7

__asm	SSE3_MOVE	SSE3,				[esi + 2*ebx + 2]	
		neighbourdiff(SSE6, SSE7, SSE1, SSE2, SSE3, SSE0)
__asm	pminub		SSE4,				SSE6
__asm	pminub		SSE5,				SSE7
__asm	add			esi,				eax
		sharpen(SSE1, SSE4, SSE5, SSE6, SSE7)
__asm	SSE_MOVE	[edi],				SSE1
__asm	add			edi,				dpitch
__asm	dec			height
__asm	jnz			column_loop
}
#endif	!(defined(MODIFYPLUGIN) || defined(SHLUR))

#ifndef	MODIFYPLUGIN

#define	convolution(daddr, saddr, spitch, nullreg, bias_correction, reg0, reg1, reg2, reg3, reg4, reg5)	\
__asm	SSE3_MOVE	reg0,				[saddr + spitch + 1]	\
__asm	SSE_MOVE	reg1,				reg0					\
__asm	punpcklbw	reg0,				nullreg					\
__asm	punpckhbw	reg1,				nullreg					\
__asm	SSE3_MOVE	reg2,				[saddr + spitch]		\
__asm	paddusw		reg0,				reg0					\
__asm	SSE_MOVE	reg3,				reg2					\
__asm	paddusw		reg1,				reg1					\
__asm	punpcklbw	reg2,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	SSE3_MOVE	reg4,				[saddr + spitch + 2]	\
__asm	paddusw		reg0,				reg2					\
__asm	SSE_MOVE	reg5,				reg4					\
__asm	paddusw		reg1,				reg3					\
__asm	punpcklbw	reg4,				nullreg					\
__asm	punpckhbw	reg5,				nullreg					\
__asm	SSE3_MOVE	reg2,				[saddr + 1]				\
__asm	paddusw		reg0,				reg4					\
__asm	SSE_MOVE	reg3,				reg2					\
__asm	paddusw		reg1,				reg5					\
__asm	punpcklbw	reg2,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	SSE3_MOVE	reg4,				[saddr + 2*spitch + 1]	\
__asm	paddusw		reg0,				reg2					\
__asm	SSE_MOVE	reg5,				reg4					\
__asm	paddusw		reg1,				reg3					\
__asm	punpcklbw	reg4,				nullreg					\
__asm	punpckhbw	reg5,				nullreg					\
__asm	SSE3_MOVE	reg2,				[saddr]					\
__asm	paddusw		reg0,				reg4					\
__asm	SSE_MOVE	reg3,				reg2					\
__asm	paddusw		reg1,				reg5					\
__asm	punpcklbw	reg2,				nullreg					\
__asm	paddusw		reg0,				reg0					\
__asm	paddusw		reg1,				reg1					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	SSE3_MOVE	reg4,				[saddr + 2]				\
__asm	paddusw		reg0,				reg2					\
__asm	SSE_MOVE	reg5,				reg4					\
__asm	paddusw		reg1,				reg3					\
__asm	punpcklbw	reg4,				nullreg					\
__asm	punpckhbw	reg5,				nullreg					\
__asm	SSE3_MOVE	reg2,				[saddr + 2*spitch]		\
__asm	paddusw		reg0,				reg4					\
__asm	SSE_MOVE	reg3,				reg2					\
__asm	paddusw		reg1,				reg5					\
__asm	punpcklbw	reg2,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	SSE3_MOVE	reg4,				[saddr + 2*spitch + 2]	\
__asm	paddusw		reg0,				reg2					\
__asm	SSE_MOVE	reg5,				reg4					\
__asm	paddusw		reg1,				reg3					\
__asm	punpcklbw	reg4,				nullreg					\
__asm	punpckhbw	reg5,				nullreg					\
__asm	paddusw		reg0,				reg4					\
__asm	paddusw		reg1,				reg5					\
__asm	paddusw		reg0,				bias_correction			\
__asm	paddusw		reg1,				bias_correction			\
__asm	psraw		reg0,				4						\
__asm	psraw		reg1,				4						\
__asm	packuswb	reg0,				reg1					\
__asm	SSE_MOVE	[daddr],			reg0

#define	convolution_w1(daddr, saddr, spitch, nullreg, bias_correction, reg0, reg1, reg2, reg3, reg4, reg5)	\
__asm	SSE3_MOVE	reg0,				[saddr + spitch + 1]	\
__asm	SSE_MOVE	reg1,				reg0					\
__asm	punpcklbw	reg0,				nullreg					\
__asm	punpckhbw	reg1,				nullreg					\
__asm	SSE3_MOVE	reg2,				[saddr + spitch]		\
__asm	paddusw		reg0,				reg0					\
__asm	SSE_MOVE	reg3,				reg2					\
__asm	movd		[daddr],			reg2					\
__asm	paddusw		reg1,				reg1					\
__asm	punpcklbw	reg2,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	SSE3_MOVE	reg4,				[saddr + spitch + 2]	\
__asm	paddusw		reg0,				reg2					\
__asm	SSE_MOVE	reg5,				reg4					\
__asm	paddusw		reg1,				reg3					\
__asm	punpcklbw	reg4,				nullreg					\
__asm	punpckhbw	reg5,				nullreg					\
__asm	SSE3_MOVE	reg2,				[saddr + 1]				\
__asm	paddusw		reg0,				reg4					\
__asm	SSE_MOVE	reg3,				reg2					\
__asm	paddusw		reg1,				reg5					\
__asm	punpcklbw	reg2,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	SSE3_MOVE	reg4,				[saddr + 2*spitch + 1]	\
__asm	paddusw		reg0,				reg2					\
__asm	SSE_MOVE	reg5,				reg4					\
__asm	paddusw		reg1,				reg3					\
__asm	punpcklbw	reg4,				nullreg					\
__asm	punpckhbw	reg5,				nullreg					\
__asm	SSE3_MOVE	reg2,				[saddr]					\
__asm	paddusw		reg0,				reg4					\
__asm	SSE_MOVE	reg3,				reg2					\
__asm	paddusw		reg1,				reg5					\
__asm	punpcklbw	reg2,				nullreg					\
__asm	paddusw		reg0,				reg0					\
__asm	paddusw		reg1,				reg1					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	SSE3_MOVE	reg4,				[saddr + 2]				\
__asm	paddusw		reg0,				reg2					\
__asm	SSE_MOVE	reg5,				reg4					\
__asm	paddusw		reg1,				reg3					\
__asm	punpcklbw	reg4,				nullreg					\
__asm	punpckhbw	reg5,				nullreg					\
__asm	SSE3_MOVE	reg2,				[saddr + 2*spitch]		\
__asm	paddusw		reg0,				reg4					\
__asm	SSE_MOVE	reg3,				reg2					\
__asm	paddusw		reg1,				reg5					\
__asm	punpcklbw	reg2,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	SSE3_MOVE	reg4,				[saddr + 2*spitch + 2]	\
__asm	paddusw		reg0,				reg2					\
__asm	SSE_MOVE	reg5,				reg4					\
__asm	paddusw		reg1,				reg3					\
__asm	punpcklbw	reg4,				nullreg					\
__asm	punpckhbw	reg5,				nullreg					\
__asm	paddusw		reg0,				reg4					\
__asm	paddusw		reg1,				reg5					\
__asm	paddusw		reg0,				bias_correction			\
__asm	paddusw		reg1,				bias_correction			\
__asm	psraw		reg0,				4						\
__asm	psraw		reg1,				4						\
__asm	packuswb	reg0,				reg1					\
__asm	SSE_MOVE	[daddr + 1],		reg0


#define	convolution_w2(daddr, saddr, spitch, nullreg, bias_correction, reg0, reg1, reg2, reg3, reg4, reg5)	\
__asm	SSE3_MOVE	reg0,				[saddr + spitch + 1]	\
__asm	SSE_MOVE	reg1,				reg0					\
__asm	punpcklbw	reg0,				nullreg					\
__asm	punpckhbw	reg1,				nullreg					\
__asm	SSE3_MOVE	reg2,				[saddr + spitch]		\
__asm	paddusw		reg0,				reg0					\
__asm	SSE_MOVE	reg3,				reg2					\
__asm	paddusw		reg1,				reg1					\
__asm	punpcklbw	reg2,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	SSE3_MOVE	reg4,				[saddr + spitch + 2]	\
__asm	paddusw		reg0,				reg2					\
__asm	SSE_MOVE	reg5,				reg4					\
__asm	SSE_MOVE	[daddr + 1],		reg4					\
__asm	paddusw		reg1,				reg3					\
__asm	punpcklbw	reg4,				nullreg					\
__asm	punpckhbw	reg5,				nullreg					\
__asm	SSE3_MOVE	reg2,				[saddr + 1]				\
__asm	paddusw		reg0,				reg4					\
__asm	SSE_MOVE	reg3,				reg2					\
__asm	paddusw		reg1,				reg5					\
__asm	punpcklbw	reg2,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	SSE3_MOVE	reg4,				[saddr + 2*spitch + 1]	\
__asm	paddusw		reg0,				reg2					\
__asm	SSE_MOVE	reg5,				reg4					\
__asm	paddusw		reg1,				reg3					\
__asm	punpcklbw	reg4,				nullreg					\
__asm	punpckhbw	reg5,				nullreg					\
__asm	SSE3_MOVE	reg2,				[saddr]					\
__asm	paddusw		reg0,				reg4					\
__asm	SSE_MOVE	reg3,				reg2					\
__asm	paddusw		reg1,				reg5					\
__asm	punpcklbw	reg2,				nullreg					\
__asm	paddusw		reg0,				reg0					\
__asm	paddusw		reg1,				reg1					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	SSE3_MOVE	reg4,				[saddr + 2]				\
__asm	paddusw		reg0,				reg2					\
__asm	SSE_MOVE	reg5,				reg4					\
__asm	paddusw		reg1,				reg3					\
__asm	punpcklbw	reg4,				nullreg					\
__asm	punpckhbw	reg5,				nullreg					\
__asm	SSE3_MOVE	reg2,				[saddr + 2*spitch]		\
__asm	paddusw		reg0,				reg4					\
__asm	SSE_MOVE	reg3,				reg2					\
__asm	paddusw		reg1,				reg5					\
__asm	punpcklbw	reg2,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	SSE3_MOVE	reg4,				[saddr + 2*spitch + 2]	\
__asm	paddusw		reg0,				reg2					\
__asm	SSE_MOVE	reg5,				reg4					\
__asm	paddusw		reg1,				reg3					\
__asm	punpcklbw	reg4,				nullreg					\
__asm	punpckhbw	reg5,				nullreg					\
__asm	paddusw		reg0,				reg4					\
__asm	paddusw		reg1,				reg5					\
__asm	paddusw		reg0,				bias_correction			\
__asm	paddusw		reg1,				bias_correction			\
__asm	psraw		reg0,				4						\
__asm	psraw		reg1,				4						\
__asm	packuswb	reg0,				reg1					\
__asm	SSE_MOVE	[daddr],			reg0

static	const __declspec(align(SSE_INCREMENT)) unsigned short	convolution_bias[SSE_INCREMENT/2] =
	{
		8,8,8,8
#if	SSE_INCREMENT == 16
		,8,8,8,8
#endif
	};

void	SSE_RemoveGrain11(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
#ifdef	CVERSION
		_sp -= spitch;
		int width = (hblocks + 2) * SSE_INCREMENT + remainder;
		int	spitch2 = spitch - width;
		dpitch -= width;
		do
		{
			int	w = width;
			dp[0] = _sp[spitch];
			do
			{
				*++dp = (2*(_sp[spitch] + 2 * _sp[spitch + 1] + _sp[spitch + 2] + _sp[1] + _sp[2 * spitch + 1])
							+ _sp[0] + _sp[2] + _sp[2 * spitch] + _sp[2 * spitch  + 2] + 8) / 16;
				++_sp;
			} while( --w );
			dp[1] = _sp[spitch + 1];
			dp += dpitch;
			_sp += spitch2; 
		} while( --height );
#else
__asm	SSE_RMOVE	SSE7,				convolution_bias
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
__asm	pxor		SSE6,				SSE6
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
__asm	align		16
__asm	column_loop:
		convolution_w1(edi, esi, ebx, SSE6, SSE7, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5)
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif	// MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
		convolution(edi, esi, ebx, SSE6, SSE7, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5)
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
		convolution_w2(edi, esi, ebx, SSE6, SSE7, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5)
__asm	add			esi,				eax
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
#endif
}

#define	flatconvolution(daddr, saddr, spitch, nullreg, onenineth, reg0, reg1, reg2, reg3, reg4, reg5)	\
__asm	SSE3_MOVE	reg0,				[saddr]					\
__asm	SSE3_MOVE	reg1,				[saddr + 1]				\
__asm	SSE_RMOVE	reg2,				reg0					\
__asm	SSE_RMOVE	reg3,				reg1					\
__asm	punpcklbw	reg0,				nullreg					\
__asm	punpcklbw	reg1,				nullreg					\
__asm	punpckhbw	reg2,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	paddusw		reg0,				reg1					\
__asm	paddusw		reg2,				reg3					\
__asm	SSE3_MOVE	reg4,				[saddr + 2]				\
__asm	SSE3_MOVE	reg1,				[saddr + ebx]			\
__asm	SSE_RMOVE	reg5,				reg4					\
__asm	SSE_RMOVE	reg3,				reg1					\
__asm	punpcklbw	reg4,				nullreg					\
__asm	punpcklbw	reg1,				nullreg					\
__asm	punpckhbw	reg5,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	paddusw		reg0,				reg4					\
__asm	paddusw		reg2,				reg5					\
__asm	paddusw		reg0,				reg1					\
__asm	paddusw		reg2,				reg3					\
__asm	SSE3_MOVE	reg4,				[saddr + ebx + 1]		\
__asm	SSE3_MOVE	reg1,				[saddr + ebx + 2]		\
__asm	SSE_RMOVE	reg5,				reg4					\
__asm	SSE_RMOVE	reg3,				reg1					\
__asm	punpcklbw	reg4,				nullreg					\
__asm	punpcklbw	reg1,				nullreg					\
__asm	punpckhbw	reg5,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	paddusw		reg0,				reg4					\
__asm	paddusw		reg2,				reg5					\
__asm	paddusw		reg0,				reg1					\
__asm	paddusw		reg2,				reg3					\
__asm	SSE3_MOVE	reg4,				[saddr + 2*ebx]			\
__asm	SSE3_MOVE	reg1,				[saddr + 2*ebx + 1]		\
__asm	SSE_RMOVE	reg5,				reg4					\
__asm	SSE_RMOVE	reg3,				reg1					\
__asm	punpcklbw	reg4,				nullreg					\
__asm	punpcklbw	reg1,				nullreg					\
__asm	punpckhbw	reg5,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	paddusw		reg0,				reg4					\
__asm	paddusw		reg2,				reg5					\
__asm	paddusw		reg0,				reg1					\
__asm	paddusw		reg2,				reg3					\
__asm	SSE3_MOVE	reg1,				[saddr + 2*ebx + 2]		\
__asm	SSE_RMOVE	reg3,				reg1					\
__asm	punpcklbw	reg1,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	paddusw		reg0,				reg1					\
__asm	paddusw		reg2,				reg3					\
__asm	paddusw		reg0,				flatconvolution_bias	\
__asm	paddusw		reg2,				flatconvolution_bias	\
__asm	pmulhuw		reg0,				onenineth				\
__asm	pmulhuw		reg2,				onenineth				\
__asm	packuswb	reg0,				reg2					\
__asm	SSE_MOVE	[daddr],			reg0

#define	flatconvolution_w1(daddr, saddr, spitch, nullreg, onenineth, reg0, reg1, reg2, reg3, reg4, reg5)	\
__asm	SSE3_MOVE	reg0,				[saddr]					\
__asm	SSE3_MOVE	reg1,				[saddr + 1]				\
__asm	SSE_RMOVE	reg2,				reg0					\
__asm	SSE_RMOVE	reg3,				reg1					\
__asm	punpcklbw	reg0,				nullreg					\
__asm	punpcklbw	reg1,				nullreg					\
__asm	punpckhbw	reg2,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	paddusw		reg0,				reg1					\
__asm	paddusw		reg2,				reg3					\
__asm	SSE3_MOVE	reg4,				[saddr + 2]				\
__asm	SSE3_MOVE	reg1,				[saddr + ebx]			\
__asm	SSE_RMOVE	reg5,				reg4					\
__asm	movd		[daddr],			reg1					\
__asm	SSE_RMOVE	reg3,				reg1					\
__asm	punpcklbw	reg4,				nullreg					\
__asm	punpcklbw	reg1,				nullreg					\
__asm	punpckhbw	reg5,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	paddusw		reg0,				reg4					\
__asm	paddusw		reg2,				reg5					\
__asm	paddusw		reg0,				reg1					\
__asm	paddusw		reg2,				reg3					\
__asm	SSE3_MOVE	reg4,				[saddr + ebx + 1]		\
__asm	SSE3_MOVE	reg1,				[saddr + ebx + 2]		\
__asm	SSE_RMOVE	reg5,				reg4					\
__asm	SSE_RMOVE	reg3,				reg1					\
__asm	punpcklbw	reg4,				nullreg					\
__asm	punpcklbw	reg1,				nullreg					\
__asm	punpckhbw	reg5,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	paddusw		reg0,				reg4					\
__asm	paddusw		reg2,				reg5					\
__asm	paddusw		reg0,				reg1					\
__asm	paddusw		reg2,				reg3					\
__asm	SSE3_MOVE	reg4,				[saddr + 2*ebx]			\
__asm	SSE3_MOVE	reg1,				[saddr + 2*ebx + 1]		\
__asm	SSE_RMOVE	reg5,				reg4					\
__asm	SSE_RMOVE	reg3,				reg1					\
__asm	punpcklbw	reg4,				nullreg					\
__asm	punpcklbw	reg1,				nullreg					\
__asm	punpckhbw	reg5,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	paddusw		reg0,				reg4					\
__asm	paddusw		reg2,				reg5					\
__asm	paddusw		reg0,				reg1					\
__asm	paddusw		reg2,				reg3					\
__asm	SSE3_MOVE	reg1,				[saddr + 2*ebx + 2]		\
__asm	SSE_RMOVE	reg3,				reg1					\
__asm	punpcklbw	reg1,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	paddusw		reg0,				reg1					\
__asm	paddusw		reg2,				reg3					\
__asm	paddusw		reg0,				flatconvolution_bias	\
__asm	paddusw		reg2,				flatconvolution_bias	\
__asm	pmulhuw		reg0,				onenineth				\
__asm	pmulhuw		reg2,				onenineth				\
__asm	packuswb	reg0,				reg2					\
__asm	SSE_MOVE	[daddr + 1],		reg0

#define	flatconvolution_w2(daddr, saddr, spitch, nullreg, onenineth, reg0, reg1, reg2, reg3, reg4, reg5)	\
__asm	SSE3_MOVE	reg0,				[saddr]					\
__asm	SSE3_MOVE	reg1,				[saddr + 1]				\
__asm	SSE_RMOVE	reg2,				reg0					\
__asm	SSE_RMOVE	reg3,				reg1					\
__asm	punpcklbw	reg0,				nullreg					\
__asm	punpcklbw	reg1,				nullreg					\
__asm	punpckhbw	reg2,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	paddusw		reg0,				reg1					\
__asm	paddusw		reg2,				reg3					\
__asm	SSE3_MOVE	reg4,				[saddr + 2]				\
__asm	SSE3_MOVE	reg1,				[saddr + ebx]			\
__asm	SSE_RMOVE	reg5,				reg4					\
__asm	SSE_RMOVE	reg3,				reg1					\
__asm	punpcklbw	reg4,				nullreg					\
__asm	punpcklbw	reg1,				nullreg					\
__asm	punpckhbw	reg5,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	paddusw		reg0,				reg4					\
__asm	paddusw		reg2,				reg5					\
__asm	paddusw		reg0,				reg1					\
__asm	paddusw		reg2,				reg3					\
__asm	SSE3_MOVE	reg4,				[saddr + ebx + 1]		\
__asm	SSE3_MOVE	reg1,				[saddr + ebx + 2]		\
__asm	SSE_RMOVE	reg5,				reg4					\
__asm	SSE_MOVE	[daddr + 1],		reg1					\
__asm	SSE_RMOVE	reg3,				reg1					\
__asm	punpcklbw	reg4,				nullreg					\
__asm	punpcklbw	reg1,				nullreg					\
__asm	punpckhbw	reg5,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	paddusw		reg0,				reg4					\
__asm	paddusw		reg2,				reg5					\
__asm	paddusw		reg0,				reg1					\
__asm	paddusw		reg2,				reg3					\
__asm	SSE3_MOVE	reg4,				[saddr + 2*ebx]			\
__asm	SSE3_MOVE	reg1,				[saddr + 2*ebx + 1]		\
__asm	SSE_RMOVE	reg5,				reg4					\
__asm	SSE_RMOVE	reg3,				reg1					\
__asm	punpcklbw	reg4,				nullreg					\
__asm	punpcklbw	reg1,				nullreg					\
__asm	punpckhbw	reg5,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	paddusw		reg0,				reg4					\
__asm	paddusw		reg2,				reg5					\
__asm	paddusw		reg0,				reg1					\
__asm	paddusw		reg2,				reg3					\
__asm	SSE3_MOVE	reg1,				[saddr + 2*ebx + 2]		\
__asm	SSE_RMOVE	reg3,				reg1					\
__asm	punpcklbw	reg1,				nullreg					\
__asm	punpckhbw	reg3,				nullreg					\
__asm	paddusw		reg0,				reg1					\
__asm	paddusw		reg2,				reg3					\
__asm	paddusw		reg0,				flatconvolution_bias	\
__asm	paddusw		reg2,				flatconvolution_bias	\
__asm	pmulhuw		reg0,				onenineth				\
__asm	pmulhuw		reg2,				onenineth				\
__asm	packuswb	reg0,				reg2					\
__asm	SSE_MOVE	[daddr],			reg0

#define	FLATBIAS	4

static	const __declspec(align(SSE_INCREMENT)) unsigned short	flatconvolution_bias[SSE_INCREMENT/2] =
	{
		FLATBIAS, FLATBIAS, FLATBIAS, FLATBIAS	
#if	SSE_INCREMENT == 16
		, FLATBIAS, FLATBIAS, FLATBIAS, FLATBIAS	
#endif
	};

#define	ONENINETH	(unsigned short)(((1u << 16) + 4) / 9)

static	const __declspec(align(SSE_INCREMENT)) unsigned short	onenineth[SSE_INCREMENT/2] =
	{
		ONENINETH, ONENINETH, ONENINETH, ONENINETH	
#if	SSE_INCREMENT == 16
		, ONENINETH, ONENINETH, ONENINETH, ONENINETH	
#endif
	};

void	SSE_RemoveGrain20(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
#ifdef	CVERSION
		_sp -= spitch;
		int width = (hblocks + 2) * SSE_INCREMENT + remainder;
		int	spitch2 = spitch - width;
		dpitch -= width;
		do
		{
			int	w = width;
			dp[0] = _sp[spitch];
			do
			{
				*++dp = (BYTE)((_sp[0] + _sp[1] + _sp[2] + _sp[spitch] + _sp[spitch + 1] + _sp[spitch + 2] + _sp[2 * spitch]
							+ _sp[2 * spitch + 1] + _sp[2 * spitch  + 2] + 4) / 9);
				++_sp;
			} while( --w );
			dp[1] = _sp[spitch + 1];
			dp += dpitch;
			_sp += spitch2; 
		} while( --height );
#else
__asm	SSE_RMOVE	SSE7,				onenineth
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
__asm	pxor		SSE6,				SSE6
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
__asm	align		16
__asm	column_loop:
		flatconvolution_w1(edi, esi, ebx, SSE6, SSE7, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5)
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif	// MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
		flatconvolution(edi, esi, ebx, SSE6, SSE7, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5)
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
		flatconvolution_w2(edi, esi, ebx, SSE6, SSE7, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5)
__asm	add			esi,				eax
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
#endif
}

#if	ISSE > 1
#define	fconvolution(daddr, saddr, spitch, nullreg, bias_correction, reg0, reg1, reg2, reg3, reg4, reg5)\
__asm	SSE3_MOVE	reg0,				[saddr]				\
__asm	SSE3_MOVE	reg1,				[saddr + 2]			\
__asm	SSE3_MOVE	reg2,				[saddr + 2*spitch]	\
__asm	SSE3_MOVE	reg3,				[saddr + 2*spitch + 2]\
__asm	pavgb		reg0,				reg1				\
__asm	pavgb		reg2,				reg3				\
__asm	SSE3_MOVE	reg1,				[saddr + 1]			\
__asm	SSE3_MOVE	reg3,				[saddr + 2*spitch + 1]\
__asm	pavgb		reg0,				reg1				\
__asm	SSE3_MOVE	reg4,				[saddr + spitch]	\
__asm	SSE3_MOVE	reg5,				[saddr + spitch + 2]\
__asm	pavgb		reg2,				reg3				\
__asm	SSE3_MOVE	reg1,				[saddr + spitch + 1]\
__asm	pavgb		reg0,				reg2				\
__asm	pavgb		reg4,				reg5				\
__asm	psubusb		reg0,				bias_correction		\
__asm	pavgb		reg1,				reg4				\
__asm	pavgb		reg0,				reg1				\
__asm	SSE_MOVE	[daddr],			reg0

#define	fconvolution_w1(daddr, saddr, spitch, nullreg, bias_correction, reg0, reg1, reg2, reg3, reg4, reg5)\
__asm	SSE3_MOVE	reg0,				[saddr]				\
__asm	SSE3_MOVE	reg1,				[saddr + 2]			\
__asm	SSE3_MOVE	reg2,				[saddr + 2*spitch]	\
__asm	SSE3_MOVE	reg3,				[saddr + 2*spitch + 2]\
__asm	pavgb		reg0,				reg1				\
__asm	pavgb		reg2,				reg3				\
__asm	SSE3_MOVE	reg1,				[saddr + 1]			\
__asm	SSE3_MOVE	reg3,				[saddr + 2*spitch + 1]\
__asm	pavgb		reg0,				reg1				\
__asm	SSE3_MOVE	reg4,				[saddr + spitch]	\
__asm	SSE3_MOVE	reg5,				[saddr + spitch + 2]\
__asm	pavgb		reg2,				reg3				\
__asm	movd		[daddr],			reg4				\
__asm	SSE3_MOVE	reg1,				[saddr + spitch + 1]\
__asm	pavgb		reg0,				reg2				\
__asm	pavgb		reg4,				reg5				\
__asm	psubusb		reg0,				bias_correction		\
__asm	pavgb		reg1,				reg4				\
__asm	pavgb		reg0,				reg1				\
__asm	SSE_MOVE	[daddr + 1],		reg0

#define	fconvolution_w2(daddr, saddr, spitch, nullreg, bias_correction, reg0, reg1, reg2, reg3, reg4, reg5)\
__asm	SSE3_MOVE	reg0,				[saddr]				\
__asm	SSE3_MOVE	reg1,				[saddr + 2]			\
__asm	SSE3_MOVE	reg2,				[saddr + 2*spitch]	\
__asm	SSE3_MOVE	reg3,				[saddr + 2*spitch + 2]\
__asm	pavgb		reg0,				reg1				\
__asm	pavgb		reg2,				reg3				\
__asm	SSE3_MOVE	reg1,				[saddr + 1]			\
__asm	SSE3_MOVE	reg3,				[saddr + 2*spitch + 1]\
__asm	pavgb		reg0,				reg1				\
__asm	SSE3_MOVE	reg4,				[saddr + spitch]	\
__asm	SSE3_MOVE	reg5,				[saddr + spitch + 2]\
__asm	pavgb		reg2,				reg3				\
__asm	SSE3_MOVE	reg1,				[saddr + spitch + 1]\
__asm	pavgb		reg0,				reg2				\
__asm	pavgb		reg4,				reg5				\
__asm	psubusb		reg0,				bias_correction		\
__asm	pavgb		reg1,				reg4				\
__asm	SSE_MOVE	[daddr + 1],		reg5				\
__asm	pavgb		reg0,				reg1				\
__asm	SSE_MOVE	[daddr],			reg0
#else
#define	fconvolution(daddr, saddr, spitch, nullreg, bias_correction, reg0, reg1, reg2, reg3, reg4, reg5)	\
__asm	SSE3_MOVE	reg0,				[saddr]				\
__asm	SSE3_MOVE	reg2,				[saddr + 2*spitch]	\
__asm	pavgb		reg0,				[saddr + 2]			\
__asm	pavgb		reg2,				[saddr + 2*spitch + 2]\
__asm	pavgb		reg0,				[saddr + 1]			\
__asm	pavgb		reg2,				[saddr + 2*spitch + 1]\
__asm	SSE3_MOVE	reg4,				[saddr + spitch]	\
__asm	pavgb		reg0,				reg2				\
__asm	pavgb		reg4,				[saddr + spitch + 2]\
__asm	psubusb		reg0,				bias_correction		\
__asm	pavgb		reg4,				[saddr + spitch + 1]\
__asm	pavgb		reg0,				reg4				\
__asm	SSE_MOVE	[daddr],			reg0


#define	fconvolution_w1(daddr, saddr, spitch, nullreg, bias_correction, reg0, reg1, reg2, reg3, reg4, reg5)	\
__asm	SSE3_MOVE	reg0,				[saddr]				\
__asm	SSE3_MOVE	reg2,				[saddr + 2*spitch]	\
__asm	pavgb		reg0,				[saddr + 2]			\
__asm	pavgb		reg2,				[saddr + 2*spitch + 2]\
__asm	pavgb		reg0,				[saddr + 1]			\
__asm	pavgb		reg2,				[saddr + 2*spitch + 1]\
__asm	SSE3_MOVE	reg4,				[saddr + spitch]	\
__asm	pavgb		reg0,				reg2				\
__asm	movd		[daddr],			reg4				\
__asm	pavgb		reg4,				[saddr + spitch + 2]\
__asm	psubusb		reg0,				bias_correction		\
__asm	pavgb		reg4,				[saddr + spitch + 1]\
__asm	pavgb		reg0,				reg4				\
__asm	SSE_MOVE	[daddr + 1],		reg0

#define	fconvolution_w2(daddr, saddr, spitch, nullreg, bias_correction, reg0, reg1, reg2, reg3, reg4, reg5)	\
__asm	SSE3_MOVE	reg0,				[saddr]				\
__asm	SSE3_MOVE	reg2,				[saddr + 2*spitch]	\
__asm	pavgb		reg0,				[saddr + 2]			\
__asm	pavgb		reg2,				[saddr + 2*spitch + 2]\
__asm	pavgb		reg0,				[saddr + 1]			\
__asm	pavgb		reg2,				[saddr + 2*spitch + 1]\
__asm	SSE3_MOVE	reg4,				[saddr + spitch + 2]\
__asm	pavgb		reg0,				reg2				\
__asm	SSE_MOVE	[daddr + 1],		reg4				\
__asm	pavgb		reg4,				[saddr + spitch]	\
__asm	psubusb		reg0,				bias_correction		\
__asm	pavgb		reg4,				[saddr + spitch + 1]\
__asm	pavgb		reg0,				reg4				\
__asm	SSE_MOVE	[daddr],			reg0

#endif // ISSE

static	const __declspec(align(SSE_INCREMENT)) unsigned char fconvolution_bias[SSE_INCREMENT] =
	{
		1,1,1,1,1,1,1,1
#if	SSE_INCREMENT == 16
		,1,1,1,1,1,1,1,1
#endif
	};

void	SSE_RemoveGrain12(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
#ifdef	CVERSION
		_sp -= spitch;
		int width = (hblocks + 2) * SSE_INCREMENT + remainder;
		int	spitch2 = spitch - width;
		dpitch -= width;
		do
		{
			int	w = width;
			dp[0] = _sp[spitch];
			do
			{
				*++dp = ((((_sp[0] + _sp[2] + 1) / 2 + _sp[1] + 1) / 2 + ((_sp[2*spitch] + _sp[2*spitch + 2] + 1) / 2 + _sp[2*spitch + 1] + 1) / 2 + 1)/2
					 + ((_sp[spitch] + _sp[spitch + 2] + 1) / 2 + _sp[spitch + 1] + 1) / 2) / 2;
				++_sp;
			} while( --w );
			dp[1] = _sp[spitch + 1];
			dp += dpitch;
			_sp += spitch2; 
		} while( --height );
#else
__asm	SSE_RMOVE	SSE7,				fconvolution_bias
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
__asm	pxor		SSE6,				SSE6
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
__asm	align		16
__asm	column_loop:
		fconvolution_w1(edi, esi, ebx, SSE6, SSE7, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5)
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif	// MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
		fconvolution(edi, esi, ebx, SSE6, SSE7, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5)
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
		fconvolution_w2(edi, esi, ebx, SSE6, SSE7, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5)
__asm	add			esi,				eax
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
#endif
}

#if	ISSE > 1
#define	rconvolution(daddr, saddr, spitch, nullreg, bias_correction, reg0, reg1, reg2, reg3, reg4, reg5)	\
__asm	SSE3_MOVE	reg0,				[saddr]				\
__asm	SSE3_MOVE	reg2,				[saddr + 2]			\
__asm	SSE3_MOVE	reg1,				[saddr + 2*spitch]	\
__asm	SSE3_MOVE	reg3,				[saddr + 2*spitch + 2]\
__asm	pavgb		reg0,				reg2				\
__asm	pavgb		reg1,				reg3				\
__asm	SSE3_MOVE	reg2,				[saddr + spitch]	\
__asm	SSE3_MOVE	reg3,				[saddr + 1]			\
__asm	pavgb		reg0,				reg1				\
__asm	SSE3_MOVE	reg4,				[saddr + 2*spitch + 1]\
__asm	SSE3_MOVE	reg5,				[saddr + spitch + 2]\
__asm	pavgb		reg2,				reg4				\
__asm	pavgb		reg3,				reg5				\
__asm	psubusb		reg0,				bias_correction		\
__asm	pavgb		reg2,				reg3				\
__asm	pavgb		reg0,				reg2				\
__asm	SSE_MOVE	[daddr],			reg0

#define	rconvolution_w1(daddr, saddr, spitch, nullreg, bias_correction, reg0, reg1, reg2, reg3, reg4, reg5)	\
__asm	SSE3_MOVE	reg0,				[saddr]				\
__asm	SSE3_MOVE	reg2,				[saddr + 2]			\
__asm	SSE3_MOVE	reg1,				[saddr + 2*spitch]	\
__asm	SSE3_MOVE	reg3,				[saddr + 2*spitch + 2]\
__asm	pavgb		reg0,				reg2				\
__asm	pavgb		reg1,				reg3				\
__asm	SSE3_MOVE	reg2,				[saddr + spitch]	\
__asm	SSE3_MOVE	reg3,				[saddr + 1]			\
__asm	pavgb		reg0,				reg1				\
__asm	movd		[daddr],			reg2				\
__asm	SSE3_MOVE	reg4,				[saddr + 2*spitch + 1]\
__asm	SSE3_MOVE	reg5,				[saddr + spitch + 2]\
__asm	pavgb		reg2,				reg4				\
__asm	pavgb		reg3,				reg5				\
__asm	psubusb		reg0,				bias_correction		\
__asm	pavgb		reg2,				reg3				\
__asm	pavgb		reg0,				reg2				\
__asm	SSE_MOVE	[daddr + 1],		reg0

#define	rconvolution_w2(daddr, saddr, spitch, nullreg, bias_correction, reg0, reg1, reg2, reg3, reg4, reg5)	\
__asm	SSE3_MOVE	reg0,				[saddr]				\
__asm	SSE3_MOVE	reg2,				[saddr + 2]			\
__asm	SSE3_MOVE	reg1,				[saddr + 2*spitch]	\
__asm	SSE3_MOVE	reg3,				[saddr + 2*spitch + 2]\
__asm	pavgb		reg0,				reg2				\
__asm	pavgb		reg1,				reg3				\
__asm	SSE3_MOVE	reg2,				[saddr + spitch]	\
__asm	SSE3_MOVE	reg3,				[saddr + 1]			\
__asm	pavgb		reg0,				reg1				\
__asm	SSE3_MOVE	reg4,				[saddr + 2*spitch + 1]\
__asm	SSE3_MOVE	reg5,				[saddr + spitch + 2]\
__asm	pavgb		reg2,				reg4				\
__asm	pavgb		reg3,				reg5				\
__asm	SSE_MOVE	[daddr + 1],		reg5				\
__asm	pavgb		reg2,				reg3				\
__asm	psubusb		reg0,				bias_correction		\
__asm	pavgb		reg0,				reg2				\
__asm	SSE_MOVE	[daddr],			reg0
#else
#define	rconvolution(daddr, saddr, spitch, nullreg, bias_correction, reg0, reg1, reg2, reg3, reg4, reg5)\
__asm	SSE3_MOVE	reg0,				[saddr]				\
__asm	SSE3_MOVE	reg1,				[saddr + 2*spitch]	\
__asm	pavgb		reg0,				[saddr + 2]			\
__asm	pavgb		reg1,				[saddr + 2*spitch + 2]\
__asm	SSE3_MOVE	reg2,				[saddr + spitch]	\
__asm	SSE3_MOVE	reg3,				[saddr + 1]			\
__asm	pavgb		reg0,				reg1				\
__asm	pavgb		reg2,				[saddr + 2*spitch + 1]\
__asm	pavgb		reg3,				[saddr + spitch + 2]\
__asm	psubusb		reg0,				bias_correction		\
__asm	pavgb		reg2,				reg3				\
__asm	pavgb		reg0,				reg2				\
__asm	SSE_MOVE	[daddr],			reg0

#define	rconvolution_w1(daddr, saddr, spitch, nullreg, bias_correction, reg0, reg1, reg2, reg3, reg4, reg5)\
__asm	SSE3_MOVE	reg0,				[saddr]				\
__asm	SSE3_MOVE	reg1,				[saddr + 2*spitch]	\
__asm	pavgb		reg0,				[saddr + 2]			\
__asm	pavgb		reg1,				[saddr + 2*spitch + 2]\
__asm	SSE3_MOVE	reg2,				[saddr + spitch]	\
__asm	SSE3_MOVE	reg3,				[saddr + 1]			\
__asm	movd		[daddr],			reg2				\
__asm	pavgb		reg0,				reg1				\
__asm	pavgb		reg2,				[saddr + 2*spitch + 1]\
__asm	pavgb		reg3,				[saddr + spitch + 2]\
__asm	psubusb		reg0,				bias_correction		\
__asm	pavgb		reg2,				reg3				\
__asm	pavgb		reg0,				reg2				\
__asm	SSE_MOVE	[daddr + 1],		reg0

#define	rconvolution_w2(daddr, saddr, spitch, nullreg, bias_correction, reg0, reg1, reg2, reg3, reg4, reg5)\
__asm	SSE3_MOVE	reg0,				[saddr]				\
__asm	SSE3_MOVE	reg1,				[saddr + 2*spitch]	\
__asm	pavgb		reg0,				[saddr + 2]			\
__asm	pavgb		reg1,				[saddr + 2*spitch + 2]\
__asm	SSE3_MOVE	reg2,				[saddr + spitch]	\
__asm	SSE3_MOVE	reg3,				[saddr + spitch + 2]\
__asm	pavgb		reg0,				reg1				\
__asm	SSE_MOVE	[daddr + 1],		reg3				\
__asm	pavgb		reg2,				[saddr + 2*spitch + 1]\
__asm	pavgb		reg3,				[saddr + 1]			\
__asm	psubusb		reg0,				bias_correction		\
__asm	pavgb		reg2,				reg3				\
__asm	pavgb		reg0,				reg2				\
__asm	SSE_MOVE	[daddr],			reg0
#endif

#define	rbias	1

static	const __declspec(align(SSE_INCREMENT)) unsigned char rconvolution_bias[SSE_INCREMENT] =
	{
		rbias, rbias, rbias, rbias, rbias, rbias, rbias, rbias
#if	SSE_INCREMENT == 16
		, rbias, rbias, rbias, rbias, rbias, rbias, rbias, rbias
#endif
	};

void	SSE_RemoveGrain19(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
#ifdef	CVERSION
		_sp -= spitch;
		int width = (hblocks + 2) * SSE_INCREMENT + remainder;
		int	spitch2 = spitch - width;
		dpitch -= width;
		do
		{
			int	w = width;
			dp[0] = _sp[spitch];
			do
			{
				*++dp = (_sp[0] + _sp[1] + _sp[2] + _sp[spitch] + _sp[spitch + 2] + _sp[2*spitch] + _sp[2*spitch + 1] + _sp[2*spitch + 2] + 4) / 8;
				++_sp;
			} while( --w );
			dp[1] = _sp[spitch + 1];
			dp += dpitch;
			_sp += spitch2; 
		} while( --height );
#else
__asm	SSE_RMOVE	SSE7,				rconvolution_bias
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
__asm	pxor		SSE6,				SSE6
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
__asm	lea			eax,				[ebx + eax + 1]	
__asm	align		16
__asm	column_loop:
		rconvolution_w1(edi, esi, ebx, SSE6, SSE7, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5)
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
__asm	align		16
__asm	middle_loop:
		rconvolution(edi, esi, ebx, SSE6, SSE7, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5)
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
		rconvolution_w2(edi, esi, ebx, SSE6, SSE7, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5)	
__asm	add			esi,				eax
__asm	add			edi,				dpitch
__asm	dec			height
__asm	jnz			column_loop
#endif
}

#define	DeringA(ldiff, udiff, center, p1, p2, reg1, reg2) \
__asm	SSE3_MOVE	reg2,				p1		\
__asm	SSE3_MOVE	reg1,				p2		\
__asm	SSE_RMOVE	ldiff,				reg2	\
__asm	SSE_RMOVE	udiff,				center	\
__asm	pmaxub		reg2,				reg1	\
__asm	pminub		ldiff,				reg1	\
__asm	psubusb		udiff,				reg2	\
__asm	psubusb		reg2,				ldiff	\
__asm	psubusb		ldiff,				center	\
__asm	pminub		udiff,				reg2	\
__asm	pminub		ldiff,				reg2

#define	DeringA_w(ldiff, udiff, center, p1, p2, mwrite, wmem, reg1, reg2) \
__asm	SSE3_MOVE	reg2,				p1		\
__asm	SSE3_MOVE	reg1,				p2		\
__asm	SSE_RMOVE	ldiff,				reg2	\
__asm	SSE_RMOVE	udiff,				center	\
__asm	pmaxub		reg2,				reg1	\
__asm	pminub		ldiff,				reg1	\
__asm	psubusb		udiff,				reg2	\
__asm	mwrite		wmem,				reg1	\
__asm	psubusb		reg2,				ldiff	\
__asm	psubusb		ldiff,				center	\
__asm	pminub		udiff,				reg2	\
__asm	pminub		ldiff,				reg2

#define	DeringM(ldiff, udiff, center, reg1)	\
__asm	psubusb		center,				udiff	\
__asm	paddusb		center,				ldiff
			

void	SSE_RemoveGrain23(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
__asm	lea			eax,				[ebx + eax + 1]	
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]
		DeringA(SSE1, SSE2, SSE0, [esi], [esi + 2*ebx + 2], SSE5, SSE6)
		DeringA(SSE3, SSE4, SSE0, [esi + 2], [esi + 2*ebx], SSE5, SSE6)
__asm	pmaxub		SSE2,				SSE4
__asm	pmaxub		SSE1,				SSE3
		DeringA_w(SSE3, SSE4, SSE0, [esi + ebx + 2], [esi + ebx], movd, [edi], SSE5, SSE6)
__asm	pmaxub		SSE2,				SSE4
__asm	pmaxub		SSE1,				SSE3
		DeringA(SSE3, SSE4, SSE0, [esi + 1], [esi + 2*ebx + 1], SSE5, SSE6)
__asm	pmaxub		SSE2,				SSE4
__asm	pmaxub		SSE1,				SSE3
		DeringM(SSE1, SSE2, SSE0, SSE5)
__asm	SSE_MOVE	[edi + 1],			SSE0
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
__asm	align		16
__asm	middle_loop:
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]
		DeringA(SSE1, SSE2, SSE0, [esi], [esi + 2*ebx + 2], SSE5, SSE6)
		DeringA(SSE3, SSE4, SSE0, [esi + 2], [esi + 2*ebx], SSE5, SSE6)
__asm	pmaxub		SSE2,				SSE4
__asm	pmaxub		SSE1,				SSE3
		DeringA(SSE3, SSE4, SSE0, [esi + ebx], [esi + ebx + 2], SSE5, SSE6)
__asm	pmaxub		SSE2,				SSE4
__asm	pmaxub		SSE1,				SSE3
		DeringA(SSE3, SSE4, SSE0, [esi + 1], [esi + 2*ebx + 1], SSE5, SSE6)
__asm	pmaxub		SSE2,				SSE4
__asm	pmaxub		SSE1,				SSE3
		DeringM(SSE1, SSE2, SSE0, SSE5)
__asm	SSE_MOVE	[edi],				SSE0		
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]
		DeringA(SSE1, SSE2, SSE0, [esi], [esi + 2*ebx + 2], SSE5, SSE6)
		DeringA(SSE3, SSE4, SSE0, [esi + 2], [esi + 2*ebx], SSE5, SSE6)
__asm	pmaxub		SSE2,				SSE4
__asm	pmaxub		SSE1,				SSE3
		DeringA_w(SSE3, SSE4, SSE0, [esi + ebx], [esi + ebx + 2], SSE_MOVE, [edi + 1], SSE5, SSE6)
__asm	pmaxub		SSE2,				SSE4
__asm	pmaxub		SSE1,				SSE3
		DeringA(SSE3, SSE4, SSE0, [esi + 1], [esi + 2*ebx + 1], SSE5, SSE6)
__asm	pmaxub		SSE2,				SSE4
__asm	pmaxub		SSE1,				SSE3
		DeringM(SSE1, SSE2, SSE0, SSE5)
__asm	SSE_MOVE	[edi],				SSE0
__asm	add			esi,				eax
__asm	add			edi,				dpitch
__asm	dec			height
__asm	jnz			column_loop
}

#define	DeringC(ldiff, udiff, center, p1, p2, reg1, reg2) \
__asm	SSE3_MOVE	reg2,				p1		\
__asm	SSE3_MOVE	reg1,				p2		\
__asm	SSE_RMOVE	ldiff,				reg2	\
__asm	SSE_RMOVE	udiff,				center	\
__asm	pmaxub		reg2,				reg1	\
__asm	pminub		ldiff,				reg1	\
__asm	psubusb		udiff,				reg2	\
__asm	psubusb		reg2,				ldiff	\
__asm	psubusb		ldiff,				center	\
__asm	SSE_RMOVE	reg1,				reg2	\
__asm	psubusb		reg2,				udiff	\
__asm	psubusb		reg1,				ldiff	\
__asm	pminub		udiff,				reg2	\
__asm	pminub		ldiff,				reg1

#define	DeringC_w(ldiff, udiff, center, p1, p2, mwrite, wmem, reg1, reg2) \
__asm	SSE3_MOVE	reg2,				p1		\
__asm	SSE3_MOVE	reg1,				p2		\
__asm	SSE_RMOVE	ldiff,				reg2	\
__asm	SSE_RMOVE	udiff,				center	\
__asm	pmaxub		reg2,				reg1	\
__asm	pminub		ldiff,				reg1	\
__asm	psubusb		udiff,				reg2	\
__asm	mwrite		wmem,				reg1	\
__asm	psubusb		reg2,				ldiff	\
__asm	psubusb		ldiff,				center	\
__asm	SSE_RMOVE	reg1,				reg2	\
__asm	psubusb		reg2,				udiff	\
__asm	psubusb		reg1,				ldiff	\
__asm	pminub		udiff,				reg2	\
__asm	pminub		ldiff,				reg1

void	SSE_RemoveGrain24(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
__asm	lea			eax,				[ebx + eax + 1]	
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]
		DeringC(SSE1, SSE2, SSE0, [esi], [esi + 2*ebx + 2], SSE5, SSE6)
		DeringC(SSE3, SSE4, SSE0, [esi + 2], [esi + 2*ebx], SSE5, SSE6)
__asm	pmaxub		SSE2,				SSE4
__asm	pmaxub		SSE1,				SSE3
		DeringC_w(SSE3, SSE4, SSE0, [esi + ebx + 2], [esi + ebx], movd, [edi], SSE5, SSE6)
__asm	pmaxub		SSE2,				SSE4
__asm	pmaxub		SSE1,				SSE3
		DeringC(SSE3, SSE4, SSE0, [esi + 1], [esi + 2*ebx + 1], SSE5, SSE6)
__asm	pmaxub		SSE2,				SSE4
__asm	pmaxub		SSE1,				SSE3
		DeringM(SSE1, SSE2, SSE0, SSE5)
__asm	SSE_MOVE	[edi + 1],			SSE0
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
__asm	align		16
__asm	middle_loop:
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]
		DeringC(SSE1, SSE2, SSE0, [esi], [esi + 2*ebx + 2], SSE5, SSE6)
		DeringC(SSE3, SSE4, SSE0, [esi + 2], [esi + 2*ebx], SSE5, SSE6)
__asm	pmaxub		SSE2,				SSE4
__asm	pmaxub		SSE1,				SSE3
		DeringC(SSE3, SSE4, SSE0, [esi + ebx], [esi + ebx + 2], SSE5, SSE6)
__asm	pmaxub		SSE2,				SSE4
__asm	pmaxub		SSE1,				SSE3
		DeringC(SSE3, SSE4, SSE0, [esi + 1], [esi + 2*ebx + 1], SSE5, SSE6)
__asm	pmaxub		SSE2,				SSE4
__asm	pmaxub		SSE1,				SSE3
		DeringM(SSE1, SSE2, SSE0, SSE5)
__asm	SSE_MOVE	[edi],				SSE0		
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]
		DeringC(SSE1, SSE2, SSE0, [esi], [esi + 2*ebx + 2], SSE5, SSE6)
		DeringC(SSE3, SSE4, SSE0, [esi + 2], [esi + 2*ebx], SSE5, SSE6)
__asm	pmaxub		SSE2,				SSE4
__asm	pmaxub		SSE1,				SSE3
		DeringC_w(SSE3, SSE4, SSE0, [esi + ebx], [esi + ebx + 2], SSE_MOVE, [edi + 1], SSE5, SSE6)
__asm	pmaxub		SSE2,				SSE4
__asm	pmaxub		SSE1,				SSE3
		DeringC(SSE3, SSE4, SSE0, [esi + 1], [esi + 2*ebx + 1], SSE5, SSE6)
__asm	pmaxub		SSE2,				SSE4
__asm	pmaxub		SSE1,				SSE3
		DeringM(SSE1, SSE2, SSE0, SSE5)
__asm	SSE_MOVE	[edi],				SSE0
__asm	add			esi,				eax
__asm	add			edi,				dpitch
__asm	dec			height
__asm	jnz			column_loop
}


#define	get_min_weightw1(min, weight, mem1, mem2, wmem, reg)		\
__asm	SSE3_MOVE	min,				mem1		\
__asm	SSE3_MOVE	reg,				mem2		\
__asm	movd		wmem,				min			\
__asm	SSE_RMOVE	weight,				min			\
__asm	pminub		min,				reg			\
__asm	pmaxub		weight,				reg			\
__asm	psubusb		weight,				min

void	WeirdBob(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
#ifdef	CVERSION
	int	width = (hblocks + 2) * SSE_INCREMENT + remainder;
	int	spitch2 = 2*spitch - width - 1, dpitch2 = 2*dpitch - width - 1;
	_sp -= spitch;
	do
	{
		dp[0] = (BYTE)(((unsigned)_sp[0] + (unsigned)(dp[dpitch] = _sp[2*spitch]) + 1) / 2);
		++dp;
		int	w = width;
		do
		{
			unsigned	weight1, min1, weight2, min2;
			min1 = _sp[0];
			weight1 = _sp[2*spitch + 2];
			if( weight1 < min1 ) 
			{
				min1 = weight1;
				weight1 = _sp[0];
			}
			weight1 -= min1;

			min2 = _sp[2];
			weight2 = _sp[2*spitch];
			if( weight2 < min2 ) 
			{
				min2 = weight2;
				weight2 = _sp[2];
			}
			weight2 -= min2;
			if( weight2 <= weight1 ) 
			{
				weight1 = weight2;
				min1 = min2;
			}
			
			++_sp;

			min2 = _sp[0];
			weight2 = dp[dpitch] = _sp[2*spitch];
			if( weight2 < min2 ) 
			{
				min2 = weight2;
				weight2 = _sp[0];
			}
			weight2 -= min2;
			if( weight2 <= weight1 ) 
			{
				weight1 = weight2;
				min1 = min2;
			}

			dp[0] = (BYTE) (min1 + (weight1 + 1)/2);

			++dp;
			
		} while( --w );
		++_sp;
		dp[0] = (BYTE)(((unsigned)_sp[0] + (unsigned)(dp[dpitch] = _sp[2*spitch]) + 1)/2);
		dp += dpitch2;
		_sp += spitch2;
	} while( --height );
#else	// CVERSION
__asm	mov			ecx,				incpitch
__asm	mov			eax,				dpitch
__asm	lea			ebx,				[2*eax + ecx]
__asm	mov			edx,				remainder
__asm	mov			dpitch,				ebx
__asm	mov			esi,				_sp
__asm	mov			ebx,				spitch
__asm	mov			edi,				dp
__asm	sub			esi,				ebx
__asm	add			ebx,				ebx
__asm	add			ecx,				ebx
__asm	mov			spitch,				ecx
__asm	mov			ecx,				hblocks
__asm	align		16
__asm	column_loop:
		get_min_weight(SSE0, SSE1, SSE7, [esi + ebx + 2], [esi], SSE7)
		get_min_weightw(SSE2, SSE3, SSE7, [esi + 2], [esi + ebx], [edi + eax], SSE6)
__asm	pavgb		SSE6,				SSE7
		mergeweighted(SSE0, SSE1, SSE2, SSE3)
__asm	movd		[edi],				SSE6		
		get_min_weight(SSE2, SSE3, SSE7, [esi + ebx + 1], [esi + 1], SSE7)
		mergeweighted(SSE0, SSE1, SSE2, SSE3)
__asm	paddusb		SSE1,				SSE0
__asm	pavgb		SSE0,				SSE1
__asm	SSE_MOVE	[edi + 1],			SSE0
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	mov			ecx,				hblocks
__asm	add			edi,				SSE_INCREMENT		
__asm	align		16
__asm	middle_loop:
		get_min_weight(SSE0, SSE1, SSE7, [esi], [esi + ebx + 2], SSE7)
		get_min_weightw(SSE2, SSE3, SSE7, [esi + 2], [esi + ebx], [edi + eax], SSE7)
		mergeweighted(SSE0, SSE1, SSE2, SSE3)
		get_min_weight(SSE2, SSE3, SSE7, [esi + 1], [esi + ebx + 1], SSE7)
		mergeweighted(SSE0, SSE1, SSE2, SSE3)
__asm	paddusb		SSE1,				SSE0
__asm	pavgb		SSE0,				SSE1
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi + 1],			SSE0
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
		get_min_weightw(SSE0, SSE1, SSE7, [esi], [esi + ebx + 2], [edi + eax + 2], SSE7)
		get_min_weightw1(SSE2, SSE3, [esi + ebx], [esi + 2], [edi + eax], SSE6)
__asm	pavgb		SSE6,				SSE7
		mergeweighted(SSE0, SSE1, SSE2, SSE3)
__asm	SSE_MOVE	[edi + 2],			SSE6		
		get_min_weight(SSE2, SSE3, SSE7, [esi + ebx + 1], [esi + 1], SSE7)
		mergeweighted(SSE0, SSE1, SSE2, SSE3)
__asm	paddusb		SSE1,				SSE0
__asm	pavgb		SSE0,				SSE1
__asm	add			esi,				spitch
__asm	SSE_MOVE	[edi + 1],			SSE0
__asm	add			edi,				dpitch
__asm	dec			height
__asm	jnz			column_loop
#endif	// CVERSION
}


void	bob_top(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
	memcpy(dp,_sp, (hblocks + 2) * SSE_INCREMENT + remainder + 2);
	WeirdBob(dp + dpitch, dpitch, _sp + spitch, spitch, hblocks, remainder, incpitch, height / 2);
}

void	bob_bottom(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
	WeirdBob(dp, dpitch, _sp, spitch, hblocks, remainder, incpitch, (height + 1)/2);
}

#define	get_min_weight_average(min, weight, average, mem1, mem2, reg)		\
__asm	SSE3_MOVE	min,				mem1		\
__asm	SSE3_MOVE	reg,				mem2		\
__asm	SSE_RMOVE	weight,				min			\
__asm	SSE_RMOVE	average,			min			\
__asm	pmaxub		weight,				reg			\
__asm	pminub		min,				reg			\
__asm	pavgb		average,			reg			\
__asm	psubusb		weight,				min

void	SmartBob(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
#ifdef	CVERSION
	int	width = (hblocks + 2) * SSE_INCREMENT + remainder;
	int	spitch2 = 2*spitch - width - 1, dpitch2 = 2*dpitch - width - 1;
	_sp -= spitch;
	do
	{
		dp[0] = (BYTE)(((unsigned)_sp[0] + (unsigned)(dp[dpitch] = _sp[2*spitch]) + 1) / 2);
		++dp;
		int	w = width;
		do
		{
#if	1
			unsigned	weight1, min1, weight2, min2, average;
			min1 = _sp[0];
			weight1 = _sp[2*spitch + 2];
			if( weight1 < min1 ) 
			{
				min1 = weight1;
				weight1 = _sp[0];
			}
			average = weight1 + min1;
			weight1 -= min1;

			min2 = _sp[2];
			weight2 = _sp[2*spitch];
			if( weight2 < min2 ) 
			{
				min2 = weight2;
				weight2 = _sp[2];
			}
			average += min2 + weight2;
			weight2 -= min2;
			if( weight2 <= weight1 ) 
			{
				weight1 = weight2;
				min1 = min2;
			}
			
			++_sp;

			min2 = _sp[0];
			weight2 = dp[dpitch] = _sp[2*spitch];
			if( weight2 < min2 ) 
			{
				min2 = weight2;
				weight2 = _sp[0];
			}
			average += 2*(weight2 + min2);
			weight2 -= min2;
			if( weight2 <= weight1 ) 
			{
				weight1 = weight2;
				min1 = min2;
			}
			average = (average + 4)/8;
			weight1 += min1;
			if( weight1 < average ) average = weight1;
			else if( min1 > average ) average = min1;
			dp[0] = (BYTE) average;

			++dp;
#else	
			unsigned	weight1, min1, weight2, min2, average;
			min1 = _sp[0];
			weight1 = _sp[2*spitch + 2];
			if( weight1 < min1 ) 
			{
				min1 = weight1;
				weight1 = _sp[0];
			}
			average = (weight1 + min1 + 1)/2;
			weight1 -= min1;

			min2 = _sp[2];
			weight2 = _sp[2*spitch];
			if( weight2 < min2 ) 
			{
				min2 = weight2;
				weight2 = _sp[2];
			}
			average = (average + (min2 + weight2 + 1)/2 + 1)/2 - 1;
			if( (int) average < 0 ) average = 0;
			weight2 -= min2;
			if( weight2 <= weight1 ) 
			{
				weight1 = weight2;
				min1 = min2;
			}
			
			++_sp;

			min2 = _sp[0];
			weight2 = dp[dpitch] = _sp[2*spitch];
			if( weight2 < min2 ) 
			{
				min2 = weight2;
				weight2 = _sp[0];
			}
			average = (average + (weight2 + min2 + 1)/2 + 1)/2;
			weight2 -= min2;
			if( weight2 <= weight1 ) 
			{
				weight1 = weight2;
				min1 = min2;
			}
			weight1 += min1;
			if( weight1 < average ) average = weight1;
			else if( min1 > average ) average = min1;
			dp[0] = (BYTE) average;

			++dp;
#endif			
		} while( --w );
		++_sp;
		dp[0] = (BYTE)(((unsigned)_sp[0] + (unsigned)(dp[dpitch] = _sp[2*spitch]) + 1)/2);
		dp += dpitch2;
		_sp += spitch2;
	} while( --height );
#else	// CVERSION
__asm	mov			ecx,				incpitch
__asm	mov			eax,				dpitch
__asm	lea			ebx,				[2*eax + ecx]
__asm	mov			edx,				remainder
__asm	mov			dpitch,				ebx
__asm	mov			esi,				_sp
__asm	mov			ebx,				spitch
__asm	mov			edi,				dp
__asm	sub			esi,				ebx
__asm	add			ebx,				ebx
//__asm	pxor		SSE6,				SSE6
__asm	SSE_RMOVE	SSE6,				fconvolution_bias
__asm	add			ecx,				ebx
__asm	mov			spitch,				ecx
__asm	mov			ecx,				hblocks
__asm	align		16
__asm	column_loop:
		get_min_weight_average(SSE0, SSE1, SSE2, [esi + ebx + 2], [esi], SSE7)
__asm	SSE3_MOVE	SSE4,				[esi + ebx]		
__asm	SSE3_MOVE	SSE5,				[esi + 2]
__asm	pavgb		SSE7,				SSE4
__asm	SSE_RMOVE	SSE3,				SSE4
__asm	movd		[edi],				SSE7
__asm	SSE_RMOVE	SSE7,				SSE4
__asm	pminub		SSE3,				SSE5
__asm	SSE_MOVE	[edi + eax],		SSE7 
__asm	pmaxub		SSE4,				SSE5
__asm	pavgb		SSE5,				SSE7
__asm	psubusb		SSE4,				SSE3
__asm	pavgb		SSE2,				SSE5
		mergeweighted(SSE0, SSE1, SSE3, SSE4)
__asm	psubusb		SSE2,				SSE6	// bias correction
		get_min_weight_average(SSE3, SSE4, SSE5, [esi + ebx + 1], [esi + 1], SSE7)
__asm	pavgb		SSE2,				SSE5
		mergeweighted(SSE0, SSE1, SSE3, SSE4)
__asm	pmaxub		SSE2,				SSE0
__asm	paddusb		SSE1,				SSE0
__asm	pminub		SSE2,				SSE1
__asm	SSE_MOVE	[edi + 1],			SSE2	
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	mov			ecx,				hblocks
__asm	add			edi,				SSE_INCREMENT		
__asm	align		16
__asm	middle_loop:
		get_min_weight_average(SSE0, SSE1, SSE2, [esi + ebx + 2], [esi], SSE7)
		get_min_weight_average(SSE3, SSE4, SSE5, [esi + 2], [esi + ebx], SSE7)
__asm	SSE_MOVE	[edi + eax],		SSE7
__asm	pavgb		SSE2,				SSE5
		mergeweighted(SSE0, SSE1, SSE3, SSE4)
__asm	psubusb		SSE2,				SSE6	// bias correction
		get_min_weight_average(SSE3, SSE4, SSE5, [esi + 1], [esi + ebx + 1], SSE7)
__asm	pavgb		SSE2,				SSE5
		mergeweighted(SSE0, SSE1, SSE3, SSE4)
__asm	pmaxub		SSE2,				SSE0
__asm	paddusb		SSE1,				SSE0
__asm	pminub		SSE2,				SSE1
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi + 1],			SSE2
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
		get_min_weight_average(SSE0, SSE1, SSE2, [esi], [esi + ebx + 2], SSE7)
__asm	SSE_MOVE	[edi + eax + 2],	SSE7
__asm	SSE3_MOVE	SSE4,				[esi + 2]		
__asm	SSE3_MOVE	SSE5,				[esi + ebx]				
__asm	pavgb		SSE7,				SSE4
__asm	SSE_RMOVE	SSE3,				SSE4
__asm	SSE_MOVE	[edi + 2],			SSE7
__asm	SSE_RMOVE	SSE7,				SSE4
__asm	pminub		SSE3,				SSE5
__asm	movd		[edi + eax],		SSE5
__asm	pmaxub		SSE4,				SSE5
__asm	pavgb		SSE5,				SSE7
__asm	psubusb		SSE4,				SSE3
__asm	pavgb		SSE2,				SSE5
		mergeweighted(SSE0, SSE1, SSE3, SSE4)
__asm	psubusb		SSE2,				SSE6	// bias correction
		get_min_weight_average(SSE3, SSE4, SSE5, [esi + ebx + 1], [esi + 1], SSE7)
__asm	pavgb		SSE2,				SSE5
		mergeweighted(SSE0, SSE1, SSE3, SSE4)
__asm	pmaxub		SSE2,				SSE0
__asm	paddusb		SSE1,				SSE0
__asm	pminub		SSE2,				SSE1
__asm	add			esi,				spitch
__asm	SSE_MOVE	[edi + 1],			SSE2	
__asm	add			edi,				dpitch
__asm	dec			height
__asm	jnz			column_loop
#endif	// CVERSION
}

void	smartbob_top(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
	memcpy(dp,_sp, (hblocks + 2) * SSE_INCREMENT + remainder + 2);
	SmartBob(dp + dpitch, dpitch, _sp + spitch, spitch, hblocks, remainder, incpitch, height / 2);
}

void	smartbob_bottom(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
	SmartBob(dp, dpitch, _sp, spitch, hblocks, remainder, incpitch, (height + 1)/2);
}

#endif	// #ifndef	MODIFYPLUGIN

#ifdef	SHARPEN
void	SmartRG(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height, int strength)
#else
void	SmartRG(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
#endif
{
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
#ifdef	SHARPEN
__asm	mov			spitch,				eax
__asm	mov			eax,				strength
#endif
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 2]
__asm	SSE_RMOVE	SSE1,				SSE0
__asm	pminub		SSE0,				SSE7
__asm	pmaxub		SSE1,				SSE7
__asm	SSE3_MOVE	SSE4,				[esi + 2]
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx]
__asm	SSE_RMOVE	SSE5,				SSE4
__asm	SSE3_MOVE	SSE2,				[esi + 1]
__asm	pminub		SSE4,				SSE7
__asm	pmaxub		SSE5,				SSE7
__asm	pmaxub		SSE0,				SSE4
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 1]
__asm	SSE_RMOVE	SSE3,				SSE2
__asm	pminub		SSE1,				SSE5

__asm	pminub		SSE2,				SSE7
__asm	SSE3_MOVE	SSE4,				[esi + ebx]
__asm	pmaxub		SSE3,				SSE7
__asm	movd		[edi],				SSE4
__asm	pmaxub		SSE0,				SSE2
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 2]
__asm	SSE_RMOVE	SSE5,				SSE4
__asm	pminub		SSE1,				SSE3

__asm	pminub		SSE4,				SSE7
__asm	pmaxub		SSE5,				SSE7
__asm	pmaxub		SSE0,				SSE4
__asm	pminub		SSE1,				SSE5
__asm	SSE_RMOVE	SSE2,				SSE0
#if	(ISSE > 1) || defined(SHLUR)
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 1]				
#endif
__asm	pminub		SSE0,				SSE1
__asm	pmaxub		SSE2,				SSE1
#ifdef	SHLUR
		sharpen(SSE4, SSE0, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
__asm	SSE_MOVE	[edi + 1],			SSE4
#else
#if	ISSE > 1
__asm	pmaxub		SSE0,				SSE4
#else
__asm	pmaxub		SSE0,				[esi + ebx + 1]	
#endif
__asm	pminub		SSE0,				SSE2
__asm	SSE_MOVE	[edi + 1],			SSE0
#endif	// SHLUR
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif // MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 2]
__asm	SSE_RMOVE	SSE1,				SSE0
__asm	SSE3_MOVE	SSE4,				[esi + 2]
__asm	pminub		SSE0,				SSE7
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx]
__asm	SSE_RMOVE	SSE5,				SSE4
__asm	pmaxub		SSE1,				SSE7
__asm	pminub		SSE4,				SSE6
__asm	SSE3_MOVE	SSE2,				[esi + 1]
__asm	pmaxub		SSE5,				SSE6
__asm	pmaxub		SSE0,				SSE4
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 1]
__asm	SSE_RMOVE	SSE3,				SSE2
__asm	pminub		SSE1,				SSE5

__asm	pminub		SSE2,				SSE7
__asm	SSE3_MOVE	SSE4,				[esi + ebx]
__asm	pmaxub		SSE3,				SSE7
__asm	pmaxub		SSE0,				SSE2
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 2]
__asm	SSE_RMOVE	SSE5,				SSE4
__asm	pminub		SSE1,				SSE3

__asm	pminub		SSE4,				SSE7
__asm	pmaxub		SSE5,				SSE7
__asm	pmaxub		SSE0,				SSE4
__asm	pminub		SSE1,				SSE5
__asm	SSE_RMOVE	SSE2,				SSE0

#if		(ISSE > 1) || defined(SHLUR)
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE4,				[edi]
#else
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 1]	
#endif
#endif
#if		MODIFYPLUGIN > 0
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]		
#endif

__asm	pminub		SSE0,				SSE1
__asm	pmaxub		SSE2,				SSE1
#if		MODIFYPLUGIN > 0
__asm	pminub		SSE0,				SSE5
__asm	pmaxub		SSE2,				SSE5
#endif

#ifdef	SHLUR
		sharpen(SSE4, SSE0, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE4
#else
#if	ISSE > 1
__asm	pmaxub		SSE0,				SSE4
#else
#ifdef	MODIFYPLUGIN
__asm	pmaxub		SSE0,				[edi]
#else
__asm	pmaxub		SSE0,				[esi + ebx + 1]	
#endif
#endif
__asm	pminub		SSE0,				SSE2
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE0
#endif	// SHLUR
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	add			edi,				edx
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 2]
__asm	SSE_RMOVE	SSE1,				SSE0
__asm	SSE3_MOVE	SSE4,				[esi + 2]
__asm	pminub		SSE0,				SSE7
__asm	pmaxub		SSE1,				SSE7
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx]
__asm	SSE_RMOVE	SSE5,				SSE4
__asm	SSE3_MOVE	SSE2,				[esi + 1]
__asm	pminub		SSE4,				SSE6
__asm	pmaxub		SSE5,				SSE6
__asm	pmaxub		SSE0,				SSE4
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 1]
__asm	SSE_RMOVE	SSE3,				SSE2
__asm	pminub		SSE1,				SSE5

__asm	pminub		SSE2,				SSE7
__asm	SSE3_MOVE	SSE4,				[esi + ebx]
__asm	pmaxub		SSE3,				SSE7
__asm	pmaxub		SSE0,				SSE2
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 2]
__asm	SSE_RMOVE	SSE5,				SSE4
#ifndef	MODIFYPLUGIN
__asm	SSE_MOVE	[edi + 1],			SSE7
#endif
__asm	pminub		SSE1,				SSE3

__asm	pminub		SSE4,				SSE7
__asm	pmaxub		SSE5,				SSE7
__asm	pmaxub		SSE0,				SSE4
__asm	pminub		SSE1,				SSE5
__asm	SSE_RMOVE	SSE2,				SSE0

#if		(ISSE > 1) || defined(SHLUR)
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE4,				[edi]
#else
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 1]	
#endif
#endif
#if		MODIFYPLUGIN > 0
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]		
#endif
__asm	pminub		SSE0,				SSE1
__asm	pmaxub		SSE2,				SSE1
#if		MODIFYPLUGIN > 0
__asm	pminub		SSE0,				SSE5
__asm	pmaxub		SSE2,				SSE5
#endif

#ifdef	SHLUR
		sharpen(SSE4, SSE0, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
#ifdef	SHARPEN
__asm	add			esi,				spitch
#else
__asm	add			esi,				eax
#endif
__asm	SSE_MOVE	[edi],				SSE4
#else
#if	ISSE > 1
__asm	pmaxub		SSE0,				SSE4
#else
#ifdef	MODIFYPLUGIN
__asm	pmaxub		SSE0,				[edi]
#else
__asm	pmaxub		SSE0,				[esi + ebx + 1]	
#endif
#endif
__asm	pminub		SSE0,				SSE2
__asm	add			esi,				eax
__asm	SSE_MOVE	[edi],				SSE0
#endif	// SHLUR
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}

#ifdef	SHARPEN
void	SmartRGC(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height, int strength)
#else
void	SmartRGC(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
#endif
{
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
#ifdef	SHARPEN
__asm	mov			spitch,				eax
__asm	mov			eax,				strength
#endif
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE7,				[esi]
__asm	SSE3_MOVE	SSE6,				[esi + 1]
__asm	SSE_RMOVE	SSE0,				SSE7
__asm	SSE_RMOVE	SSE1,				SSE6
__asm	SSE3_MOVE	SSE5,				[esi + 2]
__asm	pminub		SSE0,				SSE6
__asm	SSE_RMOVE	SSE2,				SSE5
__asm	pmaxub		SSE1,				SSE7
__asm	pminub		SSE2,				SSE6
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 2]
__asm	pmaxub		SSE6,				SSE5
__asm	pmaxub		SSE0,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE4
__asm	pminub		SSE1,				SSE6
__asm	pminub		SSE3,				SSE5
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 2]
__asm	pmaxub		SSE5,				SSE4
__asm	pmaxub		SSE0,				SSE3
__asm	SSE_RMOVE	SSE2,				SSE6
__asm	pminub		SSE1,				SSE5
__asm	pminub		SSE2,				SSE4
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx + 1]
__asm	pmaxub		SSE4,				SSE6
__asm	pmaxub		SSE0,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE5
__asm	pminub		SSE1,				SSE4
__asm	pminub		SSE3,				SSE6
__asm	SSE3_MOVE	SSE4,				[esi + 2*ebx]
__asm	pmaxub		SSE6,				SSE5
__asm	pmaxub		SSE0,				SSE3
__asm	SSE_RMOVE	SSE2,				SSE4
__asm	pminub		SSE1,				SSE6
__asm	pminub		SSE2,				SSE5
__asm	SSE3_MOVE	SSE6,				[esi + ebx]
__asm	pmaxub		SSE5,				SSE4
__asm	pmaxub		SSE0,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE6
__asm	pminub		SSE1,				SSE5
__asm	movd		[edi],				SSE6
__asm	pminub		SSE3,				SSE4
__asm	SSE_RMOVE	SSE2,				SSE7
__asm	pmaxub		SSE4,				SSE6
__asm	pmaxub		SSE0,				SSE3
__asm	pminub		SSE1,				SSE4
__asm	pminub		SSE2,				SSE6
__asm	pmaxub		SSE7,				SSE6
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE1,				SSE7

__asm	SSE_RMOVE	SSE2,				SSE0
#if	(ISSE > 1) || defined(SHLUR)
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 1]				
#endif
__asm	pminub		SSE0,				SSE1
__asm	pmaxub		SSE2,				SSE1
#ifdef	SHLUR
		sharpen(SSE4, SSE0, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
__asm	SSE_MOVE	[edi + 1],			SSE4
#else
#if	ISSE > 1
__asm	pmaxub		SSE0,				SSE4
#else
__asm	pmaxub		SSE0,				[esi + ebx + 1]	
#endif
__asm	pminub		SSE0,				SSE2
__asm	SSE_MOVE	[edi + 1],			SSE0
#endif	// SHLUR
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif // MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
__asm	SSE3_MOVE	SSE7,				[esi]
__asm	SSE3_MOVE	SSE6,				[esi + 1]
__asm	SSE_RMOVE	SSE0,				SSE7
__asm	SSE_RMOVE	SSE1,				SSE6
__asm	SSE3_MOVE	SSE5,				[esi + 2]
__asm	pminub		SSE0,				SSE6
__asm	SSE_RMOVE	SSE2,				SSE5
__asm	pmaxub		SSE1,				SSE7
__asm	pminub		SSE2,				SSE6
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 2]
__asm	pmaxub		SSE6,				SSE5
__asm	pmaxub		SSE0,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE4
__asm	pminub		SSE1,				SSE6
__asm	pminub		SSE3,				SSE5
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 2]
__asm	pmaxub		SSE5,				SSE4
__asm	pmaxub		SSE0,				SSE3
__asm	SSE_RMOVE	SSE2,				SSE6
__asm	pminub		SSE1,				SSE5
__asm	pminub		SSE2,				SSE4
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx + 1]
__asm	pmaxub		SSE4,				SSE6
__asm	pmaxub		SSE0,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE5
__asm	pminub		SSE1,				SSE4
__asm	pminub		SSE3,				SSE6
__asm	SSE3_MOVE	SSE4,				[esi + 2*ebx]
__asm	pmaxub		SSE6,				SSE5
__asm	pmaxub		SSE0,				SSE3
__asm	SSE_RMOVE	SSE2,				SSE4
__asm	pminub		SSE1,				SSE6
__asm	pminub		SSE2,				SSE5
__asm	SSE3_MOVE	SSE6,				[esi + ebx]
__asm	pmaxub		SSE5,				SSE4
__asm	pmaxub		SSE0,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE6
__asm	pminub		SSE1,				SSE5
__asm	pminub		SSE3,				SSE4
__asm	SSE_RMOVE	SSE2,				SSE7
__asm	pmaxub		SSE4,				SSE6
__asm	pmaxub		SSE0,				SSE3
__asm	pminub		SSE1,				SSE4
__asm	pminub		SSE2,				SSE6
__asm	pmaxub		SSE7,				SSE6
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE1,				SSE7

__asm	SSE_RMOVE	SSE2,				SSE0
#if		(ISSE > 1) || defined(SHLUR)
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE4,				[edi]
#else
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 1]	
#endif
#endif
#if		MODIFYPLUGIN > 0
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]		
#endif

__asm	pminub		SSE0,				SSE1
__asm	pmaxub		SSE2,				SSE1
#if		MODIFYPLUGIN > 0
__asm	pminub		SSE0,				SSE5
__asm	pmaxub		SSE2,				SSE5
#endif

#ifdef	SHLUR
		sharpen(SSE4, SSE0, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE4
#else
#if	ISSE > 1
__asm	pmaxub		SSE0,				SSE4
#else
#ifdef	MODIFYPLUGIN
__asm	pmaxub		SSE0,				[edi]
#else
__asm	pmaxub		SSE0,				[esi + ebx + 1]	
#endif
#endif
__asm	pminub		SSE0,				SSE2
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE0
#endif	// SHLUR
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	SSE3_MOVE	SSE7,				[esi]
__asm	add			edi,				edx
__asm	SSE3_MOVE	SSE6,				[esi + 1]
__asm	SSE_RMOVE	SSE0,				SSE7
__asm	SSE_RMOVE	SSE1,				SSE6
__asm	SSE3_MOVE	SSE5,				[esi + 2]
__asm	pminub		SSE0,				SSE6
__asm	SSE_RMOVE	SSE2,				SSE5
__asm	pmaxub		SSE1,				SSE7
__asm	pminub		SSE2,				SSE6
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 2]
__asm	pmaxub		SSE6,				SSE5
__asm	pmaxub		SSE0,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE4
__asm	pminub		SSE1,				SSE6
__asm	SSE_MOVE	[edi + 1],			SSE4
__asm	pminub		SSE3,				SSE5
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 2]
__asm	pmaxub		SSE5,				SSE4
__asm	pmaxub		SSE0,				SSE3
__asm	SSE_RMOVE	SSE2,				SSE6
__asm	pminub		SSE1,				SSE5
__asm	pminub		SSE2,				SSE4
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx + 1]
__asm	pmaxub		SSE4,				SSE6
__asm	pmaxub		SSE0,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE5
__asm	pminub		SSE1,				SSE4
__asm	pminub		SSE3,				SSE6
__asm	SSE3_MOVE	SSE4,				[esi + 2*ebx]
__asm	pmaxub		SSE6,				SSE5
__asm	pmaxub		SSE0,				SSE3
__asm	SSE_RMOVE	SSE2,				SSE4
__asm	pminub		SSE1,				SSE6
__asm	pminub		SSE2,				SSE5
__asm	SSE3_MOVE	SSE6,				[esi + ebx]
__asm	pmaxub		SSE5,				SSE4
__asm	pmaxub		SSE0,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE6
__asm	pminub		SSE1,				SSE5
__asm	pminub		SSE3,				SSE4
__asm	SSE_RMOVE	SSE2,				SSE7
__asm	pmaxub		SSE4,				SSE6
__asm	pmaxub		SSE0,				SSE3
__asm	pminub		SSE1,				SSE4
__asm	pminub		SSE2,				SSE6
__asm	pmaxub		SSE7,				SSE6
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE1,				SSE7

__asm	SSE_RMOVE	SSE2,				SSE0
#if		(ISSE > 1) || defined(SHLUR)
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE4,				[edi]
#else
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 1]	
#endif
#endif
#if		MODIFYPLUGIN > 0
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]		
#endif
__asm	pminub		SSE0,				SSE1
__asm	pmaxub		SSE2,				SSE1
#if		MODIFYPLUGIN > 0
__asm	pminub		SSE0,				SSE5
__asm	pmaxub		SSE2,				SSE5
#endif

#ifdef	SHLUR
		sharpen(SSE4, SSE0, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
#ifdef	SHARPEN
__asm	add			esi,				spitch
#else
__asm	add			esi,				eax
#endif
__asm	SSE_MOVE	[edi],				SSE4
#else
#if	ISSE > 1
__asm	pmaxub		SSE0,				SSE4
#else
#ifdef	MODIFYPLUGIN
__asm	pmaxub		SSE0,				[edi]
#else
__asm	pmaxub		SSE0,				[esi + ebx + 1]	
#endif
#endif
__asm	pminub		SSE0,				SSE2
__asm	add			esi,				eax
__asm	SSE_MOVE	[edi],				SSE0
#endif	// SHLUR
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}

#ifdef	SHARPEN
void	SmartRGCL(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height, int strength)
#else
void	SmartRGCL(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
#endif
{
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
#ifdef	SHARPEN
__asm	mov			spitch,				eax
__asm	mov			eax,				strength
#endif
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE4,				[esi]
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx + 2]
__asm	SSE_RMOVE	SSE0,				SSE4
__asm	SSE_RMOVE	SSE1,				SSE5
__asm	SSE3_MOVE	SSE6,				[esi + 1]
__asm	pminub		SSE0,				SSE5
__asm	SSE_RMOVE	SSE2,				SSE6
__asm	pmaxub		SSE1,				SSE4
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 1]
__asm	pminub		SSE2,				SSE4
__asm	SSE_RMOVE	SSE3,				SSE7
__asm	pmaxub		SSE4,				SSE6
__asm	pminub		SSE3,				SSE5
__asm	pmaxub		SSE0,				SSE2
__asm	pmaxub		SSE5,				SSE7
__asm	pminub		SSE1,				SSE4
__asm	pmaxub		SSE0,				SSE3
__asm	pminub		SSE1,				SSE5
__asm	SSE3_MOVE	SSE4,				[esi + 2]
__asm	SSE_RMOVE	SSE2,				SSE6
__asm	SSE_RMOVE	SSE3,				SSE7
__asm	SSE_RMOVE	SSE5,				SSE4
__asm	pminub		SSE2,				SSE7
__asm	pmaxub		SSE3,				SSE6
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE5,				SSE6
__asm	pminub		SSE1,				SSE3
__asm	pmaxub		SSE6,				SSE4
__asm	pmaxub		SSE0,				SSE5
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx]
__asm	pminub		SSE1,				SSE6
__asm	SSE_RMOVE	SSE2,				SSE5
__asm	pminub		SSE5,				SSE7
__asm	pmaxub		SSE7,				SSE2
__asm	pmaxub		SSE0,				SSE5
__asm	pminub		SSE1,				SSE7
__asm	SSE_RMOVE	SSE5,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE4
__asm	SSE3_MOVE	SSE6,				[esi + ebx + 2]
__asm	pminub		SSE2,				SSE4
__asm	pmaxub		SSE3,				SSE5
__asm	SSE_RMOVE	SSE7,				SSE6
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE7,				SSE4
__asm	SSE3_MOVE	SSE2,				[esi + ebx]
__asm	pminub		SSE1,				SSE3
__asm	pmaxub		SSE4,				SSE6
__asm	SSE_RMOVE	SSE3,				SSE2
__asm	pmaxub		SSE0,				SSE7
__asm	movd		[edi],				SSE2
__asm	pminub		SSE3,				SSE5
__asm	pminub		SSE1,				SSE4
__asm	pmaxub		SSE5,				SSE2
__asm	pmaxub		SSE0,				SSE3
__asm	pminub		SSE1,				SSE5
__asm	SSE_RMOVE	SSE7,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE6
__asm	SSE3_MOVE	SSE4,				[esi + 2*ebx + 2]
__asm	pminub		SSE2,				SSE6
__asm	pmaxub		SSE3,				SSE7
__asm	SSE_RMOVE	SSE5,				SSE4
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE5,				SSE6
__asm	SSE3_MOVE	SSE2,				[esi]
__asm	pminub		SSE1,				SSE3
__asm	pmaxub		SSE6,				SSE4
__asm	pmaxub		SSE0,				SSE5
__asm	SSE_RMOVE	SSE3,				SSE2
__asm	pminub		SSE1,				SSE6
__asm	pminub		SSE2,				SSE7
__asm	pmaxub		SSE3,				SSE7
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE1,				SSE3

__asm	SSE_RMOVE	SSE2,				SSE0
#if	(ISSE > 1) || defined(SHLUR)
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 1]				
#endif
__asm	pminub		SSE0,				SSE1
__asm	pmaxub		SSE2,				SSE1
#ifdef	SHLUR
		sharpen(SSE4, SSE0, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
__asm	SSE_MOVE	[edi + 1],			SSE4
#else
#if	ISSE > 1
__asm	pmaxub		SSE0,				SSE4
#else
__asm	pmaxub		SSE0,				[esi + ebx + 1]	
#endif
__asm	pminub		SSE0,				SSE2
__asm	SSE_MOVE	[edi + 1],			SSE0
#endif	// SHLUR
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif // MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
__asm	SSE3_MOVE	SSE4,				[esi]
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx + 2]
__asm	SSE_RMOVE	SSE0,				SSE4
__asm	SSE_RMOVE	SSE1,				SSE5
__asm	SSE3_MOVE	SSE6,				[esi + 1]
__asm	pminub		SSE0,				SSE5
__asm	SSE_RMOVE	SSE2,				SSE6
__asm	pmaxub		SSE1,				SSE4
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 1]
__asm	pminub		SSE2,				SSE4
__asm	SSE_RMOVE	SSE3,				SSE7
__asm	pmaxub		SSE4,				SSE6
__asm	pminub		SSE3,				SSE5
__asm	pmaxub		SSE0,				SSE2
__asm	pmaxub		SSE5,				SSE7
__asm	pminub		SSE1,				SSE4
__asm	pmaxub		SSE0,				SSE3
__asm	pminub		SSE1,				SSE5
__asm	SSE3_MOVE	SSE4,				[esi + 2]
__asm	SSE_RMOVE	SSE2,				SSE6
__asm	SSE_RMOVE	SSE3,				SSE7
__asm	SSE_RMOVE	SSE5,				SSE4
__asm	pminub		SSE2,				SSE7
__asm	pmaxub		SSE3,				SSE6
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE5,				SSE6
__asm	pminub		SSE1,				SSE3
__asm	pmaxub		SSE6,				SSE4
__asm	pmaxub		SSE0,				SSE5
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx]
__asm	pminub		SSE1,				SSE6
__asm	SSE_RMOVE	SSE2,				SSE5
__asm	pminub		SSE5,				SSE7
__asm	pmaxub		SSE7,				SSE2
__asm	pmaxub		SSE0,				SSE5
__asm	pminub		SSE1,				SSE7
__asm	SSE_RMOVE	SSE5,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE4
__asm	SSE3_MOVE	SSE6,				[esi + ebx + 2]
__asm	pminub		SSE2,				SSE4
__asm	pmaxub		SSE3,				SSE5
__asm	SSE_RMOVE	SSE7,				SSE6
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE7,				SSE4
__asm	SSE3_MOVE	SSE2,				[esi + ebx]
__asm	pminub		SSE1,				SSE3
__asm	pmaxub		SSE4,				SSE6
__asm	SSE_RMOVE	SSE3,				SSE2
__asm	pmaxub		SSE0,				SSE7
__asm	pminub		SSE3,				SSE5
__asm	pminub		SSE1,				SSE4
__asm	pmaxub		SSE5,				SSE2
__asm	pmaxub		SSE0,				SSE3
__asm	pminub		SSE1,				SSE5
__asm	SSE_RMOVE	SSE7,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE6
__asm	SSE3_MOVE	SSE4,				[esi + 2*ebx + 2]
__asm	pminub		SSE2,				SSE6
__asm	pmaxub		SSE3,				SSE7
__asm	SSE_RMOVE	SSE5,				SSE4
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE5,				SSE6
__asm	SSE3_MOVE	SSE2,				[esi]
__asm	pminub		SSE1,				SSE3
__asm	pmaxub		SSE6,				SSE4
__asm	pmaxub		SSE0,				SSE5
__asm	SSE_RMOVE	SSE3,				SSE2
__asm	pminub		SSE1,				SSE6
__asm	pminub		SSE2,				SSE7
__asm	pmaxub		SSE3,				SSE7
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE1,				SSE3

__asm	SSE_RMOVE	SSE2,				SSE0

#if		(ISSE > 1) || defined(SHLUR)
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE4,				[edi]
#else
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 1]	
#endif
#endif
#if		MODIFYPLUGIN > 0
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]		
#endif

__asm	pminub		SSE0,				SSE1
__asm	pmaxub		SSE2,				SSE1
#if		MODIFYPLUGIN > 0
__asm	pminub		SSE0,				SSE5
__asm	pmaxub		SSE2,				SSE5
#endif

#ifdef	SHLUR
		sharpen(SSE4, SSE0, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE4
#else
#if	ISSE > 1
__asm	pmaxub		SSE0,				SSE4
#else
#ifdef	MODIFYPLUGIN
__asm	pmaxub		SSE0,				[edi]
#else
__asm	pmaxub		SSE0,				[esi + ebx + 1]	
#endif
#endif
__asm	pminub		SSE0,				SSE2
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE0
#endif	// SHLUR
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	SSE3_MOVE	SSE4,				[esi]
__asm	add			edi,				edx
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx + 2]
__asm	SSE_RMOVE	SSE0,				SSE4
__asm	SSE_RMOVE	SSE1,				SSE5
__asm	SSE3_MOVE	SSE6,				[esi + 1]
__asm	pminub		SSE0,				SSE5
__asm	SSE_RMOVE	SSE2,				SSE6
__asm	pmaxub		SSE1,				SSE4
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 1]
__asm	pminub		SSE2,				SSE4
__asm	SSE_RMOVE	SSE3,				SSE7
__asm	pmaxub		SSE4,				SSE6
__asm	pminub		SSE3,				SSE5
__asm	pmaxub		SSE0,				SSE2
__asm	pmaxub		SSE5,				SSE7
__asm	pminub		SSE1,				SSE4
__asm	pmaxub		SSE0,				SSE3
__asm	pminub		SSE1,				SSE5
__asm	SSE3_MOVE	SSE4,				[esi + 2]
__asm	SSE_RMOVE	SSE2,				SSE6
__asm	SSE_RMOVE	SSE3,				SSE7
__asm	SSE_RMOVE	SSE5,				SSE4
__asm	pminub		SSE2,				SSE7
__asm	pmaxub		SSE3,				SSE6
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE5,				SSE6
__asm	pminub		SSE1,				SSE3
__asm	pmaxub		SSE6,				SSE4
__asm	pmaxub		SSE0,				SSE5
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx]
__asm	pminub		SSE1,				SSE6
__asm	SSE_RMOVE	SSE2,				SSE5
__asm	pminub		SSE5,				SSE7
__asm	pmaxub		SSE7,				SSE2
__asm	pmaxub		SSE0,				SSE5
__asm	pminub		SSE1,				SSE7
__asm	SSE_RMOVE	SSE5,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE4
__asm	SSE3_MOVE	SSE6,				[esi + ebx + 2]
__asm	pminub		SSE2,				SSE4
__asm	pmaxub		SSE3,				SSE5
__asm	SSE_RMOVE	SSE7,				SSE6
__asm	pmaxub		SSE0,				SSE2
__asm	SSE_MOVE	[edi + 1],			SSE6
__asm	pminub		SSE7,				SSE4
__asm	SSE3_MOVE	SSE2,				[esi + ebx]
__asm	pminub		SSE1,				SSE3
__asm	pmaxub		SSE4,				SSE6
__asm	SSE_RMOVE	SSE3,				SSE2
__asm	pmaxub		SSE0,				SSE7
__asm	pminub		SSE3,				SSE5
__asm	pminub		SSE1,				SSE4
__asm	pmaxub		SSE5,				SSE2
__asm	pmaxub		SSE0,				SSE3
__asm	pminub		SSE1,				SSE5
__asm	SSE_RMOVE	SSE7,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE6
__asm	SSE3_MOVE	SSE4,				[esi + 2*ebx + 2]
__asm	pminub		SSE2,				SSE6
__asm	pmaxub		SSE3,				SSE7
__asm	SSE_RMOVE	SSE5,				SSE4
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE5,				SSE6
__asm	SSE3_MOVE	SSE2,				[esi]
__asm	pminub		SSE1,				SSE3
__asm	pmaxub		SSE6,				SSE4
__asm	pmaxub		SSE0,				SSE5
__asm	SSE_RMOVE	SSE3,				SSE2
__asm	pminub		SSE1,				SSE6
__asm	pminub		SSE2,				SSE7
__asm	pmaxub		SSE3,				SSE7
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE1,				SSE3

__asm	SSE_RMOVE	SSE2,				SSE0

#if		(ISSE > 1) || defined(SHLUR)
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE4,				[edi]
#else
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 1]	
#endif
#endif
#if		MODIFYPLUGIN > 0
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]		
#endif
__asm	pminub		SSE0,				SSE1
__asm	pmaxub		SSE2,				SSE1
#if		MODIFYPLUGIN > 0
__asm	pminub		SSE0,				SSE5
__asm	pmaxub		SSE2,				SSE5
#endif

#ifdef	SHLUR
		sharpen(SSE4, SSE0, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
#ifdef	SHARPEN
__asm	add			esi,				spitch
#else
__asm	add			esi,				eax
#endif
__asm	SSE_MOVE	[edi],				SSE4
#else
#if	ISSE > 1
__asm	pmaxub		SSE0,				SSE4
#else
#ifdef	MODIFYPLUGIN
__asm	pmaxub		SSE0,				[edi]
#else
__asm	pmaxub		SSE0,				[esi + ebx + 1]	
#endif
#endif
__asm	pminub		SSE0,				SSE2
__asm	add			esi,				eax
__asm	SSE_MOVE	[edi],				SSE0
#endif	// SHLUR
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}

#ifdef	SHARPEN
void	SmartRGCL2(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height, int strength)
#else
void	SmartRGCL2(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
#endif
{
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
#ifdef	SHARPEN
__asm	mov			spitch,				eax
__asm	mov			eax,				strength
#endif
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE7,				[esi]
__asm	SSE3_MOVE	SSE6,				[esi + 1]
__asm	SSE_RMOVE	SSE0,				SSE7
__asm	SSE_RMOVE	SSE1,				SSE6
__asm	SSE3_MOVE	SSE5,				[esi + 2]
__asm	pminub		SSE0,				SSE6
__asm	SSE_RMOVE	SSE2,				SSE5
__asm	pmaxub		SSE1,				SSE7
__asm	pminub		SSE2,				SSE6
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 2]
__asm	pmaxub		SSE6,				SSE5
__asm	pmaxub		SSE0,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE4
__asm	pminub		SSE1,				SSE6
__asm	pminub		SSE3,				SSE5
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 2]
__asm	pmaxub		SSE5,				SSE4
__asm	pmaxub		SSE0,				SSE3
__asm	SSE_RMOVE	SSE2,				SSE6
__asm	pminub		SSE1,				SSE5
__asm	pminub		SSE2,				SSE4
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx + 1]
__asm	pmaxub		SSE4,				SSE6
__asm	pmaxub		SSE0,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE5
__asm	pminub		SSE1,				SSE4
__asm	pminub		SSE3,				SSE6
__asm	SSE3_MOVE	SSE4,				[esi + 2*ebx]
__asm	pmaxub		SSE6,				SSE5
__asm	pmaxub		SSE0,				SSE3
__asm	SSE_RMOVE	SSE2,				SSE4
__asm	pminub		SSE1,				SSE6
__asm	pminub		SSE2,				SSE5
__asm	SSE3_MOVE	SSE6,				[esi + ebx]
__asm	pmaxub		SSE5,				SSE4
__asm	pmaxub		SSE0,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE6
__asm	pminub		SSE1,				SSE5
__asm	movd		[edi],				SSE6
__asm	pminub		SSE3,				SSE4
__asm	SSE_RMOVE	SSE2,				SSE7
__asm	pmaxub		SSE4,				SSE6
__asm	pmaxub		SSE0,				SSE3
__asm	pminub		SSE1,				SSE4
__asm	pminub		SSE2,				SSE6
__asm	pmaxub		SSE7,				SSE6
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE1,				SSE7

__asm	SSE3_MOVE	SSE2,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 2]
__asm	SSE_RMOVE	SSE3,				SSE2
__asm	SSE3_MOVE	SSE4,				[esi + 2]
__asm	pminub		SSE2,				SSE7
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx]
__asm	SSE_RMOVE	SSE5,				SSE4
__asm	pmaxub		SSE3,				SSE7
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE1,				SSE3
__asm	pminub		SSE4,				SSE6
__asm	SSE3_MOVE	SSE2,				[esi + 1]
__asm	pmaxub		SSE5,				SSE6
__asm	pmaxub		SSE0,				SSE4
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 1]
__asm	SSE_RMOVE	SSE3,				SSE2
__asm	pminub		SSE1,				SSE5
__asm	pminub		SSE2,				SSE7
__asm	SSE3_MOVE	SSE4,				[esi + ebx]
__asm	pmaxub		SSE3,				SSE7
__asm	pmaxub		SSE0,				SSE2
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 2]
__asm	SSE_RMOVE	SSE5,				SSE4
__asm	pminub		SSE1,				SSE3
__asm	pminub		SSE4,				SSE7
__asm	pmaxub		SSE5,				SSE7
__asm	pmaxub		SSE0,				SSE4
__asm	pminub		SSE1,				SSE5

__asm	SSE_RMOVE	SSE2,				SSE0
#if	(ISSE > 1) || defined(SHLUR)
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 1]				
#endif
__asm	pminub		SSE0,				SSE1
__asm	pmaxub		SSE2,				SSE1
#ifdef	SHLUR
		sharpen(SSE4, SSE0, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
__asm	SSE_MOVE	[edi + 1],			SSE4
#else
#if	ISSE > 1
__asm	pmaxub		SSE0,				SSE4
#else
__asm	pmaxub		SSE0,				[esi + ebx + 1]	
#endif
__asm	pminub		SSE0,				SSE2
__asm	SSE_MOVE	[edi + 1],			SSE0
#endif	// SHLUR
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif // MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
__asm	SSE3_MOVE	SSE7,				[esi]
__asm	SSE3_MOVE	SSE6,				[esi + 1]
__asm	SSE_RMOVE	SSE0,				SSE7
__asm	SSE_RMOVE	SSE1,				SSE6
__asm	SSE3_MOVE	SSE5,				[esi + 2]
__asm	pminub		SSE0,				SSE6
__asm	SSE_RMOVE	SSE2,				SSE5
__asm	pmaxub		SSE1,				SSE7
__asm	pminub		SSE2,				SSE6
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 2]
__asm	pmaxub		SSE6,				SSE5
__asm	pmaxub		SSE0,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE4
__asm	pminub		SSE1,				SSE6
__asm	pminub		SSE3,				SSE5
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 2]
__asm	pmaxub		SSE5,				SSE4
__asm	pmaxub		SSE0,				SSE3
__asm	SSE_RMOVE	SSE2,				SSE6
__asm	pminub		SSE1,				SSE5
__asm	pminub		SSE2,				SSE4
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx + 1]
__asm	pmaxub		SSE4,				SSE6
__asm	pmaxub		SSE0,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE5
__asm	pminub		SSE1,				SSE4
__asm	pminub		SSE3,				SSE6
__asm	SSE3_MOVE	SSE4,				[esi + 2*ebx]
__asm	pmaxub		SSE6,				SSE5
__asm	pmaxub		SSE0,				SSE3
__asm	SSE_RMOVE	SSE2,				SSE4
__asm	pminub		SSE1,				SSE6
__asm	pminub		SSE2,				SSE5
__asm	SSE3_MOVE	SSE6,				[esi + ebx]
__asm	pmaxub		SSE5,				SSE4
__asm	pmaxub		SSE0,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE6
__asm	pminub		SSE1,				SSE5
__asm	pminub		SSE3,				SSE4
__asm	SSE_RMOVE	SSE2,				SSE7
__asm	pmaxub		SSE4,				SSE6
__asm	pmaxub		SSE0,				SSE3
__asm	pminub		SSE1,				SSE4
__asm	pminub		SSE2,				SSE6
__asm	pmaxub		SSE7,				SSE6
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE1,				SSE7

__asm	SSE3_MOVE	SSE2,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 2]
__asm	SSE_RMOVE	SSE3,				SSE2
__asm	SSE3_MOVE	SSE4,				[esi + 2]
__asm	pminub		SSE2,				SSE7
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx]
__asm	SSE_RMOVE	SSE5,				SSE4
__asm	pmaxub		SSE3,				SSE7
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE1,				SSE3
__asm	pminub		SSE4,				SSE6
__asm	SSE3_MOVE	SSE2,				[esi + 1]
__asm	pmaxub		SSE5,				SSE6
__asm	pmaxub		SSE0,				SSE4
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 1]
__asm	SSE_RMOVE	SSE3,				SSE2
__asm	pminub		SSE1,				SSE5
__asm	pminub		SSE2,				SSE7
__asm	SSE3_MOVE	SSE4,				[esi + ebx]
__asm	pmaxub		SSE3,				SSE7
__asm	pmaxub		SSE0,				SSE2
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 2]
__asm	SSE_RMOVE	SSE5,				SSE4
__asm	pminub		SSE1,				SSE3
__asm	pminub		SSE4,				SSE7
__asm	pmaxub		SSE5,				SSE7
__asm	pmaxub		SSE0,				SSE4
__asm	pminub		SSE1,				SSE5

__asm	SSE_RMOVE	SSE2,				SSE0
#if		(ISSE > 1) || defined(SHLUR)
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE4,				[edi]
#else
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 1]	
#endif
#endif
#if		MODIFYPLUGIN > 0
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]		
#endif

__asm	pminub		SSE0,				SSE1
__asm	pmaxub		SSE2,				SSE1
#if		MODIFYPLUGIN > 0
__asm	pminub		SSE0,				SSE5
__asm	pmaxub		SSE2,				SSE5
#endif

#ifdef	SHLUR
		sharpen(SSE4, SSE0, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE4
#else
#if	ISSE > 1
__asm	pmaxub		SSE0,				SSE4
#else
#ifdef	MODIFYPLUGIN
__asm	pmaxub		SSE0,				[edi]
#else
__asm	pmaxub		SSE0,				[esi + ebx + 1]	
#endif
#endif
__asm	pminub		SSE0,				SSE2
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE0
#endif	// SHLUR
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
__asm	SSE3_MOVE	SSE7,				[esi]
__asm	SSE3_MOVE	SSE6,				[esi + 1]
__asm	SSE_RMOVE	SSE0,				SSE7
__asm	SSE_RMOVE	SSE1,				SSE6
__asm	SSE3_MOVE	SSE5,				[esi + 2]
__asm	pminub		SSE0,				SSE6
__asm	SSE_RMOVE	SSE2,				SSE5
__asm	pmaxub		SSE1,				SSE7
__asm	pminub		SSE2,				SSE6
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 2]
__asm	pmaxub		SSE6,				SSE5
__asm	pmaxub		SSE0,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE4
__asm	pminub		SSE1,				SSE6
__asm	pminub		SSE3,				SSE5
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 2]
__asm	pmaxub		SSE5,				SSE4
__asm	pmaxub		SSE0,				SSE3
__asm	SSE_RMOVE	SSE2,				SSE6
__asm	pminub		SSE1,				SSE5
__asm	pminub		SSE2,				SSE4
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx + 1]
__asm	pmaxub		SSE4,				SSE6
__asm	pmaxub		SSE0,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE5
__asm	pminub		SSE1,				SSE4
__asm	pminub		SSE3,				SSE6
__asm	SSE3_MOVE	SSE4,				[esi + 2*ebx]
__asm	pmaxub		SSE6,				SSE5
__asm	pmaxub		SSE0,				SSE3
__asm	SSE_RMOVE	SSE2,				SSE4
__asm	pminub		SSE1,				SSE6
__asm	pminub		SSE2,				SSE5
__asm	SSE3_MOVE	SSE6,				[esi + ebx]
__asm	pmaxub		SSE5,				SSE4
__asm	pmaxub		SSE0,				SSE2
__asm	SSE_RMOVE	SSE3,				SSE6
__asm	pminub		SSE1,				SSE5
__asm	pminub		SSE3,				SSE4
__asm	SSE_RMOVE	SSE2,				SSE7
__asm	pmaxub		SSE4,				SSE6
__asm	pmaxub		SSE0,				SSE3
__asm	pminub		SSE1,				SSE4
__asm	pminub		SSE2,				SSE6
__asm	pmaxub		SSE7,				SSE6
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE1,				SSE7

__asm	SSE3_MOVE	SSE2,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 2]
__asm	SSE_RMOVE	SSE3,				SSE2
__asm	SSE3_MOVE	SSE4,				[esi + 2]
__asm	pminub		SSE2,				SSE7
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx]
__asm	SSE_RMOVE	SSE5,				SSE4
__asm	pmaxub		SSE3,				SSE7
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE1,				SSE3
__asm	pminub		SSE4,				SSE6
__asm	SSE3_MOVE	SSE2,				[esi + 1]
__asm	pmaxub		SSE5,				SSE6
__asm	pmaxub		SSE0,				SSE4
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 1]
__asm	SSE_RMOVE	SSE3,				SSE2
__asm	pminub		SSE1,				SSE5
__asm	pminub		SSE2,				SSE7
__asm	SSE3_MOVE	SSE4,				[esi + ebx]
__asm	pmaxub		SSE3,				SSE7
__asm	pmaxub		SSE0,				SSE2
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 2]
__asm	SSE_RMOVE	SSE5,				SSE4
__asm	pminub		SSE1,				SSE3
__asm	pminub		SSE4,				SSE7
__asm	pmaxub		SSE5,				SSE7
__asm	pmaxub		SSE0,				SSE4
__asm	SSE_MOVE	[edi + 1],			SSE7
__asm	pminub		SSE1,				SSE5

__asm	SSE_RMOVE	SSE2,				SSE0

#if		(ISSE > 1) || defined(SHLUR)
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE4,				[edi]
#else
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 1]	
#endif
#endif
#if		MODIFYPLUGIN > 0
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]		
#endif
__asm	pminub		SSE0,				SSE1
__asm	pmaxub		SSE2,				SSE1
#if		MODIFYPLUGIN > 0
__asm	pminub		SSE0,				SSE5
__asm	pmaxub		SSE2,				SSE5
#endif

#ifdef	SHLUR
		sharpen(SSE4, SSE0, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
#ifdef	SHARPEN
__asm	add			esi,				spitch
#else
__asm	add			esi,				eax
#endif
__asm	SSE_MOVE	[edi],				SSE4
#else
#if	ISSE > 1
__asm	pmaxub		SSE0,				SSE4
#else
#ifdef	MODIFYPLUGIN
__asm	pmaxub		SSE0,				[edi]
#else
__asm	pmaxub		SSE0,				[esi + ebx + 1]	
#endif
#endif
__asm	pminub		SSE0,				SSE2
__asm	add			esi,				eax
__asm	SSE_MOVE	[edi],				SSE0
#endif	// SHLUR
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}

#ifndef	MODIFYPLUGIN

#define	SmartAvgs(upavg, downavg, mem1, mem2, bias, reg2)\
__asm	SSE3_MOVE	downavg,			mem1		\
__asm	SSE3_MOVE	reg2,				mem2		\
__asm	SSE_RMOVE	upavg,				downavg		\
__asm	psubusb		downavg,			bias		\
__asm	pavgb		upavg,				reg2		\
__asm	pavgb		downavg,			reg2

#ifdef	MODIFYPLUGIN
#define	SmartAvgsW(upavg, downavg, wmem, mem1, mem2, bias, reg2)\
		SmartAvgs(upavg, downavg, mem1, mem2, bias, reg2)
#else
#define	SmartAvgsW(upavg, downavg, wmem, mem1, mem2, bias, reg2, mwrite)\
__asm	SSE3_MOVE	downavg,			mem1		\
__asm	SSE3_MOVE	reg2,				mem2		\
__asm	SSE_RMOVE	upavg,				downavg		\
__asm	psubusb		downavg,			bias		\
__asm	pavgb		upavg,				reg2		\
__asm	pavgb		downavg,			reg2		\
__asm	mwrite		wmem,				reg2
#endif

#ifdef	SHARPEN
void	SmartAvgRGs(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height, int strength)
#else
void	SmartAvgRGs(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
#endif
{
#ifdef	SHARPEN
// SSE5 is also used for a different purpose #ifdef MODIFYPLUGIN
__asm	mov			ebx,				strength
__asm	SSE_RMOVE	SSE4,				rshift[ebx]
__asm	SSE_RMOVE	SSE5,				shift_mask[ebx]
#endif
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
__asm	SSE_RMOVE	SSE7,				fconvolution_bias				
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
__asm	align		16
__asm	column_loop:
		SmartAvgs(SSE0, SSE1, [esi], [esi + 2*ebx + 2], SSE7, SSE6)
		SmartAvgs(SSE2, SSE3, [esi + 2], [esi + 2*ebx], SSE7, SSE6)
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE1,				SSE3
		SmartAvgs(SSE2, SSE3, [esi + 1], [esi + 2*ebx + 1], SSE7, SSE6)
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE1,				SSE3
		SmartAvgsW(SSE2, SSE3, [edi], [esi + ebx + 2], [esi + ebx], SSE7, SSE6, movd)
__asm	pmaxub		SSE0,				SSE2
#if		(ISSE > 1) || defined(SHLUR)
__asm	SSE3_MOVE	SSE6,				[esi + ebx + 1]				
#endif
__asm	pminub		SSE1,				SSE3
#ifdef	SHLUR
		sharpen(SSE6, SSE1, SSE0, SSE4, SSE5, SSE3, SSE2)
__asm	SSE_MOVE	[edi + 1],			SSE6
#else
#if	ISSE > 1
__asm	pminub		SSE0,				SSE6
#else
__asm	pminub		SSE0,				[esi + ebx + 1]	
#endif
__asm	pmaxub		SSE0,				SSE1
__asm	SSE_MOVE	[edi + 1],			SSE0
#endif	// SHLUR
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif // MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
		SmartAvgs(SSE0, SSE1, [esi], [esi + 2*ebx + 2], SSE7, SSE6)
		SmartAvgs(SSE2, SSE3, [esi + 2], [esi + 2*ebx], SSE7, SSE6)
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE1,				SSE3
		SmartAvgs(SSE2, SSE3, [esi + 1], [esi + 2*ebx + 1], SSE7, SSE6)
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE1,				SSE3
		SmartAvgs(SSE2, SSE3, [esi + ebx], [esi + ebx + 2], SSE7, SSE6)
#ifdef		MODIFYPLUGIN
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]		
#endif
__asm	pmaxub		SSE0,				SSE2
#if		(ISSE > 1) || defined(SHLUR)
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE6,				[edi]
#else
__asm	SSE3_MOVE	SSE6,				[esi + ebx + 1]	
#endif
#endif
__asm	pminub		SSE1,				SSE3
#ifdef		MODIFYPLUGIN
__asm	pmaxub		SSE0,				SSE5
__asm	pminub		SSE1,				SSE5
#endif
#ifdef	SHLUR
		sharpen(SSE6, SSE1, SSE0, SSE4, SSE5, SSE3, SSE2)
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE6
#else
#if	ISSE > 1
__asm	pminub		SSE0,				SSE6
#else
__asm	pminub		SSE0,				[esi + ebx + 1]	
#endif
__asm	pmaxub		SSE0,				SSE1
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE0
#endif	// SHLUR
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
		SmartAvgs(SSE0, SSE1, [esi], [esi + 2*ebx + 2], SSE7, SSE6)
		SmartAvgs(SSE2, SSE3, [esi + 2], [esi + 2*ebx], SSE7, SSE6)
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE1,				SSE3
		SmartAvgs(SSE2, SSE3, [esi + 1], [esi + 2*ebx + 1], SSE7, SSE6)
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE1,				SSE3
		SmartAvgsW(SSE2, SSE3, [edi + 1], [esi + ebx], [esi + ebx + 2], SSE7, SSE6, SSE_MOVE)
#ifdef		MODIFYPLUGIN
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]		
#endif
__asm	pmaxub		SSE0,				SSE2
#if		(ISSE > 1) || defined(SHLUR)
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE6,				[edi]
#else
__asm	SSE3_MOVE	SSE6,				[esi + ebx + 1]	
#endif
#endif
__asm	pminub		SSE1,				SSE3
#ifdef		MODIFYPLUGIN
__asm	pmaxub		SSE0,				SSE5
__asm	pminub		SSE1,				SSE5
#endif
#ifdef	SHLUR
		sharpen(SSE6, SSE1, SSE0, SSE4, SSE5, SSE3, SSE2)
__asm	add			esi,				eax
__asm	SSE_MOVE	[edi],				SSE6
#else
#if	ISSE > 1
__asm	pminub		SSE0,				SSE6
#else
__asm	pminub		SSE0,				[esi + ebx + 1]	
#endif
__asm	pmaxub		SSE0,				SSE1
__asm	add			esi,				eax
__asm	SSE_MOVE	[edi],				SSE0
#endif	// SHLUR
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}

#if	ISSE > 1
#define	SmartAvgf(avg1, avg2, mem1, mem2, mem3, mem4, reg1, reg2)\
__asm	SSE3_MOVE	avg1,				mem1		\
__asm	SSE3_MOVE	avg2,				mem3		\
__asm	SSE3_MOVE	reg1,				mem2		\
__asm	SSE3_MOVE	reg2,				mem4		\
__asm	pavgb		avg1,				reg1		\
__asm	pavgb		avg2,				reg2
#else
#define	SmartAvgf(avg1, avg2, mem1, mem2, mem3, mem4, reg1, reg2)\
__asm	SSE3_MOVE	avg1,				mem1		\
__asm	SSE3_MOVE	avg2,				mem3		\
__asm	pavgb		avg1,				mem2		\
__asm	pavgb		avg2,				mem4
#endif

#ifdef	MODIFYPLUGIN
#define	SmartAvgfW(avg1, avg2, wmem, mem1, mem2, mem3, mem4, reg1, reg2)\
		SmartAvgf(avg1, avg2, mem1, mem2, mem3, mem4, reg1, reg2)
#else
#if	ISSE > 1
#define	SmartAvgfW(avg1, avg2, wmem, mem1, mem2, mem3, mem4, reg1, reg2, mwrite)\
__asm	SSE3_MOVE	avg1,				mem1		\
__asm	SSE3_MOVE	avg2,				mem3		\
__asm	SSE3_MOVE	reg1,				mem2		\
__asm	SSE3_MOVE	reg2,				mem4		\
__asm	pavgb		avg1,				reg1		\
__asm	pavgb		avg2,				reg2		\
__asm	mwrite		wmem,				reg2
#else
#define	SmartAvgfW(avg1, avg2, wmem, mem4, mem3, mem2, mem1, reg1, reg2, mwrite)\
__asm	SSE3_MOVE	avg1,				mem1		\
__asm	SSE3_MOVE	avg2,				mem3		\
__asm	mwrite		wmem,				avg1		\
__asm	pavgb		avg2,				mem4		\
__asm	pavgb		avg1,				mem2
#endif
#endif

#ifdef	SHARPEN
void	SmartAvgRGf(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height, int strength)
#else
void	SmartAvgRGf(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
#endif
{
#ifdef	SHARPEN
__asm	mov			ebx,				strength
__asm	SSE_RMOVE	SSE4,				rshift[ebx]
__asm	SSE_RMOVE	SSE7,				shift_mask[ebx]
#endif
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
__asm	align		16
__asm	column_loop:
		SmartAvgf(SSE0, SSE3, [esi], [esi + 2*ebx + 2], [esi + 2], [esi + 2*ebx], SSE5, SSE6)
__asm	SSE_RMOVE	SSE1,				SSE0		
__asm	pmaxub		SSE0,				SSE3
__asm	pminub		SSE1,				SSE3
		SmartAvgfW(SSE2, SSE3, [edi], [esi + 1], [esi + 2*ebx + 1], [esi + ebx + 2], [esi + ebx], SSE5, SSE6, movd)
__asm	pmaxub		SSE0,				SSE2
__asm	pminub		SSE1,				SSE3
#if		(ISSE > 1) || defined(SHLUR)
__asm	SSE3_MOVE	SSE6,				[esi + ebx + 1]				
#endif
__asm	pmaxub		SSE0,				SSE3
__asm	pminub		SSE1,				SSE2
#ifdef	SHLUR
		sharpen(SSE6, SSE1, SSE0, SSE4, SSE7, SSE2, SSE3)
__asm	SSE_MOVE	[edi + 1],			SSE6
#else
#if	ISSE > 1
__asm	pminub		SSE0,				SSE6
#else
__asm	pminub		SSE0,				[esi + ebx + 1]	
#endif
__asm	pmaxub		SSE0,				SSE1
__asm	SSE_MOVE	[edi + 1],			SSE0
#endif	// SHLUR
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif // MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
		SmartAvgf(SSE0, SSE3, [esi], [esi + 2*ebx + 2], [esi + 2], [esi + 2*ebx], SSE5, SSE6)
__asm	SSE_RMOVE	SSE1,				SSE0		
__asm	pmaxub		SSE0,				SSE3
__asm	pminub		SSE1,				SSE3
		SmartAvgf(SSE2, SSE3, [esi + 1], [esi + 2*ebx + 1], [esi + ebx], [esi + ebx + 2], SSE5, SSE6)
__asm	pmaxub		SSE0,				SSE2
#ifdef		MODIFYPLUGIN
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]		
#endif
__asm	pminub		SSE1,				SSE3
#if		(ISSE > 1) || defined(SHLUR)
#ifdef		MODIFYPLUGIN
__asm	SSE3_MOVE	SSE6,				[edi]	
#else
__asm	SSE3_MOVE	SSE6,				[esi + ebx + 1]
#endif
#endif
__asm	pmaxub		SSE0,				SSE3
__asm	pminub		SSE1,				SSE2
#ifdef		MODIFYPLUGIN
__asm	pmaxub		SSE0,				SSE5
__asm	pminub		SSE1,				SSE5
#endif
#ifdef	SHLUR
		sharpen(SSE6, SSE1, SSE0, SSE4, SSE7, SSE2, SSE3)
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE6
#else
#if	ISSE > 1
__asm	pminub		SSE0,				SSE6
#else
#ifdef		MODIFYPLUGIN
__asm	pminub		SSE0,				[edi]	
#else
__asm	pminub		SSE0,				[esi + ebx + 1]	
#endif
#endif	// SHLUR
__asm	pmaxub		SSE0,				SSE1
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE0
#endif
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
		SmartAvgf(SSE0, SSE3, [esi], [esi + 2*ebx + 2], [esi + 2], [esi + 2*ebx], SSE5, SSE6)
__asm	SSE_RMOVE	SSE1,				SSE0		
__asm	pmaxub		SSE0,				SSE3
__asm	pminub		SSE1,				SSE3
		SmartAvgfW(SSE2, SSE3, [edi + 1], [esi + 1], [esi + 2*ebx + 1], [esi + ebx], [esi + ebx + 2], SSE5, SSE6, SSE_MOVE)
__asm	pmaxub		SSE0,				SSE2
#ifdef		MODIFYPLUGIN
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]		
#endif
__asm	pminub		SSE1,				SSE3
#if		(ISSE > 1) || defined(SHLUR)
#ifdef		MODIFYPLUGIN
__asm	SSE3_MOVE	SSE6,				[edi]	
#else
__asm	SSE3_MOVE	SSE6,				[esi + ebx + 1]	
#endif
#endif
__asm	pmaxub		SSE0,				SSE3
__asm	pminub		SSE1,				SSE2
#ifdef		MODIFYPLUGIN
__asm	pmaxub		SSE0,				SSE5
__asm	pminub		SSE1,				SSE5
#endif
#ifdef	SHLUR
		sharpen(SSE6, SSE1, SSE0, SSE4, SSE7, SSE2, SSE3)
__asm	add			esi,				eax
__asm	SSE_MOVE	[edi],				SSE6
#else
#if	ISSE > 1
__asm	pminub		SSE0,				SSE6
#else
#ifdef		MODIFYPLUGIN
__asm	pminub		SSE0,				[edi]	
#else
__asm	pminub		SSE0,				[esi + ebx + 1]
#endif
#endif
__asm	pmaxub		SSE0,				SSE1
__asm	add			esi,				eax
__asm	SSE_MOVE	[edi],				SSE0
#endif	// SHLUR
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}
#endif	// #ifndef	MODIFYPLUGIN

#ifdef	MODIFYPLUGIN

void	SSE_Repair12(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 1]
		add2(SSE0, SSE1, SSE7)
__asm	SSE3_MOVE	SSE6,				[esi + 2]
__asm	SSE3_MOVE	SSE7,				[esi + ebx]
		add3(SSE0, SSE1, SSE2, SSE6)
__asm	movd		[edi],				SSE7
__asm	SSE3_MOVE	SSE6,				[esi + ebx + 2]
		add4(SSE0, SSE1, SSE2, SSE3, SSE7)
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx]
		minmax2(SSE0, SSE1, SSE2, SSE3, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 1]
		minmax2(SSE0, SSE1, SSE2, SSE3, SSE5)
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 2]
		minmax2(SSE0, SSE1, SSE2, SSE3, SSE7)
#if		ISSE > 1
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]				
#endif
		minmax2sub(SSE0, SSE1, SSE2, SSE3, SSE6)
#if		ISSE > 1
__asm	pmaxub		SSE2,				SSE5
#else
__asm	pmaxub		SSE2,				[esi + ebx + 1]	
#endif
__asm	pminub		SSE1,				SSE2
__asm	SSE_MOVE	[edi + 1],			SSE1
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif // MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 1]
		add2(SSE0, SSE1, SSE7)
__asm	SSE3_MOVE	SSE6,				[esi + 2]
__asm	SSE3_MOVE	SSE7,				[esi + ebx]
		add3(SSE0, SSE1, SSE2, SSE6)
__asm	SSE3_MOVE	SSE6,				[esi + ebx + 2]
		add4(SSE0, SSE1, SSE2, SSE3, SSE7)
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx]
		minmax2(SSE0, SSE1, SSE2, SSE3, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 1]
		minmax2(SSE0, SSE1, SSE2, SSE3, SSE5)
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 2]
		minmax2(SSE0, SSE1, SSE2, SSE3, SSE7)
#ifdef		MODIFYPLUGIN
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 1]		
#endif
#if		ISSE > 1
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE5,				[edi]
#else
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]	
#endif
#endif
		minmax2sub(SSE0, SSE1, SSE2, SSE3, SSE6)
#ifdef	MODIFYPLUGIN
__asm	pminub		SSE2,				SSE4
__asm	pmaxub		SSE1,				SSE4
#endif
#if		ISSE > 1
__asm	pmaxub		SSE2,				SSE5
#else
#ifdef	MODIFYPLUGIN
__asm	pmaxub		SSE2,				[edi]
#else
__asm	pmaxub		SSE2,				[esi + ebx + 1]	
#endif
#endif	// ISSE > 1
__asm	pminub		SSE1,				SSE2
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE1
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 1]
		add2(SSE0, SSE1, SSE7)
__asm	SSE3_MOVE	SSE6,				[esi + 2]
__asm	SSE3_MOVE	SSE7,				[esi + ebx]
		add3(SSE0, SSE1, SSE2, SSE6)
__asm	SSE3_MOVE	SSE6,				[esi + ebx + 2]
		add4(SSE0, SSE1, SSE2, SSE3, SSE7)
#ifndef	MODIFYPLUGIN
__asm	SSE_MOVE	[edi + 1],			SSE6
#endif
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx]
		minmax2(SSE0, SSE1, SSE2, SSE3, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 1]
		minmax2(SSE0, SSE1, SSE2, SSE3, SSE5)
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 2]
		minmax2(SSE0, SSE1, SSE2, SSE3, SSE7)
#ifdef		MODIFYPLUGIN
__asm	SSE3_MOVE	SSE4,				[esi + ebx + 1]		
#endif
#if		ISSE > 1
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE5,				[edi]
#else
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]	
#endif
#endif
		minmax2sub(SSE0, SSE1, SSE2, SSE3, SSE6)
#ifdef	MODIFYPLUGIN
__asm	pminub		SSE2,				SSE4
__asm	pmaxub		SSE1,				SSE4
#endif
#if		ISSE > 1
__asm	pmaxub		SSE2,				SSE5
#else
#ifdef	MODIFYPLUGIN
__asm	pmaxub		SSE2,				[edi]
#else
__asm	pmaxub		SSE2,				[esi + ebx + 1]	
#endif
#endif	// ISSE > 1
__asm	pminub		SSE1,				SSE2
__asm	add			esi,				eax
__asm	SSE_MOVE	[edi],				SSE1
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}

void	SSE_Repair13(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{ 
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 1]
		add2(SSE0, SSE1, SSE7)
__asm	SSE3_MOVE	SSE6,				[esi + 2]
__asm	SSE3_MOVE	SSE5,				[esi + ebx]
		add3(SSE0, SSE1, SSE2, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 2]
		add4(SSE0, SSE1, SSE2, SSE3, SSE5)
__asm	movd		[edi],				SSE5
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx]				
		add5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE7)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 1]
		add6(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 2]
		minmax3sub(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE7)
#if		ISSE > 1
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 1]
#endif
		minmax2sub(SSE1, SSE2, SSE3, SSE4, SSE6)
#if		ISSE > 1
__asm	pmaxub		SSE3,				SSE7
#else
__asm	pmaxub		SSE3,				[esi + ebx + 1]
#endif
__asm	pminub		SSE3,				SSE2
__asm	SSE_MOVE	[edi + 1],			SSE3
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif // MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 1]
		add2(SSE0, SSE1, SSE7)
__asm	SSE3_MOVE	SSE6,				[esi + 2]
__asm	SSE3_MOVE	SSE5,				[esi + ebx]
		add3(SSE0, SSE1, SSE2, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 2]
		add4(SSE0, SSE1, SSE2, SSE3, SSE5)
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx]				
		add5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE7)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 1]
		add6(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 2]
		minmax3sub(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE7)
#ifdef		MODIFYPLUGIN
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]		
#endif
#if		ISSE > 1
#ifdef		MODIFYPLUGIN
__asm	SSE3_MOVE	SSE7,				[edi]
#else
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 1]
#endif
#endif
		minmax2sub(SSE1, SSE2, SSE3, SSE4, SSE6)
#ifdef	MODIFYPLUGIN
__asm	pminub		SSE3,				SSE5
__asm	pmaxub		SSE2,				SSE5
#endif
#if		ISSE > 1
__asm	pmaxub		SSE3,				SSE7
#else
#ifdef		MODIFYPLUGIN
__asm	pmaxub		SSE3,				[edi]
#else
__asm	pmaxub		SSE3,				[esi + ebx + 1]
#endif
#endif	// ISSE > 1
__asm	pminub		SSE3,				SSE2
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_MOVE	[edi],				SSE3	
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 1]
		add2(SSE0, SSE1, SSE7)
__asm	SSE3_MOVE	SSE6,				[esi + 2]
__asm	SSE3_MOVE	SSE5,				[esi + ebx]
		add3(SSE0, SSE1, SSE2, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 2]
		add4(SSE0, SSE1, SSE2, SSE3, SSE5)
#ifndef	MODIFYPLUGIN
__asm	SSE_MOVE	[edi + 1],			SSE7
#endif
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx]				
		add5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE7)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 1]
		add6(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx + 2]
		minmax3sub(SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE7)
#ifdef		MODIFYPLUGIN
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]		
#endif
#if		ISSE > 1
#ifdef		MODIFYPLUGIN
__asm	SSE3_MOVE	SSE7,				[edi]
#else
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 1]
#endif
#endif	// ISSE > 1
		minmax2sub(SSE1, SSE2, SSE3, SSE4, SSE6)
#ifdef	MODIFYPLUGIN
__asm	pminub		SSE3,				SSE5
__asm	pmaxub		SSE2,				SSE5
#endif
#if		ISSE > 1
__asm	pmaxub		SSE3,				SSE7
#else
#ifdef		MODIFYPLUGIN
__asm	pmaxub		SSE3,				[edi]
#else
__asm	pmaxub		SSE3,				[esi + ebx + 1]
#endif
#endif	// ISSE > 1
__asm	pminub		SSE3,				SSE2
__asm	add			esi,				eax
__asm	SSE_MOVE	[edi],				SSE3
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}

void	SSE_Repair14(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
{	
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 1]
		add2(SSE0, SSE1, SSE7)
__asm	SSE3_MOVE	SSE6,				[esi + 2]
__asm	SSE3_MOVE	SSE5,				[esi + ebx]
		add3(SSE0, SSE1, SSE2, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 2]
		add4(SSE0, SSE1, SSE2, SSE3, SSE5)
__asm	movd		[edi],				SSE5
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx]				
		add5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE7)
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx + 1]				
		sub5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 2]				
		sub4(SSE1, SSE2, SSE3, SSE4, SSE5)
#if		ISSE > 1
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]
#endif
		sub3(SSE2, SSE3, SSE4, SSE7)
#if		ISSE > 1
__asm	pmaxub		SSE4,				SSE5
#else
__asm	pmaxub		SSE4,				[esi + ebx + 1]
#endif
__asm	pminub		SSE3,				SSE4
__asm	SSE_MOVE	[edi + 1],			SSE3
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif // MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 1]
		add2(SSE0, SSE1, SSE7)
__asm	SSE3_MOVE	SSE6,				[esi + 2]
__asm	SSE3_MOVE	SSE5,				[esi + ebx]
		add3(SSE0, SSE1, SSE2, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 2]
		add4(SSE0, SSE1, SSE2, SSE3, SSE5)
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx]				
		add5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE7)
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx + 1]				
		sub5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 2]				
		sub4(SSE1, SSE2, SSE3, SSE4, SSE5)
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]				
#endif	// MODIFYPLUGIN
#if		ISSE > 1
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE5,				[edi]
#else
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]
#endif
#endif
		sub3(SSE2, SSE3, SSE4, SSE7)
#ifdef	MODIFYPLUGIN
__asm	pminub		SSE4,				SSE0
__asm	pmaxub		SSE3,				SSE0
#endif	// MODIFYPLUGIN
#if		ISSE > 1
__asm	pmaxub		SSE4,				SSE5
#else
#ifdef	MODIFYPLUGIN
__asm	pmaxub		SSE4,				[edi]
#else	// ISSE > 1
__asm	pmaxub		SSE4,				[esi + ebx + 1]
#endif
#endif	// ISSE > 1
__asm	add			esi,				SSE_INCREMENT
__asm	pminub		SSE3,				SSE4
__asm	SSE_MOVE	[edi],				SSE3
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
__asm	SSE3_MOVE	SSE0,				[esi]
__asm	SSE3_MOVE	SSE7,				[esi + 1]
		add2(SSE0, SSE1, SSE7)
__asm	SSE3_MOVE	SSE6,				[esi + 2]
__asm	SSE3_MOVE	SSE5,				[esi + ebx]
		add3(SSE0, SSE1, SSE2, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + ebx + 2]
		add4(SSE0, SSE1, SSE2, SSE3, SSE5)
#ifndef		MODIFYPLUGIN
__asm	SSE_MOVE	[edi + 1],			SSE7
#endif
__asm	SSE3_MOVE	SSE6,				[esi + 2*ebx]				
		add5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE7)
__asm	SSE3_MOVE	SSE5,				[esi + 2*ebx + 1]				
		sub5(SSE0, SSE1, SSE2, SSE3, SSE4, SSE6)
__asm	SSE3_MOVE	SSE7,				[esi + 2*ebx + 2]				
		sub4(SSE1, SSE2, SSE3, SSE4, SSE5)
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]				
#endif
#if		ISSE > 1
#ifdef	MODIFYPLUGIN
__asm	SSE3_MOVE	SSE5,				[edi]
#else
__asm	SSE3_MOVE	SSE5,				[esi + ebx + 1]
#endif
#endif
		sub3(SSE2, SSE3, SSE4, SSE7)
#ifdef	MODIFYPLUGIN
__asm	pminub		SSE4,				SSE0
__asm	pmaxub		SSE3,				SSE0
#endif	
#if		ISSE > 1
__asm	pmaxub		SSE4,				SSE5
#else
#ifdef	MODIFYPLUGIN
__asm	pmaxub		SSE4,				[edi]
#else	// ISSE > 1
__asm	pmaxub		SSE4,				[esi + ebx + 1]
#endif
#endif	// ISSE > 1
__asm	pminub		SSE3,				SSE4
__asm	add			esi,				eax
__asm	SSE_MOVE	[edi],				SSE3
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}

#endif	// MODIFYPLUGIN	

#if	defined(MODIFYPLUGIN) || defined(SHLUR)

#define		diagweightr5(min, max, weight, center, bound1, bound2, reg)	\
__asm	SSE3_MOVE	min,				bound1			\
__asm	SSE3_MOVE	reg,				bound2			\
__asm	SSE_RMOVE	max,				min				\
__asm	SSE_RMOVE	weight,				center			\
__asm	pminub		min,				reg				\
__asm	pmaxub		max,				reg				\
__asm	SSE_RMOVE	reg,				min				\
__asm	psubusb		weight,				max				\
__asm	psubusb		reg,				center			\
__asm	pmaxub		weight,				reg

#if	defined(MODIFYPLUGIN)
#define		diagweightwr5(min, max, weight, center, bound1, bound2, wmem, reg)	diagweightr5(min, max, weight, center, bound1, bound2, reg)
#else
// same as diagweight_5, but in addition bound2 is written to wmem
#define		diagweightwr5(min, max, weight, center, bound1, bound2, wmem, reg)	\
__asm	SSE3_MOVE	min,				bound1			\
__asm	SSE3_MOVE	reg,				bound2			\
__asm	SSE_RMOVE	max,				min				\
__asm	SSE_MOVE	wmem,				reg				\
__asm	SSE_RMOVE	weight,				center			\
__asm	pminub		min,				reg				\
__asm	pmaxub		max,				reg				\
__asm	SSE_RMOVE	reg,				min				\
__asm	psubusb		weight,				max				\
__asm	psubusb		reg,				center			\
__asm	pmaxub		weight,				reg

#endif	// MODIFYPLUGIN
#ifdef	SHARPEN
void	SSE_Repair15(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height, int strength)
#else
void	SSE_Repair15(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
#endif
{
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
#ifdef	SHARPEN
__asm	mov			spitch,				eax
__asm	mov			eax,				strength
#endif
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]		
		diagweightr5(SSE1, SSE2, SSE3, SSE0, [esi], [esi + 2*ebx + 2], SSE7)
		diagweightr5(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx], [esi + 2], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightr5(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx + 1], [esi + 1], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightwr5(SSE4, SSE5, SSE6, SSE0, [esi + ebx + 2], [esi + ebx], [edi], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
#ifdef	SHLUR
		sharpen(SSE0, SSE1, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
__asm	SSE_MOVE	[edi + 1],			SSE0
#else
__asm	pmaxub		SSE1,				SSE0
__asm	pminub		SSE1,				SSE2
__asm	SSE_MOVE	[edi + 1],			SSE1
#endif
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif	// MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]
		diagweightr5(SSE1, SSE2, SSE3, SSE0, [esi], [esi + 2*ebx + 2], SSE7)
		diagweightr5(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx], [esi + 2], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightr5(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx + 1], [esi + 1], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightr5(SSE4, SSE5, SSE6, SSE0, [esi + ebx + 2], [esi + ebx], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
__asm	add			esi,				SSE_INCREMENT
#ifdef	MODIFYPLUGIN
#if		ISSE > 1
__asm	SSE3_MOVE	SSE7,				[edi]
#endif
#if	MODIFYPLUGIN > 0
__asm	pminub		SSE1,				SSE0
__asm	pmaxub		SSE2,				SSE0
#endif
#if		ISSE > 1
__asm	pmaxub		SSE1,				SSE7
#else
__asm	pmaxub		SSE1,				[edi]
#endif
__asm	pminub		SSE1,				SSE2
__asm	SSE_MOVE	[edi],				SSE1
#else	// MODIFYPLUGIN
#ifdef	SHLUR
		sharpen(SSE0, SSE1, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
__asm	SSE_MOVE	[edi],				SSE0
#else
__asm	pmaxub		SSE1,				SSE0
__asm	pminub		SSE1,				SSE2
__asm	SSE_MOVE	[edi],				SSE1
#endif
#endif	// MODIFYPLUGIN
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]		
		diagweightr5(SSE1, SSE2, SSE3, SSE0, [esi], [esi + 2*ebx + 2], SSE7)
		diagweightr5(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx], [esi + 2], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightr5(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx + 1], [esi + 1], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightwr5(SSE4, SSE5, SSE6, SSE0, [esi + ebx], [esi + ebx + 2], [edi + 1], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
#ifdef	MODIFYPLUGIN
#if		ISSE > 1
__asm	SSE3_MOVE	SSE7,				[edi]
#endif
#if	MODIFYPLUGIN > 0
__asm	pminub		SSE1,				SSE0
__asm	pmaxub		SSE2,				SSE0
#endif
#if		ISSE > 1
__asm	pmaxub		SSE1,				SSE7
#else
__asm	pmaxub		SSE1,				[edi]
#endif
__asm	pminub		SSE1,				SSE2	
__asm	SSE_MOVE	[edi],				SSE1
#else	// MODIFYPLUGIN
#ifdef	SHLUR
		sharpen(SSE0, SSE1, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
__asm	SSE_MOVE	[edi],				SSE0
#else
__asm	pmaxub		SSE1,				SSE0
__asm	pminub		SSE1,				SSE2	
__asm	SSE_MOVE	[edi],				SSE1
#endif
#endif	// MODIFYPLUGIN
#ifdef	SHARPEN
__asm	add			esi,				spitch
#else
__asm	add			esi,				eax
#endif
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}

#define	diagweightr6(min, max, weight, center, bound1, bound2, reg)	\
__asm	SSE3_MOVE	min,				bound1			\
__asm	SSE3_MOVE	reg,				bound2			\
__asm	SSE_RMOVE	max,				min				\
__asm	SSE_RMOVE	weight,				center			\
__asm	pminub		min,				reg				\
__asm	pmaxub		max,				reg				\
__asm	SSE_RMOVE	reg,				min				\
__asm	psubusb		weight,				max				\
__asm	psubusb		reg,				center			\
__asm	pmaxub		weight,				reg				\
__asm	SSE_RMOVE	reg,				max				\
__asm	paddusb		weight,				weight			\
__asm	psubusb		reg,				min				\
__asm	paddusb		weight,				reg

#ifdef	MODIFYPLUGIN
#define		diagweightwr6(min, max, weight, center, bound1, bound2, wmem, reg)	diagweightr6(min, max, weight, center, bound1, bound2, reg)
#else
// same as diagweight_5, but in addition bound2 is written to wmem
#define		diagweightwr6(min, max, weight, center, bound1, bound2, wmem, reg)	\
__asm	SSE3_MOVE	min,				bound1			\
__asm	SSE3_MOVE	reg,				bound2			\
__asm	SSE_RMOVE	max,				min				\
__asm	SSE_MOVE	wmem,				reg				\
__asm	SSE_RMOVE	weight,				center			\
__asm	pminub		min,				reg				\
__asm	pmaxub		max,				reg				\
__asm	SSE_RMOVE	reg,				min				\
__asm	psubusb		weight,				max				\
__asm	psubusb		reg,				center			\
__asm	pmaxub		weight,				reg				\
__asm	SSE_RMOVE	reg,				max				\
__asm	paddusb		weight,				weight			\
__asm	psubusb		reg,				min				\
__asm	paddusb		weight,				reg
#endif	// MODIFYPLUGIN

#ifdef	SHARPEN
void	SSE_Repair16(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height, int strength)
#else
void	SSE_Repair16(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
#endif
{
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
#ifdef	SHARPEN
__asm	mov			spitch,				eax
__asm	mov			eax,				strength
#endif
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]		
		diagweightr6(SSE1, SSE2, SSE3, SSE0, [esi], [esi + 2*ebx + 2], SSE7)
		diagweightr6(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx], [esi + 2], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightr6(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx + 1], [esi + 1], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightwr6(SSE4, SSE5, SSE6, SSE0, [esi + ebx + 2], [esi + ebx], [edi], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
#ifdef	SHLUR
		sharpen(SSE0, SSE1, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
__asm	SSE_MOVE	[edi + 1],			SSE0
#else
__asm	pmaxub		SSE1,				SSE0
__asm	pminub		SSE1,				SSE2
__asm	SSE_MOVE	[edi + 1],			SSE1
#endif
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif	// MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]
		diagweightr6(SSE1, SSE2, SSE3, SSE0, [esi], [esi + 2*ebx + 2], SSE7)
		diagweightr6(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx], [esi + 2], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightr6(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx + 1], [esi + 1], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightr6(SSE4, SSE5, SSE6, SSE0, [esi + ebx + 2], [esi + ebx], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
__asm	add			esi,				SSE_INCREMENT
#ifdef	MODIFYPLUGIN
#if		ISSE > 1
__asm	SSE3_MOVE	SSE7,				[edi]
#endif
#if	MODIFYPLUGIN > 0
__asm	pminub		SSE1,				SSE0
__asm	pmaxub		SSE2,				SSE0
#endif
#if		ISSE > 1
__asm	pmaxub		SSE1,				SSE7
#else
__asm	pmaxub		SSE1,				[edi]
#endif
__asm	pminub		SSE1,				SSE2
__asm	SSE_MOVE	[edi],				SSE1
#else	// MODIFYPLUGIN
#ifdef	SHLUR
		sharpen(SSE0, SSE1, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
__asm	SSE_MOVE	[edi],				SSE0
#else
__asm	pmaxub		SSE1,				SSE0
__asm	pminub		SSE1,				SSE2
__asm	SSE_MOVE	[edi],				SSE1
#endif
#endif	// MODIFYPLUGIN
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]		
		diagweightr6(SSE1, SSE2, SSE3, SSE0, [esi], [esi + 2*ebx + 2], SSE7)
		diagweightr6(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx], [esi + 2], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightr6(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx + 1], [esi + 1], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightwr6(SSE4, SSE5, SSE6, SSE0, [esi + ebx], [esi + ebx + 2], [edi + 1], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
#ifdef	MODIFYPLUGIN
#if		ISSE > 1
__asm	SSE3_MOVE	SSE7,				[edi]
#endif
#if	MODIFYPLUGIN > 0
__asm	pminub		SSE1,				SSE0
__asm	pmaxub		SSE2,				SSE0
#endif
#if		ISSE > 1
__asm	pmaxub		SSE1,				SSE7
#else
__asm	pmaxub		SSE1,				[edi]
#endif
__asm	pminub		SSE1,				SSE2	
__asm	SSE_MOVE	[edi],				SSE1
#else	// MODIFYPLUGIN
#ifdef	SHLUR
		sharpen(SSE0, SSE1, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
__asm	SSE_MOVE	[edi],				SSE0
#else
__asm	pmaxub		SSE1,				SSE0
__asm	pminub		SSE1,				SSE2	
__asm	SSE_MOVE	[edi],				SSE1
#endif
#endif	// MODIFYPLUGIN
#ifdef	SHARPEN
__asm	add			esi,				spitch
#else
__asm	add			esi,				eax
#endif
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}

#endif	// defined(MODIFYPLUGIN) || defined(SHLUR)

#ifdef	SHLUR
#define	diagweightr7(min, max, weight, center, bound1, bound2, reg)	\
__asm	SSE3_MOVE	min,				bound1			\
__asm	SSE3_MOVE	reg,				bound2			\
__asm	SSE_RMOVE	max,				min				\
__asm	SSE_RMOVE	weight,				center			\
__asm	pminub		min,				reg				\
__asm	pmaxub		max,				reg				\
__asm	SSE_RMOVE	reg,				min				\
__asm	psubusb		weight,				max				\
__asm	psubusb		reg,				center			\
__asm	pmaxub		weight,				reg				\
__asm	SSE_RMOVE	reg,				max				\
__asm	psubusb		reg,				min				\
__asm	paddusb		weight,				reg

#ifdef	MODIFYPLUGIN
#define		diagweightwr7(min, max, weight, center, bound1, bound2, wmem, reg)	diagweightr7(min, max, weight, center, bound1, bound2, reg)
#else
// same as diagweight_5, but in addition bound2 is written to wmem
#define		diagweightwr7(min, max, weight, center, bound1, bound2, wmem, reg)	\
__asm	SSE3_MOVE	min,				bound1			\
__asm	SSE3_MOVE	reg,				bound2			\
__asm	SSE_RMOVE	max,				min				\
__asm	SSE_MOVE	wmem,				reg				\
__asm	SSE_RMOVE	weight,				center			\
__asm	pminub		min,				reg				\
__asm	pmaxub		max,				reg				\
__asm	SSE_RMOVE	reg,				min				\
__asm	psubusb		weight,				max				\
__asm	psubusb		reg,				center			\
__asm	pmaxub		weight,				reg				\
__asm	SSE_RMOVE	reg,				max				\
__asm	psubusb		reg,				min				\
__asm	paddusb		weight,				reg
#endif	// MODIFYPLUGIN

#ifdef	SHARPEN
void	SSE_Repair17(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height, int strength)
#else
void	SSE_Repair17(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
#endif
{
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
#ifdef	SHARPEN
__asm	mov			spitch,				eax
__asm	mov			eax,				strength
#endif
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]		
		diagweightr7(SSE1, SSE2, SSE3, SSE0, [esi], [esi + 2*ebx + 2], SSE7)
		diagweightr7(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx], [esi + 2], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightr7(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx + 1], [esi + 1], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightwr7(SSE4, SSE5, SSE6, SSE0, [esi + ebx + 2], [esi + ebx], [edi], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
#ifdef	SHLUR
		sharpen(SSE0, SSE1, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
__asm	SSE_MOVE	[edi + 1],			SSE0
#else
__asm	pmaxub		SSE1,				SSE0
__asm	pminub		SSE1,				SSE2
__asm	SSE_MOVE	[edi + 1],			SSE1
#endif
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif	// MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]
		diagweightr7(SSE1, SSE2, SSE3, SSE0, [esi], [esi + 2*ebx + 2], SSE7)
		diagweightr7(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx], [esi + 2], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightr7(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx + 1], [esi + 1], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightr7(SSE4, SSE5, SSE6, SSE0, [esi + ebx + 2], [esi + ebx], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
__asm	add			esi,				SSE_INCREMENT
#ifdef	MODIFYPLUGIN
#if		ISSE > 1
__asm	SSE3_MOVE	SSE7,				[edi]
#endif
#if	MODIFYPLUGIN > 0
__asm	pminub		SSE1,				SSE0
__asm	pmaxub		SSE2,				SSE0
#endif
#if		ISSE > 1
__asm	pmaxub		SSE1,				SSE7
#else
__asm	pmaxub		SSE1,				[edi]
#endif
__asm	pminub		SSE1,				SSE2
__asm	SSE_MOVE	[edi],				SSE1
#else	// MODIFYPLUGIN
#ifdef	SHLUR
		sharpen(SSE0, SSE1, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
__asm	SSE_MOVE	[edi],				SSE0
#else
__asm	pmaxub		SSE1,				SSE0
__asm	pminub		SSE1,				SSE2
__asm	SSE_MOVE	[edi],				SSE1
#endif
#endif	// MODIFYPLUGIN
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]		
		diagweightr7(SSE1, SSE2, SSE3, SSE0, [esi], [esi + 2*ebx + 2], SSE7)
		diagweightr7(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx], [esi + 2], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightr7(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx + 1], [esi + 1], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightwr7(SSE4, SSE5, SSE6, SSE0, [esi + ebx], [esi + ebx + 2], [edi + 1], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
#ifdef	MODIFYPLUGIN
#if		ISSE > 1
__asm	SSE3_MOVE	SSE7,				[edi]
#endif
#if	MODIFYPLUGIN > 0
__asm	pminub		SSE1,				SSE0
__asm	pmaxub		SSE2,				SSE0
#endif
#if		ISSE > 1
__asm	pmaxub		SSE1,				SSE7
#else
__asm	pmaxub		SSE1,				[edi]
#endif
__asm	pminub		SSE1,				SSE2	
__asm	SSE_MOVE	[edi],				SSE1
#else	// MODIFYPLUGIN
#ifdef	SHLUR
		sharpen(SSE0, SSE1, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
__asm	SSE_MOVE	[edi],				SSE0
#else
__asm	pmaxub		SSE1,				SSE0
__asm	pminub		SSE1,				SSE2	
__asm	SSE_MOVE	[edi],				SSE1
#endif
#endif	// MODIFYPLUGIN
#ifdef	SHARPEN
__asm	add			esi,				spitch
#else
__asm	add			esi,				eax
#endif
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}

#define	diagweightr8(min, max, weight, center, bound1, bound2, reg)	\
__asm	SSE3_MOVE	min,				bound1			\
__asm	SSE3_MOVE	reg,				bound2			\
__asm	SSE_RMOVE	max,				min				\
__asm	SSE_RMOVE	weight,				center			\
__asm	pminub		min,				reg				\
__asm	pmaxub		max,				reg				\
__asm	SSE_RMOVE	reg,				min				\
__asm	psubusb		weight,				max				\
__asm	psubusb		reg,				center			\
__asm	pmaxub		weight,				reg				\
__asm	SSE_RMOVE	reg,				max				\
__asm	psubusb		reg,				min				\
__asm	paddusb		weight,				reg				\
__asm	paddusb		weight,				reg

#ifdef	MODIFYPLUGIN
#define		diagweightwr8(min, max, weight, center, bound1, bound2, wmem, reg)	diagweightr6(min, max, weight, center, bound1, bound2, reg)
#else
// same as diagweight_5, but in addition bound2 is written to wmem
#define		diagweightwr8(min, max, weight, center, bound1, bound2, wmem, reg)	\
__asm	SSE3_MOVE	min,				bound1			\
__asm	SSE3_MOVE	reg,				bound2			\
__asm	SSE_RMOVE	max,				min				\
__asm	SSE_MOVE	wmem,				reg				\
__asm	SSE_RMOVE	weight,				center			\
__asm	pminub		min,				reg				\
__asm	pmaxub		max,				reg				\
__asm	SSE_RMOVE	reg,				min				\
__asm	psubusb		weight,				max				\
__asm	psubusb		reg,				center			\
__asm	pmaxub		weight,				reg				\
__asm	SSE_RMOVE	reg,				max				\
__asm	psubusb		reg,				min				\
__asm	paddusb		weight,				reg				\
__asm	paddusb		weight,				reg
#endif	// MODIFYPLUGIN

#ifdef	SHARPEN
void	SSE_Repair18a(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height, int strength)
#else
void	SSE_Repair18a(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
#endif
{
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
#ifdef	SHARPEN
__asm	mov			spitch,				eax
__asm	mov			eax,				strength
#endif
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]		
		diagweightr6(SSE1, SSE2, SSE3, SSE0, [esi], [esi + 2*ebx + 2], SSE7)
		diagweightr6(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx], [esi + 2], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightr6(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx + 1], [esi + 1], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightwr6(SSE4, SSE5, SSE6, SSE0, [esi + ebx + 2], [esi + ebx], [edi], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
#ifdef	SHLUR
		sharpen(SSE0, SSE1, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
__asm	SSE_MOVE	[edi + 1],			SSE0
#else
__asm	pmaxub		SSE1,				SSE0
__asm	pminub		SSE1,				SSE2
__asm	SSE_MOVE	[edi + 1],			SSE1
#endif
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif	// MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]
		diagweightr6(SSE1, SSE2, SSE3, SSE0, [esi], [esi + 2*ebx + 2], SSE7)
		diagweightr6(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx], [esi + 2], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightr6(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx + 1], [esi + 1], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightr6(SSE4, SSE5, SSE6, SSE0, [esi + ebx + 2], [esi + ebx], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
__asm	add			esi,				SSE_INCREMENT
#ifdef	MODIFYPLUGIN
#if		ISSE > 1
__asm	SSE3_MOVE	SSE7,				[edi]
#endif
#if	MODIFYPLUGIN > 0
__asm	pminub		SSE1,				SSE0
__asm	pmaxub		SSE2,				SSE0
#endif
#if		ISSE > 1
__asm	pmaxub		SSE1,				SSE7
#else
__asm	pmaxub		SSE1,				[edi]
#endif
__asm	pminub		SSE1,				SSE2
__asm	SSE_MOVE	[edi],				SSE1
#else	// MODIFYPLUGIN
#ifdef	SHLUR
		sharpen(SSE0, SSE1, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
__asm	SSE_MOVE	[edi],				SSE0
#else
__asm	pmaxub		SSE1,				SSE0
__asm	pminub		SSE1,				SSE2
__asm	SSE_MOVE	[edi],				SSE1
#endif
#endif	// MODIFYPLUGIN
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]		
		diagweightr6(SSE1, SSE2, SSE3, SSE0, [esi], [esi + 2*ebx + 2], SSE7)
		diagweightr6(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx], [esi + 2], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightr6(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx + 1], [esi + 1], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diagweightwr6(SSE4, SSE5, SSE6, SSE0, [esi + ebx], [esi + ebx + 2], [edi + 1], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
#ifdef	MODIFYPLUGIN
#if		ISSE > 1
__asm	SSE3_MOVE	SSE7,				[edi]
#endif
#if	MODIFYPLUGIN > 0
__asm	pminub		SSE1,				SSE0
__asm	pmaxub		SSE2,				SSE0
#endif
#if		ISSE > 1
__asm	pmaxub		SSE1,				SSE7
#else
__asm	pmaxub		SSE1,				[edi]
#endif
__asm	pminub		SSE1,				SSE2	
__asm	SSE_MOVE	[edi],				SSE1
#else	// MODIFYPLUGIN
#ifdef	SHLUR
		sharpen(SSE0, SSE1, SSE2, rshift[eax], shift_mask[eax], SSE7, SSE3)
__asm	SSE_MOVE	[edi],				SSE0
#else
__asm	pmaxub		SSE1,				SSE0
__asm	pminub		SSE1,				SSE2	
__asm	SSE_MOVE	[edi],				SSE1
#endif
#endif	// MODIFYPLUGIN
#ifdef	SHARPEN
__asm	add			esi,				spitch
#else
__asm	add			esi,				eax
#endif
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}
#endif	// SHLUR

#define		diag18(val1, val2, weight, center, bound1, bound2, reg)	\
__asm	SSE3_MOVE	val1,				bound1			\
__asm	SSE_RMOVE	weight,				center			\
__asm	SSE_RMOVE	reg,				val1			\
__asm	psubusb		weight,				val1			\
__asm	psubusb		reg,				center			\
__asm	SSE3_MOVE	val2,				bound2			\
__asm	pmaxub		weight,				reg				\
__asm	SSE_RMOVE	reg,				center			\
__asm	psubusb		reg,				val2			\
__asm	pmaxub		weight,				reg				\
__asm	SSE_RMOVE	reg,				val2			\
__asm	psubusb		reg,				center			\
__asm	pmaxub		weight,				reg	

#ifdef	SHARPEN
void	SSE_Repair18(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height, int strength)
#else
void	SSE_Repair18(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, int hblocks, int remainder, int incpitch, int height)
#endif
{
__asm	mov			eax,				hblocks
__asm	mov			ebx,				spitch
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				eax
#endif
__asm	mov			edx,				remainder
#if		SSE_INCREMENT == 16
__asm	add			eax,				eax
#endif
__asm	mov			esi,				_sp
#ifdef	MODIFYPLUGIN
__asm	lea			eax,				[eax * 8 + edx]
#else
__asm	lea			eax,				[eax * 8 + edx + SSE_INCREMENT + 1]
#endif
__asm	sub			esi,				ebx
__asm	sub			dpitch,				eax
__asm	neg			eax
__asm	mov			edi,				dp
#ifdef	MODIFYPLUGIN
__asm	inc			edi
__asm	lea			eax,				[ebx + eax]	
#else
__asm	lea			eax,				[ebx + eax + 1]	
#ifdef	SHARPEN
__asm	mov			spitch,				eax
__asm	mov			eax,				strength
#endif
__asm	align		16
__asm	column_loop:
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]		
		diag18(SSE1, SSE2, SSE3, SSE0, [esi], [esi + 2*ebx + 2], SSE7)
		diag18(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx], [esi + 2], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diag18(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx + 1], [esi + 1], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diag18(SSE4, SSE5, SSE6, SSE0, [esi + ebx + 2], [esi + ebx], SSE7)
__asm	movd		[edi],				SSE5		
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
__asm	SSE_RMOVE	SSE7,				SSE1
__asm	pminub		SSE1,				SSE2
__asm	pmaxub		SSE7,				SSE2
#ifdef	SHLUR
		sharpen(SSE0, SSE1, SSE7, rshift[eax], shift_mask[eax], SSE3, SSE4)
__asm	SSE_MOVE	[edi + 1],				SSE0
#else
__asm	pmaxub		SSE1,				SSE0
__asm	pminub		SSE1,				SSE7
__asm	SSE_MOVE	[edi + 1],			SSE1
#endif
// now the pixels in the middle
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT + 1
__asm	mov			ecx,				hblocks
#endif	// MODIFYPLUGIN
__asm	align		16
__asm	middle_loop:
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]
		diag18(SSE1, SSE2, SSE3, SSE0, [esi], [esi + 2*ebx + 2], SSE7)
		diag18(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx], [esi + 2], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diag18(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx + 1], [esi + 1], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diag18(SSE4, SSE5, SSE6, SSE0, [esi + ebx + 2], [esi + ebx], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
__asm	add			esi,				SSE_INCREMENT
__asm	SSE_RMOVE	SSE6,				SSE1
#ifdef	MODIFYPLUGIN
#if		ISSE > 1
__asm	SSE3_MOVE	SSE7,				[edi]
#endif
__asm	pminub		SSE1,				SSE2
__asm	pmaxub		SSE6,				SSE2
#if	MODIFYPLUGIN > 0
__asm	pminub		SSE1,				SSE0
__asm	pmaxub		SSE6,				SSE0
#endif
#if		ISSE > 1
__asm	pmaxub		SSE1,				SSE7
#else
__asm	pmaxub		SSE1,				[edi]
#endif
__asm	pminub		SSE1,				SSE6
__asm	SSE_MOVE	[edi],				SSE1
#else	// MODIFYPLUGIN
__asm	pminub		SSE1,				SSE2
__asm	pmaxub		SSE6,				SSE2
#ifdef	SHLUR
		sharpen(SSE0, SSE1, SSE6, rshift[eax], shift_mask[eax], SSE3, SSE4)
__asm	SSE_MOVE	[edi],				SSE0
#else
__asm	pmaxub		SSE1,				SSE0
__asm	pminub		SSE1,				SSE6
__asm	SSE_MOVE	[edi],				SSE1
#endif
#endif	// MODIFYPLUGIN
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				edx
__asm	add			edi,				edx
__asm	SSE3_MOVE	SSE0,				[esi + ebx + 1]		
		diag18(SSE1, SSE2, SSE3, SSE0, [esi], [esi + 2*ebx + 2], SSE7)
		diag18(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx], [esi + 2], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diag18(SSE4, SSE5, SSE6, SSE0, [esi + 2*ebx + 1], [esi + 1], SSE7)
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
		diag18(SSE4, SSE5, SSE6, SSE0, [esi + ebx], [esi + ebx + 2], SSE7)
#ifndef	MODIFYPLUGIN
__asm	SSE_MOVE	[edi + 1],			SSE5
#endif
		merge2weighted(SSE1, SSE2, SSE3, SSE4, SSE5, SSE6)
__asm	SSE_RMOVE	SSE6,				SSE1
#ifdef	MODIFYPLUGIN
#if		ISSE > 1
__asm	SSE3_MOVE	SSE7,				[edi]
#endif
__asm	pminub		SSE1,				SSE2
__asm	pmaxub		SSE6,				SSE2
#if	MODIFYPLUGIN > 0
__asm	pminub		SSE1,				SSE0
__asm	pmaxub		SSE6,				SSE0
#endif
#if		ISSE > 1
__asm	pmaxub		SSE1,				SSE7
#else
__asm	pmaxub		SSE1,				[edi]
#endif
__asm	pminub		SSE1,				SSE6	
__asm	SSE_MOVE	[edi],				SSE1
#else	// MODIFYPLUGIN
__asm	pminub		SSE1,				SSE2
__asm	pmaxub		SSE6,				SSE2
#ifdef	SHLUR
		sharpen(SSE0, SSE1, SSE6, rshift[eax], shift_mask[eax], SSE3, SSE4)
__asm	SSE_MOVE	[edi],				SSE0
#else
__asm	pmaxub		SSE1,				SSE0
__asm	pminub		SSE1,				SSE6	
__asm	SSE_MOVE	[edi],				SSE1
#endif
#endif	// MODIFYPLUGIN
#ifdef	SHARPEN
__asm	add			esi,				spitch
#else
__asm	add			esi,				eax
#endif
__asm	add			edi,				dpitch
__asm	dec			height
#ifdef	MODIFYPLUGIN
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
#else
__asm	jnz			column_loop
#endif
}


#ifdef	SHARPEN
static void	(*cleaning_methods[MAXMODE + 1])(BYTE *dp, int dpitch, const BYTE *sp, int spitch, int hblocks, int remainder, int incpitch, int height, int strength)
		= { copy_plane, SSE_RemoveGrain1, SSE_RemoveGrain2, SSE_RemoveGrain3, SSE_RemoveGrain4, SSE_Repair15, SSE_Repair16, SSE_Repair17, SSE_Repair18a, diag9, copy_plane
			, copy_plane, copy_plane, copy_plane, copy_plane, copy_plane, SmartRG, SSE_Repair18, copy_plane, copy_plane, copy_plane
			, SmartAvgRGs, SmartAvgRGf
		};
#else
static void	(*cleaning_methods[MAXMODE + 1])(BYTE *dp, int dpitch, const BYTE *sp, int spitch, int hblocks, int remainder, int incpitch, int height)
#ifdef	MODIFYPLUGIN
		= { do_nothing, SSE_RemoveGrain1, SSE_RemoveGrain2, SSE_RemoveGrain3, SSE_RemoveGrain4, diag5, diag6, diag7, diag8, diag9
			, SSE_RemoveGrain10, SSE_RemoveGrain1, SSE_Repair12, SSE_Repair13, SSE_Repair14, SSE_Repair15, SSE_Repair16, SmartRG, SSE_Repair18};
#elif	defined(BLUR)
		= { copy_plane, SSE_RemoveGrain1, SSE_RemoveGrain2, SSE_RemoveGrain3, SSE_RemoveGrain4, copy_plane, copy_plane, copy_plane, copy_plane, diag9, copy_plane
			, copy_plane, copy_plane, copy_plane, copy_plane, copy_plane, copy_plane, copy_plane, copy_plane, copy_plane, copy_plane
			, SmartAvgRGs, SmartAvgRGf
		};
#else
		= { copy_plane, SSE_RemoveGrain1, SSE_RemoveGrain2, SSE_RemoveGrain3, SSE_RemoveGrain4, diag5, diag6, diag7, diag8, diag9
			, SSE_RemoveGrain10, SSE_RemoveGrain11, SSE_RemoveGrain12, bob_top, bob_bottom, smartbob_top, smartbob_bottom, SmartRG, SSE_Repair18, SSE_RemoveGrain19, SSE_RemoveGrain20
			, SmartAvgRGs, SmartAvgRGf, SSE_RemoveGrain23, SSE_RemoveGrain24, nondestructivesharpen, SmartRGC, SmartRGCL, SmartRGCL2};
#endif
#endif	// SHARPEN

class	RemoveGrain : public GenericVideoFilter, public PlanarAccess
{
#ifdef	MODIFYPLUGIN
	PClip	oclip;
#endif
	
	int		height2[3], hblocks[3], remainder[3], incpitch[3];

#ifdef	SHARPEN
	int		strength[3];

	void	(*cleanf[3])(BYTE *dp, int dpitch, const BYTE *sp, int spitch, int hblocks, int remainder, int incpitch, int height, int strength);
#else
	void	(*cleanf[3])(BYTE *dp, int dpitch, const BYTE *sp, int spitch, int hblocks, int remainder, int incpitch, int height);
#endif

private:

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
	{
		PVideoFrame sf = child->GetFrame(n, env);
#ifdef	MODIFYPLUGIN
		PVideoFrame of = oclip->GetFrame(n, env);
#endif
		PVideoFrame df = env->NewVideoFrame(vi);
		
		int	i = planes; 
		do
		{
			BYTE* dp = GetWritePtr(df, i);
			int	dpitch = GetPitch(df, i);
#ifdef	MODIFYPLUGIN
			int	opitch = GetPitch(of, i);
			// copy the plane from sp to dp
			env->BitBlt(dp, dpitch, GetReadPtr(sf, i), GetPitch(sf, i), width[i], height[i]);
			cleanf[i](dp + dpitch, dpitch, GetReadPtr(of, i) + opitch, opitch, hblocks[i], remainder[i], incpitch[i], height2[i]);
#else	// MODIFYPLUGIN
			const BYTE* sp = GetReadPtr(sf, i);
			int	spitch = GetPitch(sf, i);
			// copy the first line
			memcpy(dp, sp, width[i]);
			dp += dpitch;
			sp += spitch; 
#ifdef	SHARPEN
			cleanf[i](dp, dpitch, sp, spitch, hblocks[i], remainder[i], incpitch[i], height2[i], strength[i]);
#else
			cleanf[i](dp, dpitch, sp, spitch, hblocks[i], remainder[i], incpitch[i], height2[i]);
#endif
			// copy the last line
			memcpy(dp + height2[i] * dpitch, sp + height2[i] * spitch, width[i]);
#endif	// MODIFYPLUGIN
		} while( --i >= 0 );
		SSE_EMMS
		return df;
	}
public:
#ifdef	MODIFYPLUGIN
	RemoveGrain(PClip clip, PClip _oclip, int *mode, bool planar) : GenericVideoFilter(clip), PlanarAccess(vi), oclip(_oclip)
#elif	defined(SHARPEN)
	RemoveGrain(PClip clip, int *mode, int *_strength, bool planar) : GenericVideoFilter(clip), PlanarAccess(vi)
#else
	RemoveGrain(PClip clip, int *mode, bool planar) : GenericVideoFilter(clip), PlanarAccess(vi)
#endif
	{
		if( vi.IsYV12() + planar == 0 )
#ifdef	MODIFYPLUGIN
			AVSenvironment->ThrowError("Repair: only planar color spaces are supported");
		CompareVideoInfo(vi, oclip->GetVideoInfo(), "Repair");;
		oclip->SetCacheHints(CACHE_NOTHING, 0);
#else
			AVSenvironment->ThrowError("RemoveGrain: only planar color spaces are supported");
#endif
		child->SetCacheHints(CACHE_NOTHING, 0);

		if( mode[2] < 0 )
		{
			planes--;
			if( mode[1] < 0 ) planes--;
		}
		
		if( mode[1] < 0 ) mode[1] = 0;

		int	i = planes;
		do
		{
#ifdef	SHARPEN
			strength[i] = (_strength[i] > MAXSTRENGTH ? MAXSTRENGTH : _strength[i]) * SSE_INCREMENT;
#endif
			if( mode[i] > MAXMODE ) AVSenvironment->ThrowError("RemoveGrain: invalid mode %u", mode[i]);
			if( mode[i] < 0 ) cleanf[i] = do_nothing;
			else cleanf[i] = cleaning_methods[mode[i]];
			height2[i] = height[i] - 2;	
			incpitch[i] = (SSE_INCREMENT + 2) - width[i];
#ifdef	MODIFYPLUGIN
			unsigned	w = width[i] - 3;	
#else
			unsigned	w = width[i] - 3 - SSE_INCREMENT;
#endif
			hblocks[i] = w / SSE_INCREMENT;
			remainder[i] = (w & (SSE_INCREMENT - 1)) - (SSE_INCREMENT - 1);
			//debug_printf("hblocks = %u, remainder = %i\n", hblocks[i], remainder[i]);
		} while( --i >= 0 );
		if( (hblocks[planes] <= 0) || (height2[planes] <= 0) ) 
				AVSenvironment->ThrowError("RemoveGrain: the width or height of the clip is too small");
	}
	//~RemoveGrain(){}
};


AVSValue __cdecl CreateRemoveGrain(AVSValue args, void* user_data, IScriptEnvironment* env)
{
#ifdef	MODIFYPLUGIN
	enum ARGS { CLIP, OCLIP, MY, MU, MV, PLANAR};
#elif	defined(SHARPEN)
	enum ARGS { CLIP, MY, MU, MV, SY, SU, SV, PLANAR};
#else
	enum ARGS { CLIP, MY, MU, MV, PLANAR};
#endif
	int		mode[3];
	mode[0] = args[MY].AsInt(DEFAULT_MODE);
	mode[1] = args[MU].AsInt(mode[0]);
	mode[2] = args[MV].AsInt(mode[1]);
#ifdef	SHARPEN
	int		strength[3];
	strength[0] = args[SY].AsInt(DEFAULT_STRENGTH);
	strength[1] = args[SU].AsInt(strength[0]);
	strength[2] = args[SV].AsInt(strength[1]);
#endif
#ifdef	MODIFYPLUGIN
	return  new RemoveGrain(args[CLIP].AsClip(), args[OCLIP].AsClip(), mode, args[PLANAR].AsBool(false));
#elif	defined(SHARPEN)
	return  new RemoveGrain(args[CLIP].AsClip(), mode, strength, args[PLANAR].AsBool(false));
#else
	return  new RemoveGrain(args[CLIP].AsClip(), mode, args[PLANAR].AsBool(false));
#endif
}

#ifdef	MODIFYPLUGIN

#if	ISSE > 1
#define	RepairPixel(dest, src1, src2, previous, next, reg1, reg2, reg3, reg4)	\
__asm	SSE3_MOVE	reg1,			next				\
__asm	SSE3_MOVE	reg3,			previous			\
__asm	SSE_RMOVE	reg2,			reg1				\
__asm	SSE3_MOVE	reg4,			src2				\
__asm	pminub		reg1,			reg3				\
__asm	pmaxub		reg2,			reg3				\
__asm	pminub		reg1,			reg4				\
__asm	SSE3_MOVE	reg3,			src1				\
__asm	pmaxub		reg2,			reg4				\
__asm	pmaxub		reg1,			reg3				\
__asm	pminub		reg1,			reg2				\
__asm	SSE_MOVE	dest,			reg1
#else
#define	RepairPixel(dest, src1, src2, previous, next, reg1, reg2, reg3, reg4)	\
__asm	SSE3_MOVE	reg1,			next				\
__asm	SSE3_MOVE	reg3,			previous			\
__asm	SSE_RMOVE	reg2,			reg1				\
__asm	SSE3_MOVE	reg4,			src2				\
__asm	pminub		reg1,			reg3				\
__asm	pmaxub		reg2,			reg3				\
__asm	pminub		reg1,			reg4				\
__asm	pmaxub		reg2,			reg4				\
__asm	pmaxub		reg1,			src1				\
__asm	pminub		reg1,			reg2				\
__asm	SSE_MOVE	dest,			reg1
#endif

static	void	temporal_repair(BYTE *dp, int dpitch, const BYTE *sp1, int spitch1, const BYTE *sp2, int spitch2, const BYTE *pp, int ppitch, const BYTE *np, int npitch, int width, int height)
{
	int	blocks = --width / SSE_INCREMENT;
	int	remainder = (width & (SSE_INCREMENT - 1)) - (SSE_INCREMENT - 1);
	width -= SSE_INCREMENT - 1;
	dpitch -= width;
	spitch1 -= width;
	spitch2 -= width;
	ppitch -= width;
	npitch -= width; 
__asm	mov			ebx,				pp
__asm	mov			edx,				sp1
__asm	mov			esi,				sp2
__asm	mov			edi,				dp
__asm	mov			eax,				np
__asm	mov			ecx,				blocks
__asm	align		16
__asm	_loop:
		RepairPixel([edi], [edx], [esi], [ebx], [eax], SSE0, SSE1, SSE2, SSE3)
__asm	add			eax,				SSE_INCREMENT
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT
__asm	add			edx,				SSE_INCREMENT
__asm	add			ebx,				SSE_INCREMENT
__asm	loop		_loop
// the last pixels
__asm	add			esi,				remainder
__asm	add			edi,				remainder
__asm	mov			ecx,				blocks
__asm	add			ebx,				remainder
__asm	add			eax,				remainder
		RepairPixel([edi], [edx], [esi], [ebx], [eax], SSE0, SSE1, SSE2, SSE3) 
__asm	add			esi,				spitch2
__asm	add			edi,				dpitch
__asm	add			edx,				spitch1
__asm	add			ebx,				ppitch
__asm	add			eax,				npitch
__asm	dec			height
__asm	jnz			_loop
}

#if	ISSE > 1
#define	BRepairPixel(dest, src1, src2, previous, next, reg1, reg2, reg3, reg4, reg5, reg6)	\
__asm	SSE3_MOVE	reg1,			next				\
__asm	SSE3_MOVE	reg3,			previous			\
__asm	SSE_RMOVE	reg2,			reg1				\
__asm	SSE3_MOVE	reg4,			src2				\
__asm	pmaxub		reg2,			reg3				\
__asm	SSE_RMOVE	reg5,			reg4				\
__asm	pminub		reg1,			reg3				\
__asm	SSE_RMOVE	reg6,			reg2				\
__asm	psubusb		reg5,			reg1				\
__asm	psubusb		reg6,			reg4				\
__asm	SSE_RMOVE	reg3,			reg2				\
__asm	paddusb		reg5,			reg5				\
__asm	paddusb		reg6,			reg6				\
__asm	paddusb		reg5,			reg1				\
__asm	psubusb		reg3,			reg6				\
__asm	pminub		reg5,			reg2				\
__asm	pmaxub		reg3,			reg1				\
__asm	SSE3_MOVE	reg6,			src1				\
__asm	pcmpeqb		reg1,			reg5				\
__asm	pcmpeqb		reg2,			reg3				\
__asm	pminub		reg5,			reg6				\
__asm	pmaxub		reg1,			reg2				\
__asm	pmaxub		reg5,			reg3				\
__asm	pminub		reg4,			reg1				\
__asm	psubusb		reg5,			reg1				\
__asm	pmaxub		reg4,			reg5				\
__asm	SSE_MOVE	dest,			reg4
#else
#define	BRepairPixel(dest, src1, src2, previous, next, reg1, reg2, reg3, reg4, reg5, reg6)	\
__asm	SSE3_MOVE	reg1,			next				\
__asm	SSE3_MOVE	reg3,			previous			\
__asm	SSE_RMOVE	reg2,			reg1				\
__asm	SSE3_MOVE	reg4,			src2				\
__asm	pmaxub		reg2,			reg3				\
__asm	SSE_RMOVE	reg5,			reg4				\
__asm	pminub		reg1,			reg3				\
__asm	SSE_RMOVE	reg6,			reg2				\
__asm	psubusb		reg5,			reg1				\
__asm	psubusb		reg6,			reg4				\
__asm	SSE_RMOVE	reg3,			reg2				\
__asm	paddusb		reg5,			reg5				\
__asm	paddusb		reg6,			reg6				\
__asm	paddusb		reg5,			reg1				\
__asm	psubusb		reg3,			reg6				\
__asm	pminub		reg5,			reg2				\
__asm	pmaxub		reg3,			reg1				\
__asm	pcmpeqb		reg1,			reg5				\
__asm	pcmpeqb		reg2,			reg3				\
__asm	pminub		reg5,			src1				\
__asm	pmaxub		reg1,			reg2				\
__asm	pmaxub		reg5,			reg3				\
__asm	pminub		reg4,			reg1				\
__asm	psubusb		reg5,			reg1				\
__asm	pmaxub		reg4,			reg5				\
__asm	SSE_MOVE	dest,			reg4
#endif	// ISSE > 1

static	void	btemporal_repair(BYTE *dp, int dpitch, const BYTE *sp1, int spitch1, const BYTE *sp2, int spitch2, const BYTE *pp, int ppitch, const BYTE *np, int npitch, int width, int height)
{
	int	blocks = --width / SSE_INCREMENT;
	int	remainder = (width & (SSE_INCREMENT - 1)) - (SSE_INCREMENT - 1);
	width -= SSE_INCREMENT - 1;
	dpitch -= width;
	spitch1 -= width;
	spitch2 -= width;
	ppitch -= width;
	npitch -= width; 
__asm	mov			ebx,				pp
__asm	mov			edx,				sp1
__asm	mov			esi,				sp2
__asm	mov			edi,				dp
__asm	mov			eax,				np
__asm	mov			ecx,				blocks
__asm	align		16
__asm	_loop:
		BRepairPixel([edi], [edx], [esi], [ebx], [eax], SSE0, SSE1, SSE2, SSE3, SSE4, SSE5)
__asm	add			eax,				SSE_INCREMENT
__asm	add			esi,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT
__asm	add			edx,				SSE_INCREMENT
__asm	add			ebx,				SSE_INCREMENT
__asm	loop		_loop
// the last pixels
__asm	add			esi,				remainder
__asm	add			edi,				remainder
__asm	mov			ecx,				blocks
__asm	add			ebx,				remainder
__asm	add			eax,				remainder
		BRepairPixel([edi], [edx], [esi], [ebx], [eax], SSE0, SSE1, SSE2, SSE3, SSE4, SSE5) 
__asm	add			esi,				spitch2
__asm	add			edi,				dpitch
__asm	add			edx,				spitch1
__asm	add			ebx,				ppitch
__asm	add			eax,				npitch
__asm	dec			height
__asm	jnz			_loop
}

class	TemporalRepair : public GenericVideoFilter, public PlanarAccess
{
	void			(*trepair)(BYTE *dp, int dpitch, const BYTE *sp1, int spitch1, const BYTE *sp2, int spitch2, const BYTE *pp, int ppitch, const BYTE *np, int npitch, int width, int height);
	unsigned		last_frame;
	PClip			orig;
	
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
	{
		if( ((unsigned)(n - 1) >= last_frame) ) return child->GetFrame(n, env);
		PVideoFrame	pf = orig->GetFrame(n - 1, env);
		PVideoFrame	sf = orig->GetFrame(n, env);
		PVideoFrame	nf = orig->GetFrame(n + 1, env);
		PVideoFrame cf = child->GetFrame(n, env);
		PVideoFrame	df = env->NewVideoFrame(vi);

		int i = planes;
		do
		{
			trepair(GetWritePtr(df, i), GetPitch(df, i), GetReadPtr(cf, i), GetPitch(cf, i), GetReadPtr(sf, i), GetPitch(sf, i), GetReadPtr(pf, i), GetPitch(pf, i), GetReadPtr(nf, i), GetPitch(nf, i), width[i], height[i]);
		} while( --i >= 0 );
		SSE_EMMS
		return df;
	}

public:
	TemporalRepair(PClip clip, PClip oclip, int mode, bool grey, bool planar) : GenericVideoFilter(clip), PlanarAccess(vi, planar && grey), orig(oclip)
	{
		CompareVideoInfo(vi, orig->GetVideoInfo(), "TemporalRepair");
		child->SetCacheHints(CACHE_RANGE, 0);
		orig->SetCacheHints(CACHE_RANGE, 2);
		trepair = mode ? btemporal_repair : temporal_repair;
		last_frame = vi.num_frames - 2;
		if( (int) last_frame < 0 ) last_frame = 0;
		if( grey ) planes = 0;
	}

	//~TemporalRepair(){}
};

#define	get_lu(lower, upper, previous, current, next, reg1, reg2)	\
__asm	SSE3_MOVE	upper,			next				\
__asm	SSE3_MOVE	reg1,			previous			\
__asm	SSE_RMOVE	reg2,			upper				\
__asm	SSE3_MOVE	lower,			current				\
__asm	pmaxub		upper,			reg1				\
__asm	pminub		reg2,			reg1				\
__asm	psubusb		upper,			lower				\
__asm	psubusb		lower,			reg2

#if	ISSE > 1
#define	SmoothTRepair(dest, lower, upper, previous, current, next, reg1, reg2)	\
__asm	SSE3_MOVE	reg1,			current				\
__asm	SSE3_MOVE	reg2,			previous			\
__asm	paddusb		upper,			reg1				\
__asm	psubusb		reg1,			lower				\
__asm	pmaxub		upper,			reg2				\
__asm	SSE3_MOVE	lower,			next				\
__asm	pminub		reg1,			reg2				\
__asm	pmaxub		upper,			lower				\
__asm	SSE3_MOVE	reg2,			dest				\
__asm	pminub		reg1,			lower				\
__asm	pminub		upper,			reg2				\
__asm	pmaxub		upper,			reg1				\
__asm	SSE_MOVE	dest,			upper
#else
#define	SmoothTRepair(dest, lower, upper, previous, current, next, reg1, reg2)	\
__asm	SSE3_MOVE	reg1,			current				\
__asm	SSE3_MOVE	reg2,			previous			\
__asm	paddusb		upper,			reg1				\
__asm	psubusb		reg1,			lower				\
__asm	pmaxub		upper,			reg2				\
__asm	SSE3_MOVE	lower,			next				\
__asm	pminub		reg1,			reg2				\
__asm	pmaxub		upper,			lower				\
__asm	pminub		reg1,			lower				\
__asm	pminub		upper,			dest				\
__asm	pmaxub		upper,			reg1				\
__asm	SSE_MOVE	dest,			upper
#endif

void	smooth_temporal_repair1(BYTE *dp, const BYTE *previous, const BYTE *_sp, const BYTE *next, int pitch, int hblocks, int height, int remainder)
{	
__asm	mov			eax,				hblocks
__asm	mov			ecx,				eax
__asm	mov			edx,				previous
__asm	mov			esi,				_sp
__asm	shl			eax,				SSE_SHIFT
__asm	mov			edi,				dp
__asm	add			eax,				remainder
__asm	mov			ebx,				pitch
__asm	sub			pitch,				eax
__asm	lea			edi,				[edi + ebx + 1]
__asm	mov			eax,				next
__asm	align		16
__asm	middle_loop:
		get_lu(SSE0, SSE1, [edx], [esi], [eax], SSE6, SSE7)
		get_lu(SSE2, SSE3, [edx + 1], [esi + 1], [eax + 1], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + 2], [esi + 2], [eax + 2], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + 2*ebx], [esi + 2*ebx], [eax + 2*ebx], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + 2*ebx + 1], [esi + 2*ebx + 1], [eax + 2*ebx + 1], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + 2*ebx + 2], [esi + 2*ebx + 2], [eax + 2*ebx + 2], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + ebx], [esi + ebx], [eax + ebx], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + ebx + 2], [esi + ebx + 2], [eax + ebx + 2], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		SmoothTRepair([edi], SSE0, SSE1, [edx + ebx + 1], [esi + ebx + 1], [eax + ebx + 1], SSE6, SSE7)
__asm	add			esi,				SSE_INCREMENT
__asm	add			edx,				SSE_INCREMENT
__asm	add			eax,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				remainder
__asm	add			edx,				remainder
__asm	add			eax,				remainder
__asm	add			edi,				remainder
		get_lu(SSE0, SSE1, [edx], [esi], [eax], SSE6, SSE7)
		get_lu(SSE2, SSE3, [edx + 1], [esi + 1], [eax + 1], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + 2], [esi + 2], [eax + 2], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + 2*ebx], [esi + 2*ebx], [eax + 2*ebx], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + 2*ebx + 1], [esi + 2*ebx + 1], [eax + 2*ebx + 1], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + 2*ebx + 2], [esi + 2*ebx + 2], [eax + 2*ebx + 2], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + ebx], [esi + ebx], [eax + ebx], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + ebx + 2], [esi + ebx + 2], [eax + ebx + 2], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		SmoothTRepair([edi], SSE0, SSE1, [edx + ebx + 1], [esi + ebx + 1], [eax + ebx + 1], SSE6, SSE7)
__asm	add			esi,				pitch
__asm	add			edx,				pitch
__asm	add			eax,				pitch
__asm	add			edi,				pitch
__asm	dec			height
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
}

#ifdef	SMOOTH2

#define	get_lu_reg(lower, upper, previous, current, next, reg1, reg2)	\
__asm	SSE3_MOVE	upper,			next				\
__asm	SSE3_MOVE	reg1,			previous			\
__asm	SSE_RMOVE	reg2,			upper				\
__asm	SSE_RMOVE	lower,			current				\
__asm	pmaxub		upper,			reg1				\
__asm	pminub		reg2,			reg1				\
__asm	psubusb		upper,			lower				\
__asm	psubusb		lower,			reg2

#if	ISSE > 1
#define	SmoothTRepair2(dest, lower, upper, previous, current, next, reg1, reg2, reg3, reg4, reg5)	\
__asm	SSE3_MOVE	reg1,			current				\
		get_lu_reg(reg4, reg5, previous, reg1, next, reg2, reg3)	\
__asm	pmaxub		upper,			reg5				\
__asm	pmaxub		lower,			reg4				\
__asm	SSE_RMOVE	reg2,			reg1				\
__asm	pmaxub		upper,			lower				\
__asm	SSE3_MOVE	reg3,			dest				\
__asm	paddusb		reg1,			upper				\
__asm	psubusb		reg2,			upper				\
__asm	pminub		reg1,			reg3				\
__asm	pmaxub		reg1,			reg2				\
__asm	SSE_MOVE	dest,			reg1
#else
#define	SmoothTRepair2(dest, lower, upper, previous, current, next, reg1, reg2, reg3, reg4, reg5)	\
__asm	SSE3_MOVE	reg1,			current				\
		get_lu_reg(reg4, reg5, previous, reg1, next, reg2, reg3)	\
__asm	pmaxub		upper,			reg5				\
__asm	pmaxub		lower,			reg4				\
__asm	SSE_RMOVE	reg2,			reg1				\
__asm	pmaxub		upper,			lower				\
__asm	paddusb		reg1,			upper				\
__asm	psubusb		reg2,			upper				\
__asm	pminub		reg1,			dest				\
__asm	pmaxub		reg1,			reg2				\
__asm	SSE_MOVE	dest,			reg1
#endif

void	smooth_temporal_repair2(BYTE *dp, const BYTE *previous, const BYTE *_sp, const BYTE *next, int pitch, int hblocks, int height, int remainder)
{	
__asm	mov			eax,				hblocks
__asm	mov			ecx,				eax
__asm	mov			edx,				previous
__asm	mov			esi,				_sp
__asm	shl			eax,				SSE_SHIFT
__asm	mov			edi,				dp
__asm	add			eax,				remainder
__asm	mov			ebx,				pitch
__asm	sub			pitch,				eax
__asm	lea			edi,				[edi + ebx + 1]
__asm	mov			eax,				next
__asm	align		16
__asm	middle_loop:
		get_lu(SSE0, SSE1, [edx], [esi], [eax], SSE6, SSE7)
		get_lu(SSE2, SSE3, [edx + 1], [esi + 1], [eax + 1], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + 2], [esi + 2], [eax + 2], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + 2*ebx], [esi + 2*ebx], [eax + 2*ebx], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + 2*ebx + 1], [esi + 2*ebx + 1], [eax + 2*ebx + 1], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + 2*ebx + 2], [esi + 2*ebx + 2], [eax + 2*ebx + 2], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + ebx], [esi + ebx], [eax + ebx], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + ebx + 2], [esi + ebx + 2], [eax + ebx + 2], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		SmoothTRepair2([edi], SSE0, SSE1, [edx + ebx + 1], [esi + ebx + 1], [eax + ebx + 1], SSE4, SSE5, SSE6, SSE7, SSE3)
__asm	add			esi,				SSE_INCREMENT
__asm	add			edx,				SSE_INCREMENT
__asm	add			eax,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				remainder
__asm	add			edx,				remainder
__asm	add			eax,				remainder
__asm	add			edi,				remainder
		get_lu(SSE0, SSE1, [edx], [esi], [eax], SSE6, SSE7)
		get_lu(SSE2, SSE3, [edx + 1], [esi + 1], [eax + 1], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + 2], [esi + 2], [eax + 2], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + 2*ebx], [esi + 2*ebx], [eax + 2*ebx], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + 2*ebx + 1], [esi + 2*ebx + 1], [eax + 2*ebx + 1], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + 2*ebx + 2], [esi + 2*ebx + 2], [eax + 2*ebx + 2], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + ebx], [esi + ebx], [eax + ebx], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		get_lu(SSE2, SSE3, [edx + ebx + 2], [esi + ebx + 2], [eax + ebx + 2], SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
__asm	pmaxub		SSE0,				SSE2
		SmoothTRepair2([edi], SSE0, SSE1, [edx + ebx + 1], [esi + ebx + 1], [eax + ebx + 1], SSE4, SSE5, SSE6, SSE7, SSE3)
__asm	add			esi,				pitch
__asm	add			edx,				pitch
__asm	add			eax,				pitch
__asm	add			edi,				pitch
__asm	dec			height
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
}

#define	get2diff(pdiff, ndiff, previous, current, next, reg1, reg2, reg3)	\
__asm	SSE3_MOVE	reg3,			current				\
__asm	SSE3_MOVE	pdiff,			previous			\
__asm	SSE_RMOVE	reg1,			reg3				\
__asm	SSE3_MOVE	ndiff,			next				\
__asm	SSE_RMOVE	reg2,			reg3				\
__asm	psubusb		reg1,			pdiff				\
__asm	psubusb		reg2,			ndiff				\
__asm	psubusb		ndiff,			reg3				\
__asm	psubusb		pdiff,			reg3				\
__asm	pmaxub		pdiff,			reg1				\
__asm	pmaxub		ndiff,			reg2

#if	ISSE > 1
#define	SmoothTRepair3(dest, pmax, nmax, previous, current, next, reg1, reg2, reg3, reg4, reg5)	\
		get2diff(reg4, reg5, previous, current, next, reg2, reg3, reg1)	\
__asm	pmaxub		pmax,			reg4				\
__asm	pmaxub		nmax,			reg5				\
__asm	SSE_RMOVE	reg2,			reg1				\
__asm	pminub		pmax,			nmax				\
__asm	SSE3_MOVE	reg3,			dest				\
__asm	paddusb		reg1,			pmax				\
__asm	psubusb		reg2,			pmax				\
__asm	pminub		reg1,			reg3				\
__asm	pmaxub		reg1,			reg2				\
__asm	SSE_MOVE	dest,			reg1
#else
#define	SmoothTRepair3(dest, pmax, nmax, previous, current, next, reg1, reg2, reg3, reg4, reg5)	\
		get2diff(reg4, reg5, previous, current, next, reg2, reg3, reg1)	\
__asm	pmaxub		pmax,			reg4				\
__asm	pmaxub		nmax,			reg5				\
__asm	SSE_RMOVE	reg2,			reg1				\
__asm	pminub		pmax,			nmax				\
__asm	paddusb		reg1,			pmax				\
__asm	psubusb		reg2,			pmax				\
__asm	pminub		reg1,			dest				\
__asm	pmaxub		reg1,			reg2				\
__asm	SSE_MOVE	dest,			reg1
#endif

void	smooth_temporal_repair3(BYTE *dp, const BYTE *previous, const BYTE *_sp, const BYTE *next, int pitch, int hblocks, int height, int remainder)
{	
__asm	mov			eax,				hblocks
__asm	mov			ecx,				eax
__asm	mov			edx,				previous
__asm	mov			esi,				_sp
__asm	shl			eax,				SSE_SHIFT
__asm	mov			edi,				dp
__asm	add			eax,				remainder
__asm	mov			ebx,				pitch
__asm	sub			pitch,				eax
__asm	lea			edi,				[edi + ebx + 1]
__asm	mov			eax,				next
__asm	align		16
__asm	middle_loop:
		get2diff(SSE0, SSE1, [edx], [esi], [eax], SSE5, SSE6, SSE7)
		get2diff(SSE2, SSE3, [edx + 1], [esi + 1], [eax + 1], SSE5, SSE6, SSE7)
__asm	pmaxub		SSE0,				SSE2
__asm	pmaxub		SSE1,				SSE3
		get2diff(SSE2, SSE3, [edx + 2], [esi + 2], [eax + 2], SSE5, SSE6, SSE7)
__asm	pmaxub		SSE0,				SSE2
__asm	pmaxub		SSE1,				SSE3
		get2diff(SSE2, SSE3, [edx + 2*ebx], [esi + 2*ebx], [eax + 2*ebx], SSE5, SSE6, SSE7)
__asm	pmaxub		SSE0,				SSE2
__asm	pmaxub		SSE1,				SSE3
		get2diff(SSE2, SSE3, [edx + 2*ebx + 1], [esi + 2*ebx + 1], [eax + 2*ebx + 1], SSE5, SSE6, SSE7)
__asm	pmaxub		SSE0,				SSE2
__asm	pmaxub		SSE1,				SSE3
		get2diff(SSE2, SSE3, [edx + 2*ebx + 2], [esi + 2*ebx + 2], [eax + 2*ebx + 2], SSE5, SSE6, SSE7)
__asm	pmaxub		SSE1,				SSE3
		get2diff(SSE2, SSE3, [edx + ebx], [esi + ebx], [eax + ebx], SSE5, SSE6, SSE7)
__asm	pmaxub		SSE0,				SSE2
__asm	pmaxub		SSE1,				SSE3
		get2diff(SSE2, SSE3, [edx + ebx + 2], [esi + ebx + 2], [eax + ebx + 2], SSE5, SSE6, SSE7)
__asm	pmaxub		SSE0,				SSE2
__asm	pmaxub		SSE1,				SSE3
		SmoothTRepair3([edi], SSE0, SSE1, [edx + ebx + 1], [esi + ebx + 1], [eax + ebx + 1], SSE4, SSE5, SSE6, SSE7, SSE3)
__asm	add			esi,				SSE_INCREMENT
__asm	add			edx,				SSE_INCREMENT
__asm	add			eax,				SSE_INCREMENT
__asm	add			edi,				SSE_INCREMENT
__asm	dec			ecx
__asm	jnz			middle_loop
// the last pixels
__asm	add			esi,				remainder
__asm	add			edx,				remainder
__asm	add			eax,				remainder
__asm	add			edi,				remainder
		get2diff(SSE0, SSE1, [edx], [esi], [eax], SSE5, SSE6, SSE7)
		get2diff(SSE2, SSE3, [edx + 1], [esi + 1], [eax + 1], SSE5, SSE6, SSE7)
__asm	pmaxub		SSE0,				SSE2
__asm	pmaxub		SSE1,				SSE3
		get2diff(SSE2, SSE3, [edx + 2], [esi + 2], [eax + 2], SSE5, SSE6, SSE7)
__asm	pmaxub		SSE0,				SSE2
__asm	pmaxub		SSE1,				SSE3
		get2diff(SSE2, SSE3, [edx + 2*ebx], [esi + 2*ebx], [eax + 2*ebx], SSE5, SSE6, SSE7)
__asm	pmaxub		SSE0,				SSE2
__asm	pmaxub		SSE1,				SSE3
		get2diff(SSE2, SSE3, [edx + 2*ebx + 1], [esi + 2*ebx + 1], [eax + 2*ebx + 1], SSE5, SSE6, SSE7)
__asm	pmaxub		SSE0,				SSE2
__asm	pmaxub		SSE1,				SSE3
		get2diff(SSE2, SSE3, [edx + 2*ebx + 2], [esi + 2*ebx + 2], [eax + 2*ebx + 2], SSE5, SSE6, SSE7)
__asm	pmaxub		SSE0,				SSE2
__asm	pmaxub		SSE1,				SSE3
		get2diff(SSE2, SSE3, [edx + ebx], [esi + ebx], [eax + ebx], SSE5, SSE6, SSE7)
__asm	pmaxub		SSE0,				SSE2
__asm	pmaxub		SSE1,				SSE3
		get2diff(SSE2, SSE3, [edx + ebx + 2], [esi + ebx + 2], [eax + ebx + 2], SSE5, SSE6, SSE7)
__asm	pmaxub		SSE0,				SSE2
__asm	pmaxub		SSE1,				SSE3
		SmoothTRepair3([edi], SSE0, SSE1, [edx + ebx + 1], [esi + ebx + 1], [eax + ebx + 1], SSE4, SSE5, SSE6, SSE7, SSE3)
__asm	add			esi,				pitch
__asm	add			edx,				pitch
__asm	add			eax,				pitch
__asm	add			edi,				pitch
__asm	dec			height
__asm	mov			ecx,				hblocks
__asm	jnz			middle_loop
}
#endif	// SMOOTH2

class SmoothTemporalRepair : public GenericVideoFilter, public PlanarAccess
{
	HomogeneousChild	oclip;

#ifdef	SMOOTH2
	void	(*st_repair)(BYTE *dp, const BYTE *previous, const BYTE *_sp, const BYTE *next,int pitch, int hblocks, int height, int remainder);
#else
#define		st_repair	smooth_temporal_repair1
#endif

	int		height2[3], hblocks[3], remainder[3];
	unsigned		last_frame;

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
	{
		if( ((unsigned)(n - 1) >= last_frame) ) return child->GetFrame(n, env);
		PVideoFrame sf = child->GetFrame(n, env);
		PVideoFrame pf = oclip->GetFrame(n - 1, env);
		PVideoFrame of = oclip->GetFrame(n, env);
		PVideoFrame nf = oclip->GetFrame(n + 1, env);
		PVideoFrame df = env->NewVideoFrame(vi);
		
		int	i = planes;
		do
		{
			BYTE* dp = GetWritePtr(df,i);
			int	pitch = GetPitch(df, i);
			// copy the plane from sp to dp
			env->BitBlt(dp, pitch, GetReadPtr(sf, i), pitch, width[i], height[i]);
			st_repair(dp, GetReadPtr(pf, i), GetReadPtr(of, i), GetReadPtr(nf, i), pitch, hblocks[i], height2[i], remainder[i]);
		} while( --i >= 0 );
		SSE_EMMS
		return df;
	}
public:
	SmoothTemporalRepair(PClip clip, PClip _oclip, int mode, bool grey, bool planar, IScriptEnvironment* env) : GenericVideoFilter(clip), PlanarAccess(vi), oclip(_oclip, grey, env)
	{
		if( vi.IsYV12() + planar == 0 ) AVSenvironment->ThrowError("TemporalRepair: only planar color spaces are supported");
		CompareVideoInfo(vi, _oclip->GetVideoInfo(), "TemporalRepair");
		_oclip->SetCacheHints(CACHE_RANGE, 2);
		child->SetCacheHints(CACHE_NOTHING, 0);

#ifdef	SMOOTH2	
		switch( mode )
		{
			case 1 :
				st_repair = smooth_temporal_repair1;
				break;
			case 2 :
				st_repair = smooth_temporal_repair2;
				break;
			default :
				st_repair = smooth_temporal_repair3;
		}
#endif	// SMOOTH2

		if( grey ) planes = 0;

		last_frame = vi.num_frames - 2;
		if( (int) last_frame < 0 ) last_frame = 0;
		
		int	i = planes;
		do
		{
			height2[i] = height[i] - 2;	
			// unsigned	w = width[i] - 1 - 2*smooth;
			unsigned	w = width[i] - 3;
			hblocks[i] = w / SSE_INCREMENT;
			remainder[i] = (w & (SSE_INCREMENT - 1)) - (SSE_INCREMENT - 1);
		} while( --i >= 0 );

		if( (hblocks[planes] <= 0) || (height2[planes] <= 0) ) 
				AVSenvironment->ThrowError("TemporalRepair: the width or height of the clip is too small");
	}
	//~SmoothTemporalRepair(){}
};

#define	MAXTMODE	4	

bool	spatial[MAXTMODE + 1] = {false, true, true, true, false };

AVSValue __cdecl CreateTemporalRepair(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	enum ARGS { CLIP, OCLIP, MODE, SMOOTH, GREY, PLANAR };
	PClip	clip = args[CLIP].AsClip();
	PClip	oclip = args[OCLIP].AsClip();
	bool	grey = args[GREY].AsBool(false);
	int		mode = args[MODE].AsInt(args[SMOOTH].AsInt(0));
	if( (unsigned) mode > MAXTMODE ) env->ThrowError("TemporalRepair: illegal mode %i", mode);
	bool	planar = args[PLANAR].AsBool(false);
	return	spatial[mode] ? (AVSValue) new SmoothTemporalRepair(clip, oclip, mode, grey, planar, env)
										: (AVSValue) new TemporalRepair(clip, oclip, mode, grey, planar);
};

#else	// MODIFYPLUGIN

class	GenericClense : public GenericVideoFilter, public PlanarAccess
{
protected:
	int	hblocks[3];
	int	remainder[3];
	int	incpitch[3];
	
public:
	GenericClense(PClip clip, bool grey, bool planar);
};

GenericClense::GenericClense(PClip clip, bool grey, bool planar) : GenericVideoFilter(clip), PlanarAccess(vi, planar && grey)
{
	if( grey ) planes = 0;
	int	i = planes;
	do
	{
		int	w = width[i];
		hblocks[i] = --w / (2*SSE_INCREMENT);
		remainder[i] = (w & (2*SSE_INCREMENT - 1)) - (2*SSE_INCREMENT - 1);
		incpitch[i] = 2*SSE_INCREMENT - width[i] + remainder[i];
	} while( --i >= 0 );
}

#if	SHARPEN == 1
// only sharpen
#define	simplesharpen(center, min, max, reg1, reg2)\
__asm	SSE_RMOVE	reg2,				center			\
__asm	psubusb		max,				center			\
__asm	psubusb		reg2,				min				\
__asm	SSE_RMOVE	reg1,				max				\
__asm	SSE_RMOVE	min,				reg2			\
__asm	psubusb		max,				reg2			\
__asm	psubusb		min,				reg1			\
__asm	pminub		reg2,				max				\
__asm	pminub		reg1,				min				\
__asm	psubusb		center,				reg2			\
__asm	paddusb		center,				reg1

#elif	SHARPEN == 2
#define	simplesharpen(center, min, max, reg1, reg2)\
__asm	pminub		center,				max				\
__asm	pmaxub		center,				min				\
__asm	SSE_RMOVE	reg2,				center			\
__asm	psubusb		max,				center			\
__asm	psubusb		reg2,				min				\
__asm	SSE_RMOVE	reg1,				max				\
__asm	SSE_RMOVE	min,				reg2			\
__asm	psubusb		max,				reg2			\
__asm	psubusb		min,				reg1			\
__asm	pminub		reg2,				max				\
__asm	pminub		reg1,				min				\
__asm	psubusb		center,				reg2			\
__asm	paddusb		center,				reg1
#endif	// SHARPEN == 2

#if	ISSE > 1
static inline void aligned_clense(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, const BYTE *pp, int ppitch, const BYTE *np, int npitch, int hblocks, int remainder, int incpitch, int height)
#else
static void clense(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, const BYTE *pp, int ppitch, const BYTE *np, int npitch, int hblocks, int remainder, int incpitch, int height)
#endif
#ifdef	SHARPEN
#define	AClensePixel(daddr, saddr, paddr, naddr, reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8)	\
__asm	SSE_RMOVE	reg1,			[naddr]					\
__asm	SSE_RMOVE	reg2,			[naddr + SSE_INCREMENT]	\
__asm	SSE_RMOVE	reg3,			reg1					\
__asm	SSE_RMOVE	reg5,			[paddr]					\
__asm	SSE_RMOVE	reg4,			reg2					\
__asm	SSE_RMOVE	reg6,			[paddr + SSE_INCREMENT]	\
__asm	pminub		reg1,			reg5					\
__asm	pminub		reg2,			reg6					\
__asm	SSE_RMOVE	reg7,			[saddr]					\
__asm	pmaxub		reg3,			reg5					\
__asm	pmaxub		reg4,			reg6					\
__asm	SSE_RMOVE	reg8,			[saddr + SSE_INCREMENT]	\
		simplesharpen(reg7, reg1, reg3, reg5, reg6)			\
		simplesharpen(reg8, reg2, reg4, reg5, reg6)			\
__asm	SSE_RMOVE	[daddr],		reg7					\
__asm	SSE_RMOVE	[daddr + SSE_INCREMENT], reg8

#define	UClensePixel(daddr, saddr, paddr, naddr, reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8)	\
__asm	SSE3_MOVE	reg1,			[naddr]					\
__asm	SSE3_MOVE	reg2,			[naddr + SSE_INCREMENT]	\
__asm	SSE_RMOVE	reg3,			reg1					\
__asm	SSE3_MOVE	reg5,			[paddr]					\
__asm	SSE_RMOVE	reg4,			reg2					\
__asm	SSE3_MOVE	reg6,			[paddr + SSE_INCREMENT]	\
__asm	pminub		reg1,			reg5					\
__asm	pminub		reg2,			reg6					\
__asm	SSE3_MOVE	reg7,			[saddr]					\
__asm	pmaxub		reg3,			reg5					\
__asm	pmaxub		reg4,			reg6					\
__asm	SSE3_MOVE	reg8,			[saddr + SSE_INCREMENT]	\
		simplesharpen(reg7, reg1, reg3, reg5, reg6)			\
		simplesharpen(reg8, reg2, reg4, reg5, reg6)			\
__asm	SSE_MOVE	[daddr],		reg7					\
__asm	SSE_MOVE	[daddr + SSE_INCREMENT], reg8
#else	// SHARPEN
#define	AClensePixel(daddr, saddr, paddr, naddr, reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8)	\
__asm	SSE_RMOVE	reg1,			[naddr]					\
__asm	SSE_RMOVE	reg2,			[naddr + SSE_INCREMENT]	\
__asm	SSE_RMOVE	reg3,			reg1					\
__asm	SSE_RMOVE	reg5,			[paddr]					\
__asm	SSE_RMOVE	reg4,			reg2					\
__asm	SSE_RMOVE	reg6,			[paddr + SSE_INCREMENT]	\
__asm	pminub		reg1,			reg5					\
__asm	pminub		reg2,			reg6					\
__asm	pmaxub		reg1,			[saddr]					\
__asm	pmaxub		reg3,			reg5					\
__asm	pmaxub		reg4,			reg6					\
__asm	pmaxub		reg2,			[saddr + SSE_INCREMENT]	\
__asm	pminub		reg1,			reg3					\
__asm	pminub		reg2,			reg4					\
__asm	SSE_RMOVE	[daddr],		reg1					\
__asm	SSE_RMOVE	[daddr + SSE_INCREMENT], reg2

#if		ISSE > 1
#define	UClensePixel(daddr, saddr, paddr, naddr, reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8)	\
__asm	SSE3_MOVE	reg1,			[naddr]					\
__asm	SSE3_MOVE	reg2,			[naddr + SSE_INCREMENT]	\
__asm	SSE_RMOVE	reg3,			reg1					\
__asm	SSE3_MOVE	reg5,			[paddr]					\
__asm	SSE_RMOVE	reg4,			reg2					\
__asm	SSE3_MOVE	reg6,			[paddr + SSE_INCREMENT]	\
__asm	pminub		reg1,			reg5					\
__asm	pminub		reg2,			reg6					\
__asm	SSE3_MOVE	reg7,			[saddr]					\
__asm	pmaxub		reg3,			reg5					\
__asm	pmaxub		reg4,			reg6					\
__asm	SSE3_MOVE	reg8,			[saddr + SSE_INCREMENT]	\
__asm	pmaxub		reg1,			reg7					\
__asm	pmaxub		reg2,			reg8					\
__asm	pminub		reg1,			reg3					\
__asm	pminub		reg2,			reg4					\
__asm	SSE_MOVE	[daddr],		reg1					\
__asm	SSE_MOVE	[daddr + SSE_INCREMENT], reg2
#else
#define	UClensePixel(daddr, saddr, paddr, naddr, reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8)	\
		AClensePixel(daddr, saddr, paddr, naddr, reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8)
#endif
#endif	// SHARPEN
{
__asm	mov			eax,				incpitch
__asm	mov			ebx,				pp
__asm	add			dpitch,				eax
__asm	add			spitch,				eax
__asm	add			ppitch,				eax
__asm	add			npitch,				eax
__asm	mov			esi,				_sp
__asm	mov			edi,				dp
__asm	mov			edx,				remainder
__asm	mov			eax,				np
__asm	mov			ecx,				hblocks
__asm	align		16
__asm	_loop:
		AClensePixel(edi, esi, ebx, eax, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6, SSE7)		 
__asm	add			eax,				2*SSE_INCREMENT
__asm	add			esi,				2*SSE_INCREMENT
__asm	add			edi,				2*SSE_INCREMENT
__asm	add			ebx,				2*SSE_INCREMENT
#if	defined(SHARPEN) && (ISSE > 1)
__asm	dec			ecx
__asm	jnz			_loop
#else
__asm	loop		_loop
#endif
// the last pixels
		UClensePixel(edi + edx, esi + edx, ebx + edx, eax + edx, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6, SSE7)
__asm	add			esi,				spitch
__asm	add			edi,				dpitch
__asm	add			ebx,				ppitch
__asm	add			eax,				npitch
__asm	dec			height
__asm	mov			ecx,				hblocks
__asm	jnz			_loop
}

#if		ISSE > 1
static inline void unaligned_clense(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, const BYTE *pp, int ppitch, const BYTE *np, int npitch, int hblocks, int remainder, int incpitch, int height)
{
__asm	mov			eax,				incpitch
__asm	mov			ebx,				pp
__asm	add			dpitch,				eax
__asm	add			spitch,				eax
__asm	add			ppitch,				eax
__asm	add			npitch,				eax
__asm	mov			esi,				_sp
__asm	mov			edi,				dp
__asm	mov			edx,				remainder
__asm	mov			eax,				np
__asm	mov			ecx,				hblocks
__asm	align		16
__asm	_loop:
		UClensePixel(edi, esi, ebx, eax, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6, SSE7)
__asm	add			eax,				2*SSE_INCREMENT
__asm	add			esi,				2*SSE_INCREMENT
__asm	add			edi,				2*SSE_INCREMENT
__asm	add			ebx,				2*SSE_INCREMENT
#if	defined(SHARPEN) 
__asm	dec			ecx
__asm	jnz			_loop
#else
__asm	loop		_loop
#endif
// the last pixels
		UClensePixel(edi + edx, esi + edx, ebx + edx, eax + edx, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6, SSE7)
__asm	add			esi,				spitch
__asm	add			edi,				dpitch
__asm	add			ebx,				ppitch
__asm	add			eax,				npitch
__asm	dec			height
__asm	mov			ecx,				hblocks
__asm	jnz			_loop
}

static void clense(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, const BYTE *pp, int ppitch, const BYTE *np, int npitch, int hblocks, int remainder, int incpitch, int height)
{
	if( (((unsigned)dp & (SSE_INCREMENT - 1)) + ((unsigned)_sp & (SSE_INCREMENT - 1)) + ((unsigned)pp & (SSE_INCREMENT - 1)) + ((unsigned)np & (SSE_INCREMENT - 1))
#ifdef	ALIGNPITCH
			+ (spitch & (SSE_INCREMENT - 1)) + (ppitch & (SSE_INCREMENT - 1)) + (npitch & (SSE_INCREMENT - 1)) 
#endif
			) == 0 ) aligned_clense(dp, dpitch, _sp, spitch, pp, ppitch, np, npitch, hblocks, remainder, incpitch, height);
	else unaligned_clense(dp, dpitch, _sp, spitch, pp, ppitch, np, npitch, hblocks, remainder, incpitch, height);
	
}
#endif	// ISSE > 1	

class	Clense : public GenericClense
{
	PVideoFrame		lframe;
	unsigned		lnr;
	bool			reduceflicker;

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
	{
		if( !reduceflicker || (lnr != n-1) )
		{
			if( n == 0 ) return child->GetFrame(n, env);
			lframe = child->GetFrame(n - 1, env);
		} 
		PVideoFrame	sf = child->GetFrame(n, env);
		if( n >= vi.num_frames ) return sf;
		PVideoFrame	nf = child->GetFrame(n + 1, env);
		PVideoFrame	df = env->NewVideoFrame(vi, 2*SSE_INCREMENT);
		int i = planes;
		do
		{
			clense(GetWritePtr(df, i), GetPitch(df, i), GetReadPtr(sf, i), GetPitch(sf, i), GetReadPtr(lframe, i), GetPitch(lframe, i), GetReadPtr(nf, i), GetPitch(nf, i), hblocks[i], remainder[i], incpitch[i], height[i]);
		} while( --i >= 0 );
		SSE_EMMS
		lframe = df;
		lnr = n;
		return df;
	}
public:
	Clense(PClip clip, bool grey, bool _reduceflicker, bool planar, int cache) 
		: GenericClense(clip, grey, planar), reduceflicker(_reduceflicker), lframe(0), lnr(-2)
	{
		if( cache >= 0 ) child->SetCacheHints(CACHE_RANGE, cache);
	}

	//~Clense(){}
};

AVSValue __cdecl CreateClense(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	enum ARGS { CLIP, GREY, FLICKER, PLANAR, CACHE };
	return new Clense(args[CLIP].AsClip(), args[GREY].AsBool(false), args[FLICKER].AsBool(true), args[PLANAR].AsBool(false), args[CACHE].AsInt(2));
};

class	BMCClense : public GenericClense
{
	PClip			pclip, nclip;

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
	{
		PVideoFrame	pf = pclip->GetFrame(n, env);
		PVideoFrame	sf = child->GetFrame(n, env);
		PVideoFrame	nf = nclip->GetFrame(n, env);
		PVideoFrame	df = env->NewVideoFrame(vi, 2*SSE_INCREMENT);
		int i = planes;
		do
		{
			clense(GetWritePtr(df, i), GetPitch(df, i), GetReadPtr(sf, i), GetPitch(sf, i), GetReadPtr(pf, i), GetPitch(pf, i), GetReadPtr(nf, i), GetPitch(nf, i), hblocks[i], remainder[i], incpitch[i], height[i]);	
		} while( --i >= 0 );
		SSE_EMMS
		return df;
	}
public:
	BMCClense(PClip clip, PClip _pclip, PClip _nclip, bool grey, bool planar) : GenericClense(clip, grey, planar), pclip(_pclip), nclip(_nclip)
	{
		child->SetCacheHints(CACHE_RANGE, 0);
		pclip->SetCacheHints(CACHE_RANGE, 0);
		nclip->SetCacheHints(CACHE_RANGE, 0);
		CompareVideoInfo(vi, pclip->GetVideoInfo(), "MCClense");
		CompareVideoInfo(vi, nclip->GetVideoInfo(), "MCClense");
	}

	//~BMCClense(){}
};

AVSValue __cdecl CreateMCClense(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new BMCClense(args[0].AsClip(), args[1].AsClip(), args[2].AsClip(), args[3].AsBool(false), args[4].AsBool(false));
};

#if	ISSE > 1
static void aligned_sclense(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, const BYTE *pp, int ppitch, const BYTE *np, int npitch, int hblocks, int remainder, int incpitch, int height)
#else
static void sclense(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, const BYTE *pp, int ppitch, const BYTE *np, int npitch, int hblocks, int remainder, int incpitch, int height)
#endif
#define	ASClensePixel(daddr, saddr, paddr, naddr, reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8)	\
__asm	SSE_RMOVE	reg1,			[paddr]					\
__asm	SSE_RMOVE	reg2,			[paddr + SSE_INCREMENT]	\
__asm	SSE_RMOVE	reg3,			reg1					\
__asm	SSE_RMOVE	reg5,			[naddr]					\
__asm	SSE_RMOVE	reg4,			reg2					\
__asm	SSE_RMOVE	reg6,			[naddr + SSE_INCREMENT]	\
__asm	pminub		reg1,			reg5					\
__asm	pminub		reg2,			reg6					\
__asm	pmaxub		reg3,			reg5					\
__asm	pmaxub		reg4,			reg6					\
__asm	SSE_RMOVE	reg7,			reg3					\
__asm	SSE_RMOVE	reg8,			reg4					\
__asm	psubusb		reg7,			reg5					\
__asm	psubusb		reg8,			reg6					\
__asm	psubusb		reg5,			reg1					\
__asm	psubusb		reg6,			reg2					\
__asm	psubusb		reg1,			reg5					\
__asm	psubusb		reg2,			reg6					\
__asm	pmaxub		reg1,			[saddr]					\
__asm	paddusb		reg3,			reg7					\
__asm	paddusb		reg4,			reg8					\
__asm	pmaxub		reg2,			[saddr + SSE_INCREMENT]	\
__asm	pminub		reg1,			reg3					\
__asm	pminub		reg2,			reg4					\
__asm	SSE_RMOVE	[daddr],		reg1					\
__asm	SSE_RMOVE	[daddr + SSE_INCREMENT], reg2
#if		ISSE > 1
#define	USClensePixel(daddr, saddr, paddr, naddr, reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8)	\
__asm	SSE3_MOVE	reg1,			[paddr]					\
__asm	SSE3_MOVE	reg2,			[paddr + SSE_INCREMENT]	\
__asm	SSE_RMOVE	reg3,			reg1					\
__asm	SSE3_MOVE	reg5,			[naddr]					\
__asm	SSE_RMOVE	reg4,			reg2					\
__asm	SSE3_MOVE	reg6,			[naddr + SSE_INCREMENT]	\
__asm	pminub		reg1,			reg5					\
__asm	pminub		reg2,			reg6					\
__asm	pmaxub		reg3,			reg5					\
__asm	pmaxub		reg4,			reg6					\
__asm	SSE_RMOVE	reg7,			reg3					\
__asm	SSE_RMOVE	reg8,			reg4					\
__asm	psubusb		reg7,			reg5					\
__asm	psubusb		reg8,			reg6					\
__asm	psubusb		reg5,			reg1					\
__asm	psubusb		reg6,			reg2					\
__asm	psubusb		reg1,			reg5					\
__asm	psubusb		reg2,			reg6					\
__asm	SSE3_MOVE	reg5,			[saddr]					\
__asm	paddusb		reg3,			reg7					\
__asm	paddusb		reg4,			reg8					\
__asm	SSE3_MOVE	reg6,			[saddr + SSE_INCREMENT]	\
__asm	pmaxub		reg1,			reg5					\
__asm	pmaxub		reg2,			reg6					\
__asm	pminub		reg1,			reg3					\
__asm	pminub		reg2,			reg4					\
__asm	SSE_MOVE	[daddr],		reg1					\
__asm	SSE_MOVE	[daddr + SSE_INCREMENT], reg2
#else
#define	USClensePixel(daddr, saddr, paddr, naddr, reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8)	\
		ASClensePixel(daddr, saddr, paddr, naddr, reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8)
#endif
{
__asm	mov			eax,				incpitch
__asm	mov			ebx,				pp
__asm	add			dpitch,				eax
__asm	add			spitch,				eax
__asm	add			ppitch,				eax
__asm	add			npitch,				eax
__asm	mov			esi,				_sp
__asm	mov			edi,				dp
__asm	mov			edx,				remainder
__asm	mov			eax,				np
__asm	mov			ecx,				hblocks
__asm	align		16
__asm	_loop:
		ASClensePixel(edi, esi, ebx, eax, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6, SSE7)		 
__asm	add			eax,				2*SSE_INCREMENT
__asm	add			esi,				2*SSE_INCREMENT
__asm	add			edi,				2*SSE_INCREMENT
__asm	add			ebx,				2*SSE_INCREMENT
__asm	loop		_loop
// the last pixels
		USClensePixel(edi + edx, esi + edx, ebx + edx, eax + edx, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6, SSE7)
__asm	add			esi,				spitch
__asm	add			edi,				dpitch
__asm	add			ebx,				ppitch
__asm	add			eax,				npitch
__asm	dec			height
__asm	mov			ecx,				hblocks
__asm	jnz			_loop
}

#if		ISSE > 1

static void unaligned_sclense(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, const BYTE *pp, int ppitch, const BYTE *np, int npitch, int hblocks, int remainder, int incpitch, int height)
{
__asm	mov			eax,				incpitch
__asm	mov			ebx,				pp
__asm	add			dpitch,				eax
__asm	add			spitch,				eax
__asm	add			ppitch,				eax
__asm	add			npitch,				eax
__asm	mov			esi,				_sp
__asm	mov			edi,				dp
__asm	mov			edx,				remainder
__asm	mov			eax,				np
__asm	mov			ecx,				hblocks
__asm	align		16
__asm	_loop:
		USClensePixel(edi, esi, ebx, eax, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6, SSE7)		 
__asm	add			eax,				2*SSE_INCREMENT
__asm	add			esi,				2*SSE_INCREMENT
__asm	add			edi,				2*SSE_INCREMENT
__asm	add			ebx,				2*SSE_INCREMENT
#if	ISSE > 1
__asm	dec			ecx
__asm	jnz			_loop
#else
__asm	loop		_loop
#endif
// the last pixels
		USClensePixel(edi + edx, esi + edx, ebx + edx, eax + edx, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6, SSE7)
__asm	add			esi,				spitch
__asm	add			edi,				dpitch
__asm	add			ebx,				ppitch
__asm	add			eax,				npitch
__asm	dec			height
__asm	mov			ecx,				hblocks
__asm	jnz			_loop
}

static void sclense(BYTE *dp, int dpitch, const BYTE *_sp, int spitch, const BYTE *pp, int ppitch, const BYTE *np, int npitch, int hblocks, int remainder, int incpitch, int height)
{
	if( (((unsigned)dp & (SSE_INCREMENT - 1)) + ((unsigned)_sp & (SSE_INCREMENT - 1)) + ((unsigned)pp & (SSE_INCREMENT - 1)) + ((unsigned)np & (SSE_INCREMENT - 1))
#ifdef	ALIGNPITCH
			+ (spitch & (SSE_INCREMENT - 1)) + (ppitch & (SSE_INCREMENT - 1)) + (npitch & (SSE_INCREMENT - 1)) 
#endif
			) == 0 ) aligned_sclense(dp, dpitch, _sp, spitch, pp, ppitch, np, npitch, hblocks, remainder, incpitch, height);
	else unaligned_sclense(dp, dpitch, _sp, spitch, pp, ppitch, np, npitch, hblocks, remainder, incpitch, height);
	
}
#endif

class	BackwardClense : public GenericClense
{	
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
	{
		PVideoFrame	sf = child->GetFrame(n, env);
		if( n < 2 ) return sf;
		
		PVideoFrame	next1 = child->GetFrame(n - 1, env);
		PVideoFrame	next2 = child->GetFrame(n - 2, env);
		PVideoFrame	df = env->NewVideoFrame(vi, 2*SSE_INCREMENT);
		int i = planes;
		do
		{
			sclense(GetWritePtr(df, i), GetPitch(df, i), GetReadPtr(sf, i), GetPitch(sf, i), GetReadPtr(next1, i), GetPitch(next1, i), GetReadPtr(next2, i), GetPitch(next2, i), hblocks[i], remainder[i], incpitch[i], height[i]);
		} while( --i >= 0 );
		SSE_EMMS
		return df;
	}
public:
	BackwardClense(PClip clip, bool grey, bool planar, int cache) : GenericClense(clip, grey, planar)
	{
		if( cache >= 0 ) child->SetCacheHints(CACHE_RANGE, cache);
	}
};

class	ForwardClense : public BackwardClense
{	
	int	lastnr;
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
	{
		PVideoFrame	sf = child->GetFrame(n, env);
		if( n >= lastnr ) return sf;
		
		PVideoFrame	next1 = child->GetFrame(n + 1, env);
		PVideoFrame	next2 = child->GetFrame(n + 2, env);
		PVideoFrame	df = env->NewVideoFrame(vi, 2*SSE_INCREMENT);
		int i = planes;
		do
		{
			sclense(GetWritePtr(df, i), GetPitch(df, i), GetReadPtr(sf, i), GetPitch(sf, i), GetReadPtr(next1, i), GetPitch(next1, i), GetReadPtr(next2, i), GetPitch(next2, i), hblocks[i], remainder[i], incpitch[i], height[i]);
		} while( --i >= 0 );
		SSE_EMMS
		return df;
	}
public:
	ForwardClense(PClip clip, bool grey, bool planar, int cache) : BackwardClense(clip, grey, planar, cache), lastnr(vi.num_frames - 2)
	{}
};

char	clenseargs[] ="c[grey]b[planar]b[cache]i";

AVSValue __cdecl CreateBackwardClense(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	enum ARGS { CLIP, GREY, PLANAR, CACHE };
	return new BackwardClense(args[CLIP].AsClip(), args[GREY].AsBool(false), args[PLANAR].AsBool(false), args[CACHE].AsInt(2));
};

AVSValue __cdecl CreateForwardClense(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	enum ARGS { CLIP, GREY, PLANAR, CACHE };
	return new ForwardClense(args[CLIP].AsClip(), args[GREY].AsBool(false), args[PLANAR].AsBool(false), args[CACHE].AsInt(2));
};



#endif // MODIFYPLUGIN

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env)
{
#ifdef	MODIFYPLUGIN
#ifdef	DEBUG_NAME
	env->AddFunction("DRepair", "cc[mode]i[modeU]i[modeV]i[planar]b", CreateRemoveGrain, 0);
	env->AddFunction("DTemporalRepair", "cc[mode]i[smooth]i[grey]b[planar]b", CreateTemporalRepair, 0);
#else
	env->AddFunction("Repair", "cc[mode]i[modeU]i[modeV]i[planar]b", CreateRemoveGrain, 0);
	env->AddFunction("TemporalRepair", "cc[mode]i[smooth]i[grey]b[planar]b", CreateTemporalRepair, 0);
#endif
#elif SHARPEN == 1
#ifdef	DEBUG_NAME
	env->AddFunction("DRSharpen", "c[mode]i[modeU]i[modeV]i[strength]i[strengthU]i[strengthV]i[planar]b", CreateRemoveGrain, 0);
	env->AddFunction("DMotionSharpen", "c[grey]b[reduceflicker]b[planar]b[cache]i", CreateClense, 0);
#else
	env->AddFunction("RSharpen", "c[mode]i[modeU]i[modeV]i[strength]i[strengthU]i[strengthV]i[planar]b", CreateRemoveGrain, 0);
	env->AddFunction("MotionSharpen", "c[grey]b[reduceflicker]b[planar]b[cache]i", CreateClense, 0);
#endif
#elif SHARPEN > 1
#ifdef	DEBUG_NAME
	env->AddFunction("DDenoiseSharpen", "c[mode]i[modeU]i[modeV]i[strength]i[strengthU]i[strengthV]i[planar]b", CreateRemoveGrain, 0);
	env->AddFunction("DClenseSharpen", "c[grey]b[reduceflicker]b[planar]b[cache]i", CreateClense, 0);
#else
	env->AddFunction("DenoiseSharpen", "c[mode]i[modeU]i[modeV]i[strength]i[strengthU]i[strengthV]i[planar]b", CreateRemoveGrain, 0);
	env->AddFunction("ClenseSharpen", "c[grey]b[reduceflicker]b[planar]b[cache]i", CreateClense, 0);
#endif
#elif	BLUR == 1
#ifdef	DEBUG_NAME
	env->AddFunction("DRBlur", "c[mode]i[modeU]i[modeV]i[planar]b", CreateRemoveGrain, 0);
#else
	env->AddFunction("RBlur", "c[mode]i[modeU]i[modeV]i[planar]b", CreateRemoveGrain, 0);
#endif
#elif	BLUR > 1
#ifdef	DEBUG_NAME
	env->AddFunction("DDenoiseBlur", "c[mode]i[modeU]i[modeV]i[planar]b", CreateRemoveGrain, 0);
#else
	env->AddFunction("DenoiseBlur", "c[mode]i[modeU]i[modeV]i[planar]b", CreateRemoveGrain, 0);
#endif
#else // MODIFYPLUGIN
#ifdef	DEBUG_NAME
	env->AddFunction("DRemoveGrain", "c[mode]i[modeU]i[modeV]i[planar]b", CreateRemoveGrain, 0);
	env->AddFunction("DClense", "c[grey]b[reduceflicker]b[planar]b[cache]i", CreateClense, 0);
	env->AddFunction("DMCClense", "ccc[grey]b[planar]b", CreateMCClense, 0);
	env->AddFunction("DBackwardClense", clenseargs, CreateBackwardClense, 0);
	env->AddFunction("DForwardClense", clenseargs, CreateForwardClense, 0);
#else
	env->AddFunction("RemoveGrain", "c[mode]i[modeU]i[modeV]i[planar]b", CreateRemoveGrain, 0);
	env->AddFunction("Clense", "c[grey]b[reduceflicker]b[planar]b[cache]i", CreateClense, 0);
	env->AddFunction("MCClense", "ccc[grey]b[planar]b", CreateMCClense, 0);
	env->AddFunction("BackwardClense", clenseargs, CreateBackwardClense, 0);
	env->AddFunction("ForwardClense", clenseargs, CreateForwardClense, 0);
#endif
#endif // MODIFYPLUGIN
	AVSenvironment = env;
	if( (CPUFLAGS & env->GetCPUFlags()) != CPUFLAGS ) 
#if	ISSE > 1
		env->ThrowError("RemoveGrain needs an SSE2 capable cpu!\n");
#else
		env->ThrowError("RemoveGrain needs an SSE capable cpu!\n");
#endif
#if	0
	debug_printf(LOGO);
#endif
	return "RemoveGrain: remove grain from film";
}


// Copyright 2007 Theodor Anschütz
// Requires Avisynth 2.5 source code to compile
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
//
// Version history:
//
// 05-26-2007   Ver. 0.1     Initial version
// T. Anschütz
//
// 06-29-2007   Ver. 0.2     Added E, S, and N modes in frameOverrides parameter
// T. Anschütz
//
// 09-09-2007   Ver. 0.3     Moved calls to child->GetFrame into getMinMax and isSceneStart
// T. Anschütz               since having them in in Autolevels::GetFrame made caching pointless.
//                           Changed sceneChgThreshDefault to 20.
//
// 11-04-2010   Ver. 0.4     + removed tabs in source code
// Jim Battle                + comments in English (thanks to google translate)
//                           + limit averaging window to the first scene change
//                             after current frame
//                           + added support for YUY2
//                           + added support for interleaved RGB
//                             the luma mapping is pretty rough; if you want something
//                             more accurate, build the chain yourself:
//                                source = <some rgb32 source>
//                                leveled = source.converttoyv12().autolevels().converttorgb32()
//                           + speedup?: don't compute ymin/ymax in inner loop
//                           + complain if the format isn't supported
//                           + complain about bad exludeframes specifier ([^ENS])
//                           + added "matrix" option for RGB luma calculation
//                           + added boolean "debug" option; text drawing
//                             courtesy of DDigit routines from StainlessS
//
// 11-26-2010   Ver. 0.5     + rgb mode processing now done in YCbCr space
// Jim Battle                + added gamma correction
//                           + added autogamma correction; the idea came from
//                             Fred Weinhaus' imagemagik filter, at
//                             http://www.fmwconcepts.com/imagemagick/autolevel/index.php
//                           + added ignore, ignore_low, ignore_high parameters
//                           + added border parameters to ignore edges
//                           + only released to one person
//
// 12-26-2010   Ver. 0.6     + removed "matrix" option -- it doesn't add much,
// Jim Battle   alpha          and removing it reduces clutter
//                           + added input_low, input_high, output_low, output_high akin
//                             to the parameters of the same name in the levels() filter
//                           + now using levels() code to do value remapping, which means
//                             autolevels() on yuv images affects all three planes, and
//                             autolevels() on rgb images is done in rgb space
//
// 01-09-2010   Ver. 0.6     + added support for the "coring" flag
// Jim Battle                + revised/improved the readme document

#include "stdafx.h"
#include "math.h"
#include "hash_map"
#include "vector"
#include "string"
#include "sstream"
#include "assert.h"

#include "avisynth.h"
#include "DDigit.h"    // DrawString()
#include "stdio.h"     // for sprintf

using namespace std;
using namespace stdext;

typedef struct {
    int   ymin;
    int   ymax;
    float ymean;
} ystat_t;
typedef pair <int, int> IntPair;
typedef pair <int, ystat_t> StatsElement;
typedef pair <int, boolean> SceneChgElement;
typedef unsigned int unit32;

// choose between looking up small integer multiplies and actually
// doing the multiply.  which is better depends on the CPU.
#define USE_LOOKUP_TABLES 0

// used as a sentinel value to detect unspecified parameters
const int undef_int = -0xC0FFEE;

const int filterRadiusDefault = 5;
const int sceneChgThreshDefault = 20;

// compatible with autolevels ver 0.3
const float ignoreLowDefault  = 1.0f/256.0f;
const float ignoreHighDefault = 1.0f/256.0f;

const boolean debugDefault = false;


// ---------------------------------------------------------------------------
// lifted from avisynth source code, convert.h, internal.h
// ---------------------------------------------------------------------------

class _PixelClip {
  enum { buffer=320 };
  BYTE clip[256+buffer*2];
public:
  _PixelClip() {
    memset(clip, 0, buffer);
    for (int i=0; i<256; ++i) clip[i+buffer] = (unsigned char) i;
    memset(clip+buffer+256, 255, buffer);
  }
  BYTE operator()(int i) { return clip[i+buffer]; }
};

_PixelClip PixelClip;


static __inline BYTE ScaledPixelClip(int i) {
  return PixelClip((i+32768) >> 16);
}

// ---------------------------------------------------------------------------
// filter class definition
// ---------------------------------------------------------------------------

class Autolevels : public GenericVideoFilter
{
    private:
        // filter parameters:
        hash_map <int, boolean> sceneChgOverrides;
        vector<IntPair> excludeFrames;   // numbers of frames (from / to) that are to remain untouched
        int filterRadius;
        int sceneChgThresh;
        // when operating on RGB material, pick a mapping to luma
        enum { luma_Rec709, luma_Rec601, luma_avg } greyscale_mode;
        // gamma parameters
        double gamma;
        boolean autogamma;
        double gamma_midpoint;  // desired midpoint for autogamma correction
        // allow disabling autolevel if only autogamma is desired
        boolean autolevel;
        // manually specified limits
        int input_low, input_high;
        int output_low, output_high;
        boolean force_input_low, force_input_high;
        // turn coring on/off
        boolean coring;
        // skip the outliers in the distribution
        float fraction_ignore_low, fraction_ignore_high;
        // optionally ignore borders when collecting statistics
        int border_l, border_r, border_t, border_b;
        // print diagnostic info
        boolean debug;

        // working variables:
        hash_map <int, ystat_t> StatsCache;   // per frame luma statistics
        char msgbuf[64];

        // internal routines:
        void getStats(int frameno, IScriptEnvironment* env,
                      int *min, int *max, float *mean);
        boolean isSceneStart(int frameno, IScriptEnvironment* env);

        void GetAvgLumaPlanarY        (PVideoFrame &frame, int *yhisto);
        void GetAvgLumaYUY2           (PVideoFrame &frame, int *yhisto);
        void GetAvgLumaInterleavedRGB (PVideoFrame &frame, int *yhisto);
        void GetAvgLumaInterleavedRGB(double Kr, double Kb, PVideoFrame &frame, int *yhisto);

        double inv_coring_func(double y) { return y * (255.0/219.0) + 16.0; }

        // code stolen from avisynth internal function levels()
        void adjust( PVideoFrame &frame,
                     int in_min, double gamma, int in_max, int out_min, int out_max,
                     bool coring);
    public:
        Autolevels(PClip _child,
                   int filterRadius, int sceneChgThresh, string frmOverrides,
                   double gamma, bool autogamma, double midpoint,
                   boolean autolevel,
                   int input_low, int input_high,
                   int output_low, int output_high,
                   boolean coring,
                   double ignore, double ignore_low, double ignore_high,
                   int border, int border_l, int border_r, int border_t, int border_b,
                   boolean debug, IScriptEnvironment *env);

        PVideoFrame __stdcall GetFrame(int frameno, IScriptEnvironment* env);
};

// ---------------------------------------------------------------------------
// filter constructor
// ---------------------------------------------------------------------------

Autolevels::Autolevels(PClip _child,
                       int filterRadius,
                       int sceneChgThresh,
                       string frmOverrides,
                       double gamma, bool autogamma, double midpoint,
                       boolean autolevel,
                       int input_low, int input_high,
                       int output_low, int output_high,
                       boolean coring,
                       double ignore, double ignore_low, double ignore_high,
                       int border, int border_l, int border_r, int border_t, int border_b,
                       boolean debug, IScriptEnvironment *env)
    : filterRadius(filterRadius),
      sceneChgThresh(sceneChgThresh),
      gamma(gamma),
      autogamma(autogamma),
      gamma_midpoint(midpoint),
      autolevel(autolevel),
      input_low(input_low),
      input_high(input_high),
      output_low(output_low),
      output_high(output_high),
      coring(coring),
      debug(debug),
      GenericVideoFilter(_child)
{
    boolean supported = vi.IsYUV() || (vi.IsRGB() && !vi.IsPlanar());
    if (!supported) {
        env->ThrowError("autolevels: planar RGB formats are not currently supported");
    }

    // fill excludeFrames and sceneChgOverrides
    for (int i=0; i<(int)frmOverrides.length(); i++) {
        if (frmOverrides[i] == ',')
            frmOverrides[i] = ' ';
    }

    stringstream ss(frmOverrides);
    string range;
    while (ss >> range) {
        size_t dashPos = range.find_first_of('-');
        int startFrm, endFrm;
        if (dashPos != string::npos) {   // specified area (from - to)
            if (!(istringstream(range.substr(1, dashPos-1)) >> startFrm))
                continue;
            if (!(istringstream(range.substr(dashPos+1)) >> endFrm))
                continue;
        }
        else {   // show unique image ID
            if (!(istringstream(range.substr(1)) >> startFrm))
                continue;
            endFrm = startFrm;
        }
        switch(range.at(0)) {
            case 'S':   // S<frame>: assume new scene at frame <frame>
                for (int i=startFrm; i<=endFrm; i++) {
                    SceneChgElement el = SceneChgElement(i, true);
                    sceneChgOverrides.insert(el);
                }
                break;
            case 'N':   // N<frame>: no new scenes in at <frame> or in range <startFrm> to <endFrm>
                for (int i=startFrm; i<=endFrm; i++) {
                    SceneChgElement el = SceneChgElement(i, false);
                    sceneChgOverrides.insert(el);
                }
                break;
            case 'E':   // E<frame>: leave frame <frame> or range <startFrm> until <endFrm> unchanged
                excludeFrames.push_back(IntPair(startFrm, endFrm));
                break;
            default:
                env->ThrowError("autolevels: bogus excludeframes specifier");
                break;
        }
    }

    greyscale_mode = luma_Rec601;
#if 0
    if (strlen(matrix) > 0) {
        if (!vi.IsRGB())
            env->ThrowError("autolevels: invalid \"matrix\" parameter (RGB data only)");
        if (!lstrcmpi(matrix, "rec709"))
            greyscale_mode = luma_Rec709;
        else if (!lstrcmpi(matrix, "average"))
            greyscale_mode = luma_avg;
        else if (!lstrcmpi(matrix, "rec601"))
            greyscale_mode = luma_Rec601;
        else
            env->ThrowError("autolevels: invalid \"matrix\" parameter (must be matrix=\"Rec601\", \"Rec709\" or \"Average\")");
    }
#endif

    // validate (auto)gamma parameters
    if (gamma <= 0.0)
        env->ThrowError("autolevels: gamma must be > 0.0");
    if (midpoint <= 0.0)
        env->ThrowError("autolevels: midpoint must be more than 0.0");
    if (midpoint >= 1.0)
        env->ThrowError("autolevels: midpoint must be less than 1.0");

    // if ignore low or ignore high are specified, use that value.
    // otherwise, if ignore is specified, use that value,
    // otherwise use the default value.
    fraction_ignore_low = (ignore_low >= 0.0) ? (float)ignore_low
                        : (ignore     >= 0.0) ? (float)ignore
                                              : ignoreLowDefault;

    fraction_ignore_high = (ignore_high >= 0.0) ? (float)ignore_high
                         : (ignore      >= 0.0) ? (float)ignore
                                                : ignoreHighDefault;

    if (fraction_ignore_low > 0.45f)
        env->ThrowError("autolevels: ignore_low fraction is very large; try something under 0.45");
    if (fraction_ignore_high > 0.45f)
        env->ThrowError("autolevels: ignore_high fraction is very large; try something under 0.45");

    // pick the more constraining boundary for each side
    this->border_l = max(0, max(border, border_l));
    this->border_r = max(0, max(border, border_r));
    this->border_t = max(0, max(border, border_t));
    this->border_b = max(0, max(border, border_b));

    // check if the border parameters make sense
    if (border_l + border_r >= vi.width)
        env->ThrowError("autolevels: the left+right border is greater than the image width");
    if (border_t + border_b >= vi.height)
        env->ThrowError("autolevels: the top+bottom border is greater than the image height");

    // check input/output levels, if specified
    force_input_low  = (input_low  != undef_int);
    force_input_high = (input_high != undef_int);
    if (force_input_low && (input_low < 0x00 || input_low > 0xFF))
        env->ThrowError("autolevels: input_low is not in range [0x00 ... 0xFF]");
    if (force_input_high && (input_high < 0x00 || input_high > 0xFF))
        env->ThrowError("autolevels: input_high is not in range [0x00 ... 0xFF]");
    if (force_input_low && force_input_high && (input_low >= input_high))
        env->ThrowError("autolevels: input_low must be smaller than input_high if both are specified");

    if ((output_low != undef_int) && (output_low < 0x00 || output_low > 0xFF))
        env->ThrowError("autolevels: output_low is not in range [0x00 ... 0xFF]");
    if ((output_high != undef_int) && (output_high < 0x00 || output_high > 0xFF))
        env->ThrowError("autolevels: output_high is not in range [0x00 ... 0xFF]");
//  if ((output_low != undef_int) && (output_high != undef_int) &&
//      (output_low >= output_high))
//      env->ThrowError("autolevels: output_low must be smaller than output_high if both are specified");

    // affects the output range for the luma channel.
    // if coring is enabled, the output_{low,high} values refer to the [0..255]
    // luma range, before the range gets remapped to 16..235.
    this->output_low  = (output_low  != undef_int) ? output_low
                      : (vi.IsRGB())               ?   0
                      : (coring)                   ?   0
                                                   :  16;  // compatible with autolevels 0.3
    this->output_high = (output_high != undef_int) ? output_high
                      : (vi.IsRGB())               ? 255
                      : (coring)                   ? 255
                                                   : 235;  // compatible with autolevels 0.3
}

// ---------------------------------------------------------------------------
// process a frame
// ---------------------------------------------------------------------------

// When processing RGB, we convert to YCbCr to collect brightness info.
// The equations of note are:
//    Y' = Kr*r' + (1-Kr-Kb)*g' + Kb*b'
//    Pb = (1/2)*(b'-Y') / (1 - Kb)
//    Pr = (1/2)*(r'-Y') / (1 - Kr)
// where r', g', and b' are gamma corrected and range from 0.0 to 1.0,
// where y varies from 0.0 to 1.0, and
// where Pb, Pr vary from -0.5 to +0.5.
//
// These are scaled and offset as appropriate into an integer 8b value.
//
// For rec 601, Kr = 0.299,  Kb = 0.114
// For rec 709, Kr = 0.2126, Kb = 0.0722
// For average, Kr = 0.3333, Kb = 0.3333
//
// These get scaled into YCbCr as
//    Y = (235-16)*Y' + 16
//   Cb = (240-16)*Pb + 128
//   Cr = (240-16)*Pr + 128

PVideoFrame __stdcall Autolevels::GetFrame(int frameno, IScriptEnvironment* env)
{
    // check if the image is to be processed
    for (int i=0; i<(int)excludeFrames.size(); i++) {
        if (frameno >= excludeFrames[i].first && frameno <= excludeFrames[i].second)
            return child->GetFrame(frameno, env);   // return it unchanged
    }

    // average of the minimum and maximum luma values over the nearby frames
    int this_ymin = 0, this_ymax = 0;
    float this_ymean = 0.0f;
    boolean this_newscene = false;

    double ymin_avg = 0.0;
    double ymax_avg = 255.0;
    double ymean_avg = 127.5;

    boolean inputs_known = force_input_low && force_input_high;

    // collect image statistics, if we must
    if ((autolevel && !inputs_known) || autogamma || debug) {

        // range [avgFrom..avgTo] for determining the average of the
        // ymin and ymax values
        int avgFrom = max(0, frameno-filterRadius);
        int avgTo = min(vi.num_frames-1, frameno+filterRadius);

        // restrict the range by scene changes found between avgFrom and avgTo
        for (int i=avgFrom+1; i<=avgTo; i++) {
            boolean sceneChange = (i > 1) && isSceneStart(i, env);
            if (sceneChange) {
                if (i <= frameno) {
                    avgFrom = i;
                    this_newscene |= (i == frameno);
                } else {
                    avgTo = i - 1;
                    break;
                }
            }
        }

        int     frame_count = (avgTo-avgFrom+1);
        IntPair yMinMaxAccum = IntPair(0, 0);
        float   ymeanAccum = 0.0f;

        for (int i=avgFrom; i<=avgTo; i++) {
            int iymin, iymax;
            float ymean;
            getStats(i, env, &iymin, &iymax, &ymean);
            yMinMaxAccum.first  += iymin;
            yMinMaxAccum.second += iymax;
            ymeanAccum += ymean;
            if (i == frameno) {
                this_ymin = iymin;
                this_ymax = iymax;
                this_ymean = ymean;
            }
        }

        ymin_avg = double(yMinMaxAccum.first)  / frame_count;
        ymax_avg = double(yMinMaxAccum.second) / frame_count;
        ymean_avg =       ymeanAccum           / frame_count;
    }

    // pick a gamma curve
    double do_gamma = (autogamma) ? log(ymean_avg/255.0f) / log(gamma_midpoint)
                                  : gamma;

    PVideoFrame frame = child->GetFrame(frameno, env);
    env->MakeWritable(&frame);

    bool do_coring = coring && vi.IsYUV();

    // use user specified input high/low points, if specified.
    // if coring is specified but input high/low aren't specified,
    // we must do the inverse mapping to counteract the mapping that
    // gets done inside adjust().
    double in_low  = (force_input_low)  ? double(input_low)
                   : (do_coring)        ? inv_coring_func(ymin_avg)
                                        : ymin_avg;
    double in_high = (force_input_high) ? double(input_high)
                   : (do_coring)        ? inv_coring_func(ymax_avg)
                                        : ymax_avg;

    // use levels() code to do remapping
    adjust(frame, int(in_low+0.5), do_gamma, int(in_high+0.5),
                  int(output_low+0.5), int(output_high+0.5), do_coring);

    if (debug) {
        // first line
        int xoff = 10;
        int yoff = 5;
        if (this_newscene) {
            int color_orange = 10;
            DrawString(vi, frame, xoff, yoff, color_orange, "<<scene change>>");
        }
        yoff += 20;
        if (autolevel || !autogamma) {
            // next line
            sprintf_s(msgbuf, sizeof(msgbuf),
                      "ymin=%d, ymax=%d, ymin_avg=%5.1f, ymax_avg=%5.1f",
                      this_ymin, this_ymax, in_low, in_high);
            DrawString(vi, frame, xoff, yoff, msgbuf);
            yoff += 20;
        }
        if (autogamma || !autolevel) {
            // next line
            sprintf_s(msgbuf, sizeof(msgbuf),
                      "mean=%5.1f, ymean_avg=%5.1f, gamma=%3.2f",
                      this_ymean, ymean_avg, do_gamma);
            DrawString(vi, frame, xoff, yoff, msgbuf);
            yoff += 20;
        }
    }

    return frame;
}

// ---------------------------------------------------------------------------
// get the distribution of high and low luma values for the frame
// ---------------------------------------------------------------------------

// This builds builds the limits of the histogram of the luma values for a
// given frame.  Instead of finding the true min/max luma, the darkest and
// brightest outliers are ignored so they don't overly influence the result.

void Autolevels::getStats(int frameno, IScriptEnvironment* env,
                          int *ymin, int *ymax, float *ymean)
{
    hash_map<int, ystat_t>::const_iterator mmIter = StatsCache.find(frameno);
    int ymininner, ymaxinner;
    float mean;

    if (mmIter == StatsCache.end()) {
        // compute the ymin/ymax values if not yet known

        int yhisto[256];
        for (int i=0; i<256; i++) {
            yhisto[i] = 0;
        }

        PVideoFrame frame = child->GetFrame(frameno, env);

        if (vi.IsYUY2()) {
            GetAvgLumaYUY2(frame, yhisto);
        } else if (vi.IsPlanar() || vi.IsYUV()) {
            GetAvgLumaPlanarY(frame, yhisto);
        } else if (!vi.IsPlanar() && vi.IsRGB()) {
            GetAvgLumaInterleavedRGB(frame, yhisto);
        }

        // skip over the bottom and top 1/256-th quantile of the histogram
        // to find a more representative min/max value
        int num_pixels = (vi.width  - border_l - border_r)
                       * (vi.height - border_t - border_b);
        int pixels_ignore_low  = int(num_pixels * fraction_ignore_low);
        int pixels_ignore_high = int(num_pixels * fraction_ignore_high);

        ymininner = 0;
        int sumOutliers = yhisto[ymininner];
        while (sumOutliers < pixels_ignore_low) {
            ymininner++;
            sumOutliers += yhisto[ymininner];
        }

        ymaxinner = 255;
        sumOutliers = yhisto[ymaxinner];
        while (sumOutliers < pixels_ignore_high) {
            ymaxinner--;
            sumOutliers += yhisto[ymaxinner];
        }

        // paranoia: make sure max > min
        // this could only happen if the user specified very large ignore fractions.
        // NB: this is probably impossible now that the ignore fractions are limited
        if (ymaxinner <= ymininner) {
           ymininner = 16;
           ymaxinner = 235;
        }

        // compute the mean from the histogram
        int wsum = 0;
        int pixcnt = 0;
        for(int bkt=ymininner; bkt <= ymaxinner; bkt++) {
            wsum   += yhisto[bkt] * bkt;
            pixcnt += yhisto[bkt];
        }
        mean = (float)wsum / (float)pixcnt;

        ystat_t stats;
        stats.ymin  = ymininner;
        stats.ymax  = ymaxinner;
        stats.ymean = mean;
        StatsElement el = StatsElement(frameno, stats);
        StatsCache.insert(el);
    } else {
        // we've computed stats before; just look it up
        ystat_t stats = mmIter->second;
        ymininner = stats.ymin;
        ymaxinner = stats.ymax;
        mean      = stats.ymean;
    }

    *ymin  = ymininner;
    *ymax  = ymaxinner;
    *ymean = mean;
}

// ---------------------------------------------------------------------------
// build the luma histogram for the frame
// ---------------------------------------------------------------------------

void Autolevels::GetAvgLumaPlanarY(PVideoFrame &frame, int *yhisto)
{
    const int src_pitch = frame->GetPitch(PLANAR_Y);
    const Pixel8 *srcp  = (Pixel8 *)frame->GetReadPtr(PLANAR_Y)
                        + (src_pitch * border_t);

    for (int y=border_t; y<vi.height-border_b; y++) {
        for (int x=border_l; x<vi.width-border_r; x++) {
            yhisto[srcp[x]]++;
        }
        srcp += src_pitch;
    }
}

void Autolevels::GetAvgLumaYUY2(PVideoFrame &frame, int *yhisto)
{
    const int src_pitch = frame->GetPitch();
    const Pixel8 *srcp  = (Pixel8 *)frame->GetReadPtr(PLANAR_Y)
                        + (src_pitch * border_t);

    for (int y=border_t; y<vi.height-border_b; y++) {
        const Pixel8 *pp = &srcp[2*border_l];
        for (int x=border_l; x<vi.width-border_r; x++) {
            yhisto[*pp]++;
            pp += 2;
        }
        srcp += src_pitch;
    }
}

void Autolevels::GetAvgLumaInterleavedRGB(PVideoFrame &frame, int *yhisto)
{
    switch (greyscale_mode) {

        case luma_Rec709:
            GetAvgLumaInterleavedRGB(0.2126, 0.0722, frame, yhisto);
            break;

        case luma_Rec601:
            GetAvgLumaInterleavedRGB(0.299, 0.114, frame, yhisto);
            break;

        case luma_avg:
            GetAvgLumaInterleavedRGB(0.333333, 0.333333, frame, yhisto);
            break;
    }
}

// RGB packing has these byte offsets:
//    rgb24: blue = *(ptr+0), green = *(ptr+1), red = *(ptr+2)
//    rgb32: blue = *(ptr+0), green = *(ptr+1), red = *(ptr+2), alpha = *(ptr+3)
void Autolevels::GetAvgLumaInterleavedRGB(double Kr, double Kb, PVideoFrame &frame, int *yhisto)
{
    const int yMin =   0; //  16;
    const int yMax = 255; // 235;
    const int yRange = (yMax-yMin);

    const double Kg = 1.0 - Kr - Kb;

    // RGB to YCbCr constants
    const int cyb = int(Kb*yRange/255.0*65536.0+0.5);
    const int cyg = int(Kg*yRange/255.0*65536.0+0.5);
    const int cyr = int(Kr*yRange/255.0*65536.0+0.5);

#if USE_LOOKUP_TABLES
    int cyb_table[256];
    int cyg_table[256];
    int cyr_table[256];

    for(int n=0; n<256; n++) {
        cyb_table[n] = int(n*Kb*yRange/255.0*65536.0+0.5);
        cyg_table[n] = int(n*Kg*yRange/255.0*65536.0+0.5) + (yMin * 65536);
        cyr_table[n] = int(n*Kr*yRange/255.0*65536.0+0.5);
    }
#endif

    const int src_pitch = frame->GetPitch();
    Pixel8   *srcp      = (Pixel8 *)frame->GetReadPtr()
                        + (src_pitch * border_b);
    const int rgb_inc   = (vi.IsRGB32()) ? 4 : 3;
    int luma;

    // note: rgb buffers are stored bottom row first
    for(int y=border_b; y<vi.height-border_t; y++) {
        const Pixel8 *pp = &srcp[rgb_inc*border_l];
        for(int x=border_l; x<vi.width-border_r; x++) {
#if USE_LOOKUP_TABLES
            luma = (cyb_table[pp[0]] + cyg_table[pp[1]] + cyr_table[pp[2]] + 0x8000) >> 16;
#else
            luma = (cyb*pp[0] + cyg*pp[1] + cyr*pp[2] + 0x8000 + (yMin<<16)) >> 16;
#endif
        assert(luma >= 0x00 && luma <= 0xFF);
            yhisto[luma]++;
            pp += rgb_inc;
        }
        srcp += src_pitch;
    }
}

// ---------------------------------------------------------------------------
// code lifted from avisynth src/filters/levels.cpp
// ---------------------------------------------------------------------------

void Autolevels::adjust( PVideoFrame &frame,
             int in_min, double gamma, int in_max, int out_min, int out_max,
             bool coring)
{
  BYTE map[256], mapchroma[256];

  // ----- this section is taken from: ------
  //Levels::Levels( PClip _child, int in_min, double gamma, int in_max, int out_min, int out_max, bool coring, 
  //                IScriptEnvironment* env )
  gamma = 1/gamma;
  int divisor = (in_max == in_min) ? 1 : (in_max - in_min);

  if (vi.IsYUV()) 
  {
    for (int i=0; i<256; ++i) 
    {
      double p;

      if (coring) 
        p = ((i-16)*(255.0/219.0) - in_min) / divisor;
      else 
        p = double(i - in_min) / divisor;

      p = pow(min(max(p, 0.0), 1.0), gamma);
      p = p * (out_max - out_min) + out_min;
      int pp;

      if (coring) 
        pp = int(p*(219.0/255.0)+16.5);
      else 
        pp = int(p+0.5);

      map[i] = min(max(pp, (coring) ? 16 : 0), (coring) ? 235 : 255);

      int q = ((i-128) * (out_max-out_min) + (divisor>>1)) / divisor + 128;
      mapchroma[i] = min(max(q, (coring) ? 16 : 0), (coring) ? 240 : 255);
    }
  } else if (vi.IsRGB()) {
    for (int i=0; i<256; ++i) 
    {
      double p = double(i - in_min) / divisor;
      p = pow(min(max(p, 0.0), 1.0), gamma);
      p = p * (out_max - out_min) + out_min;
      map[i] = PixelClip(int(p+0.5));
    }
  }

  // ----- this section is taken from: ------
  // PVideoFrame __stdcall Levels::GetFrame(int n, IScriptEnvironment* env) 

  BYTE* p = frame->GetWritePtr();
  int pitch = frame->GetPitch();
  if (vi.IsYUY2()) {
    for (int y=0; y<vi.height; ++y) {
      for (int x=0; x<vi.width; ++x) {
        p[x*2] = map[p[x*2]];
        p[x*2+1] = mapchroma[p[x*2+1]];
      }
      p += pitch;
    }
  } 
  else if (vi.IsPlanar()){
    for (int y=0; y<vi.height; ++y) {
      for (int x=0; x<vi.width; ++x) {
        p[x] = map[p[x]];
      }
      p += pitch;
    }
    pitch = frame->GetPitch(PLANAR_U);
    p = frame->GetWritePtr(PLANAR_U);
    int w=frame->GetRowSize(PLANAR_U);
    int h=frame->GetHeight(PLANAR_U);
    for (int y=0; y<h; ++y) {
      for (int x=0; x<w; ++x) {
        p[x] = mapchroma[p[x]];
      }
      p += pitch;
    }
    p = frame->GetWritePtr(PLANAR_V);
    for (int y=0; y<h; ++y) {
      for (int x=0; x<w; ++x) {
        p[x] = mapchroma[p[x]];
      }
      p += pitch;
    }

  } else if (vi.IsRGB()) {
    const int row_size = frame->GetRowSize();
    for (int y=0; y<vi.height; ++y) {
      for (int x=0; x<row_size; ++x) {
        p[x] = map[p[x]];
      }
      p += pitch;
    }
  }
  // return frame;
}

// ---------------------------------------------------------------------------
// scene change detection
// ---------------------------------------------------------------------------

// This is not a scene change detection in the strict sense, but only checks
// whether the minimum or maximum luma changes greatly.  This simple approach
// is sufficient here, but perfect, for a scene change can be ignored without
// changing the minimum / maximum.

boolean Autolevels::isSceneStart(int frameno, IScriptEnvironment* env)
{
    hash_map<int, boolean>::const_iterator scgIter = sceneChgOverrides.find(frameno);
    if (scgIter != sceneChgOverrides.end()) {   // check if user overrides are in effect
        return scgIter->second;
    } else {
        if (frameno > 1) {
            int ymin1, ymax1, ymin2, ymax2;
            float ymean1, ymean2;
            getStats(frameno-1, env, &ymin1, &ymax1, &ymean1);
            getStats(frameno,   env, &ymin2, &ymax2, &ymean2);
            int minDiff = abs(ymin2 - ymin1);
            int maxDiff = abs(ymax2 - ymax2);
            boolean sceneChange = (minDiff >= sceneChgThresh) ||
                                  (maxDiff >= sceneChgThresh);
            return sceneChange;
        } else {
            return false;
        }
    }
}

// ---------------------------------------------------------------------------
// declare the filter interface
// ---------------------------------------------------------------------------

AVSValue __cdecl Create_Autolevels(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    bool autolevel = (*(int *)user_data == 1);
    bool autogamma = (*(int *)user_data == 2);

    return new Autolevels(
                args[0].AsClip(),
                args[1].AsInt(filterRadiusDefault),
                args[2].AsInt(sceneChgThreshDefault),
                args[3].AsString(""),                 // overrrides
                args[4].AsFloat(1.0f),                // gamma
                args[5].AsBool(autogamma),            // autogamma
                args[6].AsFloat(0.5f),                // midpoint
                args[7].AsBool(autolevel),            // autolevel
                args[8].AsInt(undef_int),             // input_low
                args[9].AsInt(undef_int),             // input_high
                args[10].AsInt(undef_int),            // output_low
                args[11].AsInt(undef_int),            // output_high
                args[12].AsBool(false),               // coring
                args[13].AsFloat(-1.0f),              // ignore
                args[14].AsFloat(-1.0f),              // ignore_low
                args[15].AsFloat(-1.0f),              // ignore_high
                args[16].AsInt(0),                    // border
                args[17].AsInt(0),                    // border_l
                args[18].AsInt(0),                    // border_r
                args[19].AsInt(0),                    // border_t
                args[20].AsInt(0),                    // border_b
                args[21].AsBool(debugDefault),
                env
               );
}

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env)
{
    // these are used to identify which flavor of filter we're building
    static int filtertype1 = 1;
    static int filtertype2 = 2;

    env->AddFunction("Autolevels",
                      // the argument string has been broken up,
                      // but note that it is actually one string!
                         "c"
                         "[filterRadius]i"
                         "[sceneChgThresh]i"
                         "[frameOverrides]s"
                         "[gamma]f"
                         "[autogamma]b"
                         "[midpoint]f"
                         "[autolevel]b"
                         "[input_low]i"
                         "[input_high]i"
                         "[output_low]i"
                         "[output_high]i"
                         "[coring]b"
                         "[ignore]f"
                         "[ignore_low]f"
                         "[ignore_high]f"
                         "[border]i"
                         "[border_l]i"
                         "[border_r]i"
                         "[border_t]i"
                         "[border_b]i"
                         "[debug]b",
                     Create_Autolevels, &filtertype1);

    env->AddFunction("Autogamma",
                      // the argument string has been broken up,
                      // but note that it is actually one string!
                         "c"
                         "[filterRadius]i"
                         "[sceneChgThresh]i"
                         "[frameOverrides]s"
                         "[gamma]f"
                         "[autogamma]b"
                         "[midpoint]f"
                         "[autolevel]b"
                         "[input_low]i"
                         "[input_high]i"
                         "[output_low]i"
                         "[output_high]i"
                         "[coring]b"
                         "[ignore]f"
                         "[ignore_low]f"
                         "[ignore_high]f"
                         "[border]i"
                         "[border_l]i"
                         "[border_r]i"
                         "[border_t]i"
                         "[border_b]i"
                         "[debug]b",
                     Create_Autolevels, &filtertype2);

    return "`Autolevels' Histogram stretch plugin";
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif

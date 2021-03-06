/*******************************************************************************
  Copyright(c) 2000 - 2003 Radu Corlan. All rights reserved.
  
  This program is free software; you can redistribute it and/or modify it 
  under the terms of the GNU General Public License as published by the Free 
  Software Foundation; either version 2 of the License, or (at your option) 
  any later version.
  
  This program is distributed in the hope that it will be useful, but WITHOUT 
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for 
  more details.
  
  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 59 
  Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
  The full GNU General Public License is included in this distribution in the
  file called LICENSE.
  
  Contact Information: radu@corlan.net
*******************************************************************************/

/* Guiding functions */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "gcx.h"
#include "guide.h"
#include "params.h"

/* Search the frame for suitable guide star
 * Return 0 if found, -1 for an error. update x & y
 * with the star's coordinates if found.
 */
int detect_guide_star(struct ccd_frame *fr, double *x, double *y)
{
	struct sources *src;
	double score, max_score = 0;
	int gsi = 0, i = 0;
	double xc, yc, rf, ds;

	src = new_sources(P_INT(SD_MAX_STARS));
	if (src == NULL) {
		err_printf("detect_guide_star: cannot create sources\n");
		return -1;
	}
	rf = (fr->w * fr->w + fr->h * fr->h) / PI / 2;
	xc = fr->w / 2;
	yc = fr->h / 2;
	extract_stars(fr, NULL, 0, P_DBL(SD_SNR), src);
	if (src->ns <= 0)
		return -1;
	for (i = 0; i < src->ns; i++) {
		if (src->s[i].peak > P_DBL(AP_SATURATION))
			continue;
		xc = cos((src->s[i].x - fr->w / 2) / fr->w * PI);
		yc = cos((src->s[i].y - fr->h / 2) / fr->h * PI);
		ds = sqr(xc * yc);
		score = src->s[i].flux * ds;
		d3_printf("%5.1f %5.1f ds: %.3f score: %.1f\n", 
			  src->s[i].x, src->s[i].y, ds, score);
		if (score > max_score) {
			max_score = score;
			gsi = i;
		}
	}
	*x = src->s[gsi].x;
	*y = src->s[gsi].y;
	return 0;
}

/* compute first moments of disk of radius r */
static int star_first_moments(struct ccd_frame *fr, double x, double y, 
		double r, double *mxp, double *myp, double *fluxp, int *npixp)
{
	int ix, iy;
	int xs, ys;
	int xe, ye;
	int xc, yc;
	int w;
	int nring = 0;
	float *dp;
	float v;
	double sum, rn;
	double mx, my;

//	d3_printf("x:%f y:%f r1:%f r2:%f\n", x, y, r1, r2);

// round the coords off to make sure we use the same number of pixels for the
// flux measurement

	xc = (int)floor(x+0.5);
	yc = (int)floor(y+0.5); 

	xs = (int)(xc-r); 
	xe = (int)(xc+r+1); 
	ys = (int)(yc-r); 
	ye = (int)(yc+r+1); 
	w = fr->w;

	if (xs < 0)
		xs = 0;
	if (ys < 0)
		ys = 0;
	if (xe >= w)
		xe = w - 1;
	if (ye >= fr->h)
		ye = fr->h - 1;

//	d3_printf("xc:%d yc:%d r1:%f r2:%f\n", xc, yc, r1, r2);

//	d3_printf("xs:%d xe:%d ys:%d ye:%d w:%d\n", xs, xe, ys, ye, w);

//	d3_printf("enter ringstats\n");

// walk the ring and collect statistics
	sum=0.0;
	mx = 0;
	my = 0;
	dp = ((float *)(fr->dat)) + xs + ys * w;
	for (iy = ys; iy < ye; iy++) {
		for (ix = xs; ix < xe; ix++) {
			v = *dp;
			dp ++;
			if (((rn = sqr(ix - xc) + sqr(iy - yc)) > sqr(r)))
				continue;
			nring ++;
			mx += v * (ix);
			my += v * (iy);
			sum += v;
		}
		dp += w - xe + xs;
	}
	*mxp = mx/sum;
	*myp = my/sum;
	*fluxp = sum;
	*npixp = nring;
	return 0;
}


/* get guide star position by the centroid method; return 0 if the star was found
 * within the centroid area, 1 if it was found elsewhere in the guiding box and -1
 * if it couldn't been found anymore. */

int guide_star_position_centroid(struct ccd_frame *fr, double x, double y, 
				 double *dx, double *dy, double *derr)
{
	struct star s;
	double d, snr;
	int ret;

	ret = get_star_near(fr, x, y, 10.0, &s);
	if (ret) {
		*dx = 0;
		*dy = 0;
		*derr = 0;
		return -1;
	}
	d = sqrt(sqr(x - s.x) + sqr(y - s.y));
	if (d > 0.7 * P_INT(GUIDE_CENTROID_AREA)) {
		/* we just use the position from get_star, as 
		 * it's not possible to use the same area for
		 * centroiding */
		*dx = s.x - x;
		*dy = s.y - y;
		snr = fabs(s.flux) / 
			sqrt((fabs(s.flux)) / (fr->exp.scale) + 
			     s.npix * sqr(fr->exp.rdnoise));
		*derr = 4 * 0.42 * s.fwhm / snr;
		/* we quadruple the error to account for the fact that the star
		 * does not use the same centroid area as the ref position */
		return 1;
	} else {
		/* we recalculate the centroid using a fixed area around the 
		 * target */
		double cx, cy, flux;
		int npix;
		star_first_moments(fr, x, y, P_INT(GUIDE_CENTROID_AREA),
				   &cx, &cy, &flux, &npix);
		*dx = cx - x;
		*dy = cy - y;
		snr = fabs(flux - s.sky * npix) / 
			sqrt((fabs(s.flux - s.sky * npix)) / (fr->exp.scale) + 
			     npix * sqr(fr->exp.rdnoise));
		*derr = 0.42 * s.fwhm / snr;
		return 0;
	}
}


/* measure the position of the guidestar and add it to the err list; return
 * 0 for success, negative codes for errors */
int guider_get_errpoint(struct guider *guider, struct ccd_frame *fr)
{
	return 0;	
}


/* tell the guider to use the star at x,y as a guide target */
void guider_set_target(struct guider *guider, struct ccd_frame *fr, 
		       double x, double y)
{
	double dx, dy, derr;
	int ret;

	ret = guide_star_position_centroid(fr, x, y, &dx, &dy, &derr);
	d3_printf("spc dx:%.3f dy:%.3f derr:%.3f ret:%d\n",
		  dx, dy, derr, ret);
	if (ret >= 0) {
		guider->xbias = dx;
		guider->ybias = dy;
	} else {
		guider->xbias = 0;
		guider->ybias = 0;
	}

	guider->state |= GUIDER_TARGET_SET;
	guider->xtgt = x;
	guider->ytgt = y;
}



struct guider *guider_new(void)
{
	struct guider *guider;
	guider = calloc(1, sizeof(struct guider));
	guider->ref_count = 1;
	return guider;
}

void guider_ref(struct guider *guider)
{
	if (guider == NULL)
		return;
	guider->ref_count ++;
}


void guider_release(struct guider *guider)
{
	if (guider == NULL)
		return;
	if (guider->ref_count == 1) {
		free(guider);
	} else {
		guider->ref_count --;
	}
}

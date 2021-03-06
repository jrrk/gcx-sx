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

// warp.c: non-linear image coordinates transform functions
// and other geometric transforms and filters

// $Revision: 1.1 $
// $Date: 2003/12/01 00:19:37 $

#include <math.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "ccd.h"
//#include "x11ops.h"
//#include "warp.h"


float smooth3[9] = {0.25, 0.5, 0.25, 0.5, 1.0, 0.5, 0.25, 0.5, 0.25};

// fast 7x7 convolution; dp points to the upper-left of the convolution box 
// in the image, ker is a linear vector that contains the kerner
// scanned l->r and t>b; w is the width of the image
static inline float conv7(float *dp, float *ker, int w)
{
	register int nl;
	register float d;

	nl = w - 6;
	d = *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	dp += nl;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	dp += nl;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	dp += nl;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	dp += nl;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	dp += nl;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	dp += nl;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	return d;
}

// fast 5x5 convolution; dp points to the upper-left of the convolution box 
// in the image, ker is a linear vector that contains the kerner
// scanned l->r and t>b; w is the width of the image
static inline float conv5(float *dp, float *ker, int w)
{
	int nl;
	float d;
 
	nl = w - 4;
	d = *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	dp += nl;
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp * (*ker++);
	dp += nl;
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp * (*ker++);
	dp += nl;
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp * (*ker++);
	dp += nl;
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp * (*ker++);
	return d;
}

// fast 3x3 convolution; dp points to the upper-left of the convolution box 
// in the image, ker is a linear vector that contains the kerner
// scanned l->r and t>b; w is the width of the image
static inline float conv3(float *dp, float *ker, int w)
{
	register int nl;
	register float d;

	nl = w - 2;
	d = *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	dp += nl;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	dp += nl;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	return d;
}


// filter a frame using the supplied kernel
// size must be an odd number >=3
// returns 0 for ok, -1 for error
int filter_frame(struct ccd_frame *fr, struct ccd_frame *fro, float *kern, int size)
{
	int w;
	float *dpi;
	float *dpo;
	int i, all;

	if (size % 2 != 1 || size < 3 || size > 7) {
		err_printf("filter_frame: bad size %d\n");
		return -1;
	} 

	w = fr->w;

	all = w * (fr->h - (size - 1)) - (size) ;
	dpi = (float *)fr->dat;
	dpo = (float *)fro->dat;


//	for(i=0; i<all; i++)
//		*dpo++ = *dpi++;
//	return;

//	d3_printf("dpi %x dpo %x\n", dpi, dpo);

	for (i = 0; i < (size / 2) * w + (size / 2); i++) {
		*dpo++ = fr->stats.cavg;
	}

//	d3_printf("all: %d (%d) w: %d allo: %d\n", all, w * fr->h, w, 
//		  fro->w*fro->h);

//	d3_printf("dpi %x dpo %x\n", dpi, dpo);

	switch(size) {
	case 3:
		for (i=0; i<all; i++) {
			*dpo = conv3(dpi, kern, w);
			dpi ++;
			dpo ++;
		}
		break;
	case 5:
		for (i=0; i<all; i++) {
			*dpo = conv5(dpi, kern, w);
			dpi ++;
  			dpo ++;
		}
		break;
	case 7:
		for (i=0; i<all; i++) {
			*dpo = conv5(dpi, kern, w);
			dpi ++;
			dpo ++;
		}
		break;
	}

//	d3_printf("dpi %x dpo %x\n", dpi, dpo);

	for (i = 0; i < (size / 2) * w + (size / 2); i++) {
		*dpo++ = fr->stats.cavg;
	}

	fr->stats.statsok = 0;

	return 0;

}

// same as filter_frame, but operates in-place
// allocs a new frame for temporary data, then frees it.
int filter_frame_inplace(struct ccd_frame *fr, float *kern, int size)
{
	struct ccd_frame *new;
	int ret, all;

	new = clone_frame(fr);
	if (new == NULL)
		return -1;

	ret = filter_frame(fr, new, kern, size);
	if (ret < 0) {
		release_frame(new);
		return ret;
	}
	all = fr->w * fr->h;
	memcpy(fr->dat, new->dat, all * fr->pix_size);
	fr->stats.statsok = 0;
	release_frame(new);
	return ret;
}


// create a shift-only ctrans
// a positive dx will cause an image to shift to the right after the transform
int make_shift_ctrans(struct ctrans *ct, double dx, double dy)
{
	int i,j;

	ct->order = 1;

	for (i = 0; i < MAX_ORDER; i++)
		for (j = 0; j < MAX_ORDER; j++) {
			ct->a[i][j] = 0.0;
			ct->b[i][j] = 0.0;
		}
	ct->u0 = dx;
	ct->v0 = dy;
	ct->a[1][0] = 1.0;
	ct->b[0][1] = 1.0;
	return 0;
}

// create a shift-scale-rotate ctrans
// a positive dx will cause an image to shift to the right after the transform
// scales > 1 will cause the transformed image to appear larger
// rot is in radians!

int make_roto_translate(struct ctrans *ct, double dx, double dy, double xs, double ys, double rot)
{
	int i,j;
	double sa, ca;

	ct->order = 1;

	for (i = 0; i < MAX_ORDER; i++)
		for (j = 0; j < MAX_ORDER; j++) {
			ct->a[i][j] = 0.0;
			ct->b[i][j] = 0.0;
		}
	ct->u0 = dx;
	ct->v0 = dy;
	sa = sin(-rot);
	ca = cos(-rot);
	ct->a[1][0] = 1.0 / xs * ca;
	ct->a[0][1] = - 1.0 / xs * sa;
	ct->b[0][1] = 1.0 / ys * ca;
	ct->b[1][0] = 1.0 / ys * sa;
	return 0;
}

/*
// compute the coordinate transform
static void do_ctrans(struct ctrans *ct, double u, double v, double *x, double *y)
{
	double up[MAX_ORDER+1];
	double vp[MAX_ORDER+1];
	int k,i;

	u -= ct->u0;
	v -= ct->v0;
	*x = ct->a[0][0] + u * ct->a[1][0] + v * ct->a[0][1];
	*y = ct->b[0][0] + u * ct->b[1][0] + v * ct->b[0][1];
	if (ct->order <= 1)
		return;

// tabulate the powers
	up[0] = 1;
	vp[0] = 1;
	for (i = 1; i <= ct->order; i++) {
		up[i] = u * up[i-1];
		vp[i] = v * vp[i-1];
	}
// now do the calculations of the higher-order terms
	for (k = 2; k <= ct->order; k++) {
		for (i=0; i<=k; i++) {
			*x += up[i]*vp[k-i]*ct->a[i][k-i];
			*y += up[i]*vp[k-i]*ct->b[i][k-i];
		}
	}
}
*/
// do linear shear transform in the x direction
// x = c ( u - a * v )
// y = v

int linear_x_shear(struct ccd_frame *in, struct ccd_frame *out, double a, double c)
{
	int wi, hi, wo, ho; // frame geometries
	float *dpi, *dpo; // frame pointers

	double dwi; // double input frame w/h to avoid repeated transforms

	double xl; // output pixel size in input units
	double xl0; // pixel slice factors

	int ui, vi; //coordinates in the output frame
	double x; //coordinates in the input frame 
	double ox;
	double v;

	float filler; // filler value for out-of-frame spots

//	d3_printf("x_shear a=%.1f c=%.1f\n", a, c);

// handy frame geometries
	wi = in->w;
	hi = in->h;
	wo = out->w;
	ho = out->h;
	dwi = 1.0 * wi;

	if (!in->stats.statsok)
		frame_stats(in);
	filler = in->stats.cavg;

//	d3_printf("avg = %.1f\n", in->stats.avg);

	if (c < 0) {
		err_printf("linear_x_shear: frame needs 'x' flipping\n");
		return -1;
	}

//	d3_printf("wi:%d, wo:%d, ho:%d, hi:%d, filler:%.1f\n", wi, wo, ho, hi, filler); 

	for (vi = 0; vi < ho; vi++) { //output lines 
		dpo = (float *)(out->dat) + wo * vi; // line origins
		dpi = (float *)(in->dat) + wi * vi;
		ox = - a * c * 1.0 * vi; // x of pixel with u = 0;
		x = ox - c;
		xl = c;
//		read = 0;
//		d3_printf("vi=%d ", vi);
		for (ui = 0; ui < wo; ui++) { //output pixels
			xl = c;
			x += xl;
			if ((x <= 0) || (x > dwi - xl) || (vi >= hi)) { // we are outside the input frame
				*dpo++ = filler;
				continue;
 			}
			xl0 = ceil(x) - x;
			if (xl0 > xl) { // it's all within one input pixel
				*dpo++ = *dpi * xl;
				continue;
			}
			v = (*dpi++) * xl0; // get the first slice
//			read ++;
			xl -= xl0;
			while (xl > 1.0) { // add the whole pixels
				v += (*dpi++);
//				read ++;
				xl -= 1.0;
			}
			v += (*dpi) * xl; // add the last bit 
			*dpo ++ = v;
		}
//		d3_printf("read %d vi=%d\n", read, vi);
	}
//	d3_printf("x_shear: exiting\n");
	return 0;
}

// do linear shear transform in the y direction
// x = u - u0
// y = c (v - v0 + b * (u - u0))
int linear_y_shear(struct ccd_frame *in, struct ccd_frame *out, double b, double c, 
		   double u0, double v0)
{
	int wi, wo, ho; // frame geometries
	float *dpi, *dpo; // frame pointers

	double dhi; // double input frame w/h to avoid repeated transforms

	double yl; // output pixel size in input units
	double yl0;

	int ui, uip, vi; //coordinates in the output frame
	double y; //coordinates in the input frame 
	double oy;
	double v;

	float filler; // filler value for out-of-frame spots

// handy frame geometries
	wi = in->w;
	wo = out->w;
	ho = out->h;
	dhi = 1.0 * in->h;

	if (!in->stats.statsok)
		frame_stats(in);
	filler = in->stats.cavg;

	if (c < 0) {
		err_printf("linear_y_shear: frame needs 'y' flipping\n");
		return -1;
	}

	dpo = out->dat; // output pointer

	for (ui = 0; ui < wo; ui++) { //output columns
		uip = (int)floor(ui - u0);
		dpo = (float *)(out->dat) + uip; // line origins
		dpi = (float *)(in->dat) + uip;
		oy = c * (b * (1.0 * ui - u0) - v0); // y of pixel with v = 0;
		y = oy - c;
		for (vi = 0; vi < ho; vi++) { //output pixels
			yl = c;
			y += yl;
			if (y <= 0 || y > dhi - yl || ui >= wi) { // we are outside the input frame
				*dpo = filler;
				dpo += wo;
				continue;
 			}
			yl0 = ceil(y) - y;
			if (yl0 > yl) { // it's all within one input pixel
				*dpo = *dpi * yl;
				dpo += wo;
				continue;
			}
			v = (*dpi) * yl0; // get the first slice
			dpi += wi;
			yl -= yl0;
			while (yl > 1.0) { // add the whole pixels
				v += (*dpi);
				dpi += wi;
				yl -= 1.0;
			}
			v += (*dpi) * yl; // add the last bit 
			*dpo = v;
			dpo += wo;
		}
	}
	return 0;
}


// change frame coordinates
int warp_frame(struct ccd_frame *in, struct ccd_frame *out, struct ctrans *ct)
{
	return -1;
}


// fast shift-only functions

// shift frame left
static void shift_right(float *dat, int w, int h, int dn, double a, double b)
{
	int x, y;
	float *sp, *dp;
	for (y = 0; y < h; y++) {
		dp = dat + y * w + w - 1;
		sp = dat + y * w - dn + w - 1;
		for (x = 0; x < w - dn - 1; x++) {
			*dp = *sp * b + *(sp-1) * a;
			dp--;
			sp--;
		}
		*dp-- = *sp-- * b;
		for (x = 0; x < dn; x++)
			*dp-- = 0.0;
	} 
}

// shift frame left
static void shift_left(float *dat, int w, int h, int dn, double a, double b)
{
	int x, y;
	float *sp, *dp;
	for (y = 0; y < h; y++) {
		dp = dat + w * y;
		sp = dat + w * y + dn;
		for (x = 0; x < w - dn - 1; x++) {
			*dp = *sp * b + *(sp+1) * a;
			dp++;
			sp++;
		}
		*dp++ = *sp * b;
		for (x = 0; x < dn; x++)
			*dp++ = 0.0;
	} 
}

// shift frame up
static void shift_up(float *dat, int w, int h, int dn, double a, double b)
{
	int x, y;
	float *dp, *sp1, *sp2;
	dp = dat;
	sp1 = dat + w * (dn);
	sp2 = dat + w * (dn + 1);
	for (y = 0; y < h - dn - 1; y++) {
		for (x = 0; x < w; x++) {
			*dp = *sp1 * b + * sp2 * a;
			dp++; sp1++; sp2++;
		}
	}
	for (x = 0; x < w; x++) {
		*dp = *sp1 * b;
		dp++; sp1++;
	}
	for (y = 0; y < dn; y++)
		for (x = 0; x < w; x++)
			*dp-- = 0.0;
}

// shift frame down
static void shift_down(float *dat, int w, int h, int dn, double a, double b)
{
	int x, y;
	float *dp, *sp1, *sp2;

//	d3_printf("down\n");

	dp = dat + w * h - 1;
	sp1 = dp - w * dn;
	sp2 = dp - w * (dn + 1);
	for (y = 0; y < h - dn - 1; y++) {
		for (x = 0; x < w; x++) {
			*dp = *sp1 * b + * sp2 * a;
			dp--; sp1--; sp2--;
		}
	}
	for (x = 0; x < w; x++) {
		*dp = *sp1 * b;
		dp--; sp1--;
	}
	for (y = 0; y < dn; y++)
		for (x = 0; x < w; x++)
			*dp-- = 0.0;
 
}
// shift_frame rebins a frame in-place maintaining it's orientation and scale
// frame size is maintained; new pixels are 0-filled

int shift_frame(struct ccd_frame *fr, double dx, double dy)
{
	double a, b;
	int dn;
	int h, w;
	float *dat;

	w = fr->w;
	h = fr->h;
	dat = (float *)fr->dat;

	if (dx > 0) { // shift right
		a = dx - floor(dx);
		b = 1 - a;
		dn = floor(dx);
		shift_right(dat, w, h, dn, a, b);
	} else if (dx < 0) { // shift left
		dx = -dx;
		a = dx - floor(dx);
		b = 1 - a;
		dn = floor(dx);
		shift_left(dat, w, h, dn, a, b);
	}

	if (dy < 0.0) { // shift up
		dy = -dy;
		a = dy - floor(dy);
		b = 1 - a;
		dn = floor(dy);
		shift_up(dat, w, h, dn, a, b);
	} else if (dy > 0.0) {
		a = dy - floor(dy);
		b = 1 - a;
		dn = floor(dy);
		shift_down(dat, w, h, dn, a, b);
	}

	return 0;
}

// create a gaussian filter kernel of given sigma
// requires a prealloced table of floats of suitable size (size*size)
// only odd-sized kernels are produced
// returns 0 for success, -1 for error

int make_gaussian(float sigma, int size, float *kern)
{
	int mid, all;
	float *mp;
	float sum, v;
	int x, y;

	if (sigma < 0.01) {
		err_printf("make_gaussian: clipping sigma to 0.01\n");
		sigma = 0.01;
	}

	if (size % 2 != 1 || size < 3 || size > 127) {
		err_printf("make_gaussian: bad size %d\n");
		return -1;
	} 
	mid = size / 2;
	mp = kern + mid * size + mid;
	sum = 0;
	for (y = 0; y < mid + 1; y++)
		for(x = 0; x < mid + 1; x++) {
			v = exp(- sqrt(1.0 * (sqr(x) + sqr(y))) / sigma);
//			d3_printf("x%d y%d v%.2f\n", x, y, v);
			if (x == 0 && y == 0)
				sum += v;
			else if (x == 0 || y == 0)
				sum += 2 * v;
			else
				sum += 4 * v;

			*(mp + x + size * y) = v;
			*(mp + x - size * y) = v;
			*(mp - x - size * y) = v;
			*(mp - x + size * y) = v;
		}
	all = size * size;
	for (y = 0; y < all; y++)
		kern[y] /= sum;
	return 0;
}


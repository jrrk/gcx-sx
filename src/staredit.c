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

/* edit catalog star information - the gui part */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>

//#include <gtk/gtk.h>

#include "gcx.h"
#include "catalogs.h"
#include "gui.h"
#include "sourcesdraw.h"
#include "params.h"
#include "wcs.h"
#include "interface.h"
#include "obsdata.h"

static void close_star_edit( GtkWidget *widget, gpointer data );

/* make the checkboxes reflect the flags' values */
static void star_edit_update_flags(GtkWidget *dialog, struct cat_star *cats)
{
	GtkWidget *widget;

//	widget = gtk_object_get_data(GTK_OBJECT(dialog), "photometric_check_button");
//	if (widget != NULL)
//		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 
//					     ((cats->flags & CAT_PHOTOMET) != 0));
	widget = gtk_object_get_data(GTK_OBJECT(dialog), "astrometric_check_button");
	if (widget != NULL)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 
					     ((cats->flags & CAT_ASTROMET) != 0));
	widget = gtk_object_get_data(GTK_OBJECT(dialog), "variable_check_button");
	if (widget != NULL)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 
					     ((cats->flags & CAT_VARIABLE) != 0));
//	d3_printf("cats type is %d\n", CATS_TYPE(cats));
	switch(CATS_TYPE(cats)) {
	case CAT_STAR_TYPE_SREF:
		widget = gtk_object_get_data(GTK_OBJECT(dialog), "field_star_radiob");
		if (widget != NULL)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 1);
		break;
	case CAT_STAR_TYPE_APSTD:
		widget = gtk_object_get_data(GTK_OBJECT(dialog), "std_star_radiob");
		if (widget != NULL)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 1);
		break;
	case CAT_STAR_TYPE_APSTAR:
		widget = gtk_object_get_data(GTK_OBJECT(dialog), "ap_target_radiob");
		if (widget != NULL)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 1);
		break;
	case CAT_STAR_TYPE_CAT:
		widget = gtk_object_get_data(GTK_OBJECT(dialog), "cat_object_radiob");
		if (widget != NULL)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 1);
		break;
	default:
		err_printf("Unsupported star type %x\n", cats->flags);
//		widget = gtk_object_get_data(GTK_OBJECT(dialog), "field_star_radiob");
//		if (widget != NULL)
//			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 1);
	}
//	widget = gtk_object_get_data(GTK_OBJECT(dialog), "make_ref_button");
//	if (widget != NULL) {
//		gtk_widget_set_sensitive(widget, (cats->flags & CAT_PHOTOMET) == 0);
//	}
}

/* update an entry given a string 
 * the entry is selected by name
 */
void star_edit_update_entry(GtkWidget *dialog, char *entry_name, char *text)
{
	GtkWidget *widget;

	widget = gtk_object_get_data(GTK_OBJECT(dialog), entry_name);
	if(widget != NULL) 
		gtk_entry_set_text(GTK_ENTRY(widget), text);
}

/* load a start for editing. a copy of the star is made and referenced as 
 * "cat_star". A reference to the inital star is kept as "ocats" */
void star_edit_set_star(GtkWidget *dialog, struct cat_star *cats)
{
	struct cat_star *ncats;
	cat_star_ref(cats);
	ncats = cat_star_dup(cats);
	gtk_object_set_data_full(GTK_OBJECT(dialog), "ocats", ncats,
				 (GtkDestroyNotify) cat_star_release);
	gtk_object_set_data_full(GTK_OBJECT(dialog), "cat_star", cats,
				 (GtkDestroyNotify) cat_star_release);
}

/* update the dialog fields from 'cat_star' 
 * note: we don't queue a redraw here */
void update_star_edit(GtkWidget *dialog)
{
	char buf[256];
	struct cat_star *cats;

	cats = gtk_object_get_data(GTK_OBJECT(dialog), "cat_star");
	if (cats == NULL) {
		return;
	}

//	d3_printf("comments: %s\n", cats->comments);

	star_edit_update_flags(dialog, cats);
	degrees_to_dms_pr(buf, cats->ra / 15.0, 2);
	star_edit_update_entry(dialog, "ra_entry", buf);
	degrees_to_dms_pr(buf, cats->dec, 1);
	star_edit_update_entry(dialog, "declination_entry", buf);
	snprintf(buf, 255, "%.1f", cats->equinox); 
	star_edit_update_entry(dialog, "epoch_entry", buf);
	star_edit_update_entry(dialog, "designation_entry", cats->name);
	snprintf(buf, 255, "%.2f", cats->mag); 
	star_edit_update_entry(dialog, "magnitude_entry", buf);
	if (cats->comments != NULL) {
		star_edit_update_entry(dialog, "comment_entry", cats->comments);
	} else {
		star_edit_update_entry(dialog, "comment_entry", "");
	}
	if (cats->smags != NULL) {
		star_edit_update_entry(dialog, "smag_entry", cats->smags);
	} else {
		star_edit_update_entry(dialog, "smag_entry", "");
	}
	if (cats->imags != NULL) {
		star_edit_update_entry(dialog, "imag_entry", cats->imags);
	} else {
		star_edit_update_entry(dialog, "imag_entry", "");
	}
}

void star_append_comments(struct cat_star *cats, char *ncom)
{
	int cl;
	char *nc;
	if (cats->comments == NULL) {
		cats->comments = malloc(strlen(ncom) + 1);
		if (cats->comments == NULL) 
			return;
		strcpy(cats->comments, ncom);
		return;
	}
	cl = strlen(cats->comments);
	nc = realloc(cats->comments, cl + strlen(ncom) + 2);
	if (nc == NULL) 
		return;
	strcpy(nc + cl, ncom);
	cats->comments = nc;
	return;
}

/* place a copy of src into dest; in desc is non-null, free it first
 */
void update_dynamic_string(char **dest, char *src)
{
	if (*dest != NULL)
		free(*dest);
	if (src == NULL)
		*dest = NULL;
	else 
		*dest = strdup(src);
}


/* update the star in the gui */
static void gui_update_star(GtkWidget *dialog, struct cat_star *cats)
{
	GtkWidget *im_window;

	im_window = gtk_object_get_data(GTK_OBJECT(dialog), "im_window");
	g_return_if_fail(im_window != NULL);
	if (!update_gs_from_cats(im_window, cats))
		gtk_widget_queue_draw(im_window);
	else 
		d3_printf("corresponding gs not found\n");
}

/* remap catalog stars and redraw all stars */
static void update_star_cb( GtkWidget *widget, gpointer data )
{
	GtkWidget *dialog = data;
	GtkWidget *im_window;

	im_window = gtk_object_get_data(GTK_OBJECT(dialog), "im_window");
	g_return_if_fail(im_window != NULL);
	redraw_cat_stars(im_window);
	gtk_widget_queue_draw(im_window);
	close_star_edit(NULL, im_window);
}

/* parse a double value from an editable; if a valid number is found, 
 * it is updated in v and 0 is returned */
static int get_double_from_editable(GtkWidget *widget, double *v)
{
	char *endp;
	double val;
	char *text;

	text = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
	val = strtod(text, &endp);
	if (endp == text) {
		g_free(text);
		return -1;
	} else {
		g_free(text);
		*v = val;
		return 0;
	}
}

/* parse a ra value from an editable; if a valid number is found, 
 * it is updated in v and 0 is returned */
static int get_ra_from_editable(GtkWidget *widget, double *v)
{
	double val;
	char *text;
	int ret;

	text = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
	ret = dms_to_degrees(text, &val);
	if (ret < 0) {
		g_free(text);
		return -1;
	} else {
		g_free(text);
		clamp_double(&val, 0, 23.9999999999999);
		*v = val * 15;
		return 0;
	}
}

/* parse a dec value from an editable; if a valid number is found, 
 * it is updated in v and 0 is returned */
static int get_dec_from_editable(GtkWidget *widget, double *v)
{
	double val;
	char *text;
	int ret;

	text = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
	ret = dms_to_degrees(text, &val);
	if (ret < 0) {
		g_free(text);
		return -1;
	} else {
		g_free(text);
		clamp_double(&val, -90.0, 90.0);
		*v = val;
		return 0;
	}
}


/* handle checkbutton changes */
static void flags_changed_cb( GtkWidget *widget, gpointer data )
{
	GtkWidget *dialog = data;
	struct cat_star *cats;
	int state;

	cats = gtk_object_get_data(GTK_OBJECT(dialog), "cat_star");
	g_return_if_fail(cats != NULL); 

	if (widget == gtk_object_get_data(GTK_OBJECT(data), "variable_check_button")) {
		state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
		if (state) {
			cats->flags |= CAT_VARIABLE;
		} else {
			cats->flags &= ~(CAT_VARIABLE);
		}
	}
	if (widget == gtk_object_get_data(GTK_OBJECT(data), "astrometric_check_button")) {
		state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
		if (state) {
			cats->flags |= (CAT_ASTROMET);
		} else {
			cats->flags &= ~(CAT_ASTROMET);
		}
	}
	if (widget == gtk_object_get_data(GTK_OBJECT(data), "field_star_radiob")) {
		state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
		if (state) {
//			d3_printf("field chaged to %d\n", state);
			cats->flags &= ~CAT_STAR_TYPE_MASK;
			cats->flags |= CAT_STAR_TYPE_SREF;
		} 
	}
	if (widget == gtk_object_get_data(GTK_OBJECT(data), "std_star_radiob")) {
		state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
		if (state) {
//			d3_printf("std chaged to %d\n", state);
			cats->flags &= ~(CAT_STAR_TYPE_MASK | CAT_VARIABLE);
			cats->flags |= CAT_STAR_TYPE_APSTD;
			star_edit_update_flags(dialog, cats);
		} 
	}
	if (widget == gtk_object_get_data(GTK_OBJECT(data), "ap_target_radiob")) {
		state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
		if (state) {
//			d3_printf("ap chaged to %d\n", state);
			cats->flags &= ~(CAT_STAR_TYPE_MASK);
			cats->flags |= CAT_STAR_TYPE_APSTAR;
		} 
	}
	if (widget == gtk_object_get_data(GTK_OBJECT(data), "cat_object_radiob")) {
		state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
		if (state) {
//		d3_printf("cat_obj chaged to %d\n", state);
			cats->flags &= ~(CAT_STAR_TYPE_MASK);
			cats->flags |= CAT_STAR_TYPE_CAT;
		} 
	}
//	d3_printf("flags is %x\n", cats->flags);
	gui_update_star(data, cats);
}

static void entry_changed_cb( GtkWidget *widget, gpointer data )
{
	GtkWidget *dialog = data;
	struct cat_star *cats;
	char *text;

	cats = gtk_object_get_data(GTK_OBJECT(dialog), "cat_star");
	g_return_if_fail(cats != NULL); 

	if (widget == gtk_object_get_data(GTK_OBJECT(data), "magnitude_entry")) {
		if (get_double_from_editable(widget, &cats->mag)) {
			error_beep();
		}
		update_star_edit(dialog);
		gui_update_star(data, cats);
		return;
	}
	if (widget == gtk_object_get_data(GTK_OBJECT(data), "ra_entry")) {
		if (get_ra_from_editable(widget, &cats->ra)) {
			error_beep();
		}
		update_star_edit(dialog);
		gui_update_star(data, cats);
		return;
	}
	if (widget == gtk_object_get_data(GTK_OBJECT(data), "declination_entry")) {
		if (get_dec_from_editable(widget, &cats->dec)) {
			error_beep();
		}
		update_star_edit(dialog);
		gui_update_star(data, cats);
		return;
	}
	if (widget == gtk_object_get_data(GTK_OBJECT(data), "smag_entry")) {
		text = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
		update_dynamic_string(&cats->smags, text);
		g_free(text);
		return;
	}
	if (widget == gtk_object_get_data(GTK_OBJECT(data), "imag_entry")) {
		text = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
		update_dynamic_string(&cats->imags, text);
		g_free(text);
		return;
	}
	if (widget == gtk_object_get_data(GTK_OBJECT(data), "comment_entry")) {
		text = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
		update_dynamic_string(&cats->comments, text);
		g_free(text);
		return;
	}
	if (widget == gtk_object_get_data(GTK_OBJECT(data), "designation_entry")) {
		text = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
		strncpy(cats->name, text, CAT_STAR_NAME_SZ);
		g_free(text);
		return;
	}
}

/* handle entry value changes - called on activate. Also close edit window*/
static gboolean val_changed_cb( GtkWidget *widget, gpointer data )
{
	entry_changed_cb(widget, data);
	update_star_cb(widget, data);
	return FALSE;
}

static gboolean val_changed2_cb (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	entry_changed_cb(widget, data);
	return FALSE;
}

static void cancel_edit_cb( GtkWidget *widget, gpointer data )
{
	GtkWidget *dialog = data;
	struct cat_star *ocats, *cats;

	cats = gtk_object_get_data(GTK_OBJECT(dialog), "cat_star");
	g_return_if_fail(cats != NULL); 

	ocats = gtk_object_get_data(GTK_OBJECT(dialog), "ocats");
	g_return_if_fail(ocats != NULL); 

	cats->ra = ocats->ra;
	cats->dec = ocats->dec;
	cats->mag = ocats->mag;
	cats->equinox = ocats->equinox;
	strncpy(cats->name, ocats->name, CAT_STAR_NAME_SZ);
	cats->flags = ocats->flags;
	update_dynamic_string(&cats->comments, ocats->comments);
	update_dynamic_string(&cats->smags, ocats->smags);
	update_dynamic_string(&cats->imags, ocats->imags);

	update_star_edit(dialog);
	gui_update_star(dialog, cats);
}

static void star_make_std(struct cat_star *cats)
{
	char com[256];	
	if (CATS_TYPE(cats) == CAT_STAR_TYPE_SREF) {
		err_printf("Star is already standard\n");
		return;
	}
	if (cats->comments == NULL)
		snprintf(com, 255, "%s", cats->name);
	else
		snprintf(com, 255, ", %s", cats->name);
	star_append_comments(cats, com);
//	d3_printf("new_comments: %s\n", cats->comments);
	cats->flags &= ~(CAT_VARIABLE | CAT_STAR_TYPE_MASK);
	cats->flags |= CAT_STAR_TYPE_APSTD;
}

static void mkref_cb( GtkWidget *widget, gpointer data )
{
	GtkWidget *dialog = data;
	struct cat_star *cats;

	cats = gtk_object_get_data(GTK_OBJECT(dialog), "cat_star");
	if (cats == NULL) {
		err_printf("no star to mark as reference\n");
		return;
	}
	star_make_std(cats);
	update_star_edit(dialog);
}

static void close_star_edit( GtkWidget *widget, gpointer data )
{
	g_return_if_fail(data != NULL);
	gtk_object_set_data(GTK_OBJECT(data), "star_edit_dialog", NULL);
}

static void set_named_callback(void *dialog, char *name, char *callback, void *func)
{
	GtkObject *wid;
	wid = gtk_object_get_data(GTK_OBJECT(dialog), name);
	if (wid == NULL) {
		err_printf("cannot find object : %s\n", name);
	}
	g_return_if_fail(wid != NULL);
	gtk_signal_connect (GTK_OBJECT (wid), callback,
			    GTK_SIGNAL_FUNC (func), dialog);
}


/* push the cat star in the dialog for editing
 * if the dialog doesn;t exist, create it
 */ 
void star_edit_star(GtkWidget *window, struct cat_star *cats)
{
	GtkWidget *dialog;
	dialog = gtk_object_get_data(GTK_OBJECT(window), "star_edit_dialog");
	if (dialog == NULL) {
		dialog = create_pstar();
		gtk_object_set_data(GTK_OBJECT(dialog), "im_window",
					 window);
		gtk_object_set_data_full(GTK_OBJECT(window), "star_edit_dialog",
					 dialog, (GtkDestroyNotify)(gtk_widget_destroy));
		set_named_callback(dialog, "ok_button", "clicked", update_star_cb);
		set_named_callback(dialog, "cancel_button", "clicked", cancel_edit_cb);
		set_named_callback(dialog, "make_ref_button", "clicked", mkref_cb);
		set_named_callback(dialog, "magnitude_entry", "activate", val_changed_cb);
		set_named_callback(dialog, "magnitude_entry", "focus-out-event", val_changed2_cb);
		set_named_callback(dialog, "epoch_entry", "activate", val_changed_cb);
		set_named_callback(dialog, "epoch_entry", "focus-out-event", val_changed2_cb);
		set_named_callback(dialog, "ra_entry", "activate", val_changed_cb);
		set_named_callback(dialog, "ra_entry", "focus-out-event", val_changed2_cb);
		set_named_callback(dialog, "declination_entry", "activate", val_changed_cb);
		set_named_callback(dialog, "declination_entry", "focus-out-event", 
				   val_changed2_cb);
		set_named_callback(dialog, "designation_entry", "activate", val_changed_cb);
		set_named_callback(dialog, "designation_entry", "focus-out-event", 
				   val_changed2_cb);
		set_named_callback(dialog, "smag_entry", "activate", val_changed_cb);
		set_named_callback(dialog, "smag_entry", "focus-out-event", val_changed2_cb);
		set_named_callback(dialog, "imag_entry", "activate", val_changed_cb);
		set_named_callback(dialog, "imag_entry", "focus-out-event", val_changed2_cb);
		set_named_callback(dialog, "comment_entry", "activate", val_changed_cb);
		set_named_callback(dialog, "comment_entry", "focus-out-event", val_changed2_cb);

		set_named_callback(dialog, "field_star_radiob", "toggled", 
				   flags_changed_cb);
		set_named_callback(dialog, "std_star_radiob", "toggled", 
				   flags_changed_cb);
		set_named_callback(dialog, "ap_target_radiob", "toggled", 
				   flags_changed_cb);
		set_named_callback(dialog, "cat_object_radiob", "toggled", 
				   flags_changed_cb);
		set_named_callback(dialog, "astrometric_check_button", "toggled", 
				   flags_changed_cb);
		set_named_callback(dialog, "variable_check_button", "toggled", 
				   flags_changed_cb);

		gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
				    GTK_SIGNAL_FUNC (close_star_edit), window);
		star_edit_set_star(dialog, cats);
		update_star_edit(dialog);
		gtk_widget_show(dialog);
	} else {
		star_edit_set_star(dialog, cats);
		update_star_edit(dialog);
		gtk_widget_queue_draw(dialog);
		gdk_window_raise(dialog->window);
	}

}

/* do the actual work for star_edit_dialog and star_edit_make_std */
void do_edit_star(GtkWidget *window, GSList *found, int make_std)
{
	struct cat_star *cats;
	struct gui_star *gs;
	struct wcs *wcs;
	double ra, dec;

	g_return_if_fail(window != NULL);
	g_return_if_fail(found != NULL);

	wcs = gtk_object_get_data(GTK_OBJECT(window), "wcs_of_window");

	gs = GUI_STAR(found->data);
	cats = CAT_STAR(GUI_STAR(gs)->s);
	if (cats == NULL) {
		if ((wcs != NULL) && (wcs->wcsset & WCS_VALID)) {
/* we create a cat star here */
			d3_printf("making cats\n");
			w_worldpos(wcs, gs->x, gs->y, &ra, &dec);
			cats = cat_star_new();
			snprintf(cats->name, CAT_STAR_NAME_SZ, "x%.0fy%.0f", gs->x,  gs->y);
			cats->ra = ra;
			cats->dec = dec;
			cats->equinox = 1.0 * wcs->equinox;
			cats->mag = 0.0;
			cats->flags = CAT_STAR_TYPE_SREF;
			gs->s = cats;
			if (make_std) {
				gs->flags = (gs->flags & ~STAR_TYPE_M) | STAR_TYPE_APSTD;
				cats->flags = CAT_STAR_TYPE_APSTD;
			} else {
				gs->flags = (gs->flags & ~STAR_TYPE_M) | STAR_TYPE_SREF;
			}
		} else {
			err_printf("cannot make cat star: no wcs\n");
			return;
		}
	} 
	if (make_std) {
		gs->flags = (gs->flags & ~STAR_TYPE_M) | STAR_TYPE_APSTD;
		star_make_std(cats);
		gtk_widget_queue_draw(window);
		return;
	}
	star_edit_star(window, cats);
}

/* push a star from the found list in the dialog for editing
 * if the dialog doesn;t exist, create it
 */ 
void star_edit_dialog(GtkWidget *window, GSList *found)
{
	GSList *sel;
	sel = filter_selection(found, TYPE_MASK_CATREF, 0, 0);
	if (sel == NULL) {
		if (modal_yes_no("This star type cannot be edited.\nConvert to field star?", NULL))
			do_edit_star(window, found, 0);
	} else {
		do_edit_star(window, sel, 0);
		g_slist_free(sel);
	}
}

void add_star_from_catalog(gpointer window)
{
	struct wcs *wcs;
	char *name = NULL;
	int ret;
	struct cat_star *cats;
	struct ccd_frame *fr;
	struct image_channel *i_ch;
	double x, y;


	ret = modal_entry_prompt("Enter object name or\n"
				 "<catalog>:<name>", "Enter Object", NULL, &name);
	if (!ret)
		return;
	d3_printf("looking for %s\n", name);
	cats = get_object_by_name(name);
	free(name);
	if (cats == NULL) {
		error_message_sb2(window, "Cannot find object");
		return;
	}
	d3_printf("found %s (%.3f %.3f)\n", cats->name, cats->ra, cats->dec);

	i_ch = gtk_object_get_data(GTK_OBJECT(window), "i_channel");
	if (i_ch == NULL || i_ch->fr == NULL) {
		ret = modal_yes_no("An image frame is needed to load objects.\n"
				   "Create new frame using the defalult geometry?", 
				   "New Frame?");
		if (ret <= 0) {
			cat_star_release(cats);
			return;
		}
		fr = new_frame(P_INT(FILE_NEW_WIDTH), P_INT(FILE_NEW_HEIGHT));
		fr->fim.wcsset = WCS_INITIAL;
		fr->fim.xref = cats->ra;
		fr->fim.yref = cats->dec;
		fr->fim.xrefpix = fr->w / 2;
		fr->fim.yrefpix = fr->h / 2;
		fr->fim.equinox = cats->equinox;
		fr->fim.rot = 0.0;
		fr->fim.xinc = P_DBL(FILE_NEW_SECPERPIX) / 3600.0;
		fr->fim.yinc = P_DBL(FILE_NEW_SECPERPIX) / 3600.0;
		frame_to_channel(fr, window, "i_channel");
		add_cat_stars_to_window(window, &cats, 1);
		return;
	}
	fr = i_ch->fr;

	wcs = gtk_object_get_data(GTK_OBJECT(window), "wcs_of_window");
	if (wcs == NULL) {
		d4_printf("new wcs for frame\n");
		wcs = wcs_new();
		gtk_object_set_data_full(GTK_OBJECT(window), "wcs_of_window", 
					 wcs, (GtkDestroyNotify)wcs_release);
	}

	if (wcs->wcsset <= WCS_INVALID) {
		wcs->wcsset = WCS_INITIAL;
		wcs->xref = cats->ra;
		wcs->yref = cats->dec;
		wcs->xrefpix = fr->w / 2;
		wcs->yrefpix = fr->h / 2;
		wcs->equinox = cats->equinox;
		wcs->rot = 0.0;
		wcs->xinc = P_DBL(WCS_SEC_PER_PIX) / 3600.0;
		wcs->yinc = P_DBL(WCS_SEC_PER_PIX) / 3600.0;
		if (P_INT(OBS_FLIPPED))
			wcs->yinc = -wcs->yinc;
		add_cat_stars_to_window(window, &cats, 1);
		return;
	}

	w_xypix(wcs, cats->ra, cats->dec, &x, &y);
	if (x < 0 || x > fr->w - 1 || y < 0 || y > fr->h - 1) {
		ret = modal_yes_no("The object is outside the image area.\n"
				   "Do you want to change the current wcs\n"
				   "(and remove all stars)?", 
				   "New Wcs?");
		if (ret <= 0) {
			cat_star_release(cats);
			return;
		}
		remove_stars(window, TYPE_MASK_ALL, 0);
		wcs->wcsset = WCS_INITIAL;
		wcs->xref = cats->ra;
		wcs->yref = cats->dec;
		wcs->xrefpix = fr->w / 2;
		wcs->yrefpix = fr->h / 2;
		wcs->equinox = cats->equinox;
		wcs->rot = 0.0;
	}
	add_cat_stars_to_window(window, &cats, 1);
	
}

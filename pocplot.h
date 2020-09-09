/* This file is part of PocPlot.
 *
 * PocPlot is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * PocPlot is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with PocPlot; if not, see
 * <https://www.gnu.org/licenses/>.
 *
 * Copyright 2020 Brian Stafford
 */
#ifndef _pocplot_h
#define _pocplot_h

#if !defined(__poc_h_inside__) && !defined(__poc_compile__)
# error "only <poc.h> can be included directly"
#endif

#include <gtk/gtk.h>
#include "pocaxis.h"
#include "pocdataset.h"
#include "poctypes.h"

G_BEGIN_DECLS

#define POC_TYPE_PLOT			poc_plot_get_type ()
G_DECLARE_FINAL_TYPE (PocPlot, poc_plot, POC, PLOT, GtkDrawingArea)


PocPlot *	poc_plot_new (void);

void		poc_plot_set_enable_plot_fill (PocPlot *self, gboolean value);
gboolean	poc_plot_get_enable_plot_fill (PocPlot *self);
void		poc_plot_get_plot_fill (PocPlot *self, GdkRGBA *rgba);
void		poc_plot_set_plot_fill (PocPlot *self, const GdkRGBA *rgba);
gfloat		poc_plot_get_border (PocPlot *self);
void		poc_plot_set_border (PocPlot *self, gfloat size);
void		poc_plot_set_title (PocPlot *self, const gchar *title);
const gchar *	poc_plot_get_title (PocPlot *self);
void		poc_plot_set_x_axis (PocPlot *self, PocAxis *x_axis);
PocAxis *	poc_plot_get_x_axis (PocPlot *self);
void		poc_plot_set_y_axis (PocPlot *self, PocAxis *y_axis);
PocAxis *	poc_plot_get_y_axis (PocPlot *self);
void		poc_plot_set_axis (PocPlot *self, PocAxis *axis);

void		poc_plot_add_dataset (PocPlot *self, PocDataset *dataset,
				      GtkPackType x_pack, GtkPackType y_pack);
void		poc_plot_remove_dataset (PocPlot *self, PocDataset *dataset);
void		poc_plot_clear_dataset (PocPlot *self);
PocDataset *	poc_plot_find_dataset (PocPlot *self, const gchar *nickname);
void		poc_plot_solo_dataset (PocPlot *self, PocDataset *dataset, gboolean solo);
PocAxis *	poc_plot_axis_at_point (PocPlot *self, gdouble x, gdouble y);
void		poc_plot_add_axis (PocPlot *self, PocAxis *axis,
				   gboolean hidden, GtkPackType pack,
				   GtkOrientation orientation);
void		poc_plot_remove_axis (PocPlot *self, PocAxis *axis);
void		poc_plot_clear_axes (PocPlot *self);

void		poc_plot_notify_update (PocPlot *self);

typedef gboolean (*PocPlotDatasetForEachFunc) (PocPlot *self,
					       PocDataset *dataset,
					       gpointer user_data);
PocDataset *	poc_plot_dataset_foreach (PocPlot *self,
					  PocPlotDatasetForEachFunc predicate,
					  gpointer user_data);

typedef gboolean (*PocPlotAxisForEachFunc) (PocPlot *self,
					    PocAxis *axis,
					    gpointer user_data);
PocAxis *	poc_plot_axis_foreach (PocPlot *self,
				       PocPlotAxisForEachFunc predicate,
				       gpointer user_data);

G_END_DECLS

#endif

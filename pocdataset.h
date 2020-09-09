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
#ifndef _pocdataset_h
#define _pocdataset_h

#if !defined(__poc_h_inside__) && !defined(__poc_compile__)
# error "only <poc.h> can be included directly"
#endif

#include <gtk/gtk.h>
#include "pocaxis.h"
#include "poctypes.h"

G_BEGIN_DECLS

#define POC_TYPE_DATASET		poc_dataset_get_type ()
G_DECLARE_DERIVABLE_TYPE (PocDataset, poc_dataset, POC, DATASET, GObject)

/* vvvv private */

/**
 * PocDatasetClass:
 * @draw: Method called by #PocPlot to draw visible area of plot.
 * @invalidate: Notify subclasses to invalidate cached data.
 *
 * The class structure for #PocDatasetClass.
 */
struct _PocDatasetClass
{
  /*< private >*/
  GObjectClass	parent_class;

  /*< public >*/
  void		(*draw)		(PocDataset *self, cairo_t *cr,
			         guint width, guint height);
  void		(*invalidate)	(PocDataset *self);

  /*< private >*/
  void		(*dummy3)	(PocDataset *self);
  void		(*dummy4)	(PocDataset *self);
};

/* ^^^^ private */

PocDataset *	poc_dataset_new (void);

void		poc_dataset_set_nickname (PocDataset *self, const gchar *nickname);
const gchar *	poc_dataset_get_nickname (PocDataset *self);
void		poc_dataset_set_legend (PocDataset *self, const gchar *legend);
const gchar *	poc_dataset_get_legend (PocDataset *self);
void		poc_dataset_get_line_stroke (PocDataset *self, GdkRGBA *rgba);
void		poc_dataset_set_line_stroke (PocDataset *self, const GdkRGBA *rgba);
void		poc_dataset_set_line_style (PocDataset *self, PocLineStyle line_style);
PocLineStyle	poc_dataset_get_line_style (PocDataset *self);
void		poc_dataset_set_x_axis (PocDataset *self, PocAxis *axis);
PocAxis *	poc_dataset_get_x_axis (PocDataset *self);
void		poc_dataset_set_y_axis (PocDataset *self, PocAxis *axis);
PocAxis *	poc_dataset_get_y_axis (PocDataset *self);
void		poc_dataset_set_points (PocDataset *self,
					PocPointArray *points);
PocPointArray *	poc_dataset_get_points (PocDataset *self);

void		poc_dataset_notify_update (PocDataset *self);
void		poc_dataset_invalidate (PocDataset *self);
void		poc_dataset_draw (PocDataset *self, cairo_t *cr,
				  guint width, guint height);

void		poc_dataset_set_points_array (PocDataset *self,
					      const gdouble *x,
					      const gdouble *y,
					      guint points);

G_END_DECLS

#endif

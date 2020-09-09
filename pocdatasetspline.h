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
#ifndef _pocdatasetspline_h
#define _pocdatasetspline_h

#if !defined(__poc_h_inside__) && !defined(__poc_compile__)
# error "only <poc.h> can be included directly"
#endif

#include "pocdataset.h"

G_BEGIN_DECLS

#define POC_TYPE_DATASET_SPLINE			poc_dataset_spline_get_type ()
G_DECLARE_FINAL_TYPE (PocDatasetSpline, poc_dataset_spline,
		      POC, DATASET_SPLINE, PocDataset)

PocDatasetSpline *poc_dataset_spline_new (void);

void		poc_dataset_spline_get_marker_stroke (PocDatasetSpline *self, GdkRGBA *rgba);
void		poc_dataset_spline_set_marker_stroke (PocDatasetSpline *self, const GdkRGBA *rgba);
void		poc_dataset_spline_get_marker_fill (PocDatasetSpline *self, GdkRGBA *rgba);
void		poc_dataset_spline_set_marker_fill (PocDatasetSpline *self, const GdkRGBA *rgba);
gboolean	poc_dataset_spline_get_show_markers (PocDatasetSpline *self);
void		poc_dataset_spline_set_show_markers (PocDatasetSpline *self, gboolean value);

G_END_DECLS

#endif

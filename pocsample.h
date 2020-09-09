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
#ifndef _pocsample_h
#define _pocsample_h

#if !defined(__poc_h_inside__) && !defined(__poc_compile__)
# error "only <poc.h> can be included directly"
#endif

#include <gtk/gtk.h>
#include "pocplot.h"

G_BEGIN_DECLS

#define POC_TYPE_SAMPLE   poc_sample_get_type ()
G_DECLARE_FINAL_TYPE (PocSample, poc_sample, POC, SAMPLE, GtkDrawingArea)

PocSample *	poc_sample_new (PocDataset *dataset);
void		poc_sample_set_dataset (PocSample *self, PocDataset *dataset);
PocDataset *	poc_sample_get_dataset (PocSample *self);

G_END_DECLS

#endif

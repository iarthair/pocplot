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
#ifndef _poclegend_h
#define _poclegend_h

#if !defined(__poc_h_inside__) && !defined(__poc_compile__)
# error "only <poc.h> can be included directly"
#endif

#include "pocplot.h"

G_BEGIN_DECLS

#define POC_TYPE_LEGEND   poc_legend_get_type ()
G_DECLARE_FINAL_TYPE (PocLegend, poc_legend, POC, LEGEND, GtkDrawingArea)

PocLegend *       poc_legend_new (void);

void poc_legend_set_plot (PocLegend *self, PocPlot *plot);
PocPlot *poc_legend_get_plot (PocLegend *self);
void poc_legend_set_title_text_size (PocLegend *self, gdouble title_text_size);
gdouble poc_legend_get_title_text_size (PocLegend *self);
void poc_legend_set_legend_text_size (PocLegend *self, gdouble legend_text_size);
gdouble poc_legend_get_legend_text_size (PocLegend *self);
void poc_legend_set_line_sample_size (PocLegend *self, gdouble line_sample_size);
gdouble poc_legend_get_line_sample_size (PocLegend *self);
void poc_legend_set_line_spacing (PocLegend *self, gdouble line_spacing);
gdouble poc_legend_get_line_spacing (PocLegend *self);

G_END_DECLS

#endif

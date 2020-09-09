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
#ifndef _pocspline_h
#define _pocspline_h

#if !defined(__poc_h_inside__) && !defined(__poc_compile__)
# error "only <poc.h> can be included directly"
#endif

#include "poctypes.h"

G_BEGIN_DECLS

PocPointArray * poc_spline_get_points (PocPointArray *points,
				       gdouble min_x, gdouble max_x,
				       guint veclen);
PocDoubleArray *poc_spline_get_vector (PocPointArray *points,
				       gdouble min_x, gdouble max_x,
				       guint veclen);

G_END_DECLS

#endif

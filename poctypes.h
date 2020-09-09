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
#ifndef _poctypes_h
#define _poctypes_h

#if !defined(__poc_h_inside__) && !defined(__poc_compile__)
# error "only <poc.h> can be included directly"
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* point */

typedef struct _PocPoint PocPoint;
struct _PocPoint
{
  gdouble x;
  gdouble y;
};
GType poc_point_get_type (void) G_GNUC_CONST;
#define POC_TYPE_POINT (poc_point_get_type ())

/* point array */

typedef struct _PocPointArray PocPointArray;
struct _PocPointArray
{
  PocPoint *data;
  guint len;
};
#define poc_point_array_index(a,i)	((a)->data[(i)])
#define poc_point_array_len(a)		((a)->len)
#define poc_point_array_append_val(a,v)	poc_point_array_append_vals (a, &(v), 1)
PocPointArray *poc_point_array_new (void);
PocPointArray *poc_point_array_sized_new (guint reserved_size);
PocPointArray *poc_point_array_set_size (PocPointArray *array, guint size);
PocPointArray *poc_point_array_append_vals (PocPointArray *dst, const PocPoint *src, guint len);
PocPointArray *poc_point_array_ref (PocPointArray *array);
void poc_point_array_unref (PocPointArray *array);
GType	poc_point_array_get_type (void) G_GNUC_CONST;
#define POC_TYPE_POINT_ARRAY (poc_point_array_get_type ())

/* double array */

typedef struct _PocDoubleArray PocDoubleArray;
struct _PocDoubleArray
{
  gdouble *data;
  guint len;
};
#define poc_double_array_index(a,i)	((a)->data[(i)])
#define poc_double_array_len(a)		((a)->len)
#define poc_double_array_append_vals(a,v,n)	\
		((PocDoubleArray *) g_array_append_vals ((GArray *) (a), (v), (n)))
#define poc_double_array_append_val(a,v)	\
		poc_double_array_append_vals ((a), (v), 1)
PocDoubleArray *poc_double_array_new (void);
PocDoubleArray *poc_double_array_sized_new (guint reserved_size);
PocDoubleArray *poc_double_array_set_size (PocDoubleArray *array, guint size);
PocDoubleArray *poc_double_array_ref (PocDoubleArray *array);
void poc_double_array_unref (PocDoubleArray *array);
GType	poc_double_array_get_type (void) G_GNUC_CONST;
#define POC_TYPE_DOUBLE_ARRAY (poc_double_array_get_type ())

/* Enums */

const gchar *poc_enum_to_string (GType enum_type, gint value);
gint poc_enum_from_string (GType enum_type, const gchar *string);

/* axis mode */

/**
 * PocAxisMode:
 * @POC_AXIS_LINEAR: Show a linear axis
 * @POC_AXIS_LOG_OCTAVE: Show a logarithmic axis in octaves
 * @POC_AXIS_LOG_DECADE: Show a logarithmic axis in decades
 *
 * An enumerated type specifying the axis mode.
 */
typedef enum
  {
    POC_AXIS_LINEAR,
    POC_AXIS_LOG_OCTAVE,
    POC_AXIS_LOG_DECADE
  }
PocAxisMode;

#define POC_TYPE_AXIS_MODE	poc_axis_mode_get_type ()
GType		poc_axis_mode_get_type (void) G_GNUC_CONST;

/* line styles */

/**
 * PocLineStyle:
 * @POC_LINE_STYLE_SOLID: solid line
 * @POC_LINE_STYLE_DOTS: dotted line
 * @POC_LINE_STYLE_DASH: dashed line
 * @POC_LINE_STYLE_LONG_DASH: long dashes
 * @POC_LINE_STYLE_DOT_DASH: dot-dash line
 * @POC_LINE_STYLE_LONG_SHORT_DASH: long-short dashes
 * @POC_LINE_STYLE_DOT_DOT_DASH: dot-dot-dash line
 *
 * An enumerated type specifying the plot line style. These should be self
 * explanatory.
 */
typedef enum
  {
    POC_LINE_STYLE_SOLID,
    POC_LINE_STYLE_DOTS,
    POC_LINE_STYLE_DASH,
    POC_LINE_STYLE_LONG_DASH,
    POC_LINE_STYLE_DOT_DASH,
    POC_LINE_STYLE_LONG_SHORT_DASH,
    POC_LINE_STYLE_DOT_DOT_DASH
  }
PocLineStyle;

#define POC_TYPE_LINE_STYLE	poc_line_style_get_type ()
GType		poc_line_style_get_type (void) G_GNUC_CONST;
const double *	poc_line_style_get_dashes (PocLineStyle line_style,
					   int *num_dashes);
G_END_DECLS

#endif

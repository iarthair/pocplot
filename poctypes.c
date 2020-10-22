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
#include "poctypes.h"

/* Point {{{1 */

/**
 * SECTION: poctypes
 * @title: Poc Types
 * @short_description: Boxed types and enums used by PocPlot.
 *
 * Boxed types and enums used by PocPlot.
 */

/**
 * PocPoint:
 * @x: an X coordinate
 * @y: a Y coordinate
 *
 * A boxed type representing an (x,y) coordinate.
 */

static PocPoint *
poc_point_copy (const PocPoint *pt)
{
  return g_slice_dup (PocPoint, pt);
}

static void
poc_point_free (PocPoint *pt)
{
  g_slice_free (PocPoint, pt);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
G_DEFINE_BOXED_TYPE (PocPoint, poc_point, poc_point_copy, poc_point_free)
#pragma GCC diagnostic pop

/* Point array {{{1 */

/**
 * PocPointArray:
 * @data: pointer to #PocPoint array. The data may be moved as elements are
 * added to the array.
 * @len: number of values in the array.
 *
 * A wrapper for #GArray to create an array of #PocPoint values
 */

/**
 * poc_point_array_new:
 *
 * A wrapper for #GArray to create an array of #PocPointArray values.
 *
 * Returns: (transfer full): a #PocPointArray
 */
PocPointArray *
poc_point_array_new (void)
{
  return (PocPointArray *) g_array_new (FALSE, FALSE, sizeof (PocPoint));
}

/**
 * poc_point_array_sized_new:
 * @reserved_size: number of elements preallocated
 *
 * A wrapper for #GArray to create an array of #PocPointArray values with
 * @reserved_size elements preallocated.  This avoids frequent reallocation, if
 * you are going to add many elements to the array. Note however that the size
 * of the array is still zero.
 *
 * Returns: (transfer full): a #PocPointArray
 */
PocPointArray *
poc_point_array_sized_new (guint reserved_size)
{
  return (PocPointArray *) g_array_sized_new (FALSE, FALSE, sizeof (PocPoint),
					     reserved_size);
}

/**
 * poc_point_array_ref:
 * @array: a #PocPointArray
 *
 * Increments the reference count of @array by one. This function is
 * thread-safe and may be called from any thread.
 *
 * Returns: the #PocPointArray
 */
PocPointArray *
poc_point_array_ref (PocPointArray *array)
{
  return (PocPointArray *) g_array_ref ((GArray *) array);
}

/**
 * poc_point_array_unref:
 * @array: a #PocPointArray
 *
 * Increments the reference count of @array by one. This function is
 * thread-safe and may be called from any thread.
 */
void
poc_point_array_unref (PocPointArray *array)
{
  g_array_unref ((GArray *) array);
}

PocPointArray *
poc_point_array_append_vals (PocPointArray *dst, const PocPoint *src, guint len)
{
  return (PocPointArray *) g_array_append_vals ((GArray *) dst, src, len);
}

/**
 * poc_point_array_set_size:
 * @array: a #PocPointArray
 * @size: the new size of the array
 *
 * Sets the size of the array, expanding it if necessary
 * The new elements are set to 0.
 *
 * Returns: (transfer none): the #PocPointArray
 */
PocPointArray *
poc_point_array_set_size (PocPointArray *array, guint size)
{
  return (PocPointArray *) g_array_set_size ((GArray *) array, size);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
G_DEFINE_BOXED_TYPE (PocPointArray, poc_point_array,
		     poc_point_array_ref, poc_point_array_unref)
#pragma GCC diagnostic pop

/* Double array {{{1 */

/**
 * PocDoubleArray:
 * @data: pointer to a #gdouble array.
 * @len: number of values in the array.
 *
 * A wrapper for #GArray to create an array of #gdouble values. #GArray
 * functions may also be used with appropriate casts.
 */

/**
 * poc_double_array_new:
 *
 * A wrapper for #GArray to create an array of #gdouble values.
 *
 * Returns: (transfer full): a #PocDoubleArray
 */
PocDoubleArray *
poc_double_array_new (void)
{
  return (PocDoubleArray *) g_array_new (FALSE, TRUE, sizeof (gdouble));
}

/**
 * poc_double_array_sized_new:
 * @reserved_size: number of elements preallocated
 *
 * A wrapper for #GArray to create an array of #gdouble values with
 * @reserved_size elements preallocated.  This avoids frequent reallocation, if
 * you are going to add many elements to the array. Note however that the size
 * of the array is still zero.
 *
 * Returns: (transfer full): a #PocDoubleArray
 */
PocDoubleArray *
poc_double_array_sized_new (guint reserved_size)
{
  return (PocDoubleArray *) g_array_sized_new (FALSE, TRUE, sizeof (gdouble),
					     reserved_size);
}

/**
 * poc_double_array_set_size:
 * @array: a #PocDoubleArray
 * @size: the new size of the array
 *
 * Sets the size of the array, expanding it if necessary
 * The new elements are set to 0.
 *
 * Returns: (transfer none): the #PocDoubleArray
 */
PocDoubleArray *
poc_double_array_set_size (PocDoubleArray *array, guint size)
{
  return (PocDoubleArray *) g_array_set_size ((GArray *) array, size);
}

/**
 * poc_double_array_ref:
 * @array: a #PocDoubleArray
 *
 * Increments the reference count of @array by one. This function is
 * thread-safe and may be called from any thread.
 *
 * Returns: the #PocDoubleArray
 */
PocDoubleArray *
poc_double_array_ref (PocDoubleArray *array)
{
  return (PocDoubleArray *) g_array_ref ((GArray *) array);
}

/**
 * poc_double_array_unref:
 * @array: a #PocDoubleArray
 *
 * Increments the reference count of @array by one. This function is
 * thread-safe and may be called from any thread.
 */
void
poc_double_array_unref (PocDoubleArray *array)
{
  g_array_unref ((GArray *) array);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
G_DEFINE_BOXED_TYPE (PocDoubleArray, poc_double_array,
		     poc_double_array_ref, poc_double_array_unref)
#pragma GCC diagnostic pop


/* Enums {{{1 */

const gchar *
poc_enum_to_string (GType enum_type, gint value)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  enum_class = g_type_class_peek (enum_type);
  g_return_val_if_fail (enum_class != NULL, NULL);
  enum_value = g_enum_get_value (enum_class, value);
  return enum_value != NULL ? enum_value->value_nick : NULL;
}

gint
poc_enum_from_string (GType enum_type, const gchar *string)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  enum_class = g_type_class_peek (enum_type);
  g_return_val_if_fail (enum_class != NULL, 0);
  enum_value = g_enum_get_value_by_nick (enum_class, string);
  g_return_val_if_fail (enum_value != NULL, 0);
  return enum_value->value;
}

static void
poc_enum_transform_to_string (const GValue *src, GValue *dest)
{
  const gchar *string;

  string = poc_enum_to_string (G_VALUE_TYPE (src), g_value_get_enum (src));
  g_value_set_static_string (dest, string);
}

static void
poc_enum_transform_from_string (const GValue *src, GValue *dest)
{
  gint value;

  value = poc_enum_from_string (G_VALUE_TYPE (dest), g_value_get_string (src));
  g_value_set_enum (dest, value);
}

/* axis mode {{{2 */

GType
poc_axis_mode_get_type (void)
{
  GType type;
  static gsize poc_axis_mode_type;
  static const GEnumValue values[] =
    {
      { POC_AXIS_LINEAR, "POC_AXIS_LINEAR", "linear" },
      { POC_AXIS_LOG_OCTAVE, "POC_AXIS_LOG_OCTAVE", "octaves" },
      { POC_AXIS_LOG_DECADE, "POC_AXIS_LOG_DECADE", "decades" },
      { 0, NULL, NULL }
    };

  if (g_once_init_enter (&poc_axis_mode_type))
    {
      type = g_enum_register_static (g_intern_static_string ("PocAxisMode"),
      				     values);
      g_value_register_transform_func (type, G_TYPE_STRING,
				       poc_enum_transform_to_string);
      g_value_register_transform_func (G_TYPE_STRING, type,
				       poc_enum_transform_from_string);
      g_once_init_leave (&poc_axis_mode_type, type);
    }
  return poc_axis_mode_type;
}

/* line style {{{2 */

GType
poc_line_style_get_type (void)
{
  GType type;
  static gsize poc_line_style_type;
  static const GEnumValue values[] =
    {
      { POC_LINE_STYLE_SOLID,		"POC_LINE_STYLE_SOLID", 	"solid" },
      { POC_LINE_STYLE_DOTS,		"POC_LINE_STYLE_DOTS",		"dots" },
      { POC_LINE_STYLE_DASH,		"POC_LINE_STYLE_DASH",		"dash" },
      { POC_LINE_STYLE_LONG_DASH,	"POC_LINE_STYLE_LONG_DASH", 	"long-dash" },
      { POC_LINE_STYLE_DOT_DASH,	"POC_LINE_STYLE_DOT_DASH", 	"dot-dash" },
      { POC_LINE_STYLE_LONG_SHORT_DASH,	"POC_LINE_STYLE_LONG_SHORT_DASH", "long-short-dash" },
      { POC_LINE_STYLE_DOT_DOT_DASH,	"POC_LINE_STYLE_DOT_DOT_DASH", 	"dot-dot-dash" },
      { 0, NULL, NULL }
    };

  if (g_once_init_enter (&poc_line_style_type))
    {
      type = g_enum_register_static (g_intern_static_string ("PocLineStyle"),
      				     values);
      g_value_register_transform_func (type, G_TYPE_STRING,
				       poc_enum_transform_to_string);
      g_value_register_transform_func (G_TYPE_STRING, type,
				       poc_enum_transform_from_string);
      g_once_init_leave (&poc_line_style_type, type);
    }
  return poc_line_style_type;
}

const double *
poc_line_style_get_dashes (PocLineStyle line_style, int *num_dashes)
{
  static const double dots[] = { 1.0 };
  static const double dash[] = { 2.0, 3.0 };
  static const double long_dash[] = { 4.0, 3.0 };
  static const double dot_dash[] = { 1.0, 1.0, 1.0, 1.0, 4.0 };
  static const double long_short_dash[] = { 4.0, 3.0, 2.0, 3.0 };
  static const double dot_dot_dash[] = { 1.0, 3.0, 1.0, 3.0, 4.0 };

  switch (line_style)
    {
    case POC_LINE_STYLE_SOLID:
      *num_dashes = 0;
      return dots; /* just to avoid returning NULL */
    case POC_LINE_STYLE_DOTS:
      *num_dashes = G_N_ELEMENTS (dots);
      return dots;
    case POC_LINE_STYLE_DASH:
      *num_dashes = G_N_ELEMENTS (dash);
      return dash;
    case POC_LINE_STYLE_LONG_DASH:
      *num_dashes = G_N_ELEMENTS (long_dash);
      return long_dash;
    case POC_LINE_STYLE_DOT_DASH:
      *num_dashes = G_N_ELEMENTS (dot_dash);
      return dot_dash;
    case POC_LINE_STYLE_LONG_SHORT_DASH:
      *num_dashes = G_N_ELEMENTS (long_short_dash);
      return long_short_dash;
    case POC_LINE_STYLE_DOT_DOT_DASH:
      *num_dashes = G_N_ELEMENTS (dot_dot_dash);
      return dot_dot_dash;
    default:
      //FIXME - complain
      *num_dashes = 0;
      return NULL;
    }
}

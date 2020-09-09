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
#include "pocspline.h"

/**
 * SECTION: pocspline
 * @title:  Spline
 * @short_description: Solve the tridiagonal equation.
 * @see_also: #PocDatasetSpline
 *
 * Interpolate a curve based on its control points using an algorithm that
 * solves the tridiagonal equation based on Numerical Recipies 2nd Edition
 */

static void
spline_solve (guint n, PocPoint point[], gdouble y2[])
{
  gdouble p, sig, *u;
  guint i, k;

  u = g_malloc ((n - 1) * sizeof (u[0]));

  y2[0] = y2[n-1] = u[0] = 0.0;	/* set lower boundary condition to "natural" */

  for (i = 1; i < n - 1; ++i)
    {
      sig = (point[i].x - point[i-1].x) / (point[i+1].x - point[i-1].x);
      p = sig * y2[i-1] + 2.0;
      y2[i] = (sig - 1.0) / p;
      u[i] = ((point[i+1].y - point[i].y) / (point[i+1].x - point[i].x)
	      - (point[i].y - point[i-1].y) / (point[i].x - point[i-1].x));
      u[i] = (6.0 * u[i] / (point[i+1].x - point[i-1].x) - sig * u[i-1]) / p;
    }

  for (k = n - 2; k > 0; --k)
    y2[k] = y2[k] * y2[k+1] + u[k];

  g_free (u);
}

static gdouble
spline_eval (int n, PocPoint point[], gdouble y2[], gdouble val)
{
  gint k_lo, k_hi, k;
  gdouble h, b, a;

  /* do a binary search for the right interval */
  k_lo = 0; k_hi = n - 1;
  while (k_hi - k_lo > 1)
    {
      k = (k_hi + k_lo) / 2;
      if (point[k].x > val)
	k_hi = k;
      else
	k_lo = k;
    }

  h = point[k_hi].x - point[k_lo].x;
  a = (point[k_hi].x - val) / h;
  b = (val - point[k_lo].x) / h;
  return a * point[k_lo].y + b * point[k_hi].y
	   + ((a*a*a - a) * y2[k_lo] + (b*b*b - b) * y2[k_hi]) * (h*h) / 6.0;
}

/**
 * poc_spline_get_vector:
 * @points: A #PocPointArray of control points.
 * @min_x: The lowest (leftmost) X coordinate value.
 * @max_x: The highest (rightmost) X coordinate value.
 * @veclen: The number or points to be calculated in the result vector.
 *
 * Compute a vector of @veclen Y coordinates spaced evenly between and
 * including @min_x and @max_x.
 *
 * Returns: (transfer full): A #PocDoubleArray of Y coordinates.
 */
PocDoubleArray *
poc_spline_get_vector (PocPointArray *points,
		       gdouble min_x, gdouble max_x, guint veclen)
{
  PocDoubleArray *array;
  gdouble rx, ry, dx, *y2v;
  guint x;
  guint n_points;

  n_points = poc_point_array_len (points);
  g_return_val_if_fail (n_points >= 2, NULL);

  array = poc_double_array_sized_new (veclen);
  g_return_val_if_fail (array != NULL && array->data != NULL, NULL);

  y2v = g_malloc (n_points * sizeof (gdouble));
  spline_solve (n_points, points->data, y2v);

  rx = min_x;
  dx = (max_x - min_x) / (veclen - 1);
  poc_double_array_set_size (array, veclen);
  for (x = 0; x < veclen; ++x, rx += dx)
    {
      ry = spline_eval (n_points, points->data, y2v, rx);
      array->data[x] = ry;
    }

  g_free (y2v);

  return array;
}

/**
 * poc_spline_get_points:
 * @points: A #PocPointArray of control points.
 * @min_x: The lowest (leftmost) X coordinate value.
 * @max_x: The highest (rightmost) X coordinate value.
 * @veclen: The number or points to be calculated in the result vector.
 *
 * Compute a vector of @veclen points spaced evenly between and including
 * @min_x and @max_x.
 *
 * Returns: (transfer full): A #PocPointArray of (X,Y) coordinates.
 */
PocPointArray *
poc_spline_get_points (PocPointArray *points,
		       gdouble min_x, gdouble max_x, guint veclen)
{
  PocPointArray *array;
  PocPoint p;
  gdouble rx, ry, dx, *y2v;
  guint x;
  guint n_points;

  n_points = poc_point_array_len (points);
  g_return_val_if_fail (n_points >= 2, NULL);

  array = poc_point_array_sized_new (veclen);
  g_return_val_if_fail (array != NULL && array->data != NULL, NULL);

  y2v = g_malloc (n_points * sizeof (gdouble));
  spline_solve (n_points, points->data, y2v);

  rx = min_x;
  dx = (max_x - min_x) / (veclen - 1);
  for (x = 0; x < veclen; ++x, rx += dx)
    {
      ry = spline_eval (n_points, points->data, y2v, rx);
      p.x = rx;
      p.y = ry;
      poc_point_array_append_val (array, p);
    }
  g_free (y2v);

  return array;
}

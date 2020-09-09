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
#ifndef _pocproto_h
#define _pocproto_h

#if !defined(__poc_h_inside__) && !defined(__poc_compile__)
# error "only <poc.h> can be included directly"
#endif

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _PocObjectBag PocObjectBag;

PocObjectBag *	poc_object_bag_new	(void);
PocObjectBag *	poc_object_bag_ref	(PocObjectBag *bag);
void		poc_object_bag_unref	(PocObjectBag *bag);

gboolean	poc_object_bag_add	(PocObjectBag *bag, GObject *object);
gboolean	poc_object_bag_remove	(PocObjectBag *bag, GObject *object);
void		poc_object_bag_empty	(PocObjectBag *bag);
gboolean	poc_object_bag_set_data_full
					(PocObjectBag *bag, GObject *object,
					 gpointer data, GDestroyNotify destroy);
gpointer	poc_object_bag_get_data	(PocObjectBag *bag, GObject *object);
gboolean	poc_object_bag_contains	(PocObjectBag *bag, GObject *object);
GObject *	poc_object_bag_find	(PocObjectBag *bag,
					 GHRFunc predicate, gpointer user_data);
void		poc_object_bag_foreach	(PocObjectBag *bag,
					 GHFunc func, gpointer user_data);

G_END_DECLS

#endif

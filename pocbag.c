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
#include "pocbag.h"

struct PocBagItem
  {
    GObject *object;
    gint count;
    gpointer data;
    GDestroyNotify destroy;
  };

#if 0
static struct PocBagItem *
poc_bag_item_new (void)
{
  struct PocBagItem *item;

  item = g_slice_new0 (struct PocBagItem);
  item->count = 1;
  return item;
}
#endif

/**
 * poc_bag_item_destroy: (skip)
 */
static void
poc_bag_item_destroy (gpointer data)
{
  struct PocBagItem *item = data;

  g_object_unref (item->object);
  if (item->destroy != NULL)
    (*item->destroy) (item->data);
}

/**
 * poc_object_bag_new: (skip)
 */
PocObjectBag *
poc_object_bag_new (void)
{
  GArray *array;

  array = g_array_new (FALSE, TRUE, sizeof (struct PocBagItem));
  g_array_set_clear_func (array, poc_bag_item_destroy);
  return (PocObjectBag *) array;
}

/**
 * poc_object_bag_ref: (skip)
 */
PocObjectBag *
poc_object_bag_ref (PocObjectBag *bag)
{
  return (PocObjectBag *) g_array_ref ((GArray *) bag);
}

/**
 * poc_object_bag_unref: (skip)
 */
void
poc_object_bag_unref (PocObjectBag *bag)
{
  g_array_unref ((GArray *) bag);
}

static inline gboolean
poc_object_bag_find_object (PocObjectBag *bag, GObject *object, guint *ret)
{
  GArray *array = (GArray *) bag;
  struct PocBagItem *item;
  guint i;

  item = (struct PocBagItem *) array->data;
  for (i = 0; i < array->len; i++)
    if (item[i].object == object)
      {
        *ret = i;
	return TRUE;
      }
  return FALSE;
}

/* TRUE if object was already in the bag */
/**
 * poc_object_bag_add: (skip)
 */
gboolean
poc_object_bag_add (PocObjectBag *bag, GObject *object)
{
  GArray *array = (GArray *) bag;
  struct PocBagItem *item, new_item;
  guint i;

  if (poc_object_bag_find_object (bag, object, &i))
    {
      item = &g_array_index (array, struct PocBagItem, i);
      item->count += 1;
      return TRUE;
    }

  new_item.object = g_object_ref (object);
  new_item.count = 1;
  new_item.data = NULL;
  new_item.destroy = NULL;
  g_array_append_val (array, new_item);
  return FALSE;
}

/**
 * poc_object_bag_empty: (skip)
 */
void
poc_object_bag_empty (PocObjectBag *bag)
{
  GArray *array = (GArray *) bag;

  g_array_set_size (array, 0);
}

/* TRUE if object was removed from the bag */
/**
 * poc_object_bag_remove: (skip)
 */
gboolean
poc_object_bag_remove (PocObjectBag *bag, GObject *object)
{
  GArray *array = (GArray *) bag;
  struct PocBagItem *item;
  guint i;

  if (poc_object_bag_find_object (bag, object, &i))
    {
      item = &g_array_index (array, struct PocBagItem, i);
      if ((item->count -= 1) <= 0)
	{
	  g_array_remove_index (array, i);
	  return TRUE;
	}
    }
  return FALSE;
}

/**
 * poc_object_bag_set_data_full: (skip)
 */
gboolean
poc_object_bag_set_data_full (PocObjectBag *bag, GObject *object,
			      gpointer data, GDestroyNotify destroy)
{
  GArray *array = (GArray *) bag;
  struct PocBagItem *item;
  guint i;

  if (poc_object_bag_find_object (bag, object, &i))
    {
      item = &g_array_index (array, struct PocBagItem, i);
      if (item->destroy != NULL)
	(*item->destroy) (item->data);
      item->data = data;
      item->destroy = destroy;
      return TRUE;
    }
  return FALSE;
}

/**
 * poc_object_bag_get_data: (skip)
 */
gpointer
poc_object_bag_get_data (PocObjectBag *bag, GObject *object)
{
  GArray *array = (GArray *) bag;
  struct PocBagItem *item;
  guint i;

  if (poc_object_bag_find_object (bag, object, &i))
    {
      item = &g_array_index (array, struct PocBagItem, i);
      return item->data;
    }
  return NULL;
}

/* TRUE if object is in the bag */
/**
 * poc_object_bag_contains: (skip)
 */
gboolean
poc_object_bag_contains (PocObjectBag *bag, GObject *object)
{
  guint i;

  return poc_object_bag_find_object (bag, object, &i);
}

/**
 * poc_object_bag_find: (skip)
 */
GObject *
poc_object_bag_find (PocObjectBag *bag, GHRFunc predicate, gpointer user_data)
{
  GArray *array = (GArray *) bag;
  struct PocBagItem *item;
  guint i;

  item = (struct PocBagItem *) array->data;
  for (i = 0; i < array->len; i++)
    if ((*predicate) (item[i].object, item[i].data, user_data))
      return item[i].object;
  return NULL;
}

/**
 * poc_object_bag_foreach: (skip)
 */
void
poc_object_bag_foreach (PocObjectBag *bag, GHFunc func, gpointer user_data)
{
  GArray *array = (GArray *) bag;
  struct PocBagItem *item;
  guint i;

  item = (struct PocBagItem *) array->data;
  for (i = 0; i < array->len; i++)
    (*func) (item[i].object, item[i].data, user_data);
}


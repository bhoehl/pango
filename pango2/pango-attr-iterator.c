/* Pango2
 * pango-attr-iterator.c: Attribute iterator
 *
 * Copyright (C) 2000-2002 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <string.h>

#include "pango-attr-iterator-private.h"
#include "pango-attr-list-private.h"
#include "pango-attr-private.h"
#include "pango-impl-utils.h"


G_DEFINE_BOXED_TYPE (Pango2AttrIterator,
                     pango2_attr_iterator,
                     pango2_attr_iterator_copy,
                     pango2_attr_iterator_destroy)

/* {{{ Private API */

void
pango2_attr_list_init_iterator (Pango2AttrList     *list,
                                Pango2AttrIterator *iterator)
{
  iterator->attribute_stack = NULL;
  iterator->attrs = list->attributes;
  iterator->n_attrs = iterator->attrs ? iterator->attrs->len : 0;

  iterator->attr_index = 0;
  iterator->start_index = 0;
  iterator->end_index = 0;

  if (!pango2_attr_iterator_next (iterator))
    iterator->end_index = G_MAXUINT;
}

void
pango2_attr_iterator_clear (Pango2AttrIterator *iterator)
{
  if (iterator->attribute_stack)
    g_ptr_array_free (iterator->attribute_stack, TRUE);
}

gboolean
pango2_attr_iterator_advance (Pango2AttrIterator *iterator,
                              int                 index)
{
  int start_range, end_range;

  pango2_attr_iterator_range (iterator, &start_range, &end_range);

  while (index >= end_range)
    {
      if (!pango2_attr_iterator_next (iterator))
        return FALSE;
      pango2_attr_iterator_range (iterator, &start_range, &end_range);
    }

  if (start_range > index)
    g_warning ("pango2_attr_iterator_advance(): iterator had already "
               "moved beyond the index");

  return TRUE;
}

/* }}} */
/* {{{ Public API */

/**
 * pango2_attr_list_get_iterator:
 * @list: a `Pango2AttrList`
 *
 * Create a iterator initialized to the beginning of the list.
 *
 * @list must not be modified until this iterator is freed.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2AttrIterator`, which should be freed with
 *   [method@Pango2.AttrIterator.destroy]
 */
Pango2AttrIterator *
pango2_attr_list_get_iterator (Pango2AttrList *list)
{
  Pango2AttrIterator *iterator;

  g_return_val_if_fail (list != NULL, NULL);

  iterator = g_slice_new (Pango2AttrIterator);
  pango2_attr_list_init_iterator (list, iterator);

  return iterator;
}

/**
 * pango2_attr_iterator_destroy:
 * @iterator: a `Pango2AttrIterator`
 *
 * Destroy a `Pango2AttrIterator` and free all associated memory.
 */
void
pango2_attr_iterator_destroy (Pango2AttrIterator *iterator)
{
  g_return_if_fail (iterator != NULL);

  pango2_attr_iterator_clear (iterator);
  g_slice_free (Pango2AttrIterator, iterator);
}

/**
 * pango2_attr_iterator_copy:
 * @iterator: a `Pango2AttrIterator`
 *
 * Copy a `Pango2AttrIterator`.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2AttrIterator`, which should be freed with
 *   [method@Pango2.AttrIterator.destroy]
 */
Pango2AttrIterator *
pango2_attr_iterator_copy (Pango2AttrIterator *iterator)
{
  Pango2AttrIterator *copy;

  g_return_val_if_fail (iterator != NULL, NULL);

  copy = g_slice_new (Pango2AttrIterator);

  *copy = *iterator;

  if (iterator->attribute_stack)
    copy->attribute_stack = g_ptr_array_copy (iterator->attribute_stack, NULL, NULL);
  else
    copy->attribute_stack = NULL;

  return copy;
}

/**
 * pango2_attr_iterator_range:
 * @iterator: a Pango2AttrIterator
 * @start: (out): location to store the start of the range
 * @end: (out): location to store the end of the range
 *
 * Get the range of the current segment.
 *
 * Note that the stored return values are signed, not unsigned
 * like the values in `Pango2Attribute`. To deal with this API
 * oversight, stored return values that wouldn't fit into
 * a signed integer are clamped to %G_MAXINT.
 */
void
pango2_attr_iterator_range (Pango2AttrIterator *iterator,
                            int                *start,
                            int                *end)
{
  g_return_if_fail (iterator != NULL);

  if (start)
    *start = MIN (iterator->start_index, G_MAXINT);
  if (end)
    *end = MIN (iterator->end_index, G_MAXINT);
}

/**
 * pango2_attr_iterator_next:
 * @iterator: a `Pango2AttrIterator`
 *
 * Advance the iterator until the next change of style.
 *
 * Return value: %FALSE if the iterator is at the end
 *   of the list, otherwise %TRUE
 */
gboolean
pango2_attr_iterator_next (Pango2AttrIterator *iterator)
{
  int i;

  g_return_val_if_fail (iterator != NULL, FALSE);

  if (iterator->attr_index >= iterator->n_attrs &&
      (!iterator->attribute_stack || iterator->attribute_stack->len == 0))
    return FALSE;

  iterator->start_index = iterator->end_index;
  iterator->end_index = G_MAXUINT;

  if (iterator->attribute_stack)
    {
      for (i = iterator->attribute_stack->len - 1; i>= 0; i--)
        {
          const Pango2Attribute *attr = g_ptr_array_index (iterator->attribute_stack, i);

          if (attr->end_index == iterator->start_index)
            g_ptr_array_remove_index (iterator->attribute_stack, i); /* Can't use index_fast :( */
          else
            iterator->end_index = MIN (iterator->end_index, attr->end_index);
        }
    }

  while (1)
    {
      Pango2Attribute *attr;

      if (iterator->attr_index >= iterator->n_attrs)
        break;

      attr = g_ptr_array_index (iterator->attrs, iterator->attr_index);

      if (attr->start_index != iterator->start_index)
        break;

      if (attr->end_index > iterator->start_index)
        {
          if (G_UNLIKELY (!iterator->attribute_stack))
            iterator->attribute_stack = g_ptr_array_new ();

          g_ptr_array_add (iterator->attribute_stack, attr);

          iterator->end_index = MIN (iterator->end_index, attr->end_index);
        }

      iterator->attr_index++; /* NEXT! */
    }

  if (iterator->attr_index < iterator->n_attrs)
      {
      Pango2Attribute *attr = g_ptr_array_index (iterator->attrs, iterator->attr_index);

      iterator->end_index = MIN (iterator->end_index, attr->start_index);
    }

  return TRUE;
}

/**
 * pango2_attr_iterator_get:
 * @iterator: a `Pango2AttrIterator`
 * @type: the type of attribute to find
 *
 * Find the current attribute of a particular type
 * at the iterator location.
 *
 * When multiple attributes of the same type overlap,
 * the attribute whose range starts closest to the
 * current location is used.
 *
 * Return value: (nullable) (transfer none): the current
 *   attribute of the given type, or %NULL if no attribute
 *   of that type applies to the current location.
 */
Pango2Attribute *
pango2_attr_iterator_get (Pango2AttrIterator *iterator,
                          guint               type)
{
  int i;

  g_return_val_if_fail (iterator != NULL, NULL);

  if (!iterator->attribute_stack)
    return NULL;

  for (i = iterator->attribute_stack->len - 1; i>= 0; i--)
    {
      Pango2Attribute *attr = g_ptr_array_index (iterator->attribute_stack, i);

      if (attr->type == type)
        return attr;
    }

  return NULL;
}

/**
 * pango2_attr_iterator_get_font:
 * @iterator: a `Pango2AttrIterator`
 * @desc: (out caller-allocates): a `Pango2FontDescription` to fill in with the current
 *   values. The family name in this structure will be set using
 *   [method@Pango2.FontDescription.set_family_static] using
 *   values from an attribute in the `Pango2AttrList` associated
 *   with the iterator, so if you plan to keep it around, you
 *   must call:
 *   `pango2_font_description_set_family (desc, pango2_font_description_get_family (desc))`.
 * @language: (out) (optional): location to store language tag
 *   for item, or %NULL if none is found.
 * @extra_attrs: (out) (optional) (element-type Pango2.Attribute) (transfer full):
 *   location in which to store a list of non-font attributes
 *   at the the current position; only the highest priority
 *   value of each attribute will be added to this list. In
 *   order to free this value, you must call
 *   [method@Pango2.Attribute.destroy] on each member.
 *
 * Get the font and other attributes at the current
 * iterator position.
 */
void
pango2_attr_iterator_get_font (Pango2AttrIterator     *iterator,
                               Pango2FontDescription  *desc,
                               Pango2Language        **language,
                               GSList                **extra_attrs)
{
  Pango2FontMask mask = 0;
  gboolean have_language = FALSE;
  double scale = 0;
  gboolean have_scale = FALSE;
  int i;

  g_return_if_fail (iterator != NULL);
  g_return_if_fail (desc != NULL);

  if (language)
    *language = NULL;

  if (extra_attrs)
    *extra_attrs = NULL;

  if (!iterator->attribute_stack)
    return;

  for (i = iterator->attribute_stack->len - 1; i >= 0; i--)
    {
      const Pango2Attribute *attr = g_ptr_array_index (iterator->attribute_stack, i);

      switch ((int) attr->type)
        {
        case PANGO2_ATTR_FONT_DESC:
          {
            Pango2FontMask new_mask = pango2_font_description_get_set_fields (attr->font_value) & ~mask;
            mask |= new_mask;
            pango2_font_description_unset_fields (desc, new_mask);
            pango2_font_description_merge_static (desc, attr->font_value, FALSE);

            break;
          }
        case PANGO2_ATTR_FAMILY:
          if (!(mask & PANGO2_FONT_MASK_FAMILY))
            {
              mask |= PANGO2_FONT_MASK_FAMILY;
              pango2_font_description_set_family (desc, attr->str_value);
            }
          break;
        case PANGO2_ATTR_STYLE:
          if (!(mask & PANGO2_FONT_MASK_STYLE))
            {
              mask |= PANGO2_FONT_MASK_STYLE;
              pango2_font_description_set_style (desc, attr->int_value);
            }
          break;
        case PANGO2_ATTR_VARIANT:
          if (!(mask & PANGO2_FONT_MASK_VARIANT))
            {
              mask |= PANGO2_FONT_MASK_VARIANT;
              pango2_font_description_set_variant (desc, attr->int_value);
            }
          break;
        case PANGO2_ATTR_WEIGHT:
          if (!(mask & PANGO2_FONT_MASK_WEIGHT))
            {
              mask |= PANGO2_FONT_MASK_WEIGHT;
              pango2_font_description_set_weight (desc, attr->int_value);
            }
          break;
        case PANGO2_ATTR_STRETCH:
          if (!(mask & PANGO2_FONT_MASK_STRETCH))
            {
              mask |= PANGO2_FONT_MASK_STRETCH;
              pango2_font_description_set_stretch (desc, attr->int_value);
            }
          break;
        case PANGO2_ATTR_SIZE:
          if (!(mask & PANGO2_FONT_MASK_SIZE))
            {
              mask |= PANGO2_FONT_MASK_SIZE;
              pango2_font_description_set_size (desc, attr->int_value);
            }
          break;
        case PANGO2_ATTR_ABSOLUTE_SIZE:
          if (!(mask & PANGO2_FONT_MASK_SIZE))
            {
              mask |= PANGO2_FONT_MASK_SIZE;
              pango2_font_description_set_absolute_size (desc, attr->int_value);
            }
          break;
        case PANGO2_ATTR_SCALE:
          if (!have_scale)
            {
              have_scale = TRUE;
              scale = attr->double_value;
            }
          break;
        case PANGO2_ATTR_LANGUAGE:
          if (language)
            {
              if (!have_language)
                {
                  have_language = TRUE;
                  *language = attr->lang_value;
                }
            }
          break;
        default:
          if (extra_attrs)
            {
              gboolean found = FALSE;

              if (PANGO2_ATTR_MERGE (attr) == PANGO2_ATTR_MERGE_OVERRIDES)
                {
                  GSList *tmp_list = *extra_attrs;
                  while (tmp_list)
                    {
                      Pango2Attribute *old_attr = tmp_list->data;
                      if (attr->type == old_attr->type)
                        {
                          found = TRUE;
                          break;
                        }

                      tmp_list = tmp_list->next;
                    }
                }

              if (!found)
                *extra_attrs = g_slist_prepend (*extra_attrs, pango2_attribute_copy (attr));
            }
        }
    }

  if (have_scale)
    {
      /* We need to use a local variable to ensure that the compiler won't
       * implicitly cast it to integer while the result is kept in registers,
       * leading to a wrong approximation in i386 (with 387 FPU)
       */
      volatile double size = scale * pango2_font_description_get_size (desc);

      if (pango2_font_description_get_size_is_absolute (desc))
        pango2_font_description_set_absolute_size (desc, size);
      else
        pango2_font_description_set_size (desc, size);
    }
}

/**
 * pango2_attr_iterator_get_attrs:
 * @iterator: a `Pango2AttrIterator`
 *
 * Gets a list of all attributes at the current position of the
 * iterator.
 *
 * Return value: (element-type Pango2.Attribute) (transfer full):
 *   a list of all attributes for the current range. To free
 *   this value, call [method@Pango2.Attribute.destroy] on each
 *   value and [func@GLib.SList.free] on the list.
 */
GSList *
pango2_attr_iterator_get_attrs (Pango2AttrIterator *iterator)
{
  GSList *attrs = NULL;
  int i;

  if (!iterator->attribute_stack ||
      iterator->attribute_stack->len == 0)
    return NULL;

  for (i = iterator->attribute_stack->len - 1; i >= 0; i--)
    {
      Pango2Attribute *attr = g_ptr_array_index (iterator->attribute_stack, i);
      GSList *tmp_list2;
      gboolean found = FALSE;

      if (PANGO2_ATTR_MERGE (attr) == PANGO2_ATTR_MERGE_OVERRIDES)
        {
          for (tmp_list2 = attrs; tmp_list2; tmp_list2 = tmp_list2->next)
            {
              Pango2Attribute *old_attr = tmp_list2->data;
              if (attr->type == old_attr->type)
                {
                  found = TRUE;
                  break;
                }
            }
        }

      if (!found)
        attrs = g_slist_prepend (attrs, pango2_attribute_copy (attr));
    }

  return attrs;
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
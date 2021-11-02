/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.nxmc.base.widgets.helpers;

/**
 * Attribute for search query (used by content proposal provider)
 */
public class SearchQueryAttribute
{
   public String name;
   public String[] staticValues;
   public SearchQueryAttributeValueProvider valueProvider;

   /**
    * Attribute without predefined values
    *
    * @param name attribute name
    */
   public SearchQueryAttribute(String name)
   {
      this.name = name;
      this.staticValues = null;
      this.valueProvider = null;
   }

   /**
    * @param name attribute name
    * @param values predefined values
    */
   public SearchQueryAttribute(String name, String... values)
   {
      this.name = name;
      this.staticValues = values;
      this.valueProvider = null;
   }

   /**
    * Attribute with dynamically retrieved values
    *
    * @param name attribute name
    * @param valueProvider value provider
    */
   public SearchQueryAttribute(String name, SearchQueryAttributeValueProvider valueProvider)
   {
      this.name = name;
      this.staticValues = null;
      this.valueProvider = valueProvider;
   }
}

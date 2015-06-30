/**
 * Java-Bridge NetXMS subagent
 * Copyright (C) 2013 TEMPEST a.s.
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
package org.netxms.agent;

/**
 * Table column information
 */
public class TableColumn
{
   private String name;
   private String displayName;
   private ParameterType type;
   private boolean instance;

   /**
    * @param name
    * @param displayName
    * @param type
    * @param instance
    */
   public TableColumn(final String name, final String displayName, final ParameterType type, final boolean instance)
   {
      this.name = name;
      this.displayName = displayName;
      this.type = type;
      this.instance = instance;
   }

   /**
    * @param name
    * @param displayName
    * @param type
    */
   public TableColumn(final String name, final String displayName, final ParameterType type)
   {
      this(name, displayName, type, false);
   }
   
   /**
    * @param name
    * @param type
    */
   public TableColumn(final String name, final ParameterType type)
   {
      this(name, name, type, false);
   }
   
   /**
    * @param name
    * @param type
    * @param instance
    */
   public TableColumn(final String name, final ParameterType type, final boolean instance)
   {
      this(name, name, type, instance);
   }

   /**
    * @return
    */
   public String getName()
   {
      return name;
   }

   /**
    * @return the displayName
    */
   public String getDisplayName()
   {
      return displayName;
   }

   /**
    * @return
    */
   public ParameterType getType()
   {
      return type;
   }

   /**
    * @return
    */
   public boolean isInstance()
   {
      return instance;
   }
}

/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
package org.netxms.nxmc.base.views;

import org.eclipse.jface.resource.ImageDescriptor;

/**
 * Configuration view
 */
public abstract class ConfigurationView extends View
{   
   /**
    * Standard return code constant (value 0) indicating that the view needs to be
    * saved.
    */
   public static int YES = 0;

   /**
    * Standard return code constant (value 1) indicating that the view does not
    * need to be saved and the part should be closed.
    */
   public static int NO = 1;

   /**
    * Standard return code constant (value 2) indicating that the view does not
    * need to be saved and the view should not be closed.
    */
   public static int CANCEL = 2;
   
   /**
    * Default constructor.
    */
   protected ConfigurationView()
   {
      super();
   }

   /**
    * Create new view with specific ID. This will not create actual widgets that composes view - creation can be delayed by
    * framework until view actually has to be shown for the first time.
    *
    * @param name view name
    * @param image view image
    * @param id view ID
    * @param hasFileter true if view should contain filter
    */
   public ConfigurationView(String name, ImageDescriptor image, String id, boolean hasFilter)
   {
      super(name, image, id, hasFilter);
   }
   
   /**
    * Check if view content is modified and requires saving.
    *
    * @return true if view content is modified and requires saving
    */
   public abstract boolean isModified();

   /**
    * Save content. After successful save() call isModified() should return false.
    */
   public abstract void save();
   

   /** 
    * Open save dialog on close and return if save is required
    * Default implementation returns that save is not required
    */
   protected int promptToSaveOnClose()
   {
      return NO;
   }
}

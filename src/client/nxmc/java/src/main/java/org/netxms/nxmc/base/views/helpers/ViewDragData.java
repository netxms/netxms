/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.base.views.helpers;

import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewFolder;

/**
 * Data object for drag-and-drop of view tabs between view folders.
 */
public class ViewDragData
{
   public final ViewFolder sourceFolder;
   public final View view;

   /**
    * Create new view drag data.
    *
    * @param sourceFolder source view folder
    * @param view view being dragged
    */
   public ViewDragData(ViewFolder sourceFolder, View view)
   {
      this.sourceFolder = sourceFolder;
      this.view = view;
   }
}

/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectbrowser.api;

import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.IShellProvider;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.dialogs.PropertyDialogAction;
import org.netxms.ui.eclipse.console.resources.GroupMarkers;

/**
 * Helper class for object context menu manipulation
 */
public final class ObjectContextMenu
{
   /**
    * Fill default object context menu
    * 
    * @param manager
    */
   public static void fill(IMenuManager manager, IShellProvider shellProvider, ISelectionProvider selectionProvider)
   {
      manager.add(new GroupMarker(GroupMarkers.MB_OBJECT_CREATION));
      manager.add(new Separator());
      manager.add(new GroupMarker(GroupMarkers.MB_ATM));
      manager.add(new Separator());
      manager.add(new GroupMarker(GroupMarkers.MB_NXVS));
      manager.add(new Separator());
      manager.add(new GroupMarker(GroupMarkers.MB_OBJECT_MANAGEMENT));
      manager.add(new Separator());
      manager.add(new GroupMarker(GroupMarkers.MB_OBJECT_BINDING));
      manager.add(new Separator());
      manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
      manager.add(new Separator());
      manager.add(new GroupMarker(GroupMarkers.MB_EVENTS_AND_LOGS));
      manager.add(new Separator());
      manager.add(new GroupMarker(GroupMarkers.MB_TOPOLOGY));
      manager.add(new Separator());
      manager.add(new GroupMarker(GroupMarkers.MB_DATA_COLLECTION));
      manager.add(new Separator());
      manager.add(new GroupMarker(GroupMarkers.MB_PROPERTIES));
      if ((shellProvider != null) && (selectionProvider != null) &&
          (((IStructuredSelection)selectionProvider.getSelection()).size() == 1))
      {
         PropertyDialogAction action = new PropertyDialogAction(shellProvider, selectionProvider);
         manager.add(action);
         action.setEnabled((selectionProvider.getSelection() instanceof IStructuredSelection) &&
               (((IStructuredSelection)selectionProvider.getSelection()).size() == 1));
      }
   }
}

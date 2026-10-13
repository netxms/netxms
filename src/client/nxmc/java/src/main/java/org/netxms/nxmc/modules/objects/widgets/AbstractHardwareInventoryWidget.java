/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.objects.widgets;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Asset;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.objects.views.ObjectView;

/**
 * Common base class for hardware inventory widgets
 */
public abstract class AbstractHardwareInventoryWidget extends Composite
{
   protected NXCSession session = Registry.getSession();
   protected ObjectView view;

   /**
    * Create new hardware inventory widget.
    *
    * @param parent parent composite
    * @param style widget style
    * @param view owning view
    */
   public AbstractHardwareInventoryWidget(Composite parent, int style, ObjectView view)
   {
      super(parent, style);
      this.view = view;
      setLayout(new FillLayout());
   }

   /**
    * Get underlying viewer.
    *
    * @return underlying viewer
    */
   public abstract ColumnViewer getViewer();

   /**
    * Check if underlying viewer is a tree viewer.
    *
    * @return true if underlying viewer is a tree viewer
    */
   public boolean isTreeViewer()
   {
      return getViewer() instanceof TreeViewer;
   }

   /**
    * Refresh widget
    */
   public abstract void refresh();

   /**
    * Reload assets linked to components and update their display without re-reading inventory from server.
    */
   public abstract void refreshComponentAssets();

   /**
    * Get assets linked to components of the node displayed by this widget, keyed by component serial number.
    *
    * @return assets keyed by component serial number
    */
   protected Map<String, Asset> getComponentAssets()
   {
      Map<String, Asset> assets = new HashMap<>();
      for(Asset a : session.getComponentAssets(view.getObjectId()))
         assets.put(a.getComponentSerial(), a);
      return assets;
   }

   /**
    * Clear data form view
    */
   public void clear()
   {
      getViewer().setInput(new Object[0]);
   }

   /**
    * Set menu manager for underlying viewer.
    *
    * @param manager menu manager
    */
   public void setViewerMenu(MenuManager manager)
   {
      Menu menu = manager.createContextMenu(getViewer().getControl());
      getViewer().getControl().setMenu(menu);
   }

   /**
    * Copy currently selected line(s) to clipboard.
    *
    * @param column column index or -1 to copy all columns
    */
   public abstract void copySelectionToClipboard(int column);

   /**
    * Get index of "name" column
    *
    * @return index of "name" column
    */
   public abstract int getNameColumnIndex();

   /**
    * Get index of "description" column
    *
    * @return index of "description" column
    */
   public abstract int getDescriptionColumnIndex();

   /**
    * Get index of "model" column
    *
    * @return index of "model" column
    */
   public abstract int getModelColumnIndex();

   /**
    * Get index of "serial" column
    *
    * @return index of "serial" column
    */
   public abstract int getSerialColumnIndex();

   /**
    * Get action for toggling automatic column resize on the underlying viewer.
    *
    * @return check-box action for toggling automatic column resize
    */
   public abstract Action getAutoSizeColumnsAction();
}

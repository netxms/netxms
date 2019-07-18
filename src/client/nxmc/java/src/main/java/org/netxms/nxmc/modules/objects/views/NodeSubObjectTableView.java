/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
package org.netxms.nxmc.modules.objects.views;

import org.apache.commons.lang3.SystemUtils;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableItem;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.views.helpers.NodeSubObjectFilter;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Base class for object views that display node sub-objects using table viewer.
 */
public abstract class NodeSubObjectTableView extends NodeSubObjectView
{
   protected SortableTableViewer viewer;
   protected Action actionCopyToClipboard;
   protected Action actionExportToCsv;
   protected boolean showFilter = true;
   protected FilterText filterText;
   protected NodeSubObjectFilter filter;

   /**
    * @param name
    * @param image
    * @param id
    */
   public NodeSubObjectTableView(String name, ImageDescriptor image, String id)
   {
      super(name, image, id);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.NodeSubObjectView#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      super.createContent(parent);

      final PreferenceStore settings = PreferenceStore.getInstance();
      showFilter = settings.getAsBoolean(getFilterSettingName(), showFilter);

      // Create filter
      filterText = new FilterText(mainArea, SWT.NONE);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });
      filterText.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            settings.set(getFilterSettingName(), showFilter);
         }
      });

      Action action = new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
         }
      };
      filterText.setCloseAction(action);

      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);

      createViewer();
      createActions();
      createPopupMenu();

      // Set initial focus to filter input line
      if (showFilter)
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly
   }

   /**
    * Create filter control
    * 
    * @return filter control
    */
   protected FilterText createFilterText()
   {
      return new FilterText(mainArea, SWT.NONE);
   }

   /**
    * Returns created viewer
    * 
    * @return viewer
    */
   protected abstract void createViewer();

   /**
    * Return filter setting name
    * 
    * @return filter setting name
    */
   public abstract String getFilterSettingName();

   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   public void enableFilter(boolean enable)
   {
      showFilter = enable;
      filterText.setVisible(showFilter);
      FormData fd = (FormData)viewer.getTable().getLayoutData();
      fd.top = enable ? new FormAttachment(filterText, 0, SWT.BOTTOM) : new FormAttachment(0, 0);
      mainArea.layout();
      if (enable)
      {
         filterText.setFocus();
      }
      else
      {
         filterText.setText(""); //$NON-NLS-1$
         onFilterModify();
      }
   }

   /**
    * Handler for filter modification
    */
   public void onFilterModify()
   {
      if (filter == null)
         return;

      final String text = filterText.getText();
      filter.setFilterString(text);
      viewer.refresh(false);
   }

   /**
    * Create actions
    */
   protected void createActions()
   {
      actionCopyToClipboard = new Action("Copy to clipboard", SharedIcons.COPY) {
         @Override
         public void run()
         {
            copyToClipboard(-1);
         }
      };

      actionExportToCsv = new ExportToCsvAction(this, viewer, true);
   }

   /**
    * Create pop-up menu
    */
   protected void createPopupMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager manager)
         {
            fillContextMenu(manager);
         }
      });

      // Create menu.
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected abstract void fillContextMenu(IMenuManager manager);

   /**
    * Copy content to clipboard
    * 
    * @param column column number or -1 to copy all columns
    */
   protected void copyToClipboard(int column)
   {
      final TableItem[] selection = viewer.getTable().getSelection();
      if (selection.length > 0)
      {
         final String newLine = SystemUtils.IS_OS_WINDOWS ? "\r\n" : "\n";
         final StringBuilder sb = new StringBuilder();
         for(int i = 0; i < selection.length; i++)
         {
            if (i > 0)
               sb.append(newLine);
            if (column == -1)
            {
               for(int j = 0; j < viewer.getTable().getColumnCount(); j++)
               {
                  if (j > 0)
                     sb.append('\t');
                  sb.append(selection[i].getText(j));
               }
            }
            else
            {
               sb.append(selection[i].getText(column));
            }
         }
         WidgetHelper.copyToClipboard(sb.toString());
      }
   }
}

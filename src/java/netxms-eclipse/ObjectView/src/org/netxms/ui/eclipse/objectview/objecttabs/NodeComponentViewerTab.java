/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectview.objecttabs;

import org.eclipse.core.commands.Command;
import org.eclipse.core.commands.State;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ISelectionProvider;
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
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.commands.ICommandService;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.NodeComponentTabFilter;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Common node component tab class with table or tree view
 */
public abstract class NodeComponentViewerTab extends NodeComponentTab
{
   protected SortableTableViewer viewer;
   protected Action actionCopyToClipboard;
   protected Action actionExportToCsv;

   protected boolean showFilter = true;
   protected FilterText filterText;
   protected NodeComponentTabFilter filter;

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createTabContent(Composite parent)
   {
      super.createTabContent(parent);

      final IDialogSettings settings = Activator.getDefault().getDialogSettings();
      showFilter = safeCast(settings.get(getFilterSettingName()), settings.getBoolean(getFilterSettingName()), showFilter); // $NON-NLS-1$

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
            settings.put(getFilterSettingName(), showFilter); // $NON-NLS-1$
         }
      });

      Action action = new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
            ICommandService service = (ICommandService)PlatformUI.getWorkbench().getService(ICommandService.class);
            Command command = service.getCommand("org.netxms.ui.eclipse.objectview.commands.show_filter"); //$NON-NLS-1$
            State state = command.getState("org.netxms.ui.eclipse.objectview.commands.show_filter.state"); //$NON-NLS-1$
            state.setValue(false);
            service.refreshElements(command.getId(), null);
         }
      };
      setFilterCloseAction(action);

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
    * @see org.netxms.ui.eclipse.objectview.objecttabs.NodeComponentTab#selected()
    */
   @Override
   public void selected()
   {
      // Check/uncheck menu items
      ICommandService service = (ICommandService)PlatformUI.getWorkbench().getService(ICommandService.class);

      Command command = service.getCommand("org.netxms.ui.eclipse.objectview.commands.show_filter"); //$NON-NLS-1$
      State state = command.getState("org.netxms.ui.eclipse.objectview.commands.show_filter.state"); //$NON-NLS-1$
      state.setValue(showFilter);
      service.refreshElements(command.getId(), null);

      super.selected();
   }

   /**
    * @param b
    * @param defval
    * @return
    */
   protected static boolean safeCast(String s, boolean b, boolean defval)
   {
      return (s != null) ? b : defval;
   }

   /**
    * @param action
    */
   private void setFilterCloseAction(Action action)
   {
      filterText.setCloseAction(action);
   }

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

      actionExportToCsv = new ExportToCsvAction(getViewPart(), viewer, true);
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

      // Register menu for extension.
      if (getViewPart() != null)
         getViewPart().getSite().registerContextMenu(menuMgr, viewer);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected abstract void fillContextMenu(IMenuManager manager);

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#currentObjectUpdated(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public void currentObjectUpdated(AbstractObject object)
   {
      objectChanged(object);
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#showForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean showForObject(AbstractObject object)
   {
      return (object instanceof AbstractNode);
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#getSelectionProvider()
    */
   @Override
   public ISelectionProvider getSelectionProvider()
   {
      return viewer;
   }

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
         final String newLine = WidgetHelper.getNewLineCharacters();
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

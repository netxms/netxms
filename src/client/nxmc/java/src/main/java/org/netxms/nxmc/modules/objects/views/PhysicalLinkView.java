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
package org.netxms.nxmc.modules.objects.views;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.PhysicalLink;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Rack;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.PhysicalLinkEditDialog;
import org.netxms.nxmc.modules.objects.views.helpers.PhysicalLinkComparator;
import org.netxms.nxmc.modules.objects.views.helpers.PhysicalLinkFilter;
import org.netxms.nxmc.modules.objects.views.helpers.PhysicalLinkLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Physical link view
 */
public class PhysicalLinkView extends ObjectView
{
   private static I18n i18n = LocalizationHelper.getI18n(PhysicalLinkView.class);

   private static final String TABLE_CONFIG_PREFIX = "PhysicalLinkView";

   public static final int COL_PHYSICAL_LINK_ID = 0;
   public static final int COL_DESCRIPTOIN = 1;
   public static final int COL_LEFT_OBJECT = 2;
   public static final int COL_LEFT_PORT = 3;
   public static final int COL_RIGHT_OBJECT = 4;
   public static final int COL_RIGHT_PORT = 5;

   private NXCSession session;
   private SessionListener sessionListener;
   private SortableTableViewer viewer;
   private PhysicalLinkFilter filter;
   private long patchPanelId = 0;
   private Action actionAdd;
   private Action actionDelete;
   private Action actionEdit;

   /**
    * @param name
    * @param image
    * @param id
    * @param hasFilter
    */
   public PhysicalLinkView()
   {
      super(i18n.tr("Physical links"), ResourceManager.getImageDescriptor("icons/object-views/physical_links.png"), "PhysicalLink", true);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && ((context instanceof Node) || (context instanceof Interface) || (context instanceof Rack));
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 55;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      session = Registry.getSession();

      final int[] widths = { 50, 200, 200, 400, 200, 400 };
      final String[] names = { i18n.tr("ID"), i18n.tr("Description"), i18n.tr("Object 1"), i18n.tr("Port 1"), i18n.tr("Object 2"), i18n.tr("Port 2") };
      viewer = new SortableTableViewer(parent, names, widths, COL_PHYSICAL_LINK_ID, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      PhysicalLinkLabelProvider labelPrivater = new PhysicalLinkLabelProvider();
      viewer.setLabelProvider(labelPrivater);
      viewer.setComparator(new PhysicalLinkComparator(labelPrivater));
      filter = new PhysicalLinkFilter(labelPrivater);
      viewer.addFilter(filter);

      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            actionEdit.run();
         }
      });

      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)event.getSelection();
            if (selection != null)
            {
               actionEdit.setEnabled(selection.size() == 1);
               actionDelete.setEnabled(selection.size() > 0);
            }
         }
      });

      WidgetHelper.restoreTableViewerSettings(viewer, TABLE_CONFIG_PREFIX);
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, TABLE_CONFIG_PREFIX);
         }
      });

      createActions();
      createPopupMenu();

      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.PHYSICAL_LINK_UPDATE)
            {
               getDisplay().asyncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     refresh();
                  }
               });
            }
         }
      };
      session.addListener(sessionListener);
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionAdd = new Action(i18n.tr("&New...")) {
         @Override
         public void run()
         {
            addPhysicalLink();
         }
      };
      actionAdd.setImageDescriptor(SharedIcons.ADD_OBJECT);

      actionDelete = new Action(i18n.tr("&Delete")) {
         @Override
         public void run()
         {
            deletePhysicalLink();
         }
      };
      actionDelete.setImageDescriptor(SharedIcons.DELETE_OBJECT);

      actionEdit = new Action(i18n.tr("&Edit...")) {
         @Override
         public void run()
         {
            editPhysicalLink();
         }
      };
      actionEdit.setImageDescriptor(SharedIcons.EDIT);
   }

   /**
    * Create popup menu
    */
   private void createPopupMenu()
   {
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param mgr menu manager
    */
   protected void fillContextMenu(IMenuManager mgr)
   {
      mgr.add(actionAdd);
      mgr.add(actionEdit);
      mgr.add(actionDelete);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      if (object != null)
      {
         refresh();
      }
      else
      {
         viewer.setInput(new Object[0]);
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      final long objectId = getObjectId();
      if (objectId == 0)
         return;

      new Job(i18n.tr("Load physical links"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<PhysicalLink> links = session.getPhysicalLinks(objectId, patchPanelId);
            syncAdditionalObjects(links);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(links);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Error getting physical link list");
         }
      }.start();
   }

   /**
    * Sync physical links interface objects
    * 
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   private void syncAdditionalObjects(Collection<PhysicalLink> links) throws IOException, NXCException
   {
      List<Long> additionalSyncInterfaces = new ArrayList<Long>();
      for(PhysicalLink link : links)
      {
         long id = link.getLeftObjectId();
         if (id != 0)
            additionalSyncInterfaces.add(id);
         id = link.getRightObjectId();
         if (id != 0)
            additionalSyncInterfaces.add(id);
      }

      if (!additionalSyncInterfaces.isEmpty())
         session.syncMissingObjects(additionalSyncInterfaces, true, NXCSession.OBJECT_SYNC_WAIT);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(sessionListener);
      super.dispose();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#onFilterModify()
    */
   @Override
   protected void onFilterModify()
   {
      final String text = getFilterText();
      filter.setFilterString(text);
      viewer.refresh(false);
   }

   /**
    * Edit selected physical link
    */
   private void editPhysicalLink()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      PhysicalLink link = (PhysicalLink)selection.getFirstElement();
      PhysicalLinkEditDialog dlg = new PhysicalLinkEditDialog(getWindow().getShell(), link);
      if (dlg.open() != Window.OK)
         return;

      final PhysicalLink editedLink = dlg.getLink();
      new Job(i18n.tr("Modify physical link"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.updatePhysicalLink(editedLink);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot modify physical link");
         }
      }.start();

   }

   /**
    * Delete selected physical link
    */
   private void deletePhysicalLink()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      final String message = (selection.size() == 1) ? i18n.tr("Do you really want to delete the selected physical link?") : i18n.tr("Do you really want to delete the selected physical links?");
      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Confirm Delete"), message))
         return;

      final List<PhysicalLink> links = new ArrayList<PhysicalLink>(selection.size());
      for(Object o : selection.toList())
         links.add((PhysicalLink)o);
      new Job(i18n.tr("Delete physical link"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(PhysicalLink link : links)
               session.deletePhysicalLink(link.getId());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete physical link");
         }
      }.start();
   }

   /**
    * Add new physical link
    */
   private void addPhysicalLink()
   {
      PhysicalLinkEditDialog dlg = new PhysicalLinkEditDialog(getWindow().getShell(), null);
      if (dlg.open() != Window.OK)
         return;

      final PhysicalLink link = dlg.getLink();
      new Job(i18n.tr("Create new physical link"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.updatePhysicalLink(link);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create physical link");
         }
      }.start();
   }
}

/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCException;
import org.netxms.client.SoftwarePackage;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.actions.ViewerProvider;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.helpers.SoftwareInventoryContentProvider;
import org.netxms.nxmc.modules.objects.views.helpers.SoftwareInventoryNode;
import org.netxms.nxmc.modules.objects.views.helpers.SoftwarePackageComparator;
import org.netxms.nxmc.modules.objects.views.helpers.SoftwarePackageFilter;
import org.netxms.nxmc.modules.objects.views.helpers.SoftwarePackageLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Software inventory view
 */
public class SoftwareInventoryView extends ObjectView
{
   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_VERSION = 1;
   public static final int COLUMN_VENDOR = 2;
   public static final int COLUMN_DATE = 3;
   public static final int COLUMN_DESCRIPTION = 4;
   public static final int COLUMN_URL = 5;

   private final I18n i18n = LocalizationHelper.getI18n(SoftwareInventoryView.class);

   private final String[] names = { i18n.tr("Name"), i18n.tr("Version"), i18n.tr("Vendor"), i18n.tr("Install Date"), i18n.tr("Description"), i18n.tr("URL") };
   private final int[] widths = { 200, 100, 200, 100, 300, 200 };

   private ColumnViewer viewer;
   private SoftwarePackageFilter filter = new SoftwarePackageFilter();
   private MenuManager menuManager = null;
	private Action actionExportToCsv;
	private Action actionExportAllToCsv;
   private Action actionUninstall;

   /**
    * @param name
    * @param image
    * @param id
    */
   public SoftwareInventoryView()
   {
      super(LocalizationHelper.getI18n(SoftwareInventoryView.class).tr("Software Inventory"), ResourceManager.getImageDescriptor("icons/object-views/software.png"), "objects.software-inventory", true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      if ((context != null) && (context instanceof Node))
      {
         return (((Node)context).getCapabilities() & Node.NC_IS_NATIVE_AGENT) != 0;
      }
      return (context != null) && ((context instanceof Container) || (context instanceof Collector));
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 60;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      createTableViewer();
		createActions();
		createContextMenu();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
      ViewerProvider vp = () -> viewer;
		actionExportToCsv = new ExportToCsvAction(this, vp, true);
		actionExportAllToCsv = new ExportToCsvAction(this, vp, false);

      actionUninstall = new Action(i18n.tr("&Uninstall")) {
         @Override
         public void run()
         {
            uninstallPackage();
         }
      };
	}

   /**
    * Fill context menu
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      IStructuredSelection selection = viewer.getStructuredSelection();

      manager.add(actionExportToCsv);
      manager.add(new Separator());
      if ((selection.size() == 1) && (selection.getFirstElement() instanceof SoftwarePackage) && !((SoftwarePackage)selection.getFirstElement()).getUninstallKey().isEmpty())
         manager.add(actionUninstall);
   }

	/**
	 * Create pop-up menu
	 */
	private void createContextMenu()
	{
      menuManager = new MenuManager();
      menuManager.setRemoveAllWhenShown(true);
      menuManager.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});
      if (viewer != null)
      {
         Menu menu = menuManager.createContextMenu(viewer.getControl());
         viewer.getControl().setMenu(menu);
      }
	}

   /**
    * Fill view's local toolbar
    *
    * @param manager toolbar manager
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionExportAllToCsv);
      manager.add(new Separator());
   }

   /**
    * Refresh
    */
   @Override
   public void refresh()
   {
      final AbstractObject object = getObject();
      new Job(i18n.tr("Reading software package information"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            if (object instanceof AbstractNode)
            {
               final List<SoftwarePackage> packages = session.getNodeSoftwarePackages(object.getObjectId());
               runInUIThread(() -> {
                  if (viewer.getControl().isDisposed() || (object.getObjectId() != getObjectId()))
                     return;
                  viewer.setInput(packages.toArray());
                  ((SortableTableViewer)viewer).packColumns();
               });
            }
            else
            {
               final List<SoftwareInventoryNode> nodes = new ArrayList<SoftwareInventoryNode>();
               for(final AbstractObject o : object.getAllChildren(AbstractObject.OBJECT_NODE))
               {
                  try
                  {
                     List<SoftwarePackage> packages = session.getNodeSoftwarePackages(o.getObjectId());
                     nodes.add(new SoftwareInventoryNode((Node)o, packages));
                  }
                  catch(NXCException e)
                  {
                     if (e.getErrorCode() != RCC.NO_SOFTWARE_PACKAGE_DATA)
                        throw e;
                  }
               }
               runInUIThread(() -> {
                  if (viewer.getControl().isDisposed() || (object.getObjectId() != getObjectId()))
                     return;
                  viewer.setInput(nodes);
                  ((SortableTreeViewer)viewer).packColumns();
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get software package information for node");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      viewer.setInput(new ArrayList<Objects>());
      if (object == null)
         return;

      setRootObjectId(object.getObjectId());
      refresh();
   }

   /**
    * Create viewer for table mode
    */
   private void createTableViewer()
   {
      viewer = new SortableTableViewer(getClientArea(), names, widths, 0, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new SoftwarePackageLabelProvider(false));
      viewer.setComparator(new SoftwarePackageComparator());
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);
      if (menuManager != null)
      {
         Menu menu = menuManager.createContextMenu(viewer.getControl());
         viewer.getControl().setMenu(menu);
      }
   }

   /**
    * Create viewer for tree mode
    */
   private void createTreeViewer()
   {
      viewer = new SortableTreeViewer(getClientArea(), names, widths, 0, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new SoftwareInventoryContentProvider());
      viewer.setLabelProvider(new SoftwarePackageLabelProvider(true));
      viewer.setComparator(new SoftwarePackageComparator());
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);
      if (menuManager != null)
      {
         Menu menu = menuManager.createContextMenu(viewer.getControl());
         viewer.getControl().setMenu(menu);
      }
   }

   /**
    * @param rootObjectId the rootObjectId to set
    */
   public void setRootObjectId(long rootObjectId)
   {
      AbstractObject object = session.findObjectById(rootObjectId);
      if (object instanceof Node)
      {
         if (!(viewer instanceof SortableTableViewer))
         {
            viewer.getControl().dispose();
            createTableViewer();
            getClientArea().layout(true, true);
         }
      }
      else
      {
         if (!(viewer instanceof SortableTreeViewer))
         {
            viewer.getControl().dispose();
            createTreeViewer();
            getClientArea().layout(true, true);
         }
      }
   }

   /**
    * Uninstall selected package
    */
   private void uninstallPackage()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;
      
      final SoftwarePackage pkg = (SoftwarePackage)selection.getFirstElement();
      if (pkg.getUninstallKey().isEmpty())
         return;

      if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Confirm Uninstall"),
            i18n.tr("You are about to request uninstall of software package \"{0}\".\nUninstallation may fail if selected package does not support unattended installation.\nDo you want to continue?",
                  pkg.getName())))
         return;

      final long nodeId = (pkg.getData() != null) ? ((SoftwareInventoryNode)pkg.getData()).getNode().getObjectId() : getObjectId();
      new Job(i18n.tr("Uninstalling software package"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.executeAction(nodeId, "System.UninstallProduct", new String[] { pkg.getUninstallKey() });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot uninstall software package");
         }
      }.start();
   }
}

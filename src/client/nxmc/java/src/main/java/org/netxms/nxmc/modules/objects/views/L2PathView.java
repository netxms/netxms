/**
 * NetXMS - open source network management system
 * Copyright (C) 2023-2026 Victor Kirhenshtein
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

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.Severity;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.topology.HopInfo;
import org.netxms.client.topology.NetworkPath;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.CopyTableRowsAction;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * L2 (switch-level) path view
 */
public class L2PathView extends AdHocObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(L2PathView.class);

   public static final int COLUMN_HOP = 0;
   public static final int COLUMN_NODE = 1;
   public static final int COLUMN_NAME = 2;

   /**
    * Generate name for the view.
    *
    * @param source source object
    * @param destination destination object
    * @param contextId context object
    * @return generated view name
    */
   private static String generateName(AbstractNode source, AbstractNode destination, long contextId)
   {
      if (source.getObjectId() == contextId)
         return LocalizationHelper.getI18n(L2PathView.class).tr("L2 path to {0}", destination.getObjectName());
      if (destination.getObjectId() == contextId)
         return LocalizationHelper.getI18n(L2PathView.class).tr("L2 path from {0}", source.getObjectName());
      return LocalizationHelper.getI18n(L2PathView.class).tr("L2 path {0} - {1}", source.getObjectName(), destination.getObjectName());
   }

   private CLabel status;
   private SortableTableViewer viewer;
   private AbstractNode source;
   private AbstractNode destination;
   private Action actionExportToCsv;
   private Action actionExportAllToCsv;
   private Action actionCopyRowToClipboard;

   /**
    * @param source source node
    * @param destination destination node
    * @param contextId context object ID
    */
   public L2PathView(AbstractNode source, AbstractNode destination, long contextId)
   {
      super(generateName(source, destination, contextId), ResourceManager.getImageDescriptor("icons/object-views/route.png"),
            "objects.l2path." + source.getObjectId() + "." + destination.getObjectId(), contextId, contextId, false);
      this.source = source;
      this.destination = destination;
   }

   /**
    * Constructor for cloning
    */
   protected L2PathView()
   {
      super("", ResourceManager.getImageDescriptor("icons/object-views/route.png"), "objects.l2path.0.0", 0, 0, false);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.AdHocObjectView#cloneView()
    */
   @Override
   public View cloneView()
   {
      L2PathView view = (L2PathView)super.cloneView();
      view.source = source;
      view.destination = destination;
      return view;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      parent.setLayout(layout);

      status = new CLabel(parent, SWT.NONE);
      status.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));

      new Label(parent, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));

      final String[] names = { i18n.tr("Hop"), i18n.tr("Node"), i18n.tr("Outbound interface") };
      final int[] widths = { 80, 200, 200 };
      viewer = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.disableSorting();
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new PathLabelProvider());
      viewer.getControl().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      actionCopyRowToClipboard = new CopyTableRowsAction(viewer, true);
      actionExportToCsv = new ExportToCsvAction(this, viewer, true);
      actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);

      viewer.enableColumnReordering();
      WidgetHelper.restoreColumnOrder(viewer, getBaseId());
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveColumnOrder(viewer, getBaseId());
         }
      });

      createContextMenu();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionExportAllToCsv);
      Action resetAction = viewer.getResetColumnOrderAction();
      if (resetAction != null)
         manager.add(resetAction);
      Action showAllAction = viewer.getShowAllColumnsAction();
      if (showAllAction != null)
         manager.add(showAllAction);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionExportAllToCsv);
   }

   /**
    * Create pop-up menu
    */
   private void createContextMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> fillContextMenu(m));

      // Create menu.
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    *
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionCopyRowToClipboard);
      manager.add(actionExportToCsv);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#getFullName()
    */
   @Override
   public String getFullName()
   {
      return generateName(source, destination, 0);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Reading L2 network path"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final NetworkPath path = session.getL2NetworkPath(source.getObjectId(), destination.getObjectId());
            runInUIThread(() -> {
               List<HopInfo> hops = path.getPath();
               if (!hops.isEmpty())
               {
                  status.setImage(StatusDisplayInfo.getStatusImage(path.isComplete() ? Severity.NORMAL : Severity.MINOR));
                  if (hops.size() == 2)
                  {
                     status.setText(i18n.tr("{0}; 1 hop", path.isComplete() ? i18n.tr("Complete") : i18n.tr("Incomplete")));
                  }
                  else
                  {
                     status.setText(i18n.trn("{0}; {1} hops", "{0}; {1} hops", hops.size() - 1, path.isComplete() ? i18n.tr("Complete") : i18n.tr("Incomplete"), hops.size() - 1));
                  }
               }
               else
               {
                  status.setImage(StatusDisplayInfo.getStatusImage(Severity.CRITICAL));
                  status.setText(i18n.tr("No L2 path from source"));
               }
               if (!path.isComplete())
               {
                  hops.add(new HopInfo(hops.size()));
               }
               viewer.setInput(hops);
               viewer.packColumns();
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot retrieve L2 network path between {0} and {1}", source.getObjectName(), destination.getObjectName());
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);
      memento.set("source", source.getObjectId());
      memento.set("destination", destination.getObjectId());
   }

   /**
    * @throws ViewNotRestoredException
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {
      super.restoreState(memento);
      source = session.findObjectById(memento.getAsLong("source", 0), AbstractNode.class);
      destination = session.findObjectById(memento.getAsLong("destination", 0), AbstractNode.class);
      if (source == null)
         throw new ViewNotRestoredException(i18n.tr("Source object no longer exists or is not accessible"));
      if (destination == null)
         throw new ViewNotRestoredException(i18n.tr("Destination object no longer exists or is not accessible"));
   }

   private static class PathLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      private NXCSession session = Registry.getSession();

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
       */
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return null;
      }

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
       */
      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         HopInfo hop = (HopInfo)element;
         switch(columnIndex)
         {
            case L2PathView.COLUMN_HOP:
               return Integer.toString(hop.getIndex());
            case L2PathView.COLUMN_NAME:
               return hop.getName();
            case L2PathView.COLUMN_NODE:
               return (hop.getNodeId() > 0) ? session.getObjectName(hop.getNodeId()) : "??";
         }
         return null;
      }
   }
}

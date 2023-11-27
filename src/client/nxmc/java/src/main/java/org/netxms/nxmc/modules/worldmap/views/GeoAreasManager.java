/**
 * NetXMS - open source network management system
 * Copyright (C) 2020 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.worldmap.views;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Menu;
import org.netxms.base.GeoLocation;
import org.netxms.base.KMLParser;
import org.netxms.client.GeoArea;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.RCC;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.worldmap.dialogs.GeoAreaEditDialog;
import org.netxms.nxmc.modules.worldmap.views.helpers.GeoAreaComparator;
import org.netxms.nxmc.modules.worldmap.views.helpers.GeoAreaFilter;
import org.netxms.nxmc.modules.worldmap.views.helpers.GeoAreaLabelProvider;
import org.netxms.nxmc.modules.worldmap.widgets.GeoAreaViewer;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Object category manager
 *
 */
public class GeoAreasManager extends ConfigurationView implements SessionListener
{
   private final I18n i18n = LocalizationHelper.getI18n(GeoAreasManager.class);

   public static final int COL_ID = 0;
   public static final int COL_NAME = 1;
   public static final int COL_COMMENTS = 2;

   private static final String TABLE_CONFIG_PREFIX = "GeoAreaManager";

   private Map<Integer, GeoArea> areas = new HashMap<Integer, GeoArea>();
   private SashForm splitter;
   private SortableTableViewer viewer;
   private NXCSession session;
   private GeoAreaFilter filter;
   private Composite details;
   private TableViewer coordinateListViewer;
   private GeoAreaViewer mapViewer;
   private Action actionNew;
   private Action actionEdit;
   private Action actionDelete;
   private Action actionImport;

   /**
    * Create geo area manager view
    */
   public GeoAreasManager()
   {
      super(LocalizationHelper.getI18n(GeoAreasManager.class).tr("Geographical Areas"), ResourceManager.getImageDescriptor("icons/config-views/geo-areas.png"), "GeographicalAreas", true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      splitter = new SashForm(parent, SWT.HORIZONTAL);

      final String[] names = { "ID", "Name", "Comments" };
      final int[] widths = { 100, 400, 800 };
      viewer = new SortableTableViewer(splitter, names, widths, 1, SWT.DOWN, SWT.MULTI | SWT.FULL_SELECTION);
      WidgetHelper.restoreTableViewerSettings(viewer, TABLE_CONFIG_PREFIX);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new GeoAreaLabelProvider());
      viewer.setComparator(new GeoAreaComparator());
      filter = new GeoAreaFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);

      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)event.getSelection();
            if (selection != null)
            {
               actionEdit.setEnabled(selection.size() == 1);
               actionDelete.setEnabled(selection.size() > 0);
               updateAreaDetails((GeoArea)selection.getFirstElement());
            }
         }
      });
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editArea();
         }
      });
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, TABLE_CONFIG_PREFIX);
         }
      });

      details = new Composite(splitter, SWT.NONE);
      details.setLayout(new FillLayout(SWT.VERTICAL));

      coordinateListViewer = new TableViewer(details);
      coordinateListViewer.setContentProvider(new ArrayContentProvider());
      
      mapViewer = new GeoAreaViewer(details, SWT.BORDER, this);
      mapViewer.enableMapControls(false);

      splitter.setWeights(new int[] { 60, 40 });

      createActions();
      createContextMenu();

      session.addListener(this);

      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
   @Override
   public void setFocus()
   {
      viewer.getControl().setFocus();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(this);
      super.dispose();
   }

   /**
    * @see org.netxms.client.SessionListener#notificationHandler(org.netxms.client.SessionNotification)
    */
   @Override
   public void notificationHandler(final SessionNotification n)
   {
      if (n.getCode() == SessionNotification.GEO_AREA_UPDATED)
      {
         getDisplay().asyncExec(new Runnable() {
            @Override
            public void run()
            {
               GeoArea area = (GeoArea)n.getObject();
               areas.put(area.getId(), area);
               viewer.refresh(true);
            }
         });
      }
      else if (n.getCode() == SessionNotification.GEO_AREA_DELETED)
      {
         getDisplay().asyncExec(new Runnable() {
            @Override
            public void run()
            {
               areas.remove((int)n.getSubCode());
               viewer.refresh();
            }
         });
      }
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionNew = new Action("New...", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createArea();
         }
      };
      addKeyBinding("M1+N", actionNew);

      actionEdit = new Action("&Edit...", SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editArea();
         }
      };

      actionDelete = new Action("&Delete", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteAreas();
         }
      };
      addKeyBinding("M1+D", actionDelete);

      actionImport = new Action("&Import...", SharedIcons.IMPORT) {
         @Override
         public void run()
         {
            importAreas();
         }
      };
      addKeyBinding("M1+I", actionImport);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionNew);
      manager.add(actionImport);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionNew);
      manager.add(actionImport);
   }

   /**
    * Create context menu for area list
    */
   private void createContextMenu()
   {
      // Create menu manager
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      // Create menu
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(final IMenuManager mgr)
   {
      mgr.add(actionNew);
      mgr.add(actionImport);
      mgr.add(actionEdit);
      mgr.add(actionDelete);
   }

   /**
    * Create new area
    */
   private void createArea()
   {
      GeoAreaEditDialog dlg = new GeoAreaEditDialog(getWindow().getShell(), null);
      if (dlg.open() == Window.OK)
      {
         updateArea(dlg.getArea());
      }
   }

   /**
    * Edit selected category
    */
   private void editArea()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      GeoAreaEditDialog dlg = new GeoAreaEditDialog(getWindow().getShell(), (GeoArea)selection.getFirstElement());
      if (dlg.open() == Window.OK)
      {
         updateArea(dlg.getArea());
      }
   }

   /**
    * Update geo area on server
    *
    * @param area geo area object
    */
   private void updateArea(final GeoArea area)
   {
      new Job(i18n.tr("Updating geographical area"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final int id = session.modifyGeoArea(area);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  GeoArea newArea = areas.get(id);
                  if (newArea == null)
                  {
                     newArea = new GeoArea(area);
                     areas.put(id, newArea);
                  }
                  viewer.refresh();
                  viewer.setSelection(new StructuredSelection(newArea));
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update geographical area");
         }
      }.start();
   }

   /**
    * Delete selected areas
    */
   private void deleteAreas()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete Geographical Areas"), i18n.tr("Selected geographical areas will be deleted. Are you sure?")))
         return;

      final int[] idList = new int[selection.size()];
      int index = 0;
      for(Object o : selection.toList())
         idList[index++] = ((GeoArea)o).getId();

      new Job(i18n.tr("Delete geographical areas"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            monitor.beginTask(i18n.tr("Delete geographical areas"), idList.length);
            for(final int id : idList)
            {
               try
               {
                  session.deleteGeoArea(id, false);
               }
               catch(NXCException e)
               {
                  if (e.getErrorCode() != RCC.GEO_AREA_IN_USE)
                     throw e;

                  final boolean[] retry = new boolean[1];
                  getDisplay().syncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        GeoArea a = areas.get(id);
                        retry[0] = MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Confirm Delete"),
                              String.format(i18n.tr("Geographical area \"%s\" is in use. Are you sure you want to delete it?"),
                                    (a != null) ? a.getName() : "[" + id + "]"));
                     }
                  });
                  if (retry[0])
                  {
                     session.deleteGeoArea(id, true);
                  }
               }
               monitor.worked(1);
            }
            monitor.done();
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete geographical area");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Loading configured geographical areas"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<GeoArea> serverAreaList = session.getGeoAreas();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  areas.clear();
                  for(GeoArea a : serverAreaList)
                     areas.put(a.getId(), a);
                  viewer.setInput(areas.values());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load configured geographical areas");
         }
      }.start();
   }

   /**
    * Update details view for selected area
    */
   private void updateAreaDetails(GeoArea area)
   {
      if (area != null)
         coordinateListViewer.setInput(area.getBorder());
      else
         coordinateListViewer.setInput(new Object[0]);
      mapViewer.setArea(area);
   }

   /**
    * Import areas from KML file
    */
   private void importAreas()
   {
      FileDialog dlg = new FileDialog(getWindow().getShell(), SWT.OPEN);
      WidgetHelper.setFileDialogFilterExtensions(dlg, new String[] { "*.kml", "*.*" });
      WidgetHelper.setFileDialogFilterNames(dlg, new String[] { "KML files", "All files" });
      String fileName = dlg.open();
      if (fileName == null)
         return;

      final File file = new File(fileName);
      new Job(i18n.tr("Import areas from file"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final Map<String, List<GeoLocation>> importedAreas = KMLParser.importPolygons(file);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  validateImportedAreas(importedAreas);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot read import file");
         }
      }.start();
   }

   /**
    * Validate imported areas and create on server on success
    *
    * @param areas imported areas
    */
   private void validateImportedAreas(final Map<String, List<GeoLocation>> importedAreas)
   {
      final Set<String> skipSet = new HashSet<String>();
      final List<GeoArea> replaceList = new ArrayList<GeoArea>();
      for(String n : importedAreas.keySet())
      {
         for(GeoArea a : areas.values())
         {
            if (a.getName().equalsIgnoreCase(n))
            {
               if (MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Override Geographical Area"),
                     String.format(i18n.tr("Geographical area with name \"%s\" already exist. Do you want to override it's content?"), a.getName())))
               {
                  replaceList.add(new GeoArea(a.getId(), a.getName(), a.getComments(), importedAreas.get(n)));
               }
               skipSet.add(n);
            }
         }
      }

      for(String n : skipSet)
         importedAreas.remove(n);

      new Job(i18n.tr("Importing geographical area"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(GeoArea a : replaceList)
               session.modifyGeoArea(a);
            
            for(Entry<String, List<GeoLocation>> e : importedAreas.entrySet())
            {
               GeoArea a = new GeoArea(0, e.getKey(), "Imported from KML", e.getValue());
               session.modifyGeoArea(a);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update geographical area on server");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#isModified()
    */
   @Override
   public boolean isModified()
   {
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
   {
   }
}

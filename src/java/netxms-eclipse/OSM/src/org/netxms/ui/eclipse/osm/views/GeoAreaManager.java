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
package org.netxms.ui.eclipse.osm.views;

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
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.dialogs.IDialogSettings;
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
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.base.GeoLocation;
import org.netxms.base.KMLParser;
import org.netxms.client.GeoArea;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.RCC;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.osm.Activator;
import org.netxms.ui.eclipse.osm.dialogs.GeoAreaEditDialog;
import org.netxms.ui.eclipse.osm.views.helpers.GeoAreaComparator;
import org.netxms.ui.eclipse.osm.views.helpers.GeoAreaFilter;
import org.netxms.ui.eclipse.osm.views.helpers.GeoAreaLabelProvider;
import org.netxms.ui.eclipse.osm.widgets.GeoAreaViewer;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Object category manager
 *
 */
public class GeoAreaManager extends ViewPart implements SessionListener
{
   public static final String ID = "org.netxms.ui.eclipse.osm.views.GeoAreaManager";

   public static final int COL_ID = 0;
   public static final int COL_NAME = 1;
   public static final int COL_COMMENTS = 2;

   private static final String TABLE_CONFIG_PREFIX = "GeoAreaManager";

   private Map<Integer, GeoArea> areas = new HashMap<Integer, GeoArea>();
   private Display display;
   private SashForm splitter;
   private SortableTableViewer viewer;
   private NXCSession session;
   private Composite content;
   private FilterText filterText;
   private GeoAreaFilter filter;
   private Composite details;
   private TableViewer coordinateListViewer;
   private GeoAreaViewer mapViewer;
   private boolean initShowFilter = true;
   private Action actionRefresh;
   private Action actionNew;
   private Action actionEdit;
   private Action actionDelete;
   private Action actionImport;
   private Action actionShowFilter;

   /**
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      initShowFilter = getBooleanSetting("ObjectTools.showFilter", initShowFilter);
      session = ConsoleSharedData.getSession();
   }

   /**
    * Get boolean value from settings.
    * 
    * @param name parameter name
    * @param defval default value
    * @return value from settings or default
    */
   private static boolean getBooleanSetting(String name, boolean defval)
   {
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      if (settings.get(name) == null)
         return defval;
      return settings.getBoolean(name);
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      display = parent.getDisplay();

      splitter = new SashForm(parent, SWT.HORIZONTAL);

      // Create content area
      content = new Composite(splitter, SWT.NONE);
      FormLayout formLayout = new FormLayout();
      content.setLayout(formLayout);

      // Create filter
      filterText = new FilterText(content, SWT.NONE);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });
      filterText.setCloseAction(new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
            actionShowFilter.setChecked(initShowFilter);
         }
      });

      final String[] names = { "ID", "Name", "Comments" };
      final int[] widths = { 100, 400, 800 };
      viewer = new SortableTableViewer(content, names, widths, 1, SWT.DOWN, SWT.MULTI | SWT.FULL_SELECTION);
      WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new GeoAreaLabelProvider());
      viewer.setComparator(new GeoAreaComparator());
      filter = new GeoAreaFilter();
      viewer.addFilter(filter);

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
            actionEdit.run();
         }
      });
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
         }
      });

      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterText);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      viewer.getControl().setLayoutData(fd);

      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);

      details = new Composite(splitter, SWT.NONE);
      details.setLayout(new FillLayout(SWT.VERTICAL));

      coordinateListViewer = new TableViewer(details);
      coordinateListViewer.setContentProvider(new ArrayContentProvider());
      
      mapViewer = new GeoAreaViewer(details, SWT.BORDER);
      mapViewer.enableMapControls(false);

      splitter.setWeights(new int[] { 60, 40 });

      getSite().setSelectionProvider(viewer);

      createActions();
      contributeToActionBars();
      createPopupMenu();
      activateContext();

      session.addListener(this);

      // Set initial focus to filter input line
      if (initShowFilter)
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly

      refresh();
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      viewer.getControl().setFocus();
   }

   /**
    * @see org.netxms.client.SessionListener#notificationHandler(org.netxms.client.SessionNotification)
    */
   @Override
   public void notificationHandler(final SessionNotification n)
   {
      if (n.getCode() == SessionNotification.GEO_AREA_UPDATED)
      {
         display.asyncExec(new Runnable() {
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
         display.asyncExec(new Runnable() {
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
    * Activate context
    */
   private void activateContext()
   {
      IContextService contextService = (IContextService)getSite().getService(IContextService.class);
      if (contextService != null)
      {
         contextService.activateContext("org.netxms.ui.eclipse.osm.context.GeoAreaManager"); //$NON-NLS-1$
      }
   }

   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   public void enableFilter(boolean enable)
   {
      initShowFilter = enable;
      filterText.setVisible(initShowFilter);
      FormData fd = (FormData)viewer.getTable().getLayoutData();
      fd.top = enable ? new FormAttachment(filterText, 0, SWT.BOTTOM) : new FormAttachment(0, 0);
      content.layout();
      if (enable)
      {
         filterText.setFocus();
      }
      else
      {
         filterText.setText("");
         onFilterModify();
      }
   }

   /**
    * Handler for filter modification
    */
   public void onFilterModify()
   {
      final String text = filterText.getText();
      filter.setFilterString(text);
      viewer.refresh(false);
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);

      actionShowFilter = new Action("Show &filter", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            enableFilter(!initShowFilter);
            actionShowFilter.setChecked(initShowFilter);
         }
      };
      actionShowFilter.setChecked(initShowFilter);
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.osm.commands.showFilter"); //$NON-NLS-1$
      handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), new ActionHandler(actionShowFilter));

      actionRefresh = new RefreshAction() {
         @Override
         public void run()
         {
            refresh();
         }
      };

      actionNew = new Action("New...", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createArea();
         }
      };

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

      actionImport = new Action("&Import...", Activator.getImageDescriptor("icons/import.gif")) {
         @Override
         public void run()
         {
            importAreas();
         }
      };
   }

   /**
    * Contribute actions to action bar
    */
   private void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * Fill local pull-down menu
    * 
    * @param manager Menu manager for pull-down menu
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionNew);
      manager.add(actionImport);
      manager.add(actionEdit);
      manager.add(actionDelete);
      manager.add(new Separator());
      manager.add(actionShowFilter);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager Menu manager for local toolbar
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionNew);
      manager.add(actionImport);
      manager.add(actionEdit);
      manager.add(actionDelete);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * Create pop-up menu for user list
    */
   private void createPopupMenu()
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

      // Register menu for extension.
      getSite().registerContextMenu(menuMgr, viewer);
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
      GeoAreaEditDialog dlg = new GeoAreaEditDialog(getSite().getShell(), null);
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

      GeoAreaEditDialog dlg = new GeoAreaEditDialog(getSite().getShell(), (GeoArea)selection.getFirstElement());
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
      new ConsoleJob("Modify geographical area", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
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
            return "Cannot modify object category";
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

      if (!MessageDialogHelper.openQuestion(getSite().getShell(), "Delete Geographical Areas",
            "Selected geographical areas will be deleted. Are you sure?"))
         return;

      final int[] idList = new int[selection.size()];
      int index = 0;
      for(Object o : selection.toList())
         idList[index++] = ((GeoArea)o).getId();

      new ConsoleJob("Delete geographical areas", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            monitor.beginTask("Delete geographical areas", idList.length);
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
                        retry[0] = MessageDialogHelper.openQuestion(getSite().getShell(), "Confirm Delete",
                              String.format("Geographical area \"%s\" is in use. Are you sure you want to delete it?",
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
            return "Cannot delete geographical area";
         }
      }.start();
   }

   /**
    * Refresh view
    */
   private void refresh()
   {
      new ConsoleJob("Read configured geographical areas", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
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
            return "Cannot get configured geographical areas";
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
      FileDialog dlg = new FileDialog(getSite().getShell(), SWT.OPEN);
      dlg.setFilterExtensions(new String[] { "*.kml", "*.*" });
      dlg.setFilterNames(new String[] { "KML files", "All files" });
      String fileName = dlg.open();
      if (fileName == null)
         return;
      
      final File file = new File(fileName);
      new ConsoleJob("Import areas from file", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
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
            return "Cannot read import file";
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
               if (MessageDialogHelper.openQuestion(getSite().getShell(), "Override Geographical Area",
                     String.format("Geographical area with name \"%s\" already exist. Do you want to override it's content?", a.getName())))
               {
                  replaceList.add(new GeoArea(a.getId(), a.getName(), a.getComments(), importedAreas.get(n)));
               }
               skipSet.add(n);
            }
         }
      }

      for(String n : skipSet)
         importedAreas.remove(n);

      new ConsoleJob("Import geographical area", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
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
            return "Cannot update geographical area on server";
         }
      }.start();
   }
}

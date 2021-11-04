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
package org.netxms.nxmc.modules.datacollection.views;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
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
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.LocalChangeListener;
import org.netxms.client.datacollection.RemoteChangeListener;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objects.Template;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.windows.PopOutViewWindow;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.DataCollectionObjectEditor;
import org.netxms.nxmc.modules.datacollection.dialogs.BulkUpdateDialog;
import org.netxms.nxmc.modules.datacollection.dialogs.helpers.BulkDciUpdateElementUI;
import org.netxms.nxmc.modules.datacollection.propertypages.AccessControl;
import org.netxms.nxmc.modules.datacollection.propertypages.ClusterOptions;
import org.netxms.nxmc.modules.datacollection.propertypages.Comments;
import org.netxms.nxmc.modules.datacollection.propertypages.CustomSchedule;
import org.netxms.nxmc.modules.datacollection.propertypages.GeneralItem;
import org.netxms.nxmc.modules.datacollection.propertypages.GeneralTable;
import org.netxms.nxmc.modules.datacollection.propertypages.InstanceDiscovery;
import org.netxms.nxmc.modules.datacollection.propertypages.OtherOptions;
import org.netxms.nxmc.modules.datacollection.propertypages.OtherOptionsTable;
import org.netxms.nxmc.modules.datacollection.propertypages.PerfTab;
import org.netxms.nxmc.modules.datacollection.propertypages.TableColumns;
import org.netxms.nxmc.modules.datacollection.propertypages.Thresholds;
import org.netxms.nxmc.modules.datacollection.propertypages.Transformation;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DciComparator;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DciFilter;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DciLabelProvider;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.LastValuesComparator;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.LastValuesFilter;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.LastValuesLabelProvider;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.DialogData;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.netxms.nxmc.tools.VisibilityValidator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Data Collection" view
 */
public class DataCollectionView extends ObjectView
{
   public static final String JOB_FAMILY = "DataCollectionEditorJob"; //$NON-NLS-1$
   private static final I18n i18n = LocalizationHelper.getI18n(DataCollectionView.class);

   // Columns for "last values" mode
   public static final int LV_COLUMN_ID = 0;
   public static final int LV_COLUMN_DESCRIPTION = 1;
   public static final int LV_COLUMN_VALUE = 2;
   public static final int LV_COLUMN_TIMESTAMP = 3;
   public static final int LV_COLUMN_THRESHOLD = 4;
   
   // Columns for "data collection configuration" mode
   public static final int DC_COLUMN_ID = 0;
   public static final int DC_COLUMN_ORIGIN = 1;
   public static final int DC_COLUMN_DESCRIPTION = 2;
   public static final int DC_COLUMN_PARAMETER = 3;
   public static final int DC_COLUMN_DATATYPE = 4;
   public static final int DC_COLUMN_INTERVAL = 5;
   public static final int DC_COLUMN_RETENTION = 6;
   public static final int DC_COLUMN_STATUS = 7;
   public static final int DC_COLUMN_THRESHOLD = 8;
   public static final int DC_COLUMN_TEMPLATE = 9;
   public static final int DC_COLUMN_RELATEDOBJ = 10;
   public static final int DC_COLUMN_STATUSCALC = 11;

   private boolean editMode;
   private Composite parent;
   private NXCSession session;
   private AbstractObject object;
   private SortableTableViewer viewer;
   private SessionListener clientListener = null;
   private DataCollectionConfiguration dciConfig = null;

   private LastValuesLabelProvider labelProvider;
   private LastValuesComparator comparator;
   private LastValuesFilter lvFilter;
   private boolean autoRefreshEnabled = false;
   private int autoRefreshInterval = 30;  // in seconds
   private ViewRefreshController refreshController;

   private DciFilter dcFilter;
   private boolean hideModificationWarnings;
   private RemoteChangeListener changeListener;

   private Action actionToggleEditMode;
   private Action actionCreateItem;
   private Action actionCreateTable;
   private Action actionEdit;
   private Action actionDelete;
   private Action actionCopy;
   private Action actionMove;
   private Action actionConvert;
   private Action actionDuplicate;
   private Action actionActivate;
   private Action actionDisable;
   private Action actionBulkUpdate;
   private Action actionLineChart;
   private Action actionRawLineChart;
   private Action actionUseMultipliers;
   private Action actionShowErrors;
   private Action actionShowDisabled;
   private Action actionShowUnsupported;
   private Action actionShowHidden;
   private Action actionExportToCsv;
   private Action actionExportAllToCsv;
   private Action actionCopyToClipboard;
   private Action actionCopyDciName;
   private Action actionShowHistoryData;

   /**
    * @param name
    * @param image
    */
   public DataCollectionView()
   {
      super(i18n.tr("Data Collection"), ResourceManager.getImageDescriptor("icons/object-views/last_values.png"), "DataCollection", true); 
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      this.parent = parent;
      session = Registry.getSession();

      VisibilityValidator validator = new VisibilityValidator() { 
         @Override
         public boolean isVisible()
         {
            return DataCollectionView.this.isActive();
         }
      };

      if (editMode)
         createDataCollectionViewer(parent);  
      else 
         createLastValuesViewer(parent, validator);

      createActions();
   }

   /**
    * Create 
    */
   private void createDataCollectionViewer(Composite parent)
   {
      String configPrefix = "DataCollectionConfiguration";
      object = ((getObject() instanceof DataCollectionTarget) || (getObject() instanceof Template)) ? getObject() : null;

      final PreferenceStore ds = PreferenceStore.getInstance();

      parent.setLayout(new FillLayout());
      final String[] names = { i18n.tr("ID"), i18n.tr("Origin"), i18n.tr("Description"), i18n.tr("Parameter"), i18n.tr("Data Type"), i18n.tr("Polling Interval"), i18n.tr("Retention Time"), i18n.tr("Status"), i18n.tr("Thresholds"), i18n.tr("Template"), i18n.tr("Related Object"), i18n.tr("Is status calculation") };
      final int[] widths = { 60, 100, 250, 200, 90, 90, 90, 100, 200, 150, 150, 90 };
      viewer = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new DciLabelProvider());
      viewer.setComparator(new DciComparator((DciLabelProvider)viewer.getLabelProvider()));
      dcFilter = new DciFilter();
      setFilterClient(viewer, dcFilter); 
      viewer.addFilter(dcFilter);
      WidgetHelper.restoreTableViewerSettings(viewer, configPrefix); //$NON-NLS-1$

      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)event.getSelection();
            if (selection != null)
            {
               actionEdit.setEnabled(selection.size() == 1);
               actionDelete.setEnabled(selection.size() > 0);
               actionCopy.setEnabled(selection.size() > 0);
               actionMove.setEnabled(selection.size() > 0);
               actionConvert.setEnabled(selection.size() > 0);
               actionDuplicate.setEnabled(selection.size() > 0);
               actionBulkUpdate.setEnabled(selection.size() > 0);
               
               Iterator<?> it = selection.iterator();
               boolean canActivate = false;
               boolean canDisable = false;
               while(it.hasNext() && (!canActivate || !canDisable))
               {
                  DataCollectionObject dci = (DataCollectionObject)it.next();
                  if (dci.getStatus() != DataCollectionObject.ACTIVE)
                     canActivate = true;
                  if (dci.getStatus() != DataCollectionObject.DISABLED)
                     canDisable = true;
               }
               actionActivate.setEnabled(canActivate);
               actionDisable.setEnabled(canDisable);
            }
         }
      });
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editSelectedObject();
         }
      });
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, configPrefix); //$NON-NLS-1$
            ds.set(configPrefix + ".hideModificationWarnings", hideModificationWarnings);
         }
      });
      
      createPopupMenu();

      hideModificationWarnings = ds.getAsBoolean(configPrefix + ".hideModificationWarnings", false); 
      
      final Display display = viewer.getControl().getDisplay();
      changeListener = new RemoteChangeListener() {
         @Override
         public void onUpdate(DataCollectionObject object)
         {
            display.asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(dciConfig.getItems());
               }
            }); 
         }

         @Override
         public void onDelete(long id)
         {
            display.asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(dciConfig.getItems());
               }
            }); 
         }

         @Override
         public void onStatusChange(long id, int status)
         {
            display.asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  viewer.refresh();
               }
            }); 
         }
      };   
   }

   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
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
   protected void fillContextMenu(final IMenuManager manager)
   {
      boolean isTemplate = object instanceof Template;

      if (!editMode)
      {
         if (!isTemplate)
         {
            manager.add(actionLineChart);
            manager.add(actionRawLineChart);
            manager.add(actionShowHistoryData); 
            manager.add(new Separator());
         }
         manager.add(actionCopyToClipboard);
         manager.add(actionCopyDciName);
         manager.add(actionExportToCsv);
         manager.add(actionExportAllToCsv);
         manager.add(new Separator());
         manager.add(actionUseMultipliers);
         manager.add(actionShowErrors);
         manager.add(actionShowDisabled);
         manager.add(actionShowUnsupported);
         manager.add(actionShowHidden);
         manager.add(new Separator());
      }
      manager.add(actionCreateItem);
      manager.add(actionCreateTable);
      manager.add(actionEdit);
      manager.add(actionBulkUpdate);
      manager.add(actionDelete);
      manager.add(actionCopy);
      manager.add(actionMove);
      if (!isTemplate)
         manager.add(actionConvert);
      manager.add(actionDuplicate);
      manager.add(new Separator());
      manager.add(actionActivate);
      manager.add(actionDisable);
      if (editMode)
      {
         if (!isTemplate)
         {
            manager.add(new Separator());
            manager.add(actionLineChart);
            manager.add(actionRawLineChart);
            manager.add(actionShowHistoryData);  
         }
         manager.add(new Separator());
         manager.add(actionExportToCsv);
         manager.add(actionExportAllToCsv);
      }
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionCreateItem = new Action(i18n.tr("&New parameter..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createItem();
         }
      };

      actionCreateTable = new Action(i18n.tr("Ne&w table...")) {
         @Override
         public void run()
         {
            createTable();
         }
      };

      actionEdit = new Action(i18n.tr("&Edit..."), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editSelectedObject();
         }
      };
      actionEdit.setEnabled(false);
      
      actionBulkUpdate = new Action("Bulk update...") {
         @Override
         public void run()
         {
            openBulkUpdateDialog();
         }
      };

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteItems();
         }
      };
      actionDelete.setEnabled(false);

      actionCopy = new Action(i18n.tr("&Copy to other node(s)...")) {
         @Override
         public void run()
         {
            copyItems(false);
         }
      };
      actionCopy.setEnabled(false);

      actionMove = new Action(i18n.tr("&Move to other node(s)...")) {
         @Override
         public void run()
         {
            copyItems(true);
         }
      };
      actionMove.setEnabled(false);

      actionConvert = new Action(i18n.tr("Convert to &template item...")) {
         @Override
         public void run()
         {
            convertToTemplate();
         }
      };
      actionConvert.setEnabled(false);

      actionDuplicate = new Action(i18n.tr("D&uplicate")) {
         @Override
         public void run()
         {
            duplicateItems();
         }
      };
      actionDuplicate.setEnabled(false);

      actionActivate = new Action(i18n.tr("&Activate"), ResourceManager.getImageDescriptor("icons/dci/active.gif")) { //$NON-NLS-1$
         @Override
         public void run()
         {
            setItemStatus(DataCollectionObject.ACTIVE);
            actionActivate.setEnabled(false);
            actionDisable.setEnabled(true);
         }
      };
      actionActivate.setEnabled(false);

      actionDisable = new Action(i18n.tr("D&isable"), ResourceManager.getImageDescriptor("icons/dci/disabled.gif")) { //$NON-NLS-1$
         @Override
         public void run()
         {
            setItemStatus(DataCollectionObject.DISABLED);
            actionActivate.setEnabled(true);
            actionDisable.setEnabled(false);
         }
      };
      actionDisable.setEnabled(false);

      actionExportToCsv = new ExportToCsvAction(this, viewer, true); 
      actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);
      
      /*last values */
      
      actionUseMultipliers = new Action(i18n.tr("Use &multipliers"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            setUseMultipliers(actionUseMultipliers.isChecked());
         }
      };
      actionUseMultipliers.setChecked(areMultipliersUsed());

      actionShowErrors = new Action(i18n.tr("Show collection &errors"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            setShowErrors(actionShowErrors.isChecked());
         }
      };
      actionShowErrors.setChecked(isShowErrors());

      actionShowUnsupported = new Action(i18n.tr("Show &unsupported"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            setShowUnsupported(actionShowUnsupported.isChecked());
         }
      };
      actionShowUnsupported.setChecked(isShowUnsupported());
      
      actionShowHidden = new Action(i18n.tr("Show &hidden"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            setShowHidden(actionShowHidden.isChecked());
         }
      };
      actionShowHidden.setChecked(isShowHidden());
      
      actionShowDisabled = new Action(i18n.tr("Show disabled"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            setShowDisabled(actionShowDisabled.isChecked());
         }
      };
      actionShowDisabled.setChecked(isShowDisabled());      
      
      actionCopyToClipboard = new Action(i18n.tr("&Copy to clipboard"), SharedIcons.COPY) {
         @Override
         public void run()
         {
            copySelection();
         }
      };
      
      actionCopyDciName = new Action(i18n.tr("Copy DCI name"), SharedIcons.COPY) {
         @Override
         public void run()
         {
            copySelectionDciName();
         }
      };

      actionLineChart = new Action(i18n.tr("&Line chart"), ResourceManager.getImageDescriptor("icons/chart_line.png")) {
         @Override
         public void run()
         {
            showLineChart(false);
         }
      };

      actionRawLineChart = new Action(i18n.tr("&Raw Data Line chart"), ResourceManager.getImageDescriptor("icons/chart_line.png")) {
         @Override
         public void run()
         {
            showLineChart(true);
         }
      };
      
      actionShowHistoryData = new Action(i18n.tr("History data"), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            showHistoryData();
         }
      };

      actionToggleEditMode = new Action(i18n.tr("&Edit mode"), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editMode = actionToggleEditMode.isChecked();
            switchMode();
         }
      }; 
      actionToggleEditMode.setChecked(editMode);
      addKeyBinding("Ctrl+E", actionToggleEditMode);
   }

   /**
    * Refresh DCI list
    */
   @Override
   public void refresh()
   {
      if (editMode)
      {
         new Job(String.format(i18n.tr("Reftesh data collection configuration for %s"), object.getObjectName()), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               dciConfig.refreshDataCollectionList();
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     viewer.setInput(dciConfig.getItems());
                  }
               });
            }
   
            @Override
            protected String getErrorMessage()
            {
               return String.format(i18n.tr("Cannot refresh data collection configuration for %s"), object.getObjectName());
            }
         }.start();  
      }
      else
      {
         getDataFromServer();
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      if (dciConfig != null)
      {
         new Job(String.format(i18n.tr("Unlock data collection configuration for %s"), object.getObjectName()), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               dciConfig.close();
               dciConfig = null;
            }

            @Override
            protected String getErrorMessage()
            {
               return String.format(i18n.tr("Cannot unlock data collection configuration for %s"), object.getObjectName());
            }
         }.start();
      }
      super.dispose();
   }
   
   /**
    * Get object id
    */
   private long getObjectId(Object dci)
   {
      return editMode ? ((DataCollectionObject)dci).getId() : ((DciValue)dci).getId();
   }

   private DataCollectionObject getDataCollectionObject(Object dci)
   {
      DataCollectionObject dco;
      if (editMode)
      {
         dco = (DataCollectionObject)dci;
      }
      else
      {
         DciValue value = (DciValue)dci;
         dco = dciConfig.findItem(value.getId());
      }
      return dco;
   }

   /**
    * Change status for selected items
    * 
    * @param newStatus New status
    */
   private void setItemStatus(final int newStatus)
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;
      
      new Job(String.format(i18n.tr("Change status of data collection items for %s"), object.getObjectName()), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final long[] itemList = new long[selection.size()];
            int pos = 0;
            for(Object dci : selection.toList())
            {
               itemList[pos++] = getObjectId(dci);
            }
            dciConfig.setObjectStatus(itemList, newStatus);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  for(Object dci : selection.toList())
                  {
                     DataCollectionObject dco = getDataCollectionObject(dci);
                     dco.setStatus(newStatus);
                     new DataCollectionObjectEditor(dco).modify();
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot change status of data collection items for %s"), object.getObjectName());
         }
      }.start();
   }

   /**
    * Delete currently selected DCIs
    */
   private void deleteItems()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;
      
      if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Delete Data Collection Items"), i18n.tr("Do you really want to delete selected data collection items?")))
         return;

      new Job(i18n.tr(String.format("Delete data collection items for %s"), object.getObjectName()), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object dci : selection.toList())
            {
               dciConfig.deleteObject(getObjectId(dci));
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot delete data collection items for %s"), object.getObjectName());
         }
      }.start();
   }
   
   /**
    * Create new data collection item
    */
   private void createItem()
   {
      DataCollectionItem dci = new DataCollectionItem(dciConfig, 0);
      if ((object instanceof AbstractNode) && !((AbstractNode)object).hasAgent())
      {
         if (((AbstractNode)object).hasSnmpAgent())
         {
            dci.setOrigin(DataOrigin.SNMP);
         }
         else
         {
            dci.setOrigin(DataOrigin.INTERNAL);
         }
      }
      showDCIPropertyPages(dci);
   }
   
   /**
    * Create new data collection table
    */
   private void createTable()
   {     
      DataCollectionTable dci = new DataCollectionTable(dciConfig, 0);
      if((object instanceof AbstractNode) && !((AbstractNode)object).hasAgent())
      {
         if(((AbstractNode)object).hasSnmpAgent())
         {
            dci.setOrigin(DataOrigin.SNMP);
         }
         else
         {
            dci.setOrigin(DataOrigin.INTERNAL);
         }
      }
      showDCIPropertyPages(dci);
   }

   /**
    * Edit selected object
    */
   private void editSelectedObject()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      DataCollectionObject dco = getDataCollectionObject(selection.getFirstElement());

      DialogData data = null;
      if (!hideModificationWarnings && dco.getTemplateId() != 0)
      {
         String message = DataCollectionObjectEditor.createModificationWarningMessage(dco);
         if (message != null)
         {
            data = MessageDialogHelper.openWarningWithCheckbox(getWindow().getShell(), i18n.tr("Warning"), i18n.tr("Don't show this message again"), message);
            hideModificationWarnings = data.getSaveSelection();
         }
      }
      
      if ((data == null) || data.isOkPressed())
      {
         showDCIPropertyPages(dco);
      }
   }

   /**
    * Duplicate selected item(s)
    */
   private void duplicateItems()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      Iterator<?> it = selection.iterator();
      final long[] dciList = new long[selection.size()];
      for(int i = 0; (i < dciList.length) && it.hasNext(); i++)
         dciList[i] = getObjectId(it.next());
      
      new Job(String.format(i18n.tr("Duplicate data collection items for %s"), object.getObjectName()), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            dciConfig.copyObjects(dciConfig.getOwnerId(), dciList);
            dciConfig.close();
            dciConfig.open(changeListener);
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot duplicate data collection item for %s"), object.getObjectName());
         }
      }.start();
   }
   
   /**
    * Copy items to another node
    */
   private void copyItems(final boolean doMove)
   {
      final ObjectSelectionDialog dlg = new ObjectSelectionDialog(getWindow().getShell(),
            ObjectSelectionDialog.createDataCollectionOwnerSelectionFilter());
      if (dlg.open() != Window.OK)
         return;

      final Set<AbstractObject> targets = new HashSet<AbstractObject>();
      for(AbstractObject o : dlg.getSelectedObjects())
         if ((o instanceof DataCollectionTarget) || (o instanceof Template))
            targets.add(o);
      if (targets.isEmpty())
      {
         MessageDialogHelper.openWarning(getWindow().getShell(), i18n.tr("Warning"), i18n.tr("Target selection is invalid or empty!"));
         return;
      }

      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      Iterator<?> it = selection.iterator();
      final long[] dciList = new long[selection.size()];
      for(int i = 0; (i < dciList.length) && it.hasNext(); i++)
         dciList[i] = getObjectId(it.next());

      new Job(String.format(i18n.tr("Copy data collection items from %s"), object.getObjectName()), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(AbstractObject o : targets)
               dciConfig.copyObjects(o.getObjectId(), dciList);
            if (doMove)
            {
               for(long id : dciList)
                  dciConfig.deleteObject(id);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot copy data collection item from %s"), object.getObjectName());
         }
      }.start();
   }
   
   /**
    * Open bulk update dialog
    */
   private void openBulkUpdateDialog()
   {      
      IStructuredSelection selection = viewer.getStructuredSelection();
      final Set<Long> dciList = new HashSet<Long>(selection.size());

      boolean isCustomRetention = true;
      boolean isCustomInterval = true;      
      Iterator<?> it = selection.iterator();
      while(it.hasNext())
      {
         DataCollectionObject dco = getDataCollectionObject(it.next());
         dciList.add(dco.getId());
         if (dco.getRetentionType() != DataCollectionObject.RETENTION_CUSTOM)
            isCustomRetention = false;
         if (dco.getPollingScheduleType() != DataCollectionObject.POLLING_SCHEDULE_CUSTOM)
            isCustomInterval = false;
      }      

      BulkUpdateDialog dlg = new BulkUpdateDialog(getWindow().getShell(), isCustomRetention, isCustomInterval);
      if (dlg.open() != Window.OK)
         return;      

      final List<BulkDciUpdateElementUI> elements = dlg.getBulkUpdateElements();
      
      boolean changed = false;
      for (BulkDciUpdateElementUI e : elements)
      {
         if (e.isModified())
            changed = true;
      }

      if (!changed)
         return;

      new Job(i18n.tr("Executing bulk DCI update"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            dciConfig.bulkUpdateDCIs(dciList, elements);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Failed to execute bulk DCI update");
         }
      }.start();
   }

   /**
    * Convert selected item(s) to template items
    */
   private void convertToTemplate()
   {
      final ObjectSelectionDialog dlg = new ObjectSelectionDialog(getWindow().getShell(), ObjectSelectionDialog.createTemplateSelectionFilter());
      dlg.showFilterToolTip(false);
      if (dlg.open() != Window.OK)
         return;

      AbstractObject[] objects = dlg.getSelectedObjects(Template.class);
      if (objects.length == 0)
         return;
      final Template template = (Template)objects[0];

      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      Iterator<?> it = selection.iterator();
      final long[] dciList = new long[selection.size()];
      for(int i = 0; (i < dciList.length) && it.hasNext(); i++)
         dciList[i] = getObjectId(it.next());

      new Job(i18n.tr("Convert data collection items for ") + object.getObjectName() + i18n.tr(" to template items"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            monitor.beginTask(i18n.tr("Convert DCIs to template DCIs"), 4);
            
            boolean needApply = true;
            for(long id : template.getChildIdList())
            {
               if (id == dciConfig.getOwnerId())
               {
                  needApply = false;
                  break;
               }
            }
            monitor.worked(1);
            
            dciConfig.copyObjects(template.getObjectId(), dciList);
            for(long id : dciList)
               dciConfig.deleteObject(id);
            dciConfig.close();
            monitor.worked(1);
                  
            if (needApply)
            {
               boolean success = false;
               int retries = 5;
               do
               {
                  try
                  {
                     session.applyTemplate(template.getObjectId(), dciConfig.getOwnerId());
                     success = true;
                  }
                  catch(NXCException e)
                  {
                     if (e.getErrorCode() != RCC.COMPONENT_LOCKED)
                        throw e;
                     Thread.sleep(200);
                  }
                  retries--;
               } while(!success && (retries > 0));
            }
            monitor.worked(1);
            
            boolean success = false;
            int retries = 5;
            do
            {
               try
               {
                  Thread.sleep(500);
                  dciConfig.open(changeListener);
                  success = true;
               }
               catch(NXCException e)
               {
                  if (e.getErrorCode() != RCC.COMPONENT_LOCKED)
                     throw e;
               }
               retries--;
            } while(!success && (retries > 0));
           
            monitor.done();
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot convert data collection item for ") + object.getObjectName() + i18n.tr(" to template item");
         }
      }.start();
   }

   /**
    * Display message with information about policy deploy
    */
   public void showInformationMessage()
   {
      if (!viewer.getTable().isDisposed())
      {
         addMessage(MessageArea.INFORMATION,
               "Changes in policies will be deployed to nodes the moment when the tab is closed", true);
      }
   }

   /**
    * Show Graph configuration dialog
    * 
    * @param trap Object tool details object
    * @return true if OK was pressed
    */
   private boolean showDCIPropertyPages(final DataCollectionObject object)
   {
      DataCollectionObjectEditor dce = new DataCollectionObjectEditor(object);
      PreferenceManager pm = new PreferenceManager();  
      if (dce.getObject() instanceof DataCollectionItem)
         pm.addToRoot(new PreferenceNode("general", new GeneralItem(dce)));             
      else
         pm.addToRoot(new PreferenceNode("general", new GeneralTable(dce)));   
      pm.addToRoot(new PreferenceNode("clisterOptions", new ClusterOptions(dce)));    
      pm.addToRoot(new PreferenceNode("customSchedulte", new CustomSchedule(dce)));    
      if (dce.getObject() instanceof DataCollectionTable)
         pm.addToRoot(new PreferenceNode("columns", new TableColumns(dce)));    
      pm.addToRoot(new PreferenceNode("transformation", new Transformation(dce)));             
      pm.addToRoot(new PreferenceNode("thresholds", new Thresholds(dce)));        
      pm.addToRoot(new PreferenceNode("instanceDiscovery", new InstanceDiscovery(dce)));
      if (dce.getObject() instanceof DataCollectionItem)
         pm.addToRoot(new PreferenceNode("performanceTab", new PerfTab(dce)));
      pm.addToRoot(new PreferenceNode("accessControl", new AccessControl(dce)));
      if (dce.getObject() instanceof DataCollectionItem)
         pm.addToRoot(new PreferenceNode("otherOptions", new OtherOptions(dce)));             
      else
         pm.addToRoot(new PreferenceNode("otherOptions", new OtherOptionsTable(dce)));   
      pm.addToRoot(new PreferenceNode("comments", new Comments(dce)));  

      PreferenceDialog dlg = new PreferenceDialog(getWindow().getShell(), pm) {
         @Override
         protected void configureShell(Shell newShell)
         {
            super.configureShell(newShell);
            newShell.setText(String.format(i18n.tr("Properties for %s"), object.getName()));
         }
      };
      dlg.setBlockOnOpen(true);
      return dlg.open() == Window.OK;
   }
   
   /**
    * Create last values view
    */
   private void createLastValuesViewer(Composite parent, VisibilityValidator validator)
   {
      String configPrefix = "LastValues";

      final PreferenceStore ds = PreferenceStore.getInstance();
      
      parent.setLayout(new FillLayout());
      
      // Setup table columns
      final String[] names = { i18n.tr("ID"), i18n.tr("Description"), i18n.tr("Value"), i18n.tr("Timestamp"), i18n.tr("Threshold") };
      final int[] widths = { 70, 250, 150, 120, 150 };
      viewer = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SortableTableViewer.DEFAULT_STYLE);
   
      labelProvider = new LastValuesLabelProvider();
      comparator = new LastValuesComparator();
      lvFilter = new LastValuesFilter();
      viewer.setLabelProvider(labelProvider);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setComparator(comparator);
      viewer.addFilter(lvFilter);
      setFilterClient(viewer, lvFilter); 
      WidgetHelper.restoreTableViewerSettings(viewer, configPrefix);

      refreshController = new ViewRefreshController(this, -1, new Runnable() {
         @Override
         public void run()
         {
            if (viewer.getTable().isDisposed())
               return;
            
            getDataFromServer();
         }
      }, validator);
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            refreshController.dispose();
         }
      });
      
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            showLineChart(false);
         }
      });

      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, configPrefix);
            ds.set(configPrefix + ".autoRefresh", autoRefreshEnabled);
            ds.set(configPrefix + ".autoRefreshInterval", autoRefreshEnabled);
            ds.set(configPrefix + ".useMultipliers", labelProvider.areMultipliersUsed());
            ds.set(configPrefix + ".showErrors", isShowErrors());
            ds.set(configPrefix + ".showDisabled", isShowDisabled());
            ds.set(configPrefix + ".showUnsupported", isShowUnsupported());
            ds.set(configPrefix + ".showHidden", isShowHidden());
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
               actionCopy.setEnabled(selection.size() > 0);
               actionMove.setEnabled(selection.size() > 0);
               actionConvert.setEnabled(selection.size() > 0);
               actionDuplicate.setEnabled(selection.size() > 0);
               actionBulkUpdate.setEnabled(selection.size() > 0);
               
               Iterator<?> it = selection.iterator();
               boolean canActivate = false;
               boolean canDisable = false;
               while(it.hasNext() && (!canActivate || !canDisable))
               {
                  DciValue dci = (DciValue)it.next();
                  if (dci.getStatus() != DataCollectionObject.ACTIVE)
                     canActivate = true;
                  if (dci.getStatus() != DataCollectionObject.DISABLED)
                     canDisable = true;
               }
               actionActivate.setEnabled(canActivate);
               actionDisable.setEnabled(canDisable);
            }
         }
      });
      
      autoRefreshInterval = ds.getAsInteger(configPrefix + ".autoRefreshInterval", autoRefreshInterval);
      setAutoRefreshEnabled(ds.getAsBoolean(configPrefix + ".autoRefresh", true));
      labelProvider.setUseMultipliers(ds.getAsBoolean(configPrefix + ".useMultipliers", true));

      boolean showErrors = ds.getAsBoolean(configPrefix + ".showErrors", true);
      labelProvider.setShowErrors(showErrors);
      comparator.setShowErrors(showErrors);

      lvFilter.setShowDisabled(ds.getAsBoolean(configPrefix + ".showDisabled", false));
      lvFilter.setShowUnsupported(ds.getAsBoolean(configPrefix + ".showUnsupported", false));
      lvFilter.setShowHidden(ds.getAsBoolean(configPrefix + ".showHidden", false));
      
      createPopupMenu();

      if ((validator == null) || validator.isVisible())
         getDataFromServer();

      final Display display = viewer.getControl().getDisplay();
      clientListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (((n.getCode() == SessionNotification.FORCE_DCI_POLL) ||
                  (n.getCode() == SessionNotification.DCI_UPDATE) ||
                  (n.getCode() == SessionNotification.DCI_DELETE) ||
                  (n.getCode() == SessionNotification.DCI_STATE_CHANGE) ) &&
                (DataCollectionView.this.object != null) &&
                (n.getSubCode() == DataCollectionView.this.object.getObjectId()))
            {
               display.asyncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     refresh();
                  }
               }); 
            }
         }        
      };
      
      session.addListener(clientListener);
   }
   
   /**
    * Switch between "data collection configuration" and "last values" modes
    */
   private void switchMode()
   {
      if (viewer != null)
      {
         viewer.getControl().dispose();
         session.removeListener(clientListener);
         dciConfig.setRemoteChangeListener(null);
         dciConfig.setUserData(null);
      }

      VisibilityValidator validator = new VisibilityValidator() { 
         @Override
         public boolean isVisible()
         {
            return DataCollectionView.this.isActive();
         }
      };
      if (editMode)
      {
         createDataCollectionViewer(parent);  
         if (object != null)
         {
            dciConfig.setUserData(viewer);
            dciConfig.setRemoteChangeListener(changeListener);  
         }
      }
      else 
      {
         createLastValuesViewer(parent, validator);
      }
      refresh();
      
      viewer.getTable().layout();  
      parent.layout();       
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      if (context == null)
         return false;
      
      if (context instanceof DataCollectionTarget)
         return true;
      
      if (context instanceof Template)
      {
         editMode = true;
         return true;
      }
      
      return false;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      this.object = ((object != null) && ((object instanceof DataCollectionTarget) || (object instanceof Template))) ? object : null;
      if (this.object == null)
      {
         actionToggleEditMode.setEnabled(false);
         viewer.setInput(new Object[0]);
         return;
      }

      actionToggleEditMode.setEnabled(!(object instanceof Template));

      // Request server to open data collection configuration
      new Job(String.format(i18n.tr("Open data collection configuration for %s"), object.getObjectName()), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            if (dciConfig != null)
               dciConfig.close();

            dciConfig = session.openDataCollectionConfiguration(object.getObjectId(), changeListener);
            if (object instanceof Template)
            {
               dciConfig.setLocalChangeListener(new LocalChangeListener() {                  
                  @Override
                  public void onObjectChange()
                  {         
                     runInUIThread(new Runnable() {
                        @Override
                        public void run()
                        {
                           DataCollectionView.this.showInformationMessage();
                        }
                     });
                  }
               });
            }

            //load all related objects
            if (!session.areObjectsSynchronized())
            {
               List<Long> relatedOpbjects = new ArrayList<Long>();
               for(DataCollectionObject dco : dciConfig.getItems())
               {
                  if(dco.getRelatedObject() != 0)
                     relatedOpbjects.add(dco.getRelatedObject());
               }
               if (relatedOpbjects.size() > 0) 
               {
                  session.syncMissingObjects(relatedOpbjects, true, NXCSession.OBJECT_SYNC_WAIT);
               }
            }

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (editMode)
                  {
                     dciConfig.setUserData(viewer);
                     dciConfig.setRemoteChangeListener(changeListener);  
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot open data collection configuration for "), object.getObjectName());
         }
      }.start();  
      
      if (isActive())
         refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolbar(org.eclipse.jface.action.ToolBarManager)
    */
   @Override
   protected void fillLocalToolbar(ToolBarManager manager)
   {
      super.fillLocalToolbar(manager);
      manager.add(actionToggleEditMode);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 30;
   }

   /**
    * Get data from server
    */
   private void getDataFromServer()
   {
      if (object == null)
      {
         viewer.setInput(new DciValue[0]);
         return;
      }

      final DataCollectionTarget jobTarget = (DataCollectionTarget)object;
      Job job = new Job(String.format(i18n.tr("Get DCI values for node %s"), jobTarget.getObjectName()), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final DciValue[] data = session.getLastValues(jobTarget.getObjectId());
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (!viewer.getTable().isDisposed() && (object != null) && (object.getObjectId() == jobTarget.getObjectId()))
                  {
                     viewer.setInput(data);
                     clearMessages();
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot get DCI values for node %s"), jobTarget.getObjectName());
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * @return the autoRefreshEnabled
    */
   public boolean isAutoRefreshEnabled()
   {
      return autoRefreshEnabled;
   }

   /**
    * @param autoRefreshEnabled the autoRefreshEnabled to set
    */
   public void setAutoRefreshEnabled(boolean autoRefreshEnabled)
   {
      this.autoRefreshEnabled = autoRefreshEnabled;
      refreshController.setInterval(autoRefreshEnabled ? autoRefreshInterval : -1);
   }

   /**
    * @return the autoRefreshInterval
    */
   public int getAutoRefreshInterval()
   {
      return autoRefreshInterval;
   }

   /**
    * @param autoRefreshInterval the autoRefreshInterval to set
    */
   public void setAutoRefreshInterval(int autoRefreshInterval)
   {
      this.autoRefreshInterval = autoRefreshInterval;
   }
   
   /**
    * @return the useMultipliers
    */
   public boolean areMultipliersUsed()
   {
      return (labelProvider != null) ? labelProvider.areMultipliersUsed() : false;
   }

   /**
    * @param useMultipliers the useMultipliers to set
    */
   public void setUseMultipliers(boolean useMultipliers)
   {
      if (labelProvider != null)
      {
         labelProvider.setUseMultipliers(useMultipliers);
         if (viewer != null)
         {
            viewer.refresh(true);
         }
      }
   }
   
   /**
    * @return
    */
   public boolean isShowErrors()
   {
      return (labelProvider != null) ? labelProvider.isShowErrors() : false;
   }

   /**
    * @param show
    */
   public void setShowErrors(boolean show)
   {
      labelProvider.setShowErrors(show);
      comparator.setShowErrors(show);
      if (viewer != null)
      {
         viewer.refresh(true);
      }
   }
   
   /**
    * @return
    */
   public boolean isShowDisabled()
   {
      return (lvFilter != null) ? lvFilter.isShowDisabled() : false;
   }

   /**
    * @param show
    */
   public void setShowDisabled(boolean show)
   {
      lvFilter.setShowDisabled(show);
      if (viewer != null)
      {
         viewer.refresh(true);
      }
   }

   /**
    * @return
    */
   public boolean isShowUnsupported()
   {
      return (lvFilter != null) ? lvFilter.isShowUnsupported() : false;
   }

   /**
    * @param show
    */
   public void setShowUnsupported(boolean show)
   {
      lvFilter.setShowUnsupported(show);
      if (viewer != null)
      {
         viewer.refresh(true);
      }
   }

   /**
    * @return
    */
   public boolean isShowHidden()
   {
      return (lvFilter != null) ? lvFilter.isShowHidden() : false;
   }

   /**
    * @param show
    */
   public void setShowHidden(boolean show)
   {
      lvFilter.setShowHidden(show);
      if (viewer != null)
      {
         viewer.refresh(true);
      }
   }

   /**
    * @return the actionUseMultipliers
    */
   public Action getActionUseMultipliers()
   {
      return actionUseMultipliers;
   }

   /**
    * @return the actionUseMultipliers
    */
   public Action getActionShowDisabled()
   {
      return actionShowDisabled;
   }

   /**
    * @return the actionUseMultipliers
    */
   public Action getActionShowErrors()
   {
      return actionShowErrors;
   }

   /**
    * @return the actionUseMultipliers
    */
   public Action getActionShowUnsupported()
   {
      return actionShowUnsupported;
   }

   /**
    * @return the actionUseMultipliers
    */
   public Action getActionShowHidden()
   {
      return actionShowHidden;
   }

   /**
    * Show line chart for selected items
    */
   private void showLineChart(boolean useRawValues)
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      List<ChartDciConfig> items = new ArrayList<ChartDciConfig>(selection.size());
      for(Object o : selection.toList())
      {
         ChartDciConfig config = editMode ? new ChartDciConfig((DataCollectionObject)o) : new ChartDciConfig((DciValue)o);
         config.useRawValues = useRawValues;
         items.add(config);
      }

      AbstractObject object = getObject();
      Perspective p = getPerspective();
      if (p != null)
      {
         p.addMainView(new HistoricalGraphView(object, items), true, false);
      }
      else
      {
         PopOutViewWindow window = new PopOutViewWindow(new HistoricalGraphView(object, items));
         window.open();
      }
   }

   /**
    * Show line chart for selected items
    */
   private void showHistoryData()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      AbstractObject object = getObject();
      Perspective p = getPerspective();      
      for(Object o : selection.toList())
      {
         if ((o instanceof DataCollectionTable) || ((o instanceof DciValue) &&
               ((DciValue)o).getDcObjectType() == DataCollectionObject.DCO_TYPE_TABLE))
         {
            if (p != null)
            {
               p.addMainView(new TableLastValuesView(object, getObjectId(o)), true, false);
            }
            else
            {
               PopOutViewWindow window = new PopOutViewWindow(new TableLastValuesView(object, getObjectId(o)));
               window.open();
            }
         }
         else 
         {
            if (p != null)
            {
               p.addMainView(new HistoricalDataView(object, getObjectId(o), null, null, null), true, false);
            }
            else
            {
               PopOutViewWindow window = new PopOutViewWindow(new HistoricalDataView(object, getObjectId(o), null, null, null));
               window.open();
            }
         }
      }
   }

   /**
    * Copy selection to clipboard
    */
   private void copySelection()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      final String nl = WidgetHelper.getNewLineCharacters();
      StringBuilder sb = new StringBuilder();
      for(Object o : selection.toList())
      {
         if (sb.length() > 0)
            sb.append(nl);
         DciValue v = (DciValue)o;
         sb.append(v.getDescription());
         sb.append(" = "); //$NON-NLS-1$
         sb.append(v.getValue());
      }
      if (selection.size() > 1)
         sb.append(nl);
      WidgetHelper.copyToClipboard(sb.toString());
   }
   
   /**
    * Copy DCI name of selection
    */
   private void copySelectionDciName()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;
      
      StringBuilder dciName = new StringBuilder();
      for(Object o : selection.toList())
      {
         if (dciName.length() > 0)
            dciName.append(' ');
         DciValue v = (DciValue)o;
         dciName.append(v.getName());
      }
      WidgetHelper.copyToClipboard(dciName.toString());
   }

   /**
    * @see org.netxms.nxmc.base.views.View#activate()
    */
   @Override
   public void activate()
   {
      refresh();
      super.activate();
   }
}

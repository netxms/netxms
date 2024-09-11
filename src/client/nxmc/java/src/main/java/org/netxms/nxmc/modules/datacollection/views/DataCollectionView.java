/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.LocalChangeListener;
import org.netxms.client.datacollection.RemoteChangeListener;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objects.Template;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.windows.MainWindow;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.DataCollectionObjectEditor;
import org.netxms.nxmc.modules.datacollection.ShowHistoricalDataMenuItems;
import org.netxms.nxmc.modules.datacollection.dialogs.BulkUpdateDialog;
import org.netxms.nxmc.modules.datacollection.dialogs.helpers.BulkDciUpdateElementUI;
import org.netxms.nxmc.modules.datacollection.views.helpers.DciComparator;
import org.netxms.nxmc.modules.datacollection.views.helpers.DciFilter;
import org.netxms.nxmc.modules.datacollection.views.helpers.DciLabelProvider;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.DialogData;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.VisibilityValidator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * "Data Collection" view
 */
public class DataCollectionView extends BaseDataCollectionView
{
   private static final Logger logger = LoggerFactory.getLogger(DataCollectionView.class);

   private final I18n i18n = LocalizationHelper.getI18n(DataCollectionView.class);

   // Columns for "data collection configuration" mode
   public static final int DC_COLUMN_ID = 0;
   public static final int DC_COLUMN_DESCRIPTION = 1;
   public static final int DC_COLUMN_ORIGIN = 2;
   public static final int DC_COLUMN_PARAMETER = 3;
   public static final int DC_COLUMN_DATAUNIT = 4;
   public static final int DC_COLUMN_DATATYPE = 5;
   public static final int DC_COLUMN_INTERVAL = 6;
   public static final int DC_COLUMN_RETENTION = 7;
   public static final int DC_COLUMN_STATUS = 8;
   public static final int DC_COLUMN_THRESHOLD = 9;
   public static final int DC_COLUMN_TEMPLATE = 10;
   public static final int DC_COLUMN_RELATEDOBJ = 11;
   public static final int DC_COLUMN_STATUSCALC = 12;

   private boolean editMode = false;
   private Composite parent;
   private SessionListener clientListener = null;
   private DataCollectionConfiguration dciConfig = null;
   private int messageId = 0;

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
   private Action actionHideTemplateItems;
   private Action actionApplyChanges;
   private Action actionShowTemplate;

   /**
    * Constructor
    */
   public DataCollectionView()
   {
      super("objects.data-collection", true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      this.parent = parent;

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

      final PreferenceStore ds = PreferenceStore.getInstance();

      parent.setLayout(new FillLayout());
      final String[] names = { i18n.tr("ID"), i18n.tr("Display name"), i18n.tr("Origin"), i18n.tr("Metric"), i18n.tr("Units"),i18n.tr("Data Type"), i18n.tr("Polling Interval"), i18n.tr("Retention Time"), i18n.tr("Status"), i18n.tr("Thresholds"), i18n.tr("Template"), i18n.tr("Related Object"), i18n.tr("Is status calculation") };
      final int[] widths = { 60, 250, 150, 200, 90, 90, 90, 90, 100, 200, 150, 150, 90 };
      viewer = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new DciLabelProvider());
      viewer.setComparator(new DciComparator((DciLabelProvider)viewer.getLabelProvider()));
      dcFilter = new DciFilter();
      setFilterClient(viewer, dcFilter); 
      dcFilter.setHideTemplateItems(ds.getAsBoolean(configPrefix + ".hideTemplateItems", false));
      viewer.addFilter(dcFilter);
      WidgetHelper.restoreTableViewerSettings(viewer, configPrefix);

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
               boolean hasTemplate = false;
               while(it.hasNext() && (!canActivate || !canDisable))
               {
                  DataCollectionObject dci = (DataCollectionObject)it.next();
                  if (dci.getStatus() != DataCollectionObject.ACTIVE)
                     canActivate = true;
                  if (dci.getStatus() != DataCollectionObject.DISABLED)
                     canDisable = true;
                  if (dci.getTemplateId() != 0 && dci.getTemplateId() != getObjectId())
                     hasTemplate = true;    
               }
               actionActivate.setEnabled(canActivate);
               actionDisable.setEnabled(canDisable);
               actionShowTemplate.setEnabled((selection.size() == 1) && hasTemplate);
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
            ds.set(configPrefix + ".hideTemplateItems", actionHideTemplateItems.isChecked());
         }
      });

      // Update viewer in export to CSV actions
      actionExportToCsv.setViewer(viewer);
      actionExportAllToCsv.setViewer(viewer);

      createContextMenu();

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
                  updateItems();
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
                  updateItems();
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
   
   void updateItems()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      viewer.setInput(dciConfig.getItems());
      List<DataCollectionObject> selected = new ArrayList<DataCollectionObject>(selection.size());
      Iterator<?> it = selection.iterator();
      while(it.hasNext())
      {
         DataCollectionObject obj = (DataCollectionObject)it.next();
         for (DataCollectionObject item : (DataCollectionObject[])viewer.getInput())
         {
            if (obj.getId() == item.getId())
            {
               selected.add(item);
               break;
            }
         }
      }   
      viewer.setSelection(new StructuredSelection(selected.toArray()));     
   }
   
   /**
    * Actions to make after last values view was created
    * 
    * @param configPrefix
    * @param validator
    */
   @Override
   protected void postLastValueViewCreation(String configPrefix, VisibilityValidator validator)
   {
      super.postLastValueViewCreation(configPrefix, validator);

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
               boolean hasTemplate = false;
               while(it.hasNext() && (!canActivate || !canDisable))
               {
                  DciValue dci = (DciValue)it.next();
                  if (dci.getStatus() != DataCollectionObject.ACTIVE)
                     canActivate = true;
                  if (dci.getStatus() != DataCollectionObject.DISABLED)
                     canDisable = true;
                  if (dci.getTemplateDciId() != 0)
                  {
                     DataCollectionObject dco = getDataCollectionObject(dci);
                     if (dco != null && dco.getTemplateId() != 0 && dco.getTemplateId() != getObjectId())
                        hasTemplate = true;                            
                  }         
               }
               actionActivate.setEnabled(canActivate);
               actionDisable.setEnabled(canDisable);
               actionShowTemplate.setEnabled((selection.size() == 1) && hasTemplate);
            }
         }
      });

      final Display display = viewer.getControl().getDisplay();
      clientListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            AbstractObject object = getObject();
            if (((n.getCode() == SessionNotification.FORCE_DCI_POLL) ||
                 (n.getCode() == SessionNotification.DCI_UPDATE) ||
                 (n.getCode() == SessionNotification.DCI_DELETE) ||
                 (n.getCode() == SessionNotification.DCI_STATE_CHANGE)) &&
                (object != null) &&
                (n.getSubCode() == object.getObjectId()))
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
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   @Override
   protected void fillContextMenu(final IMenuManager manager)
   {
      boolean isTemplate = getObject() instanceof Template;
      int selectionType = getDciSelectionType();

      if (!editMode)
      {
         if (!isTemplate)
         {
            ShowHistoricalDataMenuItems.populateMenu(manager, this, getObject(), viewer, selectionType);
         }
         manager.add(actionCopyToClipboard);
         manager.add(actionCopyDciName);
         manager.add(actionExportToCsv);
         manager.add(actionExportAllToCsv);
         manager.add(new Separator());
         manager.add(actionForcePoll);
         manager.add(actionRecalculateData);
         manager.add(actionClearData);
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
      if (actionActivate.isEnabled())
         manager.add(actionActivate);
      if (actionDisable.isEnabled())
         manager.add(actionDisable);
      if (!isTemplate)
      {
         manager.add(new Separator());
         manager.add(actionShowTemplate);
         manager.add(new Separator());
      }
      if (editMode)
      {
         if (!isTemplate)
         {
            ShowHistoricalDataMenuItems.populateMenu(manager, this, getObject(), viewer, selectionType);
         }
         manager.add(new Separator());
         manager.add(actionExportToCsv);
         manager.add(actionExportAllToCsv);
         manager.add(new Separator());
         manager.add(actionForcePoll);
         manager.add(actionRecalculateData);
         manager.add(actionClearData);
         manager.add(new Separator());
         manager.add(actionHideTemplateItems);
      }
   }

   /**
    * Create actions
    */
   @Override
   protected void createActions()
   {
      super.createActions();      

      actionEdit = new Action(i18n.tr("&Edit..."), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editSelectedObject();
         }
      };
      actionEdit.setEnabled(false);
      addKeyBinding("M1+E", actionEdit);

      actionBulkUpdate = new Action("&Bulk update...") {
         @Override
         public void run()
         {
            openBulkUpdateDialog();
         }
      };
      addKeyBinding("M1+B", actionBulkUpdate);

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteItems();
         }
      };
      actionDelete.setEnabled(false);
      addKeyBinding("M1+D", actionDelete);

      actionCopy = new Action(i18n.tr("&Copy to other object(s)...")) {
         @Override
         public void run()
         {
            copyItems(false);
         }
      };
      actionCopy.setEnabled(false);

      actionMove = new Action(i18n.tr("&Move to other object(s)...")) {
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

      actionActivate = new Action(i18n.tr("&Activate"), ResourceManager.getImageDescriptor("icons/dci/active.gif")) {
         @Override
         public void run()
         {
            setItemStatus(DataCollectionObject.ACTIVE);
            actionActivate.setEnabled(false);
            actionDisable.setEnabled(true);
         }
      };
      actionActivate.setEnabled(false);

      actionDisable = new Action(i18n.tr("D&isable"), ResourceManager.getImageDescriptor("icons/dci/disabled.gif")) {
         @Override
         public void run()
         {
            setItemStatus(DataCollectionObject.DISABLED);
            actionActivate.setEnabled(true);
            actionDisable.setEnabled(false);
         }
      };
      actionDisable.setEnabled(false);
      
      actionCreateItem = new Action(i18n.tr("&New parameter..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createItem();
         }
      };
      addKeyBinding("M1+N", actionCreateItem);

      actionCreateTable = new Action(i18n.tr("Ne&w table...")) {
         @Override
         public void run()
         {
            createTable();
         }
      };
      addKeyBinding("M1+M2+N", actionCreateTable);

      actionToggleEditMode = new Action(i18n.tr("&Edit mode"), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editMode = actionToggleEditMode.isChecked();
            switchMode();
            refresh();
         }
      }; 
      actionToggleEditMode.setChecked(editMode);
      addKeyBinding("M1+M", actionToggleEditMode);

      actionHideTemplateItems = new Action(i18n.tr("Hide &template items"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            setHideTemplateItems(actionHideTemplateItems.isChecked());
         }
      };
      actionHideTemplateItems.setImageDescriptor(ResourceManager.getImageDescriptor("icons/ignore-template-objects.png"));
      actionHideTemplateItems.setChecked(PreferenceStore.getInstance().getAsBoolean("DataCollectionConfiguration.hideTemplateItems", false));
      addKeyBinding("M1+M2+T", actionHideTemplateItems);

      actionApplyChanges = new Action("Appl&y changes", ResourceManager.getImageDescriptor("icons/commit.png")) {
         @Override
         public void run()
         {
            commitDciChanges(false);
         }
      };
      actionApplyChanges.setEnabled(false);      
      addKeyBinding("M1+S", actionApplyChanges);

      actionShowTemplate = new Action(i18n.tr("Go to &Template DCI")) {
         @Override
         public void run()
         {
            showTemplate();
         }
      };
      actionDisable.setEnabled(false);
   }

   /**
    * Refresh DCI list
    */
   @Override
   public void refresh()
   {
      refresh(null);
   }

   /**
    * Refresh DCI list
    */
   public void refresh(Runnable callback)
   {
      if (editMode)
      {
         if (dciConfig != null)
         {
            new Job(i18n.tr("Reloading data collection configuration for {0}", getObjectName()), this) {
               @Override
               protected void run(IProgressMonitor monitor) throws Exception
               {
                  dciConfig.refreshDataCollectionList();
                  runInUIThread(new Runnable() {
                     @Override
                     public void run()
                     {
                        final IStructuredSelection selection = viewer.getStructuredSelection();
                        viewer.setInput(dciConfig.getItems());
                        if (callback != null)
                           callback.run();
                        else 
                        {
                           List<DataCollectionObject> selected = new ArrayList<DataCollectionObject>(selection.size());
                           Iterator<?> it = selection.iterator();
                           while(it.hasNext())
                           {
                              DataCollectionObject obj = (DataCollectionObject)it.next();
                              for (DataCollectionObject item : (DataCollectionObject[])viewer.getInput())
                              {
                                 if (obj.getId() == item.getId())
                                 {
                                    selected.add(item);
                                    break;
                                 }
                              }
                           }   
                           viewer.setSelection(new StructuredSelection(selected.toArray()));                           
                        }
                     }
                  });
               }

               @Override
               protected String getErrorMessage()
               {
                  return i18n.tr("Cannot refresh data collection configuration for {0}", getObjectName());
               }
            }.start();
         }
      }
      else
      {
         getDataFromServer(callback);
      }
   }

   /**
    * Get DCI id
    */
   @Override
   protected long getDciId(Object dci)
   {
      return editMode ? ((DataCollectionObject)dci).getId() : ((DciValue)dci).getId();
   }

   /**
    * Get object id
    */
   @Override
   protected long getObjectId(Object dci)
   {
      return editMode ? ((DataCollectionObject)dci).getNodeId() : ((DciValue)dci).getNodeId();
   }

   /**
    * Get data collection object from given object
    * 
    * @param dci dci object
    * @return DCO
    */
   protected DataCollectionObject getDataCollectionObject(Object dci)
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
    * Open template and it's DCI
    */
   private void showTemplate()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      DataCollectionObject dco = getDataCollectionObject(selection.getFirstElement());
      
      MainWindow.switchToObject(dco.getTemplateId(), dco.getTemplateItemId());
   }
   
   /**
    * Create new data collection item
    */
   private void createItem()
   {
      DataCollectionItem dci = new DataCollectionItem(dciConfig, 0);
      AbstractObject object = getObject();
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
      AbstractObject object = getObject();
      if ((object instanceof AbstractNode) && !((AbstractNode)object).hasAgent())
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
    * Change status for selected items
    * 
    * @param newStatus New status
    */
   private void setItemStatus(final int newStatus)
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;
      
      new Job(i18n.tr("Changing status of data collection items for {0}", getObjectName()), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final long[] itemList = new long[selection.size()];
            int pos = 0;
            for(Object dci : selection.toList())
            {
               itemList[pos++] = getDciId(dci);
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
            return i18n.tr("Cannot change status of data collection items for {0}", getObjectName());
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

      new Job(i18n.tr("Deleting data collection items for {0}", getObjectName()), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object dci : selection.toList())
            {
               dciConfig.deleteObject(getDciId(dci));
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete data collection items for {0}", getObjectName());
         }
      }.start();
   }

   /**
    * Edit selected object
    */
   protected void editSelectedObject()
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
      IStructuredSelection selection = viewer.getStructuredSelection();
      Iterator<?> it = selection.iterator();
      final long[] dciList = new long[selection.size()];
      for(int i = 0; (i < dciList.length) && it.hasNext(); i++)
         dciList[i] = getDciId(it.next());
      
      new Job(i18n.tr("Duplicating data collection items for {0}", getObjectName()), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            dciConfig.copyObjects(dciConfig.getOwnerId(), dciList);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot duplicate data collection items for {0}", getObjectName());
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

      IStructuredSelection selection = viewer.getStructuredSelection();
      Iterator<?> it = selection.iterator();
      final long[] dciList = new long[selection.size()];
      for(int i = 0; (i < dciList.length) && it.hasNext(); i++)
         dciList[i] = getDciId(it.next());

      new Job(i18n.tr("Copying data collection items from {0}", getObjectName()), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            if ((targets.size() == 1) && doMove)
            {
               dciConfig.moveObjects(targets.iterator().next().getObjectId(), dciList);
            }
            else
            {
               for(AbstractObject o : targets)
               {
                  dciConfig.copyObjects(o.getObjectId(), dciList);
               }
               if (doMove)
               {
                  for(long id : dciList)
                     dciConfig.deleteObject(id);
               }
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot copy data collection items from {0}", getObjectName());
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
         {
            changed = true;
            break;
         }
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

      IStructuredSelection selection = viewer.getStructuredSelection();
      Iterator<?> it = selection.iterator();
      final long[] dciList = new long[selection.size()];
      for(int i = 0; (i < dciList.length) && it.hasNext(); i++)
         dciList[i] = getDciId(it.next());

      new Job(i18n.tr("Converting data collection items for {0} to template items", getObjectName()), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            monitor.beginTask(i18n.tr("Convert DCIs to template DCIs"), 3);

            boolean needApply = true;
            for(long id : template.getChildIdList())
            {
               if (id == dciConfig.getOwnerId())
               {
                  needApply = false;
                  break;
               }

               // Check if this template applied on parent cluster
               Cluster cluster = session.findObjectById(id, Cluster.class);
               if ((cluster != null) && cluster.isDirectParentOf(dciConfig.getOwnerId()))
               {
                  needApply = false;
                  break;
               }
            }
            monitor.worked(1);

            dciConfig.copyObjects(template.getObjectId(), dciList);
            for(long id : dciList)
               dciConfig.deleteObject(id);
            monitor.worked(1);

            if (needApply)
            {
               session.applyTemplate(template.getObjectId(), dciConfig.getOwnerId());
            }
           
            monitor.done();
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot convert data collection item for {0} to template item", getObjectName());
         }
      }.start();
   }

   /**
    * Display message with information about policy deploy
    */
   public void showInformationMessage()
   {
      actionApplyChanges.setEnabled(true);
      if (!viewer.getTable().isDisposed() && messageId < 1)
      {
         messageId = addMessage(MessageArea.INFORMATION, i18n.tr("Changes in data collection configuration will be deployed to nodes the moment when the tab is closed"), true);
      }
   }

   /**
    * Switch between "data collection configuration" and "last values" modes
    */
   private void switchMode()
   {
      int savedSortColumn = -1;
      int savedSortDirection = SWT.NONE;

      if (viewer != null)
      {
         TableColumn sortColumn = viewer.getTable().getSortColumn();
         savedSortColumn = (sortColumn != null) ? (Integer)sortColumn.getData("ID") : -1;
         if (savedSortColumn >= 0)
         {
            if (editMode) // Switching from "last values" mode, column with ID 0 is "owner"
               savedSortColumn--;
            else // Switching from "edit" mode, column with ID 0 is "ID", adjust for column 0 being "owner" in "last values" mode
               savedSortColumn++;
         }
         savedSortDirection = viewer.getTable().getSortDirection();

         viewer.getControl().dispose();
         session.removeListener(clientListener);
         if (dciConfig != null)
         {
            dciConfig.setRemoteChangeListener(null);
            dciConfig.setUserData(null);
         }
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
         if (dciConfig != null)
         {
            dciConfig.setUserData(viewer);
            dciConfig.setRemoteChangeListener(changeListener);  
         }
      }
      else 
      {
         createLastValuesViewer(parent, validator);
      }

      if ((savedSortColumn != -1) && (savedSortColumn < (editMode ? 2 : 3)) && (savedSortDirection != SWT.NONE))
      {
         viewer.getTable().setSortColumn(viewer.getColumnById(savedSortColumn));
         viewer.getTable().setSortDirection(savedSortDirection);
      }
      else
      {
         viewer.getTable().setSortColumn(viewer.getColumnById(1));
         viewer.getTable().setSortDirection(SWT.DOWN);
      }

      parent.layout(true, true);

      updateToolBar();
      updateMenu();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && ((context instanceof DataCollectionTarget) || (context instanceof Template));
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      if (!editMode && (object instanceof Template))
      {
         editMode = true;
         switchMode();
      }

      // Request server to open data collection configuration
      new Job(i18n.tr("Opening data collection configuration for {0}", object.getObjectName()), this) {
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

            // load all related objects
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
                  session.syncMissingObjects(relatedOpbjects, NXCSession.OBJECT_SYNC_WAIT);
               }
            }

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (editMode)
                  {
                     dciConfig.setUserData(DataCollectionView.this);
                     dciConfig.setRemoteChangeListener(changeListener);  
                  }

                  if (isActive())
                     refresh();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot open data collection configuration for {0}", object.getObjectName());
         }
      }.start(); 
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.views.BaseDataCollectionView#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionCreateItem);
      if ((getObject() instanceof Template) || (getObject() instanceof Cluster))
      {
         manager.add(actionApplyChanges);
      }
      super.fillLocalToolBar(manager);
      if (editMode)
      {
         manager.add(actionHideTemplateItems);
      }
      if (!(getObject() instanceof Template))
      {
         manager.add(actionToggleEditMode);
      }
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.views.BaseDataCollectionView#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionCreateItem);
      manager.add(new Separator());
      if ((getObject() instanceof Template) || (getObject() instanceof Cluster))
      {
         manager.add(actionApplyChanges);
         manager.add(new Separator());
      }
      super.fillLocalMenu(manager);
      if (editMode)
      {
         manager.add(actionHideTemplateItems);
      }
      if (!(getObject() instanceof Template))
      {
         manager.add(new Separator());
         manager.add(actionToggleEditMode);
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#deactivate()
    */
   @Override
   public void deactivate()
   {
      commitDciChanges(true);
      super.deactivate();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      if (dciConfig != null)
      {
         new Job(i18n.tr("Closing data collection configuration for {0}", getObjectName()), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               dciConfig.close();
               dciConfig = null;
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot close data collection configuration for {0}", getObjectName());
            }
         }.start();
      } 
      super.dispose();
   }

   /**
    * Commit DCI changes
    * 
    * @param ignoreErrors if true, ignore possible commit errors
    */
   private void commitDciChanges(boolean ignoreErrors)
   { 
      if (dciConfig == null)
         return;

      new Job(i18n.tr("Applying data collection configuration for {0}", getObjectName()), this, this, getDisplay()) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            try
            {
               dciConfig.commit();
            }
            catch(Exception e)
            {
               if (ignoreErrors)
                  logger.debug("Ignored exception while applying data collection configuration for object {}", getObjectName());
               else
                  throw e;
            }
            runInUIThread(() -> {
               actionApplyChanges.setEnabled(false);
               if (messageId > 0)
               {
                  deleteMessage(messageId);
                  messageId = 0;
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot apply data collection configuration for {0}", getObjectName());
         }
      }.start();
   }

   /**
    * Set new view input
    * 
    * @param items
    */
   public void setInput(DataCollectionObject[] items)
   {
      viewer.setInput(items);
   }

   /**
    * Update modified object
    *  
    * @param object
    */
   public void update(DataCollectionObject object)
   {
      viewer.update(object, null);
   }

   /**
    * Set visibility mode for template items
    * 
    * @param hide true to hide template items
    */
   private void setHideTemplateItems(boolean hide)
   {
      dcFilter.setHideTemplateItems(hide);
      viewer.refresh(false);
   }

   /**
    * Select DCI 
    * 
    * @param dciId DCI id
    */
   public void selectDci(long dciId)
   {
      refresh( new Runnable() {         
         @Override
         public void run()
         {
            if (editMode)
            {
               for (DataCollectionObject item : (DataCollectionObject[])viewer.getInput())
               {
                  if (item.getId() == dciId)
                     viewer.setSelection(new StructuredSelection(item), true); 
               }
            }
            else 
            {
               for (DciValue item : (DciValue[])viewer.getInput())
               {
                  if (item.getId() == dciId)
                     viewer.setSelection(new StructuredSelection(item), true); 
               }
               
            }
         }
      });
   }
}

package org.netxms.nxmc.modules.datacollection.widgets;

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
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredViewer;
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
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionTable;
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
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewerFilterInternal;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
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
import org.netxms.nxmc.modules.datacollection.propertypages.TableColumns;
import org.netxms.nxmc.modules.datacollection.propertypages.Thresholds;
import org.netxms.nxmc.modules.datacollection.propertypages.Transformation;
import org.netxms.nxmc.modules.datacollection.views.PerformanceView;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DataCollectionCommon;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DciComparator;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DciFilter;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DciLabelProvider;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.DialogData;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.VisibilityValidator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

public class DataCollectionWidget extends Composite implements DataCollectionCommon
{   
   public static final String JOB_FAMILY = "DataCollectionEditorJob"; //$NON-NLS-1$
   private I18n i18n = LocalizationHelper.getI18n(DataCollectionWidget.class);

   // Columns
   public static final int COLUMN_ID = 0;
   public static final int COLUMN_ORIGIN = 1;
   public static final int COLUMN_DESCRIPTION = 2;
   public static final int COLUMN_PARAMETER = 3;
   public static final int COLUMN_DATATYPE = 4;
   public static final int COLUMN_INTERVAL = 5;
   public static final int COLUMN_RETENTION = 6;
   public static final int COLUMN_STATUS = 7;
   public static final int COLUMN_THRESHOLD = 8;
   public static final int COLUMN_TEMPLATE = 9;
   public static final int COLUMN_RELATEDOBJ = 10;
   public static final int COLUMN_STATUSCALC = 11;

   private final View view;
   private SortableTableViewer viewer;
   private NXCSession session;
   private AbstractObject object;
   private DataCollectionConfiguration dciConfig = null;
   private DciFilter filter;
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
   private Action actionExportToCsv;
   private Action actionExportAllToCsv;
   private Action actionBulkUpdate;
   private boolean hideModificationWarnings;
   private RemoteChangeListener changeListener;

   
   public DataCollectionWidget(View view, Composite parent, int style, AbstractObject obj, final String configPrefix, VisibilityValidator validator)
   {      
      super(parent, style);
      this.view = view;
      session = Registry.getSession();
      object = ((obj instanceof DataCollectionTarget) || (obj instanceof Template)) ? obj : null;

      final PreferenceStore ds = PreferenceStore.getInstance();
      
      this.setLayout(new FillLayout());
      final String[] names = { i18n.tr("ID"), i18n.tr("Origin"), i18n.tr("Description"), i18n.tr("Parameter"), i18n.tr("Data Type"), i18n.tr("Polling Interval"), i18n.tr("Retention Time"), i18n.tr("Status"), i18n.tr("Thresholds"), i18n.tr("Template"), i18n.tr("Related Object"), i18n.tr("Is status calculation") };
      final int[] widths = { 60, 100, 250, 200, 90, 90, 90, 100, 200, 150, 150, 90 };
      viewer = new SortableTableViewer(this, names, widths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new DciLabelProvider());
      viewer.setComparator(new DciComparator((DciLabelProvider)viewer.getLabelProvider()));
      filter = new DciFilter();
      viewer.addFilter(filter);
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
            actionEdit.run();
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
      
      createActions();
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
      manager.add(actionCreateItem);
      manager.add(actionCreateTable);
      manager.add(actionEdit);
      manager.add(actionBulkUpdate);
      manager.add(actionDelete);
      manager.add(actionCopy);
      manager.add(actionMove);
      if(!(object instanceof Template))
         manager.add(actionConvert);
      manager.add(actionDuplicate);
      manager.add(actionExportToCsv);
      manager.add(new Separator());
      manager.add(actionActivate);
      manager.add(actionDisable);
      //TODO: check how to corectly add menu items form the groups
      /*
      manager.add(new Separator());
      manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
      manager.add(new Separator());
      manager.add(new GroupMarker(GroupMarkers.MB_SECONDARY));
      */
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionCreateItem = new Action(i18n.tr("&New parameter..."), ResourceManager.getImageDescriptor("icons/dci/new.png")) { //$NON-NLS-1$
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
      actionEdit.setText(i18n.tr("&Edit..."));
      actionEdit.setImageDescriptor(ResourceManager.getImageDescriptor("icons/dci/edit.png")); //$NON-NLS-1$
      actionEdit.setEnabled(false);
      
      actionBulkUpdate = new Action("Bulk update...") {
         @Override
         public void run()
         {
            openBulkUpdateDialog();
         }
      };

      actionDelete = new Action(i18n.tr("&Delete"), ResourceManager.getImageDescriptor("icons/dci/delete.png")) { //$NON-NLS-1$
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

      actionExportToCsv = new ExportToCsvAction(view, viewer, true); 
      actionExportAllToCsv = new ExportToCsvAction(view, viewer, false);
   }
   
   /**
    * Refresh DCI list
    */
   public void refresh()
   {
      new Job(String.format(i18n.tr("Reftesh data collection configuration for %s"), object.getObjectName()), view) {
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
            return String.format("Cannot refresh data collection configuration for %s", object.getObjectName());
         }
      }.start();      
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      if (dciConfig != null)
      {
         new Job(String.format(i18n.tr("Unlock data collection configuration for "), object.getObjectName()), view) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               dciConfig.close();
               dciConfig = null;
            }

            @Override
            protected String getErrorMessage()
            {
               return String.format(i18n.tr("Cannot unlock data collection configuration for ") + object.getObjectName());
            }
         }.start();
      }
      super.dispose();
   }

   /**
    * Change status for selected items
    * 
    * @param newStatus New status
    */
   private void setItemStatus(final int newStatus)
   {
      final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() <= 0)
         return;
      
      new Job(String.format(i18n.tr("Change status of data collection items for "), object.getObjectName()), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final long[] itemList = new long[selection.size()];
            int pos = 0;
            for(Object dci : selection.toList())
            {
               itemList[pos++] = ((DataCollectionObject)dci).getId();
            }
            dciConfig.setObjectStatus(itemList, newStatus);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  for(Object dci : selection.toList())
                  {
                     ((DataCollectionObject)dci).setStatus(newStatus);
                     viewer.update(dci, null);
                     new DataCollectionObjectEditor((DataCollectionObject)dci).modify();
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot change status of data collection items for "), object.getObjectName());
         }
      }.start();
   }

   /**
    * Delete currently selected DCIs
    */
   private void deleteItems()
   {
      final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() <= 0)
         return;
      
      if (!MessageDialogHelper.openConfirm(view.getWindow().getShell(), i18n.tr("Delete Data Collection Items"),
                                     i18n.tr("Do you really want to delete selected data collection items?")))
         return;
      
      new Job(i18n.tr(String.format("Delete data collection items for "), object.getObjectName()), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object dci : selection.toList())
            {
               dciConfig.deleteObject(((DataCollectionObject)dci).getId());
            }
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
            return String.format(i18n.tr("Cannot delete data collection items for "), object.getObjectName());
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
      
      DataCollectionObject dco = (DataCollectionObject)selection.getFirstElement();
      
      DialogData data = null;
      if (!hideModificationWarnings && dco.getTemplateId() != 0)
      {
         String message = DataCollectionObjectEditor.createModificationWarningMessage(dco);
         if (message != null)
         {
            data = MessageDialogHelper.openWarningWithCheckbox(view.getWindow().getShell(), "Warning", "Don't show this message again", message);
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
         dciList[i] = ((DataCollectionObject)it.next()).getId();
      
      new Job(String.format(i18n.tr("Duplicate data collection items for "), object.getObjectName()), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            dciConfig.copyObjects(dciConfig.getOwnerId(), dciList);
            dciConfig.close();
            dciConfig.open(changeListener);
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
            return String.format(i18n.tr("Cannot duplicate data collection item for "), object.getObjectName());
         }
      }.start();
   }
   
   /**
    * Copy items to another node
    */
   private void copyItems(final boolean doMove)
   {
      final ObjectSelectionDialog dlg = new ObjectSelectionDialog(view.getWindow().getShell(),
            ObjectSelectionDialog.createDataCollectionOwnerSelectionFilter());
      if (dlg.open() != Window.OK)
         return;

      final Set<AbstractObject> targets = new HashSet<AbstractObject>();
      for(AbstractObject o : dlg.getSelectedObjects())
         if ((o instanceof DataCollectionTarget) || (o instanceof Template))
            targets.add(o);
      if (targets.isEmpty())
      {
         MessageDialogHelper.openWarning(view.getWindow().getShell(), "Warning", "Target selection is invalid or empty!");
         return;
      }

      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      Iterator<?> it = selection.iterator();
      final long[] dciList = new long[selection.size()];
      for(int i = 0; (i < dciList.length) && it.hasNext(); i++)
         dciList[i] = ((DataCollectionObject)it.next()).getId();

      new Job(String.format(i18n.tr("Copy data collection items from "), object.getObjectName()), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(AbstractObject o : targets)
               dciConfig.copyObjects(o.getObjectId(), dciList);
            if (doMove)
            {
               for(long id : dciList)
                  dciConfig.deleteObject(id);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     viewer.setInput(dciConfig.getItems());
                  }
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot copy data collection item from ") + object.getObjectName();
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
         DataCollectionObject dco = (DataCollectionObject)it.next();
         dciList.add(dco.getId());
         if (dco.getRetentionType() != DataCollectionObject.RETENTION_CUSTOM)
            isCustomRetention = false;
         if (dco.getPollingScheduleType() != DataCollectionObject.POLLING_SCHEDULE_CUSTOM)
            isCustomInterval = false;
      }      

      BulkUpdateDialog dlg = new BulkUpdateDialog(view.getWindow().getShell(), isCustomRetention, isCustomInterval);
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

      new Job("Bulk DCI update", view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            dciConfig.bulkUpdateDCIs(dciList, elements);
         }

         @Override
         protected String getErrorMessage()
         {
            return "Failed to bulk update DCIs";
         }
      }.start();
   }

   /**
    * Convert selected item(s) to template items
    */
   private void convertToTemplate()
   {
      final ObjectSelectionDialog dlg = new ObjectSelectionDialog(view.getWindow().getShell(), ObjectSelectionDialog.createTemplateSelectionFilter());
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
         dciList[i] = ((DataCollectionObject)it.next()).getId();

      new Job(i18n.tr("Convert data collection items for ") + object.getObjectName() + i18n.tr(" to template items"), view) {
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
            
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(dciConfig.getItems());
               }
            });
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
         view.addMessage(MessageArea.INFORMATION,
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
      //TODO: missing performance tab only for DCI
      pm.addToRoot(new PreferenceNode("accessControl", new AccessControl(dce)));
      if (dce.getObject() instanceof DataCollectionItem)
         pm.addToRoot(new PreferenceNode("otherOptions", new OtherOptions(dce)));             
      else
         pm.addToRoot(new PreferenceNode("otherOptions", new OtherOptionsTable(dce)));   
      pm.addToRoot(new PreferenceNode("comments", new Comments(dce)));  

      PreferenceDialog dlg = new PreferenceDialog(view.getWindow().getShell(), pm) {
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

   public StructuredViewer getViewer()
   {
      return viewer;
   }

   public ViewerFilterInternal getFilter()
   {
      return filter;
   }

   /**
    * Change data collection target object
    * 
    * @param dcTarget new data collection target object
    */
   public void setDataCollectionTarget(AbstractObject obj)
   {
      object = ((obj != null) && ((obj instanceof DataCollectionTarget) || (obj instanceof Template))) ? obj : null;
   }

   /**
    * Post content create method
    */
   public void postContentCreate()
   {      
      // Request server to open data collection configuration
      new Job(String.format(i18n.tr("Open data collection configuration for %s"), object.getObjectName()), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            dciConfig = session.openDataCollectionConfiguration(object.getObjectId(), changeListener);
            dciConfig.setUserData(viewer);
            if (object instanceof Template)
            {
               dciConfig.setLocalChangeCallback(new LocalChangeListener() {                  
                  @Override
                  public void onObjectChange()
                  {         
                     runInUIThread(new Runnable() {
                        @Override
                        public void run()
                        {
                           DataCollectionWidget.this.showInformationMessage();
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
                  viewer.setInput(dciConfig.getItems());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot open data collection configuration for "), object.getObjectName());
         }
      }.start();      
   }
}

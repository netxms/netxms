package org.netxms.ui.eclipse.serverconfig.views;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.ScheduledTask;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.dialogs.SetEntryEditDialog;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ObjectLabelComparator;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;
import org.netxms.ui.eclipse.widgets.helpers.AttributeLabelProvider;

public class PersistentStorageView extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.serverconfig.views.PersistentStorageView"; //$NON-NLS-1$

   private NXCSession session;
   private SortableTableViewer viewerSetValue;
   private Action actionRefresh;
   private Action actionCreate;
   private Action actionEdit;
   private Action actionDelete;
   private Map<String, String> pStorageSet = new HashMap<String, String>();
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      session = ConsoleSharedData.getSession();
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   { 
      //GridLayout layout = new GridLayout();
      //layout.verticalSpacing = WidgetHelper.INNER_SPACING;
      //layout.marginWidth = 0;
      //layout.marginHeight = 0;
      //parent.setLayout(layout);
      
      final String[] setColumnNames = { "Key", "Value" };
      final int[] setColumnWidths = { 150, 200 };
      viewerSetValue = new SortableTableViewer(parent, setColumnNames, setColumnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewerSetValue.setContentProvider(new ArrayContentProvider());
      viewerSetValue.setLabelProvider(new AttributeLabelProvider());
      viewerSetValue.setComparator(new ObjectLabelComparator((ILabelProvider)viewerSetValue.getLabelProvider()));
      viewerSetValue.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            int size = ((IStructuredSelection)viewerSetValue.getSelection()).size();
            actionEdit.setEnabled(size == 1);
            actionDelete.setEnabled(size > 0);
         }
      });
      createActions();
      contributeToActionBars();
      createPopupMenu();
      refresh();
   }
   
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
      Menu menu = menuMgr.createContextMenu(viewerSetValue.getControl());
      viewerSetValue.getControl().setMenu(menu);

      // Register menu for extension.
      getSite().registerContextMenu(menuMgr, viewerSetValue);      
   }

   private void fillContextMenu(IMenuManager mgr)
   {
      IStructuredSelection selection = (IStructuredSelection)viewerSetValue.getSelection();
      if (selection.size() == 1) 
      {         
         mgr.add(actionEdit);
      }

      if (selection.size() > 0)
         mgr.add(actionDelete);

      mgr.add(new Separator());
      mgr.add(actionCreate);
   }

   private void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * @param manager
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionCreate);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * @param manager
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionCreate);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
      
      actionRefresh = new RefreshAction(this) {
         @Override
         public void run()
         {
            refresh();
         }
      };
      
      actionCreate = new Action("Create new", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createValue();
         }
      };
      actionCreate.setActionDefinitionId("org.netxms.ui.eclipse.serverconfig.commands.new_task"); //$NON-NLS-1$
      handlerService.activateHandler(actionCreate.getActionDefinitionId(), new ActionHandler(actionCreate));
      
      actionEdit = new Action("Set", SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editValue();
         }
      };
      actionEdit.setActionDefinitionId("org.netxms.ui.eclipse.serverconfig.commands.edit_task"); //$NON-NLS-1$
      handlerService.activateHandler(actionEdit.getActionDefinitionId(), new ActionHandler(actionEdit));
      
      actionDelete = new Action("Delete", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteValue();
         }
      };
   }

   /**
    * Refresh this window
    */
   private void refresh()
   {
      new ConsoleJob("Reloading scheduled task list", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            pStorageSet = session.getPersistentStorageList();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewerSetValue.setInput(pStorageSet.entrySet().toArray());
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot get list of scheduled tasks";
         }
      }.start();
   }
   
   /**
    * Create 
    */
   private void createValue()
   {
      final SetEntryEditDialog dlg = new SetEntryEditDialog(getSite().getShell(), null, null, true, true);
      if (dlg.open() != Window.OK)
         return;
      
      new ConsoleJob("Reloading scheduled task list", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.setPersistentStorageValue(dlg.getAtributeName(), dlg.getAttributeValue());
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {                  
                  pStorageSet.put(dlg.getAtributeName(), dlg.getAttributeValue());
                  viewerSetValue.setInput(pStorageSet.entrySet().toArray());   
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot get list of scheduled tasks";
         }
      }.start();   
   }

   /**
    * Delete value
    */
   private void deleteValue()
   {      
      final IStructuredSelection selection = (IStructuredSelection)viewerSetValue.getSelection();
      new ConsoleJob("Reloading scheduled task list", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {            
            Iterator<?> it = selection.iterator();
            if (it.hasNext())
            {
               while(it.hasNext())
               {
                  @SuppressWarnings("unchecked")
                  final Entry<String, String> e = (Entry<String, String>) it.next();
                  session.deletePersistentStorageValue(e.getKey());
                  runInUIThread(new Runnable() {
                     @Override
                     public void run()
                     {                  
                        pStorageSet.remove(e.getKey());
                        viewerSetValue.setInput(pStorageSet.entrySet().toArray());   
                     }
                  });
               }
            }
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot get list of scheduled tasks";
         }
      }.start();        
   }   

   /**
    * Edit value
    */
   private void editValue()
   {
      IStructuredSelection selection = (IStructuredSelection)viewerSetValue.getSelection();
      if (selection.size() != 1)
         return;
      
      @SuppressWarnings("unchecked")
      final Entry<String, String> attr = (Entry<String, String>)selection.getFirstElement();
      final SetEntryEditDialog dlg = new SetEntryEditDialog(getSite().getShell(), attr.getKey(), attr.getValue(), true, false);
      if (dlg.open() != Window.OK)
         return;
      
      new ConsoleJob("Reloading scheduled task list", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.setPersistentStorageValue(dlg.getAtributeName(), dlg.getAttributeValue());
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {            
                  pStorageSet.put(dlg.getAtributeName(), dlg.getAttributeValue());
                  viewerSetValue.setInput(pStorageSet.entrySet().toArray());   
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot get list of scheduled tasks";
         }
      }.start();      
   }

   @Override
   public void setFocus()
   {
      viewerSetValue.getTable().setFocus();
   }
}

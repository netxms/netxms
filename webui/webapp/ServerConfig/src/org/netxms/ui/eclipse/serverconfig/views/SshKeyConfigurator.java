/**
 * NetXMS - open source network management system
 * Copyright (C) 2021 Raden Solutions
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
package org.netxms.ui.eclipse.serverconfig.views;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.SshKeyData;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.dialogs.CerateKeyNameDialog;
import org.netxms.ui.eclipse.serverconfig.dialogs.EditSshKeysDialog;
import org.netxms.ui.eclipse.serverconfig.views.helpers.SSHKeyLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.ObjectLabelComparator;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * SSH keys configuration view
 */
public class SshKeyConfigurator extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.serverconfig.views.SshKeyConfigurator"; //$NON-NLS-1$   

   public static final int COLUMN_ID = 0;
   public static final int COLUMN_NMAE = 1;
   
   private SortableTableViewer viewer;
   private Action actionRefresh;
   private Action actionCopyToClipboard;
   private Action actionImport;
   private Action actionEdit;
   private Action actionDelete;
   private Action actionGenerateNew;
   private NXCSession session;
   private SessionListener listener;
   private List<SshKeyData> keyList;
   

   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      session = ConsoleSharedData.getSession();
   }

   @Override
   public void createPartControl(Composite parent)
   { 
      final String[] setColumnNames = { "ID", "Name" };
      final int[] setColumnWidths = { 150, 250 };
      viewer = new SortableTableViewer(parent, setColumnNames, setColumnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new SSHKeyLabelProvider());
      viewer.setComparator(new ObjectLabelComparator((ILabelProvider)viewer.getLabelProvider()));
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            int size = ((IStructuredSelection)viewer.getSelection()).size();
            actionCopyToClipboard.setEnabled(size == 1);
            actionEdit.setEnabled(size == 1);
            actionDelete.setEnabled(size > 0);
         }
      });
      
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            actionEdit.run();
         }
      });
      
      activateContext();
      createActions();
      contributeToActionBars();
      createPopupMenu();
      refresh();

      final Display display = getSite().getShell().getDisplay();
      listener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.SSH_KEY_DATA_CHANGED)
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
      session.addListener(listener);
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      if ((listener != null) && (session != null))
         session.removeListener(listener);
      super.dispose();
   }

   /**
    * Create popup menu
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

      // Register menu for extension.
      getSite().setSelectionProvider(viewer);
      getSite().registerContextMenu(menuMgr, viewer);  
   }

   /**
    * Fill context menu
    * 
    * @param mgr menu manager
    */
   private void fillContextMenu(IMenuManager mgr)
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() == 1) 
      {         
         mgr.add(actionCopyToClipboard);
         mgr.add(actionEdit);
      }

      if (selection.size() > 0)
         mgr.add(actionDelete);

      mgr.add(new Separator());
      mgr.add(actionImport);
      mgr.add(actionGenerateNew);
   }

   /**
    * Fill action bar
    */
   private void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * Fill pull down menu
    * 
    * @param manager menu manager
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionImport);
      manager.add(actionGenerateNew);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * Fill local toolbar
    * 
    * @param manager
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionImport);
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
      
      actionImport = new Action("Import new", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            importKeys();
         }
      };
      
      actionGenerateNew = new Action("Generate new", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            generateKeys();
         }
      };
      
      actionEdit = new Action("Edit", SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editKey();
         }
      };
      //actionEdit.setActionDefinitionId("org.netxms.ui.eclipse.serverconfig.commands.edit_key_task"); //$NON-NLS-1$
      //handlerService.activateHandler(actionEdit.getActionDefinitionId(), new ActionHandler(actionEdit));
      
      actionDelete = new Action("Delete", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteKey();
         }
      };
      
      actionCopyToClipboard = new Action("Copy public key to clipboard", SharedIcons.COPY) {
         @Override
         public void run()
         {
            copyToClipboard();
         }
      };
      actionCopyToClipboard.setActionDefinitionId("org.netxms.ui.eclipse.serverconfig.commands.copy_key_task"); //$NON-NLS-1$
      handlerService.activateHandler(actionCopyToClipboard.getActionDefinitionId(), new ActionHandler(actionEdit));
   }

   /**
    * Refresh view
    */
   private void refresh()
   {
      new ConsoleJob("Reloading SSH key list", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<SshKeyData> list = session.getSshKeys(true);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  keyList = list;
                  viewer.setInput(keyList.toArray());
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot get list of SSH key";
         }
      }.start();
   }

   /**
    * Copy to clipboard
    */
   protected void copyToClipboard()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;
      
      final SshKeyData key = (SshKeyData)selection.getFirstElement();      
      WidgetHelper.copyToClipboard(key.getPublicKey());
   }

   /**
    * Delete key
    */
   protected void deleteKey()
   {
      final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      
      if ((selection == null) || (selection.size() == 0))
         return;

      if (!MessageDialogHelper.openQuestion(getSite().getShell(), "Delete Confirmation", "Are you sure you want to delete selected SSH keys?"))
         return;
      
      final List<SshKeyData> list = new ArrayList<SshKeyData>(selection.size());
      Iterator<?> it = selection.iterator();
      while(it.hasNext())
      {
         list.add((SshKeyData)it.next());         
      }
      
      new ConsoleJob("Delete SSH key", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {     
            for (SshKeyData key : list)
            {
               try
               {
                  session.deleteSshKey(key.getId(), false);
               }
               catch(NXCException e)
               {
                  if (e.getErrorCode() != RCC.SSH_KEY_IN_USE)
                     throw e;
                  
                  final boolean[] retry = new boolean[1];
                  getDisplay().syncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        List<AbstractObject> nodes = session.findMultipleObjects(e.getRelatedObjects(), false);
                        StringBuilder sb = new StringBuilder();
                        for (int i = 0; i < nodes.size(); i++)
                        {
                           sb.append(nodes.get(i).getObjectName());
                           if ((i + 1) != nodes.size())
                           sb.append(", ");
                        }
                        retry[0] = MessageDialogHelper.openQuestion(getSite().getShell(), "Confirm Delete",
                              String.format("SSH key \"%s\" is in use by %s node(s). Are you sure you want to delete it?", key.getName(), sb.toString()));
                     }
                  });
                  if (retry[0])
                  {
                     session.deleteSshKey(key.getId(), true);
                  }
               }
            }
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot delete SSH key";
         }
      }.start(); 
   }

   /**
    * Edit key
    */
   protected void editKey()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() != 1)
         return;
      
      final SshKeyData key = (SshKeyData)selection.getFirstElement();
      final EditSshKeysDialog dlg = new EditSshKeysDialog(getSite().getShell(), new SshKeyData(key));
      if (dlg.open() != Window.OK)
         return;
      
      new ConsoleJob("Edit SSH key", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.updateSshKey(dlg.getSshKeyData());
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot update SSH key";
         }
      }.start(); 
   }

   /**
    * Import existing keys
    */
   protected void importKeys()
   {
      final EditSshKeysDialog dlg = new EditSshKeysDialog(getSite().getShell(), null);
      if (dlg.open() != Window.OK)
         return;
      
      new ConsoleJob("Import SSH keys", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.updateSshKey(dlg.getSshKeyData());
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot import SSH keys";
         }
      }.start(); 
   }

   /**
    * Generate new keys 
    */
   private void generateKeys()
   {
      final CerateKeyNameDialog dlg = new CerateKeyNameDialog(getSite().getShell());
      if (dlg.open() != Window.OK)
         return;
      
      new ConsoleJob("Generate SSH keys", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.generateSshKeys(dlg.getName());            
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot generate SSH keys";
         }
      }.start(); 
   }

   @Override
   public void setFocus()
   {
      viewer.getTable().setFocus();
   }

   /**
    * Activate context
    */
   private void activateContext()
   {
      IContextService contextService = (IContextService)getSite().getService(IContextService.class);
      if (contextService != null)
      {
         contextService.activateContext("org.netxms.ui.eclipse.serverconfig.context.SshKeyConfigurator"); //$NON-NLS-1$
      }
   }

}

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
package org.netxms.nxmc.modules.serverconfig.views;

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
import org.eclipse.jface.dialogs.IInputValidator;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.SshKeyPair;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.serverconfig.dialogs.SSHKeyEditDialog;
import org.netxms.nxmc.modules.serverconfig.views.helpers.SSHKeyLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.ElementLabelComparator;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * SSH keys configuration view
 */
public class SSHKeys extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(SSHKeys.class);

   public static final int COLUMN_ID = 0;
   public static final int COLUMN_NMAE = 1;

   private SortableTableViewer viewer;
   private String filterString;
   private Action actionCopyToClipboard;
   private Action actionImport;
   private Action actionEdit;
   private Action actionDelete;
   private Action actionGenerateNew;
   private NXCSession session;
   private SessionListener listener;
   private List<SshKeyPair> keyList;

   /**
    * Create "SSH keys" view
    */
   public SSHKeys()
   {
      super(LocalizationHelper.getI18n(SSHKeys.class).tr("SSH Keys"), ResourceManager.getImageDescriptor("icons/config-views/ssh-keys.png"), "config.ssh-keys", true);
      session = Registry.getSession();
      filterString = "";
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
   { 
      final String[] setColumnNames = { "ID", "Name" };
      final int[] setColumnWidths = { 150, 250 };
      viewer = new SortableTableViewer(parent, setColumnNames, setColumnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new SSHKeyLabelProvider());
      viewer.setComparator(new ElementLabelComparator((ILabelProvider)viewer.getLabelProvider()));
      viewer.addFilter(new ViewerFilter() {
         @Override
         public boolean select(Viewer viewer, Object parentElement, Object element)
         {
            return filterString.isEmpty() || ((SshKeyPair)element).getName().toLowerCase().contains(filterString);
         }
      });
      setFilterClient(viewer, new AbstractViewerFilter() {
         @Override
         public void setFilterString(String string)
         {
            filterString = string.toLowerCase();
         }
      });
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            int size = viewer.getStructuredSelection().size();
            actionCopyToClipboard.setEnabled(size == 1);
            actionEdit.setEnabled(size == 1);
            actionDelete.setEnabled(size > 0);
         }
      });
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editKey();
         }
      });

      createActions();
      createContextMenu();

      listener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.SSH_KEY_DATA_CHANGED)
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
      session.addListener(listener);
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
    * @see org.netxms.nxmc.base.views.View#dispose()
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
   private void createContextMenu()
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
    * @param mgr menu manager
    */
   private void fillContextMenu(IMenuManager mgr)
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() == 1) 
      {         
         mgr.add(actionCopyToClipboard);
         mgr.add(new Separator());
         mgr.add(actionEdit);
      }

      if (selection.size() > 0)
         mgr.add(actionDelete);

      mgr.add(new Separator());
      mgr.add(actionImport);
      mgr.add(actionGenerateNew);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionImport);
      manager.add(actionGenerateNew);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionImport);
      manager.add(actionGenerateNew);
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionImport = new Action(i18n.tr("&Import..."), SharedIcons.IMPORT) {
         @Override
         public void run()
         {
            importKeys();
         }
      };
      addKeyBinding("M1+I", actionImport);

      actionGenerateNew = new Action(i18n.tr("&Generate..."), ResourceManager.getImageDescriptor("icons/generate-key.gif")) {
         @Override
         public void run()
         {
            generateKeys();
         }
      };
      addKeyBinding("M1+G", actionGenerateNew);

      actionEdit = new Action(i18n.tr("&Edit..."), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editKey();
         }
      };
      addKeyBinding("M3+ENTER", actionEdit);

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteKey();
         }
      };
      addKeyBinding("M1+D", actionDelete);

      actionCopyToClipboard = new Action(i18n.tr("&Copy public key to clipboard"), SharedIcons.COPY) {
         @Override
         public void run()
         {
            copyToClipboard();
         }
      };
      addKeyBinding("M1+C", actionCopyToClipboard);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Loading SSH key list"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<SshKeyPair> list = session.getSshKeys(true);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (viewer.getControl().isDisposed())
                     return;
                  keyList = list;
                  viewer.setInput(keyList.toArray());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get SSH key list");
         }
      }.start();
   }

   /**
    * Copy to clipboard
    */
   private void copyToClipboard()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final SshKeyPair key = (SshKeyPair)selection.getFirstElement();      
      WidgetHelper.copyToClipboard(key.getPublicKey());
   }

   /**
    * Delete key
    */
   private void deleteKey()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete Key"), i18n.tr("Are you sure you want to delete selected SSH keys?")))
         return;

      final List<SshKeyPair> list = new ArrayList<SshKeyPair>(selection.size());
      Iterator<?> it = selection.iterator();
      while(it.hasNext())
      {
         list.add((SshKeyPair)it.next());         
      }

      new Job(i18n.tr("Delete SSH key"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {     
            for (SshKeyPair key : list)
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
                        StringBuilder nodeNames = new StringBuilder();
                        for(AbstractObject n : nodes)
                        {
                           if (nodeNames.length() > 0)
                              nodeNames.append(", ");
                           nodeNames.append(n.getObjectName());
                        }
                        retry[0] = MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete Key"),
                              String.format(i18n.tr("SSH key \"%s\" is in use by the following nodes: %s.\nAre you sure you want to delete it?"), key.getName(), nodeNames.toString()));
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
   private void editKey()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final SshKeyPair key = (SshKeyPair)selection.getFirstElement();
      final SSHKeyEditDialog dlg = new SSHKeyEditDialog(getWindow().getShell(), new SshKeyPair(key));
      if (dlg.open() != Window.OK)
         return;

      new Job(i18n.tr("Updating SSH key"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.updateSshKey(dlg.getSshKeyData());
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update SSH key");
         }
      }.start(); 
   }

   /**
    * Import existing keys
    */
   protected void importKeys()
   {
      final SSHKeyEditDialog dlg = new SSHKeyEditDialog(getWindow().getShell(), null);
      if (dlg.open() != Window.OK)
         return;

      new Job(i18n.tr("Importing SSH keys"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.updateSshKey(dlg.getSshKeyData());
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot import SSH keys");
         }
      }.start(); 
   }

   /**
    * Generate new keys 
    */
   private void generateKeys()
   {
      final InputDialog dlg = new InputDialog(getWindow().getShell(), i18n.tr("Generate SSH Key"), i18n.tr("Key name"), "", new IInputValidator() {
         @Override
         public String isValid(String newText)
         {
            return newText.isBlank() ? i18n.tr("Key name should not be empty") : null;
         }
      });
      if (dlg.open() != Window.OK)
         return;

      new Job(i18n.tr("Generate SSH keys"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.generateSshKeys(dlg.getValue().trim());
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot generate SSH keys");
         }
      }.start(); 
   }

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
   @Override
   public void setFocus()
   {
      viewer.getTable().setFocus();
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

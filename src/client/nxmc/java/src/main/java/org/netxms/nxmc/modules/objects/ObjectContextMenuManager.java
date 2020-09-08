/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.CreateChassisDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateInterfaceDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateMobileDeviceDialog;
import org.netxms.nxmc.modules.objects.dialogs.CreateObjectDialog;
import org.xnap.commons.i18n.I18n;

/**
 * Helper class for building object context menu
 */
public class ObjectContextMenuManager extends MenuManager
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectContextMenuManager.class);

   private View view;
   private ISelectionProvider selectionProvider;
   private Action actionCreateChassis;
   private Action actionCreateCluster;
   private Action actionCreateCondition;
   private Action actionCreateContainer;
   private Action actionCreateInterface;
   private Action actionCreateMobileDevice;
   private Action actionCreateNode;
   private Action actionCreateRack;
   private Action actionCreateVpnConnector;
   private Action actionProperties;

   public ObjectContextMenuManager(View view, ISelectionProvider selectionProvider)
   {
      this.view = view;
      this.selectionProvider = selectionProvider;
      setRemoveAllWhenShown(true);
      addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu();
         }
      });
      createActions();
   }

   /**
    * Create object actions
    */
   private void createActions()
   {
      actionCreateChassis = new Action(i18n.tr("C&hassis...")) {
         @Override
         public void run()
         {
            final long parentId = getObjectIdFromSelection();
            if (parentId == 0)
               return;

            final NXCSession session = Registry.getSession();
            CreateChassisDialog dlg = null;
            do
            {
               dlg = new CreateChassisDialog(getShell(), dlg);
               if (dlg.open() != Window.OK)
                  return;

               final NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_CHASSIS, dlg.getObjectName(), parentId);
               cd.setControllerId(dlg.getControllerId());

               new Job(i18n.tr("Creating chassis"), view) {
                  @Override
                  protected void run(IProgressMonitor monitor) throws Exception
                  {
                     session.createObject(cd);
                  }

                  @Override
                  protected String getErrorMessage()
                  {
                     return String.format(i18n.tr("Cannot create chassis object %s"), cd.getName());
                  }
               }.start();
            } while(dlg.isShowAgain());
         }
      };

      actionCreateCluster = new GenericObjectCreationAction(i18n.tr("C&luster..."), AbstractObject.OBJECT_CLUSTER, i18n.tr("Cluster"));
      actionCreateCondition = new GenericObjectCreationAction(i18n.tr("C&ondition..."), AbstractObject.OBJECT_CONDITION, i18n.tr("Condition"));
      actionCreateContainer = new GenericObjectCreationAction(i18n.tr("&Container..."), AbstractObject.OBJECT_CONTAINER, i18n.tr("Container"));

      actionCreateInterface = new Action(i18n.tr("&Interface...")) {
         @Override
         public void run()
         {
            final long parentId = getObjectIdFromSelection();
            if (parentId == 0)
               return;

            final CreateInterfaceDialog dlg = new CreateInterfaceDialog(getShell());
            if (dlg.open() != Window.OK)
               return;

            final NXCSession session = Registry.getSession();
            new Job(i18n.tr("Creating interface"), view, null) {
               @Override
               protected void run(IProgressMonitor monitor) throws Exception
               {
                  NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_INTERFACE, dlg.getName(), parentId);
                  cd.setMacAddress(dlg.getMacAddress());
                  cd.setIpAddress(dlg.getIpAddress());
                  cd.setPhysicalPort(dlg.isPhysicalPort());
                  cd.setModule(dlg.getSlot());
                  cd.setPort(dlg.getPort());
                  session.createObject(cd);
               }

               @Override
               protected String getErrorMessage()
               {
                  return String.format(i18n.tr("Cannot create interface object %s"), dlg.getName());
               }
            }.start();
         }
      };

      actionCreateMobileDevice = new Action(i18n.tr("&Mobile device...")) {
         @Override
         public void run()
         {
            final long parentId = getObjectIdFromSelection();
            if (parentId == 0)
               return;

            final CreateMobileDeviceDialog dlg = new CreateMobileDeviceDialog(getShell());
            if (dlg.open() != Window.OK)
               return;

            final NXCSession session = Registry.getSession();
            new Job(i18n.tr("Creating rack"), view, null) {
               @Override
               protected void run(IProgressMonitor monitor) throws Exception
               {
                  NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_MOBILEDEVICE, dlg.getName(), parentId);
                  cd.setDeviceId(dlg.getDeviceId());
                  session.createObject(cd);
               }

               @Override
               protected String getErrorMessage()
               {
                  return String.format(i18n.tr("Cannot create mobile device object %s"), dlg.getName());
               }
            }.start();
         }
      };

      actionCreateRack = new GenericObjectCreationAction(i18n.tr("&Rack..."), AbstractObject.OBJECT_RACK, i18n.tr("Rack"));
      actionCreateVpnConnector = new GenericObjectCreationAction(i18n.tr("&VPN connector..."), AbstractObject.OBJECT_VPNCONNECTOR, i18n.tr("VPN Connector"));

      actionProperties = new Action("&Properties...") {
         @Override
         public void run()
         {
            ObjectPropertiesManager.openObjectPropertiesDialog(getObjectFromSelection(), getShell());
         }
      };
   }

   /**
    * Fill object context menu
    */
   protected void fillContextMenu()
   {
      MenuManager createMenu = new MenuManager(i18n.tr("&Create"));
      addAction(createMenu, actionCreateChassis, (AbstractObject o) -> (o instanceof Container) || (o instanceof ServiceRoot));
      addAction(createMenu, actionCreateCluster, (AbstractObject o) -> (o instanceof Container) || (o instanceof ServiceRoot));
      addAction(createMenu, actionCreateCondition, (AbstractObject o) -> (o instanceof Container) || (o instanceof ServiceRoot));
      addAction(createMenu, actionCreateContainer, (AbstractObject o) -> (o instanceof Container) || (o instanceof ServiceRoot));
      addAction(createMenu, actionCreateInterface, (AbstractObject o) -> o instanceof Node);
      addAction(createMenu, actionCreateMobileDevice, (AbstractObject o) -> (o instanceof Container) || (o instanceof ServiceRoot));
      addAction(createMenu, actionCreateRack, (AbstractObject o) -> (o instanceof Container) || (o instanceof ServiceRoot));
      addAction(createMenu, actionCreateVpnConnector, (AbstractObject o) -> o instanceof Node);
      if (!createMenu.isEmpty())
         add(createMenu);
      add(new Separator());
      add(actionProperties);
   }

   /**
    * Add action to given menu manager
    *
    * @param manager menu manager
    * @param action action to add
    * @param filter filter for current selection
    */
   protected void addAction(IMenuManager manager, Action action, ObjectFilter filter)
   {
      if (filter.accept(getObjectFromSelection()))
         manager.add(action);
   }

   /**
    * Get parent shell for dialog windows.
    *
    * @return parent shell for dialog windows
    */
   protected Shell getShell()
   {
      return Registry.getMainWindow().getShell();
   }

   /**
    * Get object from current selection
    *
    * @return object or null
    */
   protected AbstractObject getObjectFromSelection()
   {
      IStructuredSelection selection = (IStructuredSelection)selectionProvider.getSelection();
      if (selection.size() != 1)
         return null;
      return (AbstractObject)selection.getFirstElement();
   }

   /**
    * Get object ID from selection
    *
    * @return object ID or 0
    */
   protected long getObjectIdFromSelection()
   {
      IStructuredSelection selection = (IStructuredSelection)selectionProvider.getSelection();
      if (selection.size() != 1)
         return 0;
      return ((AbstractObject)selection.getFirstElement()).getObjectId();
   }
   
   /**
    * Action for generic object creation (when only object name is requested)
    */
   private class GenericObjectCreationAction extends Action
   {
      private int objectClass;
      private String className;

      GenericObjectCreationAction(String name, int objectClass, String className)
      {
         super(name);
         this.objectClass = objectClass;
         this.className = className;
      }
   
      /**
       * @see org.eclipse.jface.action.Action#run()
       */
      @Override
      public void run()
      {
         final long parentId = getObjectIdFromSelection();
         if (parentId == 0)
            return;

         final CreateObjectDialog dlg = new CreateObjectDialog(getShell(), className);
         if (dlg.open() != Window.OK)
            return;

         final NXCSession session = Registry.getSession();
         new Job(String.format(i18n.tr("Creating %s"), className), view, null) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               NXCObjectCreationData cd = new NXCObjectCreationData(objectClass, dlg.getObjectName(), parentId);
               session.createObject(cd);
            }

            @Override
            protected String getErrorMessage()
            {
               return String.format(i18n.tr("Cannot create %s object %s"), objectClass, dlg.getObjectName());
            }
         }.start();
      }
   }
}

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
package org.netxms.nxmc.modules.objects.views;

import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.HardwareComponent;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.PhysicalComponent;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Asset;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.actions.ViewerProvider;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.assetmanagement.CreateAssetFromObjectAction;
import org.netxms.nxmc.modules.assetmanagement.dialogs.CreateAssetDialog;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.widgets.AbstractHardwareInventoryWidget;
import org.netxms.nxmc.modules.objects.widgets.EntityMibTreeViewer;
import org.netxms.nxmc.modules.objects.widgets.HardwareInventoryTable;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Hardware inventory view
 */
public class HardwareInventoryView extends ObjectView
{
   private static enum DisplayMode
   {
      AGENT, ENTITY_MIB
   };

   private final I18n i18n = LocalizationHelper.getI18n(HardwareInventoryView.class);

   private DisplayMode displayMode = DisplayMode.AGENT;
   private AbstractHardwareInventoryWidget inventoryWidget;
   private Action actionExportToCsv;
   private Action actionExportAllToCsv;
   private Action actionCopy;
   private Action actionCopyName;
   private Action actionCopyDescription;
   private Action actionCopyModel;
   private Action actionCopySerial;
   private Action actionCollapseAll;
   private Action actionExpandAll;
   private Action actionCreateAsset;
   private Action actionLinkAsset;
   private Action actionUnlinkAsset;
   private SessionListener assetListener;

   /**
    * Identification of component selected in inventory widget
    */
   private static class SelectedComponent
   {
      String name;
      String serial;
      String vendor;
      String model;
   }

   /**
    * @param name
    * @param image
    * @param id
    */
   public HardwareInventoryView()
   {
      super(LocalizationHelper.getI18n(HardwareInventoryView.class).tr("Hardware Inventory"), ResourceManager.getImageDescriptor("icons/object-views/hardware.png"), "objects.hardware-inventory",
            false);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      HardwareInventoryView view = (HardwareInventoryView)super.cloneView();
      view.displayMode = displayMode;
      return view;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof Node) && (((Node)context).hasAgent() || ((Node)context).isEntityMibSupported());
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 60;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      inventoryWidget = (displayMode == DisplayMode.AGENT) ? new HardwareInventoryTable(parent, SWT.NONE, this) : new EntityMibTreeViewer(parent, SWT.NONE, this);
      createActions();
      createContextMenu();

      // Component assets are separate objects, so their link changes do not arrive as updates of the displayed node
      assetListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (((n.getCode() == SessionNotification.OBJECT_CHANGED) || (n.getCode() == SessionNotification.OBJECT_DELETED)) && (n.getObject() instanceof Asset))
            {
               getDisplay().asyncExec(() -> {
                  if (!inventoryWidget.isDisposed())
                     inventoryWidget.refreshComponentAssets();
               });
            }
         }
      };
      session.addListener(assetListener);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(assetListener);
      super.dispose();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      ViewerProvider vp = new ViewerProvider() {         
         @Override
         public ColumnViewer getViewer()
         {
            return inventoryWidget.getViewer();
         }
      };
      actionExportToCsv = new ExportToCsvAction(this, vp, true);
      actionExportAllToCsv = new ExportToCsvAction(this, vp, false);

      actionCopy = new Action(i18n.tr("&Copy to clipboard"), SharedIcons.COPY) {
         @Override
         public void run()
         {
            inventoryWidget.copySelectionToClipboard(-1);
         }
      };

      actionCopyName = new Action(i18n.tr("Copy &name to clipboard")) {
         @Override
         public void run()
         {
            inventoryWidget.copySelectionToClipboard(inventoryWidget.getNameColumnIndex());
         }
      };

      actionCopyDescription = new Action(i18n.tr("Copy &description to clipboard")) {
         @Override
         public void run()
         {
            inventoryWidget.copySelectionToClipboard(inventoryWidget.getDescriptionColumnIndex());
         }
      };

      actionCopyModel = new Action(i18n.tr("Copy &model to clipboard")) {
         @Override
         public void run()
         {
            inventoryWidget.copySelectionToClipboard(inventoryWidget.getModelColumnIndex());
         }
      };

      actionCopySerial = new Action(i18n.tr("Copy &serial number to clipboard")) {
         @Override
         public void run()
         {
            inventoryWidget.copySelectionToClipboard(inventoryWidget.getSerialColumnIndex());
         }
      };

      actionCollapseAll = new Action(i18n.tr("C&ollapse all"), SharedIcons.COLLAPSE_ALL) {
         @Override
         public void run()
         {
            ((TreeViewer)inventoryWidget.getViewer()).collapseAll();
         }
      };

      actionExpandAll = new Action(i18n.tr("&Expand all"), SharedIcons.EXPAND_ALL) {
         @Override
         public void run()
         {
            ((TreeViewer)inventoryWidget.getViewer()).expandAll();
         }
      };

      actionCreateAsset = new Action(i18n.tr("Create &asset..."), ResourceManager.getImageDescriptor("icons/objects/asset.png")) {
         @Override
         public void run()
         {
            createComponentAsset();
         }
      };

      actionLinkAsset = new Action(i18n.tr("&Link to asset..."), ResourceManager.getImageDescriptor("icons/link-objects.png")) {
         @Override
         public void run()
         {
            linkComponentToAsset();
         }
      };

      actionUnlinkAsset = new Action(i18n.tr("&Unlink from asset"), ResourceManager.getImageDescriptor("icons/disconnect.png")) {
         @Override
         public void run()
         {
            unlinkComponentFromAsset();
         }
      };
   }

   /**
    * Get component selected in inventory widget. Only components with serial number can be represented by assets.
    *
    * @return selected component or null if selection is empty, contains multiple elements, or selected component has no serial number
    */
   private SelectedComponent getSelectedComponent()
   {
      IStructuredSelection selection = (IStructuredSelection)inventoryWidget.getViewer().getSelection();
      if (selection.size() != 1)
         return null;

      SelectedComponent c = new SelectedComponent();
      Object element = selection.getFirstElement();
      if (element instanceof PhysicalComponent)
      {
         PhysicalComponent pc = (PhysicalComponent)element;
         c.name = pc.getDisplayName();
         c.serial = pc.getSerialNumber();
         c.vendor = pc.getVendor();
         c.model = pc.getModel();
      }
      else if (element instanceof HardwareComponent)
      {
         HardwareComponent hc = (HardwareComponent)element;
         c.name = hc.getType().isEmpty() ? hc.getModel() : hc.getType();
         c.serial = hc.getSerialNumber();
         c.vendor = hc.getVendor();
         c.model = hc.getModel();
      }
      else
      {
         return null;
      }
      return ((c.serial == null) || c.serial.isEmpty()) ? null : c;
   }

   /**
    * Find asset currently linked to component with given serial number.
    *
    * @param serial component serial number
    * @return linked asset or null
    */
   private Asset findComponentAsset(String serial)
   {
      for(Asset a : session.getComponentAssets(getObjectId()))
         if (a.getComponentSerial().equals(serial))
            return a;
      return null;
   }

   /**
    * Create new asset for selected component and link it to that component.
    */
   private void createComponentAsset()
   {
      final SelectedComponent c = getSelectedComponent();
      if (c == null)
         return;

      Asset currentAsset = findComponentAsset(c.serial);
      if (currentAsset != null)
      {
         String question = String.format(i18n.tr("Component \"%s\" already linked to asset \"%s\". Are you sure you want to link it to a new asset?"), c.serial, currentAsset.getObjectName());
         if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Confirm Asset Creation"), question))
            return;
      }

      Map<String, String> properties = CreateAssetFromObjectAction.buildPrefilledProperties(c.serial, c.vendor, c.model, null, null);
      String defaultName = ((c.name != null) && !c.name.isEmpty()) ? getObjectName() + " - " + c.name : getObjectName() + " - " + c.serial;
      final CreateAssetDialog dlg = new CreateAssetDialog(getWindow().getShell(), defaultName, properties);
      if (dlg.open() != Window.OK)
         return;

      new Job(i18n.tr("Creating asset for component {0}", c.serial), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_ASSET, dlg.getName(), dlg.getParentId());
            cd.setObjectAlias(dlg.getAlias());
            cd.setAssetProperties(dlg.getProperties());
            cd.setLinkedObjectId(getObjectId());
            cd.setComponentSerial(c.serial);
            session.createObject(cd);
            runInUIThread(() -> addMessage(MessageArea.SUCCESS, i18n.tr("Asset \"{0}\" successfully created and linked with component {1}", dlg.getName(), c.serial)));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create asset \"{0}\"", dlg.getName());
         }
      }.start();
   }

   /**
    * Link selected component to existing asset.
    */
   private void linkComponentToAsset()
   {
      final SelectedComponent c = getSelectedComponent();
      if (c == null)
         return;

      Asset currentAsset = findComponentAsset(c.serial);
      if (currentAsset != null)
      {
         String question = String.format(i18n.tr("Component \"%s\" already linked to asset \"%s\". Are you sure you want to link it to another asset?"), c.serial, currentAsset.getObjectName());
         if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Confirm Link"), question))
            return;
      }

      ObjectSelectionDialog dlg = new ObjectSelectionDialog(getWindow().getShell(), ObjectSelectionDialog.createAssetSelectionFilter());
      if (dlg.open() != Window.OK)
         return;
      final AbstractObject asset = dlg.getSelectedObjects().get(0);
      if (!(asset instanceof Asset))
         return; // Incompatible object selected

      if (((Asset)asset).getLinkedObjectId() != 0)
      {
         String question = String.format(i18n.tr("Asset \"%s\" already linked to object \"%s\". Are you sure you want to link it to another object?"), asset.getObjectName(),
               session.getObjectName(((Asset)asset).getLinkedObjectId()));
         if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Confirm Link"), question))
            return;
      }

      new Job(i18n.tr("Linking asset \"{0}\" to component {1}", asset.getObjectName(), c.serial), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.linkAsset(asset.getObjectId(), getObjectId(), c.serial);
            runInUIThread(() -> addMessage(MessageArea.SUCCESS, i18n.tr("Component {0} successfully linked with asset \"{1}\"", c.serial, asset.getObjectName())));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot link asset \"{0}\"", asset.getObjectName());
         }
      }.start();
   }

   /**
    * Unlink selected component from its asset.
    */
   private void unlinkComponentFromAsset()
   {
      final SelectedComponent c = getSelectedComponent();
      if (c == null)
         return;

      final Asset asset = findComponentAsset(c.serial);
      if (asset == null)
         return;

      new Job(i18n.tr("Unlinking asset \"{0}\" from component {1}", asset.getObjectName(), c.serial), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.unlinkAsset(asset.getObjectId());
            runInUIThread(() -> addMessage(MessageArea.SUCCESS, i18n.tr("Component {0} successfully unlinked from asset \"{1}\"", c.serial, asset.getObjectName())));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot unlink asset \"{0}\"", asset.getObjectName());
         }
      }.start();
   }

   /**
    * Fill view's local toolbar
    *
    * @param manager toolbar manager
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionExportAllToCsv);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionExportAllToCsv);
      Action autoSize = inventoryWidget.getAutoSizeColumnsAction();
      if (autoSize != null)
      {
         manager.add(new Separator());
         manager.add(autoSize);
      }
   }

   /**
    * Create pop-up menu
    */
   private void createContextMenu()
   {
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });
      inventoryWidget.setViewerMenu(menuMgr);
   }

   /**
    * Fill context menu
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionCopy);
      if (inventoryWidget.getNameColumnIndex() != -1)
         manager.add(actionCopyName);
      manager.add(actionCopyDescription);
      manager.add(actionCopyModel);
      manager.add(actionCopySerial);
      SelectedComponent c = getSelectedComponent();
      if (c != null)
      {
         manager.add(new Separator());
         manager.add(actionCreateAsset);
         manager.add(actionLinkAsset);
         if (findComponentAsset(c.serial) != null)
            manager.add(actionUnlinkAsset);
      }
      manager.add(new Separator());
      manager.add(actionExportToCsv);
      manager.add(actionExportAllToCsv);
      if (inventoryWidget.isTreeViewer())
      {
         manager.add(new Separator());
         manager.add(actionCollapseAll);
         manager.add(actionExpandAll);
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      inventoryWidget.refresh();      
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      inventoryWidget.clear();
      if (object == null)
         return;

      if ((displayMode == DisplayMode.AGENT) && !((Node)object).hasAgent())
      {
         inventoryWidget.dispose();
         inventoryWidget = new EntityMibTreeViewer(getClientArea(), SWT.NONE, this);
         displayMode = DisplayMode.ENTITY_MIB;
         createContextMenu();
      }
      else if ((displayMode == DisplayMode.ENTITY_MIB) && !((Node)object).isEntityMibSupported())
      {
         inventoryWidget.dispose();
         inventoryWidget = new HardwareInventoryTable(getClientArea(), SWT.NONE, this);
         displayMode = DisplayMode.AGENT;
         createContextMenu();
      }

      inventoryWidget.refresh();
   }
   
   /**
    * @see org.netxms.nxmc.base.views.Perspective#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {      
      super.saveState(memento);
      memento.set("displayMode", displayMode.toString());
   }
   
   /**
    * @throws ViewNotRestoredException 
    * @see org.netxms.nxmc.base.views.Perspective#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {
      super.restoreState(memento);
      displayMode = DisplayMode.valueOf(memento.getAsString("displayMode"));
   }
}

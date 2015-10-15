/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectmanager.propertypages;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.ui.dialogs.PropertyPage;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.netxms.base.InetAddressEx;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.VPNConnector;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.objectmanager.dialogs.AddSubnetDialog;
import org.netxms.ui.eclipse.objectmanager.propertypages.helpers.SubnetComparator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 *"Subnets" property page for NetXMS VPN objects
 */
public class VPNSubnets extends PropertyPage
{
   private VPNConnector connector;
   private ObjectSelector objectSelector;
   private TableViewer localNetworksList;
   private TableViewer remoteNetworksList;
   private boolean modified = false; 
   private List<InetAddressEx> localNetworksElements;
   private List<InetAddressEx> remoteNetworksElements;

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);      
      connector = (VPNConnector)getElement().getAdapter(VPNConnector.class);
      
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      dialogArea.setLayoutData(gd);
      
      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, true);
      objectSelector.setLabel(Messages.get().VPNSubnets_PeerGateway);
      objectSelector.setObjectClass(AbstractObject.class);
      objectSelector.setObjectId(connector.getPeerGatewayId());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      objectSelector.setLayoutData(gd);
      
      Composite clientArea =  new Composite(dialogArea, SWT.NONE);
      layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 2;
      gd = new GridData();
      clientArea.setLayout(layout);
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      clientArea.setLayoutData(gd);
      
      //networks lists
      localNetworksElements = new ArrayList<InetAddressEx>(connector.getLocalSubnets());
      remoteNetworksElements = new ArrayList<InetAddressEx>(connector.getRemoteSubnets());
      createNetworkList(clientArea, Messages.get().VPNSubnets_LocalNetworks, localNetworksList, localNetworksElements);
      createNetworkList(clientArea, Messages.get().VPNSubnets_RemoteNetworks, remoteNetworksList, remoteNetworksElements);
      
      return dialogArea;
   }
   
   /**
    * Creates network list 
    * 
    * @param dialogArea
    * @param viewList viewer to be created and added to view
    * @param data elements that should be added as a content of this viewer
    */
   private void createNetworkList(Composite dialogArea, String title, TableViewer viewList, final List<InetAddressEx> data) 
   {
      Group clientArea = new Group(dialogArea, SWT.NONE);
      clientArea.setText(title);      
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      GridData gd = new GridData();
      clientArea.setLayout(layout);
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      clientArea.setLayoutData(gd);
      
      viewList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.verticalSpan = 2;
      gd.heightHint = 100;
      viewList.getTable().setLayoutData(gd);
      viewList.getTable().setSortDirection(SWT.UP);
      viewList.setContentProvider(new ArrayContentProvider());
      viewList.setComparator(new SubnetComparator());
      viewList.setInput(data.toArray());
      
      final TableViewer list = viewList;
      
      final ImageHyperlink linkAdd = new ImageHyperlink(clientArea, SWT.NONE); 
      linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkAdd.setLayoutData(gd);
      linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            addTargetAddressListElement(list, data);
         }
      });
      
      final ImageHyperlink linkRemove = new ImageHyperlink(clientArea, SWT.NONE);
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkRemove.setLayoutData(gd);
      linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            removeTargetAddressListElements(list, data);
         }
      });
   }
   
   /**
    * Add element to subnet list
    */
   private void addTargetAddressListElement(TableViewer elementList, List<InetAddressEx> elements)
   {
      AddSubnetDialog dlg = new AddSubnetDialog(getShell());
      if (dlg.open() == Window.OK)
      {
         InetAddressEx subnet = dlg.getSubnet();
         if (!elements.contains(subnet))
         {
            elements.add(subnet);
            elementList.setInput(elements.toArray());
            modified = true;
         }
      }
   }
   
   /**
    * Remove element(s) from subnet list
    */
   private void removeTargetAddressListElements(TableViewer elementList, List<InetAddressEx> elements)
   {
      IStructuredSelection selection = (IStructuredSelection)elementList.getSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            elements.remove(o);
         }
         elementList.setInput(elements.toArray());
         modified = true;
      }
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
   @Override
   protected void performApply()
   {
      applyChanges(true);
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#createControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createControl(Composite parent)
   {
      super.createControl(parent);
      getDefaultsButton().setVisible(false);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      applyChanges(false);
      return true;
   }

   /**
    * Saves changes
    */
   private void applyChanges(final boolean isApply)
   {
      if (!modified)
         return;     // Nothing to apply
      
      if (isApply)
         setValid(false);
      
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      final NXCObjectModificationData md = new NXCObjectModificationData(connector.getObjectId());
      md.setVpnNetworks(localNetworksElements, remoteNetworksElements);
      md.setPeerGatewayId(objectSelector.getObjectId());
      new ConsoleJob(Messages.get().VPNSubnets_JobName, null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
            {
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     VPNSubnets.this.setValid(true);
                  }
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().VPNSubnets_JobError;
         }
      }.schedule();
      
   }
}

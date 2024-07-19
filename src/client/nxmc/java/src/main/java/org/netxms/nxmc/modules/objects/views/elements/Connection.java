/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views.elements;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.LinkLayerDiscoveryProtocol;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.Interface;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectIcons;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.modules.objects.widgets.helpers.DecoratingObjectLabelProvider;
import org.xnap.commons.i18n.I18n;

/**
 * "Connection" element - shows peer information for interface
 */
public class Connection extends OverviewPageElement
{
   private final I18n i18n = LocalizationHelper.getI18n(Connection.class);

   private NXCSession session;
   private CLabel nodeLabel;
   private CLabel interfaceLabel;
   private CLabel protocolLabel;
   private DecoratingObjectLabelProvider labelProvider;
   private Image interfaceIcon;
   private Image nodeIcon;

   /**
    * @param parent
    * @param anchor
    * @param objectView
    */
   public Connection(Composite parent, OverviewPageElement anchor, ObjectView objectView)
   {
      super(parent, anchor, objectView);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#createClientArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createClientArea(Composite parent)
   {
      labelProvider = new DecoratingObjectLabelProvider();
      parent.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            labelProvider.dispose();
         }
      });

      ObjectIcons objectIcons = Registry.getSingleton(ObjectIcons.class);
      interfaceIcon = objectIcons.getImage(AbstractObject.OBJECT_INTERFACE);
      nodeIcon = objectIcons.getImage(AbstractObject.OBJECT_NODE);

      Composite area = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      area.setLayout(layout);
      area.setBackground(parent.getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));

      nodeLabel = new CLabel(area, SWT.NONE);
      nodeLabel.setBackground(area.getBackground());
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      nodeLabel.setLayoutData(gd);

      interfaceLabel = new CLabel(area, SWT.NONE);
      interfaceLabel.setBackground(area.getBackground());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalIndent = 15;
      interfaceLabel.setLayoutData(gd);

      protocolLabel = new CLabel(area, SWT.NONE);
      protocolLabel.setBackground(area.getBackground());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalIndent = 15;
      protocolLabel.setLayoutData(gd);

      return area;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#getTitle()
    */
   @Override
   protected String getTitle()
   {
      return i18n.tr("Connection");
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#onObjectChange()
    */
   @Override
   protected void onObjectChange()
   {
      if (getObject() == null)
         return;

      long peerNodeId, peerInterfaceId;
      LinkLayerDiscoveryProtocol peerDiscoveryProtocol;
      if (getObject() instanceof Interface)
      {
         Interface iface = (Interface)getObject();
         peerNodeId = iface.getPeerNodeId();
         peerInterfaceId = iface.getPeerInterfaceId();
         peerDiscoveryProtocol = iface.getPeerDiscoveryProtocol();
      }
      else if (getObject() instanceof AccessPoint)
      {
         AccessPoint ap = (AccessPoint)getObject();
         peerNodeId = ap.getPeerNodeId();
         peerInterfaceId = ap.getPeerInterfaceId();
         peerDiscoveryProtocol = ap.getPeerDiscoveryProtocol();
      }
      else
      {
         peerNodeId = 0;
         peerInterfaceId = 0;
         peerDiscoveryProtocol = LinkLayerDiscoveryProtocol.UNKNOWN;
      }

      if (peerNodeId != 0)
      {
         AbstractObject peer = session.findObjectById(peerNodeId);
         nodeLabel.setText((peer != null) ? peer.getObjectName() : "<" + peerNodeId + ">");
         nodeLabel.setImage((peer != null) ? labelProvider.getImage(peer) : nodeIcon);
      }
      else
      {
         nodeLabel.setText(i18n.tr("N/A"));
      }

      // If peer is an access point, peer interface ID will be set to access point ID
      if ((peerInterfaceId != 0) && (peerInterfaceId != peerNodeId))
      {
         new Job(i18n.tr("Synchronize objects"), getObjectView()) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               AbstractObject object = session.findObjectById(peerNodeId);
               if (object != null)
                  session.syncChildren(object);
               runInUIThread(() -> {
                  if (Connection.this.isDisposed())
                     return;

                  AbstractObject peerIface = session.findObjectById(peerInterfaceId);
                  interfaceLabel.setText((peerIface != null) ? peerIface.getObjectName() : "<" + peerInterfaceId + ">");
                  interfaceLabel.setImage((peerIface != null) ? labelProvider.getImage(peerIface) : null);
                  protocolLabel.setText(peerDiscoveryProtocol.toString());
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot synchronize objects");
            }
         }.start();
      }
      else if (peerInterfaceId == peerNodeId)
      {
         interfaceLabel.setText("eth0");
         interfaceLabel.setImage(interfaceIcon);
         protocolLabel.setText(peerDiscoveryProtocol.toString());
      }
      else
      {
         interfaceLabel.setText(i18n.tr("N/A"));
         protocolLabel.setText("");
      }
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean isApplicableForObject(AbstractObject object)
   {
      return ((object instanceof Interface) && (((Interface)object).getPeerNodeId() != 0)) ||
             ((object instanceof AccessPoint) && (((AccessPoint)object).getPeerNodeId() != 0));
   }
}

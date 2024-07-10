/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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

import java.util.HashMap;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.ConnectionPointType;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.client.topology.ConnectionPoint;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.CommandTextBox;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * "Topology" element
 */
public class Topology extends OverviewPageElement
{
   private final I18n i18n = LocalizationHelper.getI18n(Topology.class);

   private Action actionFindSwitchPort;
   private CommandTextBox commandBox;
   private HashMap<Long, ConnectionPoint> connectionPointCache = new HashMap<>();

   /**
    * @param parent
    * @param anchor
    * @param objectView
    */
   public Topology(Composite parent, OverviewPageElement anchor, ObjectView objectView)
   {
      super(parent, anchor, objectView);
      createActionButtons();
   }

   /**
    * Create actions
    */
   private void createActionButtons()
   {
      final NXCSession session = Registry.getSession();

      actionFindSwitchPort = new Action(i18n.tr("Update connection point information"), SharedIcons.UPDATE) {
         @Override
         public void run()
         {
            final AbstractObject object = getObject();
            new Job(i18n.tr("Find switch port"), getObjectView()) {
               @Override
               protected void run(IProgressMonitor monitor) throws Exception
               {
                  final ConnectionPoint connectionPoint = session.findConnectionPoint(object.getObjectId());
                  runInUIThread(() -> {
                     if (!isDisposed())
                     {
                        connectionPointCache.put(object.getObjectId(), connectionPoint);
                        showConnectionStep(connectionPoint);
                     }
                  });
               }

               @Override
               protected String getErrorMessage()
               {
                  return i18n.tr("Cannot find switch port");
               }
            }.start();
         }
      };
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#getTitle()
    */
   @Override
   protected String getTitle()
   {
      return i18n.tr("Topology");
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#onObjectChange()
    */
   @Override
   protected void onObjectChange()
   {
      commandBox.deleteAll(false);
      long objectId = getObject().getObjectId();
      final NXCSession session = Registry.getSession();
      if (connectionPointCache.containsKey(objectId))
      {
         ConnectionPoint cp = connectionPointCache.get(objectId);
         if (cp != null)
            showConnectionStep2(session, cp);
         else
            commandBox.addOrUpdate(actionFindSwitchPort, i18n.tr("Connection point cannot be found"), false);
      }
      else
      {
         commandBox.addOrUpdate(actionFindSwitchPort, i18n.tr("Connection point information not retrieved yet"), false);
      }
      commandBox.rebuild();
   }

   /**
    * Show connection point information.
    *
    * @param cp connection point information
    */
   public void showConnectionStep(final ConnectionPoint cp)
   {
      if (cp == null)
      {
         commandBox.addOrUpdate(actionFindSwitchPort, i18n.tr("Connection point cannot be found"), false);
         return;
      }

      final NXCSession session = Registry.getSession();
      if (session.areChildrenSynchronized(cp.getNodeId()) && session.areChildrenSynchronized(cp.getLocalNodeId()))
      {
         showConnectionStep2(session, cp);
      }
      else
      {
         new Job(i18n.tr("Synchronize objects"), getObjectView()) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               AbstractObject object = session.findObjectById(cp.getNodeId());
               if (object != null)
                  session.syncChildren(object);
               session.syncChildren(getObject());
               runInUIThread(() -> {
                  if (!isDisposed())
                     showConnectionStep2(session, cp);
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot synchronize objects");
            }
         }.start();
      }
   }

   /**
    * Show connection point information - step 2.
    * 
    * @param session communication session
    * @param cp connection point information
    */
   private void showConnectionStep2(NXCSession session, ConnectionPoint cp)
   {
      Node bridge = session.findObjectById(cp.getNodeId(), Node.class);
      AbstractObject iface = session.findObjectById(cp.getInterfaceId());
      AbstractObject localInterface = session.findObjectById(cp.getLocalInterfaceId());
      if ((bridge != null) && (iface != null))
      {
         if (getObject() instanceof Node)
         {
            if (cp.getType() == ConnectionPointType.WIRELESS)
            {
               commandBox.addOrUpdate(actionFindSwitchPort,
                     String.format(i18n.tr("Connected to wireless access point %2$s radio %3$s via local interface %1$s"),
                           localInterface.getObjectName(), bridge.getObjectName(), iface.getObjectName()),
                     false);
            }
            else
            {
               commandBox.addOrUpdate(actionFindSwitchPort,
                     (cp.getType() == ConnectionPointType.DIRECT) ?
                           String.format(i18n.tr("Directly connected to network switch %2$s port %3$s via local interface %1$s"), localInterface.getObjectName(), bridge.getObjectName(), iface.getObjectName()) :
                           String.format(i18n.tr("Indirectly connected to network switch %2$s port %3$s via local interface %1$s"), localInterface.getObjectName(), bridge.getObjectName(), iface.getObjectName()),
                     false);
            }
         }
         else
         {
            if (cp.getType() == ConnectionPointType.WIRELESS)
            {
               commandBox.addOrUpdate(actionFindSwitchPort,
                     String.format(i18n.tr("Connected to wireless access point %1$s radio %2$s"), bridge.getObjectName(),
                           iface.getObjectName()),
                     false);
            }
            else
            {
               commandBox.addOrUpdate(actionFindSwitchPort,
                     (cp.getType() == ConnectionPointType.DIRECT) ? String.format(i18n.tr("Directly connected to network switch %1$s port %2$s"), bridge.getObjectName(), iface.getObjectName()) :
                           String.format(i18n.tr("Indirectly connected to network switch %1$s port %2$s"), bridge.getObjectName(), iface.getObjectName()),
                     false);
            }
         }
      }
      else if (cp.getType() == ConnectionPointType.UNKNOWN)
      {
         commandBox.addOrUpdate(actionFindSwitchPort, i18n.tr("Connection point cannot be found"), false);
      }
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#createClientArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createClientArea(Composite parent)
   {
      commandBox = new CommandTextBox(parent, SWT.NONE);
      return commandBox;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean isApplicableForObject(AbstractObject object)
   {
      return (object instanceof Node) || (object instanceof Interface) || (object instanceof AccessPoint);
   }
}

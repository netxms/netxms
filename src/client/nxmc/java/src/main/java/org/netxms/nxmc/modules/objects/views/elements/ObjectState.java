/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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

import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.SpanningTreePortState;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.Asset;
import org.netxms.client.objects.BusinessService;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.Node;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.modules.objects.views.helpers.InterfaceListLabelProvider;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.xnap.commons.i18n.I18n;

/**
 * "State" element on object overview tab
 */
public class ObjectState extends TableElement
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectState.class);
   private static final String[] ifaceExpectedState = 
      { 
         LocalizationHelper.getI18n(ObjectState.class).tr("Up"), 
         LocalizationHelper.getI18n(ObjectState.class).tr("Down"), 
         LocalizationHelper.getI18n(ObjectState.class).tr("Ignore"), 
         LocalizationHelper.getI18n(ObjectState.class).tr("Auto") 
      };

	/**
    * @param parent
    * @param anchor
    * @param objectView
    */
   public ObjectState(Composite parent, OverviewPageElement anchor, ObjectView objectView)
	{
      super(parent, anchor, objectView);
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.TableElement#fillTable()
    */
	@Override
	protected void fillTable()
	{
		final AbstractObject object = getObject();
      final NXCSession session = Registry.getSession();

      if (object.isInMaintenanceMode())
      {
         addPair(i18n.tr("Status"), StatusDisplayInfo.getStatusText(object.getStatus()) + i18n.tr(" (maintenance)"));
         AbstractUserObject user = session.findUserDBObjectById(object.getMaintenanceInitiatorId(), new Runnable() {
            @Override
            public void run()
            {
               getDisplay().asyncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     onObjectChange(); // will cause refresh of table content
                  }
               });
            }
         });
         addPair(i18n.tr("Maintenance initiator"), (user != null) ? user.getName() : "[" + object.getMaintenanceInitiatorId() + "]");
      }
		else
      {
         addPair(i18n.tr("Status"), StatusDisplayInfo.getStatusText(object.getStatus()));
      }

      switch(object.getObjectClass())
		{
         case AbstractObject.OBJECT_ACCESSPOINT:
            AccessPoint ap = (AccessPoint)object;
            addPair(i18n.tr("State"), ap.getState().toString());
            break;
         case AbstractObject.OBJECT_ASSET:
            Asset asset = (Asset)object;
            if ((asset.getLastUpdateTimestamp() != null) && (asset.getLastUpdateTimestamp().getTime() != 0))
            {
               addPair(i18n.tr("Last updated"), DateFormatFactory.getDateTimeFormat().format(asset.getLastUpdateTimestamp()));
               AbstractUserObject user = session.findUserDBObjectById(asset.getLastUpdateUserId(), () ->
                  getDisplay().asyncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        onObjectChange(); // will cause refresh of table content
                     }
                  })
               );
               addPair(i18n.tr("Updated by"), (user != null) ? user.getName() : "[" + asset.getLastUpdateUserId() + "]");
            }
            break;
         case AbstractObject.OBJECT_BUSINESSSERVICE:
            BusinessService businessService = (BusinessService)object;
            addPair(i18n.tr("Service state"), businessService.getServiceState().toString());
            break;
         case AbstractObject.OBJECT_INTERFACE:
            Interface iface = (Interface)object;
            addPair(i18n.tr("Administrative state"), iface.getAdminStateAsText());
            addPair(i18n.tr("Operational state"), iface.getOperStateAsText());
            addPair(i18n.tr("Expected state"), ifaceExpectedState[iface.getExpectedState()]);
				if (iface.isPhysicalPort())
				{
					AbstractNode node = iface.getParentNode();
               if (node != null)
					{
                  if (node.isBridge() && (iface.getStpPortState() != SpanningTreePortState.UNKNOWN))
                  {
                     addPair(i18n.tr("Spanning Tree port state"), iface.getStpPortState().getText());
                  }
                  if (node.is8021xSupported())
                  {
                     addPair(i18n.tr("802.1x PAE state"), iface.getDot1xPaeStateAsText());
                     addPair(i18n.tr("802.1x backend state"), iface.getDot1xBackendStateAsText());
                  }
					}
				}
            if (iface.isOSPF())
            {
               addPair(i18n.tr("OSPF interface state"), iface.getOSPFState().getText(), false);
            }
            if (iface.getMtu() > 0)
               addPair(i18n.tr("MTU"), Integer.toString(iface.getMtu()));
            if (iface.getSpeed() > 0)
               addPair(i18n.tr("Speed"), InterfaceListLabelProvider.ifSpeedTotext(iface.getSpeed()));
            if (iface.getInboundUtilization() >= 0)
               addPair(i18n.tr("Inbound utilization"), formatUtilizationValue(iface.getInboundUtilization()));
            if (iface.getOutboundUtilization() >= 0)
               addPair(i18n.tr("Outbound utilization"), formatUtilizationValue(iface.getOutboundUtilization()));
				break;
			case AbstractObject.OBJECT_NODE:
				AbstractNode node = (AbstractNode)object;
            if ((node.getCapabilities() & AbstractNode.NC_IS_ETHERNET_IP) != 0)
            {
               addPair(i18n.tr("Device state"), ((AbstractNode)object).getCipStateText(), false);
               addPair(i18n.tr("Device status"), ((AbstractNode)object).getCipStatusText(), false);
               if (((((AbstractNode)object).getCipStatus() >> 4) & 0x0F) != 0)
                  addPair(i18n.tr("Extended device status"), ((AbstractNode)object).getCipExtendedStatusText(), false);
            }
            if (node.getBootTime() != null)
               addPair(i18n.tr("Boot time"), DateFormatFactory.getDateTimeFormat().format(node.getBootTime()), false);
            if (node.hasAgent())
               addPair(i18n.tr("Agent status"), (node.getStateFlags() & Node.NSF_AGENT_UNREACHABLE) != 0 ? i18n.tr("Unreachable") : i18n.tr("Connected"));
            if (node.getLastAgentCommTime() != null)
               addPair(i18n.tr("Last agent contact"), DateFormatFactory.getDateTimeFormat().format(node.getLastAgentCommTime()), false);
				break;
			case AbstractObject.OBJECT_MOBILEDEVICE:
				MobileDevice md = (MobileDevice)object;
				if (md.getLastReportTime().getTime() == 0)
               addPair(i18n.tr("Last report"), i18n.tr("Never"));
				else
               addPair(i18n.tr("Last report"), DateFormatFactory.getDateTimeFormat().format(md.getLastReportTime()));
				if (md.getBatteryLevel() >= 0)
               addPair(i18n.tr("Battery level"), Integer.toString(md.getBatteryLevel()) + "%"); //$NON-NLS-1$
				break;
			default:
				break;
		}
	}

   /**
    * Format interface utilization value.
    *
    * @param v interface utilization value
    * @return formatted string
    */
   private static String formatUtilizationValue(int v)
   {
      return Integer.toString(v / 10) + "." + Integer.toString(v % 10) + "%";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#getTitle()
    */
	@Override
	protected String getTitle()
	{
      return i18n.tr("State");
	}
}

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
import org.netxms.client.maps.MapType;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Asset;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.NetworkMap;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Zone;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.xnap.commons.i18n.I18n;

/**
 * "General" element on object overview tab
 */
public class ObjectInfo extends TableElement
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectInfo.class);

	/**
    * @param parent
    * @param anchor
    * @param objectView
    */
   public ObjectInfo(Composite parent, OverviewPageElement anchor, ObjectView objectView)
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

      addPair(i18n.tr("ID"), Long.toString(object.getObjectId()));
		if (object.getGuid() != null)
         addPair(i18n.tr("GUID"), object.getGuid().toString());
      addPair(i18n.tr("Class"), object.getObjectClassName());
      addPair(i18n.tr("Alias"), object.getAlias(), false);
      if (object.getCreationTime() != null && object.getCreationTime().getTime() != 0)
         addPair(i18n.tr("Creation time"), DateFormatFactory.getDateTimeFormat().format(object.getCreationTime()), false);		
      if (object.getCategory() != null)
         addPair(i18n.tr("Category"), object.getCategory().getName());
      if ((object instanceof Dashboard) && ((Dashboard)object).isTemplateInstance())
         addPair(i18n.tr("Instance"), i18n.tr("Yes"));
      if (object.getAssetId() != 0)
      {
         AbstractObject asset = session.findObjectById(object.getAssetId(), Asset.class);
         if (asset != null)
         {
            addPair(i18n.tr("Asset"), asset.getObjectNameWithPath(), false);
         }
      }

      switch(object.getObjectClass())
		{
         case AbstractObject.OBJECT_ASSET:
            Asset asset = (Asset)object;
            if (asset.getLinkedObjectId() != 0)
               addPair(i18n.tr("Linked to"), session.getObjectNameWithAlias(asset.getLinkedObjectId()));
            break;
         case AbstractObject.OBJECT_CLUSTER:
            Cluster cluster = (Cluster)object;
            if (session.isZoningEnabled())
               addPair(i18n.tr("Zone UIN"), getZoneName(cluster.getZoneId()));
            break;
         case AbstractObject.OBJECT_NETWORKMAP:
            NetworkMap map = (NetworkMap)object;
            addPair(i18n.tr("Map type"), getMapTypeDescription(map.getMapType()));
            break;
			case AbstractObject.OBJECT_SUBNET:
				Subnet subnet = (Subnet)object;
				if (session.isZoningEnabled())
               addPair(i18n.tr("Zone UIN"), getZoneName(subnet.getZoneId()));
            addPair(i18n.tr("IP address"), subnet.getNetworkAddress().toString());
				break;
			case AbstractObject.OBJECT_ZONE:
				Zone zone = (Zone)object;
            addPair(i18n.tr("Zone UIN"), Long.toString(zone.getUIN()));
				break;
			default:
				break;
		}
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#getTitle()
    */
	@Override
	protected String getTitle()
	{
      return i18n.tr("Object");
	}

	/**
	 * Get zone name
	 * 
	 * @param zoneId
	 * @return
	 */
	private String getZoneName(int zoneId)
	{
      Zone zone = Registry.getSession().findZone(zoneId);
	   if (zone == null)
	      return Long.toString(zoneId);
	   return String.format("%d (%s)", zoneId, zone.getObjectName());
	}

   /**
    * Get textual description for given map type.
    *
    * @param mapType map type code
    * @return textual description for given map type
    */
   private String getMapTypeDescription(MapType mapType)
   {
      switch(mapType)
      {
         case CUSTOM:
            return i18n.tr("Custom");
         case HYBRID_TOPOLOGY:
            return i18n.tr("Hybrid topology");
         case INTERNAL_TOPOLOGY:
            return i18n.tr("Internal topology");
         case IP_TOPOLOGY:
            return i18n.tr("IP topology");
         case LAYER2_TOPOLOGY:
            return i18n.tr("L2 topology");
         case OSPF_TOPOLOGY:
            return i18n.tr("OSPF topology");
      }
      return i18n.tr("Unknown ({0})", Integer.toString(mapType.getValue()));
   }
}

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

import org.eclipse.swt.widgets.Composite;
import org.netxms.base.GeoLocation;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.interfaces.HardwareEntity;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.xnap.commons.i18n.I18n;

/**
 * "Location" element on object overview tab
 */
public class Location extends TableElement
{
   private final I18n i18n = LocalizationHelper.getI18n(Location.class);

	/**
    * @param parent
    * @param anchor
    * @param objectView
    */
   public Location(Composite parent, OverviewPageElement anchor, ObjectView objectView)
	{
      super(parent, anchor, objectView);
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean isApplicableForObject(AbstractObject object)
   {
      return (object.getGeolocation().getType() != GeoLocation.UNSET) || !object.getPostalAddress().isEmpty() ||
            ((object instanceof HardwareEntity) && (((HardwareEntity)object).getPhysicalContainerId() != 0));
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.TableElement#fillTable()
    */
	@Override
	protected void fillTable()
	{
		final AbstractObject object = getObject();
		if (object.getGeolocation().getType() != GeoLocation.UNSET)
      {
         addPair(i18n.tr("Coordinates"), object.getGeolocation().toString());
         addPair(i18n.tr("Source"), getLocationSource(object.getGeolocation().getType()));
         if (object.getGeolocation().getType() != GeoLocation.MANUAL)
         {
            addPair(i18n.tr("Accuracy"), object.getGeolocation().getAccuracy() + " m");
         }
         if (object instanceof MobileDevice)
         {
            MobileDevice md = (MobileDevice)object;
            if (md.getSpeed() >= 0)
               addPair(i18n.tr("Speed"), Double.toString(md.getSpeed()) + " km/h");
            if (md.getDirection() >= 0)
               addPair(i18n.tr("Direction"), Integer.toString(md.getDirection()) + "\u00b0");
            addPair(i18n.tr("Altitude"), Integer.toString(md.getAltitude()) + " m");
         }
      }
      if (!object.getPostalAddress().isEmpty())
      {
         addPair(i18n.tr("Postal Address"), object.getPostalAddress().getAddressLine());
      }
      if ((object instanceof HardwareEntity) && (((HardwareEntity)object).getPhysicalContainerId() != 0))
      {
         HardwareEntity he = ((HardwareEntity)object);
         Rack rack = Registry.getSession().findObjectById(he.getPhysicalContainerId(), Rack.class);
         if (rack != null)
         {
            addPair(i18n.tr("Rack"),
                  String.format(i18n.tr("%s (units %d-%d)"), rack.getObjectName(), rack.isTopBottomNumbering() ? he.getRackPosition() : he.getRackPosition() - he.getRackHeight() + 1,
                        rack.isTopBottomNumbering() ? he.getRackPosition() + he.getRackHeight() - 1 : he.getRackPosition()));
         }
      }
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#getTitle()
    */
	@Override
	protected String getTitle()
	{
      return i18n.tr("Location");
	}

   /**
    * Get geolocation source name from code.
    *
    * @param source source code
    * @return source name
    */
   private String getLocationSource(int source)
   {
      switch(source)
      {
         case GeoLocation.GPS:
            return "GPS";
         case GeoLocation.MANUAL:
            return i18n.tr("Manual");
         case GeoLocation.NETWORK:
            return i18n.tr("Network");
         default:
            return i18n.tr("Other");
      }
   }
}

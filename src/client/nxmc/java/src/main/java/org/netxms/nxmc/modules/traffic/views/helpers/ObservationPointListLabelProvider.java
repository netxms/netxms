/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.traffic.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.objects.ObservationPoint;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.traffic.views.TrafficObserverView;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for observation point list
 */
public class ObservationPointListLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(ObservationPointListLabelProvider.class);

   /**
    * Get text for observation point state.
    *
    * @param state state code
    * @return display text
    */
   public String getStateText(int state)
   {
      switch(state)
      {
         case ObservationPoint.STATE_ACTIVE:
            return i18n.tr("Active");
         case ObservationPoint.STATE_INACTIVE:
            return i18n.tr("Inactive");
         default:
            return i18n.tr("Unknown");
      }
   }

   /**
    * Get text for sampling rate.
    *
    * @param samplingRate sampling rate value
    * @return display text
    */
   public String getSamplingRateText(int samplingRate)
   {
      if (samplingRate == 0)
         return "";
      if (samplingRate == 1)
         return i18n.tr("unsampled");
      return "1:" + Integer.toString(samplingRate);
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      ObservationPoint point = (ObservationPoint)element;
      switch(columnIndex)
      {
         case TrafficObserverView.COLUMN_ID:
            return Long.toString(point.getObjectId());
         case TrafficObserverView.COLUMN_NAME:
            return point.getObjectName();
         case TrafficObserverView.COLUMN_EXTERNAL_ID:
            return point.getExternalId();
         case TrafficObserverView.COLUMN_TYPE:
            return point.getPointType();
         case TrafficObserverView.COLUMN_STATE:
            return getStateText(point.getState());
         case TrafficObserverView.COLUMN_PROVIDER_STATE:
            return point.getProviderState();
         case TrafficObserverView.COLUMN_SAMPLING_RATE:
            return getSamplingRateText(point.getSamplingRate());
         case TrafficObserverView.COLUMN_IN_SCOPE:
            return point.isInScope() ? i18n.tr("Yes") : i18n.tr("No");
         case TrafficObserverView.COLUMN_LAST_DISCOVERY:
            return (point.getLastDiscoveryTime() != null) && (point.getLastDiscoveryTime().getTime() > 0)
                  ? DateFormatFactory.getDateTimeFormat().format(point.getLastDiscoveryTime()) : "";
         case TrafficObserverView.COLUMN_STATUS:
            return StatusDisplayInfo.getStatusText(point.getStatus());
      }
      return null;
   }
}

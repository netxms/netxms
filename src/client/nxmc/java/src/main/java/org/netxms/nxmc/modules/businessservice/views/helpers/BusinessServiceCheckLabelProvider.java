/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc.modules.businessservice.views.helpers;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.viewers.ITableColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.businessservices.BusinessServiceCheck;
import org.netxms.client.constants.BusinessServiceCheckType;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.datacollection.DciInfo;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.interfaces.NodeComponent;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.businessservice.views.BusinessServiceChecksView;
import org.netxms.nxmc.modules.objects.widgets.helpers.BaseObjectLabelProvider;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.resources.ThemeEngine;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for interface list
 */
public class BusinessServiceCheckLabelProvider extends LabelProvider implements ITableLabelProvider, ITableColorProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(BusinessServiceCheckLabelProvider.class);
   private final String[] TYPES = { i18n.tr("None"), i18n.tr("Script"), i18n.tr("DCI threshold"), i18n.tr("Object status"), };

   private Color prototypeColor = ThemeEngine.getForegroundColor("List.DisabledItem");
   private NXCSession session = Registry.getSession();
   private Map<Long, DciInfo> dciNameCache = new HashMap<Long, DciInfo>();
   private BaseObjectLabelProvider objectLabelProvider = new BaseObjectLabelProvider();

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
   {
      if (columnIndex != BusinessServiceChecksView.COLUMN_OBJECT)
         return null;

      BusinessServiceCheck check = (BusinessServiceCheck)element;
      AbstractObject object = session.findObjectById(check.getObjectId());
      return (object != null) ? objectLabelProvider.getImage(object) : null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{		
		BusinessServiceCheck check = (BusinessServiceCheck)element;
		switch(columnIndex)
		{
         case BusinessServiceChecksView.COLUMN_ID:
				return Long.toString(check.getId());
         case BusinessServiceChecksView.COLUMN_DESCRIPTION:
            return check.getDescription();
         case BusinessServiceChecksView.COLUMN_TYPE:
            return getTypeName(check);  
         case BusinessServiceChecksView.COLUMN_OBJECT:
            return getObjectName(check);  
         case BusinessServiceChecksView.COLUMN_DCI:
            return getDciName(check);  
         case BusinessServiceChecksView.COLUMN_STATUS:
            return getCheckStateText(check);
         case BusinessServiceChecksView.COLUMN_FAIL_REASON:
            return check.getFailureReason();
         case BusinessServiceChecksView.COLUMN_ORIGIN:
            return getOriginName(check);
		}
		return null;
	}

   /**
    * Get text representation of check's state.
    *
    * @param check check to get state from
    * @return state text
    */
   public String getCheckStateText(BusinessServiceCheck check)
   {
      switch(check.getState())
      {
         case OPERATIONAL:
            return i18n.tr("OK");
         case DEGRADED:
            return i18n.tr("Degraded");
         case FAILED:
            return i18n.tr("Failed");
      }
      return null;
   }

   /**
    * Get name of related object (if any).
    *
    * @param check service check to get object name from
    * @return name of related object or empty string
    */
	public String getObjectName(BusinessServiceCheck check)
	{
	   if (check.getObjectId() == 0)
         return "";

	   StringBuilder name = new StringBuilder();
      AbstractObject object = session.findObjectById(check.getObjectId());
      if (object != null)
      {
         name.append(object.getObjectName());
         if (object instanceof NodeComponent)
         {
            name.append(" @ ");
            AbstractNode node = ((NodeComponent)object).getParentNode();
            name.append(node.getObjectName());
         }
      }
      else
      {
         name.append("[");
         name.append(Long.toString(check.getObjectId()));
         name.append("]");
      }
	   return name.toString();
	}

   /**
    * Get name of source object (if any).
    *
    * @param check service check to get object name from
    * @return name of source object or empty string
    */
   public String getOriginName(BusinessServiceCheck check)
   {
      if (check.getPrototypeServiceId() == 0)
         return "";

      StringBuilder name = new StringBuilder();
      AbstractObject object = session.findObjectById(check.getPrototypeServiceId());
      if (object != null)
      {
         name.append(object.getObjectName());
      }
      else
      {
         name.append("[");
         name.append(Long.toString(check.getObjectId()));
         name.append("]");
      }

      return name.toString();
   }

   /**
    * Get name of related DCI (if any).
    *
    * @param check service check to get DCI name from
    * @return name of related DCI or empty string
    */
   public String getDciName(BusinessServiceCheck check)
   {
      if ((check.getCheckType() != BusinessServiceCheckType.DCI) || (check.getDciId() == 0))
         return "";

      String name = dciNameCache.get(check.getDciId()).displayName;
      return (name != null) ? name : ("[" + Long.toString(check.getDciId()) + "]");
   }

   /**
    * Check type text name
    * @param check check object
    * @return cehck type name
    */
   public String getTypeName(BusinessServiceCheck check)
   {
      return TYPES[check.getCheckType().getValue()];
   }

   /**
    * @see org.eclipse.jface.viewers.ITableColorProvider#getForeground(java.lang.Object, int)
    */
	@Override
	public Color getForeground(Object element, int columnIndex)
	{
      if (columnIndex != BusinessServiceChecksView.COLUMN_STATUS)
         return null;

      switch(((BusinessServiceCheck)element).getState())
      {
         case OPERATIONAL:
            return StatusDisplayInfo.getStatusColor(ObjectStatus.NORMAL);
         case DEGRADED:
            return StatusDisplayInfo.getStatusColor(ObjectStatus.MINOR);
         case FAILED:
            return StatusDisplayInfo.getStatusColor(ObjectStatus.CRITICAL);
      }
      return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableColorProvider#getBackground(java.lang.Object, int)
    */
	@Override
	public Color getBackground(Object element, int columnIndex)
	{
	   if (((BusinessServiceCheck)element).getPrototypeServiceId() != 0)
	   {
	      return prototypeColor;
	   }
		return null;
	}

   /**
    * Update cached DCI names
    * 
    * @param names set of updated names
    */
   public void updateDciNames(Map<Long, DciInfo> names)
   {
      dciNameCache.putAll(names);
   }

   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      objectLabelProvider.dispose();
      super.dispose();
   }
}

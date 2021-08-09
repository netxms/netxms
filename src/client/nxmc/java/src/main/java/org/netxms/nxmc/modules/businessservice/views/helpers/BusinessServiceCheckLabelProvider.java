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
package org.netxms.nxmc.modules.businessservice.views.helpers;

import org.eclipse.jface.viewers.ITableColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.businessservices.ServiceCheck;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.businessservice.views.BusinessServiceChecksView;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for interface list
 */
public class BusinessServiceCheckLabelProvider extends LabelProvider implements ITableLabelProvider, ITableColorProvider
{
   private static final I18n i18n = LocalizationHelper.getI18n(BusinessServiceCheckLabelProvider.class);
   private static final String[] TYPES = {i18n.tr("Node"), i18n.tr("Script"), i18n.tr("DCI")};
	
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
		ServiceCheck check = (ServiceCheck)element;
		switch(columnIndex)
		{
         case BusinessServiceChecksView.COLUMN_ID:				
				return Long.toString(check.getId());
         case BusinessServiceChecksView.COLUMN_DESCRIPTION:    
            return check.getDescription();
         case BusinessServiceChecksView.COLUMN_TYPE:  
            return getTypeName(check);  
         case BusinessServiceChecksView.COLUMN_STATUS:    
            return getViolationStatus(check);  
         case BusinessServiceChecksView.COLUMN_FAIL_REASON:    
            return check.getFailureReason();
		}
		return null;
	}
	
	/**
	 * Get violation status string
	 * 
	 * @return violation status text
	 */
	public String getViolationStatus(ServiceCheck check)
	{
	   return check.isViolated() ? i18n.tr("Violated") : i18n.tr("Normal");
	}
   
   /**
    * Check type text name
    * @param check check object
    * @return cehck type name
    */
   public String getTypeName(ServiceCheck check)
   {
      return TYPES[check.getCheckType()];
   }

   /**
    * @see org.eclipse.jface.viewers.ITableColorProvider#getForeground(java.lang.Object, int)
    */
	@Override
	public Color getForeground(Object element, int columnIndex)
	{
      ServiceCheck check = (ServiceCheck)element;
		switch(columnIndex)
		{
         case BusinessServiceChecksView.COLUMN_STATUS:    
            return check.isViolated() ? StatusDisplayInfo.getStatusColor(ObjectStatus.MAJOR) : StatusDisplayInfo.getStatusColor(ObjectStatus.NORMAL);  
			default:
				return null;
		}
	}

   /**
    * @see org.eclipse.jface.viewers.ITableColorProvider#getBackground(java.lang.Object, int)
    */
	@Override
	public Color getBackground(Object element, int columnIndex)
	{
		return null;
	}
}

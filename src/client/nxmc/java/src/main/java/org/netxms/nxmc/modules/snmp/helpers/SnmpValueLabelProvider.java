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
package org.netxms.nxmc.modules.snmp.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.snmp.MibObject;
import org.netxms.client.snmp.SnmpValue;
import org.netxms.nxmc.modules.snmp.SnmpConstants;
import org.netxms.nxmc.modules.snmp.shared.MibCache;
import org.netxms.nxmc.modules.snmp.views.MibExplorer;

/**
 * Label provider for SNMP values
 */
public class SnmpValueLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private boolean shortTextualNames = false;

   /**
    * @param shortTextualNames the shortTextualNames to set
    */
   public void setShortTextualNames(boolean shortTextualNames)
   {
      this.shortTextualNames = shortTextualNames;
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
		SnmpValue value = (SnmpValue)element;
		switch(columnIndex)
		{
			case MibExplorer.COLUMN_NAME:
				return value.getName();
			case MibExplorer.COLUMN_TEXT:
			   MibObject object = MibCache.findObject(value.getName(), false);
			   if (object == null)
			      return "";
            return shortTextualNames ? object.getName() : object.getFullName();
			case MibExplorer.COLUMN_TYPE:
				return SnmpConstants.getAsnTypeName(value.getType());
			case MibExplorer.COLUMN_VALUE:
				return value.getValue();
         case MibExplorer.COLUMN_RAW_VALUE:
            return byteArraytoString(value.getRawValue());
		}
		return null;
	}

   /**
    * Convert byte array to string form
    *
    * @param bytes input byte array
    * @return string representation of given byte array
    */
   private static String byteArraytoString(byte[] bytes)
   {
      if (bytes.length == 0)
         return "";

      StringBuilder sb = new StringBuilder();
      int ub = (int)bytes[0] & 0xFF;
      if (ub < 0x10)
         sb.append('0');
      sb.append(Integer.toHexString(ub));
      for(int i = 1; i < bytes.length; i++)
      {
         sb.append(' ');
         ub = (int)bytes[i] & 0xFF;
         if (ub < 0x10)
            sb.append('0');
         sb.append(Integer.toHexString(ub));
      }
      return sb.toString().toUpperCase();
   }
}

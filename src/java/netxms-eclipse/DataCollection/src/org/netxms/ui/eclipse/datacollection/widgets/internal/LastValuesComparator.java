/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.widgets.internal;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciValue;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.widgets.LastValuesWidget;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for "Last Values" widget
 */
public class LastValuesComparator extends ViewerComparator
{
   private boolean showErrors = true;

   /* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;
		
		DciValue v1 = (DciValue)e1;
		DciValue v2 = (DciValue)e2;

		switch((Integer)((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
			case LastValuesWidget.COLUMN_ID:
				result = (int)(v1.getId() - v2.getId());
				break;
			case LastValuesWidget.COLUMN_DESCRIPTION:
				result = v1.getDescription().compareToIgnoreCase(v2.getDescription());
				break;
			case LastValuesWidget.COLUMN_VALUE:
				result = compareValue(v1, v2);
				break;
			case LastValuesWidget.COLUMN_TIMESTAMP:
				result = v1.getTimestamp().compareTo(v2.getTimestamp());
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
	
	/**
	 * Compare DCI values taking data type into consideration
	 * 
	 * @param v1
	 * @param v2
	 * @return
	 */
	private int compareValue(DciValue dci1, DciValue dci2)
	{
	   int dt1, dt2;
	   String v1, v2;

	   if (showErrors && (dci1.getErrorCount() > 0))
	   {
	      dt1 = DataCollectionObject.DT_STRING;
	      v1 = Messages.get().LastValuesLabelProvider_Error;
	   }
	   else if (dci1.getDcObjectType() == DataCollectionObject.DCO_TYPE_TABLE)
	   {
         dt1 = DataCollectionObject.DT_STRING;
         v1 = Messages.get().LastValuesLabelProvider_Table;
	   }
	   else
	   {
	      dt1 = dci1.getDataType();
	      v1 = dci1.getValue();
	   }
	   
      if (showErrors && (dci2.getErrorCount() > 0))
      {
         dt2 = DataCollectionObject.DT_STRING;
         v2 = Messages.get().LastValuesLabelProvider_Error;
      }
      else if (dci2.getDcObjectType() == DataCollectionObject.DCO_TYPE_TABLE)
      {
         dt2 = DataCollectionObject.DT_STRING;
         v2 = Messages.get().LastValuesLabelProvider_Table;
      }
      else
      {
         dt2 = dci2.getDataType();
         v2 = dci2.getValue();
      }
      
	   int dataType = Math.max(dt1, dt2);
	   try
	   {
   	   switch(dataType)
   	   {
   	      case DataCollectionObject.DT_INT:
   	         return Integer.signum(Integer.parseInt(v1) - Integer.parseInt(v2));
            case DataCollectionObject.DT_UINT:
            case DataCollectionObject.DT_INT64:
            case DataCollectionObject.DT_UINT64:
               return Long.signum(Long.parseLong(v1) - Long.parseLong(v2));
            case DataCollectionObject.DT_FLOAT:
               // Maybe combination of String/Float because DT_FLOAT > DT_STRING
               if ((dt1 == DataCollectionObject.DT_STRING) || (dt2 == DataCollectionObject.DT_STRING))
                  return v1.compareToIgnoreCase(v2);
               return (int)Math.signum(Double.parseDouble(v1) - Double.parseDouble(v2));
   	      default:
   	         return v1.compareToIgnoreCase(v2);
   	   }
	   }
	   catch(NumberFormatException e)
	   {
	      return v1.compareToIgnoreCase(v2);
	   }
	}

   /**
    * @return the showErrors
    */
   public boolean isShowErrors()
   {
      return showErrors;
   }

   /**
    * @param showErrors the showErrors to set
    */
   public void setShowErrors(boolean showErrors)
   {
      this.showErrors = showErrors;
   }
}

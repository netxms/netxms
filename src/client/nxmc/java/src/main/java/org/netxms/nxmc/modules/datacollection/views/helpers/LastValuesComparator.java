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
package org.netxms.nxmc.modules.datacollection.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.DataType;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.views.BaseDataCollectionView;
import org.xnap.commons.i18n.I18n;

/**
 * Comparator for "Last Values" widget
 */
public class LastValuesComparator extends ViewerComparator
{
   private I18n i18n = LocalizationHelper.getI18n(LastValuesComparator.class);

   private NXCSession session = Registry.getSession();
   private boolean showErrors = true;

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
	   TableColumn sortColumn = ((SortableTableViewer)viewer).getTable().getSortColumn();
      if (sortColumn == null)
         return 0;

      DciValue v1 = (DciValue)e1;
      DciValue v2 = (DciValue)e2;

      int result = 0;
      switch((Integer)sortColumn.getData("ID"))
      {
         case BaseDataCollectionView.LV_COLUMN_COMMENTS:
            result = v1.getComments().compareToIgnoreCase(v2.getComments());
            break;
         case BaseDataCollectionView.LV_COLUMN_DESCRIPTION:
            result = v1.getDescription().compareToIgnoreCase(v2.getDescription());
            break;
         case BaseDataCollectionView.LV_COLUMN_EVENT:
            result = getEventName(v1).compareToIgnoreCase(getEventName(v2));
            break;
         case BaseDataCollectionView.LV_COLUMN_ID:
            result = Long.compare(v1.getId(), v2.getId());
            break;
         case BaseDataCollectionView.LV_COLUMN_MESSAGE:
            result = getEventMessage(v1).compareToIgnoreCase(getEventMessage(v2));
            break;
         case BaseDataCollectionView.LV_COLUMN_OWNER:
            AbstractObject obj1 = Registry.getSession().findObjectById(v1.getNodeId());
            AbstractObject obj2 = Registry.getSession().findObjectById(v2.getNodeId());
            String name1 = (obj1 != null) ? obj1.getObjectName() : "[" + Long.toString(v1.getNodeId()) + "]";
            String name2 = (obj2 != null) ? obj2.getObjectName() : "[" + Long.toString(v2.getNodeId()) + "]";
            result = name1.compareToIgnoreCase(name2);
            break;
         case BaseDataCollectionView.LV_COLUMN_TAG:
            result = v1.getUserTag().compareToIgnoreCase(v2.getUserTag());
            break;
         case BaseDataCollectionView.LV_COLUMN_TIMESTAMP:
            result = v1.getTimestamp().compareTo(v2.getTimestamp());
            break;
         case BaseDataCollectionView.LV_COLUMN_VALUE:
            result = compareValue(v1, v2);
            break;
         default:
            result = 0;
            break;
      }
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}

   /**
    * Get threshold activation event name.
    *
    * @param value DCI value
    * @return threshold activation event name or empty string
    */
   public String getEventName(DciValue value)
   {
      Threshold threshold = value.getActiveThreshold();
      if (threshold == null)
         return "";
      return session.getEventName(threshold.getFireEvent());
   }

   /**
    * Get threshold activation event message.
    *
    * @param value DCI value
    * @return threshold activation event message or empty string
    */
   public String getEventMessage(DciValue value)
   {
      Threshold threshold = value.getActiveThreshold();
      return (threshold != null) ? threshold.getLastEventMessage() : "";
   }

   /**
    * Helper to check if value is a valid number for the given data type
    */
   private boolean isValidNumber(String value, DataType type) {
      try {
         switch(type) {
            case INT32:
            case UINT32:
            case INT64:
            case UINT64:
            case COUNTER32:
            case COUNTER64:
               Long.parseLong(value);
               break;
            case FLOAT:
               Double.parseDouble(value);
               break;
            default:
               return false;
         }
         return true;
      } catch(Exception e) {
         return false;
      }
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
	   DataType dt1, dt2;
	   String v1, v2;

	   if (showErrors && (dci1.getErrorCount() > 0))
	   {
	      dt1 = DataType.STRING;
         v1 = i18n.tr("<< ERROR >>");
	   }
	   else if (dci1.getDcObjectType() == DataCollectionObject.DCO_TYPE_TABLE)
	   {
         dt1 = DataType.STRING;
         v1 = i18n.tr("<< TABLE >>");
	   }
	   else
	   {
	      dt1 = dci1.getDataType();
	      v1 = dci1.getValue();
	   }
	   
      if (showErrors && (dci2.getErrorCount() > 0))
      {
         dt2 = DataType.STRING;
         v2 = i18n.tr("<< ERROR >>");
      }
      else if (dci2.getDcObjectType() == DataCollectionObject.DCO_TYPE_TABLE)
      {
         dt2 = DataType.STRING;
         v2 = i18n.tr("<< TABLE >>");
      }
      else
      {
         dt2 = dci2.getDataType();
         v2 = dci2.getValue();
      }

      boolean v1IsNumber = isValidNumber(v1, dt1);
      boolean v2IsNumber = isValidNumber(v2, dt2);

      // If both are valid numbers, compare numerically
      if (v1IsNumber && v2IsNumber)
      {
         DataType dataType = DataType.getTypeForCompare(dt1, dt2);
         switch(dataType)
         {
            case INT32:
            case UINT32:
            case INT64:
            case UINT64:
            case COUNTER32:
            case COUNTER64:
               return Long.compare(Long.parseLong(v1), Long.parseLong(v2));
            case FLOAT:
               return Double.compare(Double.parseDouble(v1), Double.parseDouble(v2));
            default:
               return v1.compareToIgnoreCase(v2);
         }
      }
      // If only one is a valid number, always sort numbers before strings
      if (v1IsNumber && !v2IsNumber)
         return -1;
      if (!v1IsNumber && v2IsNumber)
         return 1;
      // If neither is a valid number, compare as strings
      return v1.compareToIgnoreCase(v2);
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

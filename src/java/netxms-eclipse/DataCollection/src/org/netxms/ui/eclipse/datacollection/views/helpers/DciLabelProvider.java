/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.ui.eclipse.datacollection.views.helpers;

import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.client.datacollection.TableThreshold;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Template;
import org.netxms.ui.eclipse.console.resources.DataCollectionDisplayInfo;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.views.DataCollectionEditor;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for user manager
 */
public class DciLabelProvider implements ITableLabelProvider, IColorProvider
{
   private static final Color FONT_COLOR = new Color(Display.getDefault(), new RGB(126, 137, 185));
   
	private NXCSession session;
	private Image statusImages[];
   private HashMap<DataOrigin, String> originTexts = new HashMap<DataOrigin, String>();
	private HashMap<Integer, String> statusTexts = new HashMap<Integer, String>();
	
	/**
	 * Default constructor
	 */
	public DciLabelProvider()
	{
		session = (NXCSession)ConsoleSharedData.getSession();
		
		statusImages = new Image[3];
		statusImages[DataCollectionItem.ACTIVE] = Activator.getImageDescriptor("icons/active.gif").createImage(); //$NON-NLS-1$
		statusImages[DataCollectionItem.DISABLED] = Activator.getImageDescriptor("icons/disabled.gif").createImage(); //$NON-NLS-1$
		statusImages[DataCollectionItem.NOT_SUPPORTED] = Activator.getImageDescriptor("icons/unsupported.gif").createImage(); //$NON-NLS-1$
		
      originTexts.put(DataOrigin.AGENT, Messages.get().DciLabelProvider_SourceAgent);
      originTexts.put(DataOrigin.DEVICE_DRIVER, Messages.get().DciLabelProvider_SourceDeviceDriver);
      originTexts.put(DataOrigin.INTERNAL, Messages.get().DciLabelProvider_SourceInternal);
      originTexts.put(DataOrigin.MODBUS, "Modbus");
      originTexts.put(DataOrigin.MQTT, Messages.get().DciLabelProvider_SourceMQTT);
      originTexts.put(DataOrigin.PUSH, Messages.get().DciLabelProvider_SourcePush);
      originTexts.put(DataOrigin.SCRIPT, Messages.get().DciLabelProvider_SourceScript);
      originTexts.put(DataOrigin.SMCLP, Messages.get().DciLabelProvider_SourceILO);
      originTexts.put(DataOrigin.SNMP, Messages.get().DciLabelProvider_SourceSNMP);
      originTexts.put(DataOrigin.SSH, Messages.get().DciLabelProvider_SourceSSH);
      originTexts.put(DataOrigin.WEB_SERVICE, Messages.get().DciLabelProvider_SourceWebService);
      originTexts.put(DataOrigin.WINPERF, Messages.get().DciLabelProvider_SourceWinPerf);

		statusTexts.put(DataCollectionItem.ACTIVE, Messages.get().DciLabelProvider_Active);
		statusTexts.put(DataCollectionItem.DISABLED, Messages.get().DciLabelProvider_Disabled);
		statusTexts.put(DataCollectionItem.NOT_SUPPORTED, Messages.get().DciLabelProvider_NotSupported);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		if (columnIndex != 0)
			return null;
		int status = ((DataCollectionObject)element).getStatus();
		return ((status >= 0) && (status < statusImages.length)) ? statusImages[status] : null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		DataCollectionObject dci = (DataCollectionObject)element;
		switch(columnIndex)
		{
			case DataCollectionEditor.COLUMN_ID:
				return Long.toString(dci.getId());
			case DataCollectionEditor.COLUMN_ORIGIN:
				return originTexts.get(dci.getOrigin());
			case DataCollectionEditor.COLUMN_DESCRIPTION:
				return dci.getDescription();
			case DataCollectionEditor.COLUMN_PARAMETER:
				return dci.getName();
			case DataCollectionEditor.COLUMN_DATATYPE:
				if (dci instanceof DataCollectionItem)
					return DataCollectionDisplayInfo.getDataTypeName(((DataCollectionItem)dci).getDataType());
				return Messages.get().DciLabelProvider_Table;
			case DataCollectionEditor.COLUMN_DATAUNIT:
            if((dci instanceof DataCollectionItem))
            {
               String unitName = ((DataCollectionItem)dci).getUnitName();
               return unitName == null ? "" : unitName;
            }
            return "";
			case DataCollectionEditor.COLUMN_INTERVAL:
				if (dci.isUseAdvancedSchedule())
					return Messages.get().DciLabelProvider_CustomSchedule;
				if (dci.getPollingScheduleType() == DataCollectionObject.POLLING_SCHEDULE_DEFAULT)
				   return Messages.get().DciLabelProvider_Default;
				return dci.getPollingInterval();
			case DataCollectionEditor.COLUMN_RETENTION:
			   if (dci.getRetentionType() == DataCollectionObject.RETENTION_NONE)
			      return Messages.get().DciLabelProvider_None;
			   if (dci.getRetentionType() == DataCollectionObject.RETENTION_DEFAULT)
               return Messages.get().DciLabelProvider_Default;
			   try
			   {
   				int days = Integer.parseInt(dci.getRetentionTime());
   				return Integer.toString(days) + ((days == 1) ? Messages.get().DciLabelProvider_Day : Messages.get().DciLabelProvider_Days);
			   }
			   catch(NumberFormatException e)
			   {
			      return dci.getRetentionTime();
			   }
			case DataCollectionEditor.COLUMN_STATUS:
				return statusTexts.get(dci.getStatus());
			case DataCollectionEditor.COLUMN_THRESHOLD:
			   StringBuilder thresholds = new StringBuilder();
			   if((dci instanceof DataCollectionItem))
			   {
               List<Threshold> list = ((DataCollectionItem)dci).getThresholds();
               for(int i = 0; i < list.size(); i++)
               {
                  Threshold tr = list.get(i);
                  thresholds.append(tr.getTextualRepresentation());
                  if (i < list.size() - 1)
                     thresholds.append(", "); //$NON-NLS-1$
               }
   			   
			   }
			   if ((dci instanceof DataCollectionTable))
			   {
			      List<TableThreshold> list = ((DataCollectionTable)dci).getThresholds();
               for(int i = 0; i < list.size(); i++)
               {
                  thresholds.append(list.get(i).getConditionAsText());
                  if( i+1 != list.size() )
                     thresholds.append(", "); //$NON-NLS-1$
               }
			   }
			   return thresholds.toString();
			case DataCollectionEditor.COLUMN_TEMPLATE:
				if (dci.getTemplateId() == 0)
					return null;
				AbstractObject object = session.findObjectById(dci.getTemplateId());
				if (object == null)
				   return Messages.get().DciLabelProvider_Unknown;
				if (!(object instanceof Template))
				   return object.getObjectName();
            List<AbstractObject> parents = object.getParentChain(null);
            Collections.reverse(parents);
				StringBuilder sb = new StringBuilder();
            for(AbstractObject parent : parents)
				{
				   sb.append(parent.getObjectName());
				   sb.append("/"); //$NON-NLS-1$
				}
				sb.append(object.getObjectName());
				return sb.toString();
			case DataCollectionEditor.COLUMN_RELATEDOBJ:
            if (dci.getRelatedObject() == 0)
               return null;
            AbstractObject obj = session.findObjectById(dci.getRelatedObject());
            if (obj == null)
               return "[" + dci.getRelatedObject() + "]";
            return obj.getObjectName();
         case DataCollectionEditor.COLUMN_STATUSCALC:
            if (dci instanceof DataCollectionItem)
               return ((DataCollectionItem)dci).isUsedForNodeStatusCalculation() ? "Yes" : "No";
            return "No";
		}
		return null;
	}
	
	

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#addListener(org.eclipse.jface.viewers.ILabelProviderListener)
	 */
	@Override
	public void addListener(ILabelProviderListener listener)
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		for(int i = 0; i < statusImages.length; i++)
			statusImages[i].dispose();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#isLabelProperty(java.lang.Object, java.lang.String)
	 */
	@Override
	public boolean isLabelProperty(Object element, String property)
	{
		return false;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#removeListener(org.eclipse.jface.viewers.ILabelProviderListener)
	 */
	@Override
	public void removeListener(ILabelProviderListener listener)
	{
	}

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
    */
   @Override
   public Color getForeground(Object element)
   {
      if (((DataCollectionObject)element).getTemplateId() != 0)
      {
         return FONT_COLOR;
      }
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
    */
   @Override
   public Color getBackground(Object element)
   {
      return null;
   }
}
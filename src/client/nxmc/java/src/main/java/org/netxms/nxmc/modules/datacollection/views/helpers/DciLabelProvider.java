/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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

import java.util.HashMap;
import java.util.List;
import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.DataCollectionObjectStatus;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.client.datacollection.TableThreshold;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Template;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.views.DataCollectionView;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DataCollectionDisplayInfo;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for user manager
 */
public class DciLabelProvider extends LabelProvider implements ITableLabelProvider, IColorProvider
{
   private I18n i18n = LocalizationHelper.getI18n(DciLabelProvider.class);
   private static final Color COLOR_TEMPLATE_ITEM = new Color(Display.getDefault(), new RGB(126, 137, 185));

	private NXCSession session;
	private Image statusImages[];
   private HashMap<DataOrigin, String> originTexts = new HashMap<>();
   private HashMap<DataCollectionObjectStatus, String> statusTexts = new HashMap<>();

	/**
	 * Default constructor
	 */
	public DciLabelProvider()
	{
		session = Registry.getSession();

		statusImages = new Image[3];
      statusImages[DataCollectionObjectStatus.ACTIVE.getValue()] = ResourceManager.getImageDescriptor("icons/dci/active.gif").createImage(); //$NON-NLS-1$
      statusImages[DataCollectionObjectStatus.DISABLED.getValue()] = ResourceManager.getImageDescriptor("icons/dci/disabled.gif").createImage(); //$NON-NLS-1$
      statusImages[DataCollectionObjectStatus.UNSUPPORTED.getValue()] = ResourceManager.getImageDescriptor("icons/dci/unsupported.gif").createImage(); //$NON-NLS-1$

      originTexts.put(DataOrigin.AGENT, i18n.tr("NetXMS Agent"));
      originTexts.put(DataOrigin.DEVICE_DRIVER, i18n.tr("Network Device Driver"));
      originTexts.put(DataOrigin.INTERNAL, i18n.tr("Internal"));
      originTexts.put(DataOrigin.MODBUS, i18n.tr("Modbus"));
      originTexts.put(DataOrigin.MQTT, i18n.tr("MQTT"));
      originTexts.put(DataOrigin.PUSH, i18n.tr("Push"));
      originTexts.put(DataOrigin.SCRIPT, i18n.tr("Script"));
      originTexts.put(DataOrigin.SMCLP, i18n.tr("SM-CLP"));
      originTexts.put(DataOrigin.SNMP, i18n.tr("SNMP"));
      originTexts.put(DataOrigin.SSH, i18n.tr("SSH"));
      originTexts.put(DataOrigin.WEB_SERVICE, i18n.tr("Web Service"));
      originTexts.put(DataOrigin.WINPERF, i18n.tr("Windows Performance Counters"));

      statusTexts.put(DataCollectionObjectStatus.ACTIVE, i18n.tr("Active"));
      statusTexts.put(DataCollectionObjectStatus.DISABLED, i18n.tr("Disabled"));
      statusTexts.put(DataCollectionObjectStatus.UNSUPPORTED, i18n.tr("Not supported"));
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		if (columnIndex != 0)
			return null;
      DataCollectionObjectStatus status = ((DataCollectionObject)element).getStatus();
      return statusImages[status.getValue()];
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		DataCollectionObject dci = (DataCollectionObject)element;
		switch(columnIndex)
		{
			case DataCollectionView.DC_COLUMN_ID:
				return Long.toString(dci.getId());
			case DataCollectionView.DC_COLUMN_ORIGIN:
				return originTexts.get(dci.getOrigin());
			case DataCollectionView.DC_COLUMN_DESCRIPTION:
				return dci.getDescription();
			case DataCollectionView.DC_COLUMN_PARAMETER:
				return dci.getName();
			case DataCollectionView.DC_COLUMN_DATAUNIT:
            if((dci instanceof DataCollectionItem))
            {
               String unitName = ((DataCollectionItem)dci).getUnitName();
               return unitName == null ? "" : unitName;
            }
            return "";			   
			case DataCollectionView.DC_COLUMN_DATATYPE:
				if (dci instanceof DataCollectionItem)
					return DataCollectionDisplayInfo.getDataTypeName(((DataCollectionItem)dci).getDataType());
				return i18n.tr("<< TABLE >>");
			case DataCollectionView.DC_COLUMN_INTERVAL:
				if (dci.isUseAdvancedSchedule())
					return i18n.tr("custom schedule");
				if (dci.getPollingScheduleType() == DataCollectionObject.POLLING_SCHEDULE_DEFAULT)
				   return i18n.tr("default");
				return dci.getPollingInterval();
			case DataCollectionView.DC_COLUMN_RETENTION:
			   if (dci.getRetentionType() == DataCollectionObject.RETENTION_NONE)
			      return i18n.tr("none");
			   if (dci.getRetentionType() == DataCollectionObject.RETENTION_DEFAULT)
               return i18n.tr("default");
			   try
			   {
   				int days = Integer.parseInt(dci.getRetentionTime());
   				return Integer.toString(days) + ((days == 1) ? i18n.tr(" day") : i18n.tr(" days"));
			   }
			   catch(NumberFormatException e)
			   {
			      return dci.getRetentionTime();
			   }
			case DataCollectionView.DC_COLUMN_STATUS:
				return statusTexts.get(dci.getStatus());
         case DataCollectionView.DC_COLUMN_TAG:
            return dci.getUserTag();
			case DataCollectionView.DC_COLUMN_THRESHOLD:
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
			case DataCollectionView.DC_COLUMN_TEMPLATE:
				if (dci.getTemplateId() == 0)
					return null;
				AbstractObject object = session.findObjectById(dci.getTemplateId());
				if (object == null)
				   return i18n.tr("<unknown>");
				if (!(object instanceof Template))
				   return object.getObjectName();
				return object.getObjectNameWithPath();
			case DataCollectionView.DC_COLUMN_RELATEDOBJ:
            if (dci.getRelatedObject() == 0)
               return null;
            AbstractObject obj = session.findObjectById(dci.getRelatedObject());
            if (obj == null)
               return "[" + dci.getRelatedObject() + "]";
            return obj.getObjectName();
         case DataCollectionView.DC_COLUMN_STATUSCALC:
            if (dci instanceof DataCollectionItem)
               return ((DataCollectionItem)dci).isUsedForNodeStatusCalculation() ? "Yes" : "No";
            return i18n.tr("No");
		}
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.IBaseLabelProvider#dispose()
    */
	@Override
	public void dispose()
	{
		for(int i = 0; i < statusImages.length; i++)
			statusImages[i].dispose();
      super.dispose();
	}

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
    */
   @Override
   public Color getForeground(Object element)
   {
      if (((DataCollectionObject)element).getTemplateId() != 0)
      {
         return COLOR_TEMPLATE_ITEM;
      }
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
    */
   @Override
   public Color getBackground(Object element)
   {
      return null;
   }
}
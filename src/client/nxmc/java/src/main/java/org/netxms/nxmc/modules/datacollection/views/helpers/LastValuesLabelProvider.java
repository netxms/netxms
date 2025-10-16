/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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

import org.eclipse.jface.viewers.ITableColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.DataCollectionObjectStatus;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.constants.Severity;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.propertypages.Thresholds;
import org.netxms.nxmc.modules.datacollection.views.BaseDataCollectionView;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.ThresholdLabelProvider;
import org.netxms.nxmc.modules.objects.widgets.helpers.BaseObjectLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for last values view
 */
public class LastValuesLabelProvider extends LabelProvider implements ITableLabelProvider, ITableColorProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(LastValuesLabelProvider.class);

	private Image[] stateImages = new Image[3];
	private boolean useMultipliers = true;
	private boolean showErrors = true;
	private ThresholdLabelProvider thresholdLabelProvider;
   private BaseObjectLabelProvider objectLabelProvider;
   private NXCSession session;
   private SortableTableViewer viewer;

	/**
	 * Default constructor 
	 */
	public LastValuesLabelProvider(SortableTableViewer viewer)
	{
		super();

      this.viewer = viewer;

      stateImages[0] = ResourceManager.getImageDescriptor("icons/dci/active.gif").createImage();
      stateImages[1] = ResourceManager.getImageDescriptor("icons/dci/disabled.gif").createImage();
      stateImages[2] = ResourceManager.getImageDescriptor("icons/dci/unsupported.gif").createImage();

		thresholdLabelProvider = new ThresholdLabelProvider();
      objectLabelProvider = new BaseObjectLabelProvider();
      session = Registry.getSession();
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		switch((Integer)viewer.getTable().getColumn(columnIndex).getData("ID"))
		{
         case BaseDataCollectionView.LV_COLUMN_OWNER:
            AbstractObject object = session.findObjectById(((DciValue)element).getNodeId());
            return (object != null) ? objectLabelProvider.getImage(object) : null;
			case BaseDataCollectionView.LV_COLUMN_ID:
            return stateImages[((DciValue)element).getStatus().getValue()];
			case BaseDataCollectionView.LV_COLUMN_THRESHOLD:
            if (((DciValue)element).getThresholdDisableEndTime() != 0)
               return StatusDisplayInfo.getStatusImage(ObjectStatus.UNMANAGED);
				Threshold threshold = ((DciValue)element).getActiveThreshold();
				return (threshold != null) ? thresholdLabelProvider.getColumnImage(threshold, Thresholds.COLUMN_EVENT) : StatusDisplayInfo.getStatusImage(Severity.NORMAL);
		}
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
	   DciValue dciValue = ((DciValue)element);
		switch((Integer)viewer.getTable().getColumn(columnIndex).getData("ID"))
		{
         case BaseDataCollectionView.LV_COLUMN_COMMENTS:
            return dciValue.getComments().replace("\r", "").replace('\n', ' ');
         case BaseDataCollectionView.LV_COLUMN_DESCRIPTION:
            return dciValue.getDescription();
         case BaseDataCollectionView.LV_COLUMN_EVENT:
            return getEventName(dciValue);
         case BaseDataCollectionView.LV_COLUMN_ID:
            return Long.toString(dciValue.getId());
         case BaseDataCollectionView.LV_COLUMN_MESSAGE:
            return getEventMessage(dciValue);
         case BaseDataCollectionView.LV_COLUMN_OWNER:
            return session.getObjectNameWithAlias(dciValue.getNodeId());
         case BaseDataCollectionView.LV_COLUMN_TAG:
            return dciValue.getUserTag();
         case BaseDataCollectionView.LV_COLUMN_THRESHOLD:
            return formatThreshold(dciValue);
         case BaseDataCollectionView.LV_COLUMN_TIMESTAMP:
            if (dciValue.getTimestamp().getTime() <= 1000)
               return null;
            return DateFormatFactory.getDateTimeFormat().format(dciValue.getTimestamp());
			case BaseDataCollectionView.LV_COLUMN_VALUE:
				if (showErrors && dciValue.getErrorCount() > 0)
               return i18n.tr("<< ERROR >>");
				if (dciValue.getDcObjectType() == DataCollectionObject.DCO_TYPE_TABLE)
               return i18n.tr("<< TABLE >>");
            return dciValue.getFormattedValue(useMultipliers, DateFormatFactory.getTimeFormatter());
		}
		return null;
	}

	/**
    * Format threshold
    * 
    * @param value DCI value to format threshold for
    * @return formatted threshold
    */
   private String formatThreshold(DciValue value)
	{
      if (value.getThresholdDisableEndTime() != 0)
      {
         if (value.getThresholdDisableEndTime() == -1)
            return i18n.tr("Disabled permanently");
         return i18n.tr("Disabled until {0}", DateFormatFactory.getDateTimeFormat().format(value.getThresholdDisableEndTime() * 1000));
      }
      Threshold threshold = value.getActiveThreshold();
      if (threshold == null)
         return i18n.tr("OK");
      if (value.getDcObjectType() == DataCollectionObject.DCO_TYPE_TABLE)
         return threshold.getValue(); // For table DCIs server sends pre-formatted condition in "value" field
      return thresholdLabelProvider.getColumnText(threshold, Thresholds.COLUMN_OPERATION);
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
    * @see org.eclipse.jface.viewers.IBaseLabelProvider#dispose()
    */
	@Override
	public void dispose()
	{
		for(int i = 0; i < stateImages.length; i++)
			stateImages[i].dispose();
		thresholdLabelProvider.dispose();
      objectLabelProvider.dispose();
		super.dispose();
	}

	/**
	 * @return the useMultipliers
	 */
	public boolean areMultipliersUsed()
	{
		return useMultipliers;
	}

	/**
	 * @param useMultipliers the useMultipliers to set
	 */
	public void setUseMultipliers(boolean useMultipliers)
	{
		this.useMultipliers = useMultipliers;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableColorProvider#getForeground(java.lang.Object, int)
    */
	@Override
	public Color getForeground(Object element, int columnIndex)
	{
      if (((DciValue)element).getStatus() == DataCollectionObjectStatus.DISABLED)
			return StatusDisplayInfo.getStatusColor(ObjectStatus.UNMANAGED);
		if (showErrors && ((DciValue)element).getErrorCount() > 0)
			return StatusDisplayInfo.getStatusColor(ObjectStatus.CRITICAL);
      if (((DciValue)element).isAnomalyDetected())
         return StatusDisplayInfo.getStatusColor(ObjectStatus.MAJOR);
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableColorProvider#getBackground(java.lang.Object, int)
    */
	@Override
	public Color getBackground(Object element, int columnIndex)
	{
		return null;
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

/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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

import java.text.DateFormat;
import java.text.NumberFormat;
import org.eclipse.jface.viewers.ITableColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.constants.Severity;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.Threshold;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.ThresholdLabelProvider;
import org.netxms.ui.eclipse.datacollection.propertypages.Thresholds;
import org.netxms.ui.eclipse.datacollection.widgets.LastValuesWidget;


/**
 * Label provider for last values view
 */
public class LastValuesLabelProvider extends LabelProvider implements ITableLabelProvider, ITableColorProvider
{
	private static final long serialVersionUID = 1L;

	private Image[] stateImages = new Image[3];
	private boolean useMultipliers = true;
	private ThresholdLabelProvider thresholdLabelProvider;
	
	/**
	 * Default constructor 
	 */
	public LastValuesLabelProvider()
	{
		super();

		stateImages[0] = Activator.getImageDescriptor("icons/active.gif").createImage();
		stateImages[1] = Activator.getImageDescriptor("icons/disabled.gif").createImage();
		stateImages[2] = Activator.getImageDescriptor("icons/unsupported.gif").createImage();
		
		thresholdLabelProvider = new ThresholdLabelProvider();
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case LastValuesWidget.COLUMN_ID:
				return stateImages[((DciValue)element).getStatus()];
			case LastValuesWidget.COLUMN_THRESHOLD:
				Threshold threshold = ((DciValue)element).getActiveThreshold();
				return (threshold != null) ? thresholdLabelProvider.getColumnImage(threshold, Thresholds.COLUMN_EVENT) : StatusDisplayInfo.getStatusImage(Severity.NORMAL);
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case LastValuesWidget.COLUMN_ID:
				return Long.toString(((DciValue)element).getId());
			case LastValuesWidget.COLUMN_DESCRIPTION:
				return ((DciValue)element).getDescription();
			case LastValuesWidget.COLUMN_VALUE:
				if (((DciValue)element).getDcObjectType() == DataCollectionObject.DCO_TYPE_TABLE)
					return "<< TABLE >>";
				return useMultipliers ? getValue((DciValue)element) : ((DciValue)element).getValue();
			case LastValuesWidget.COLUMN_TIMESTAMP:
				return DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT).format(((DciValue)element).getTimestamp());
			case LastValuesWidget.COLUMN_THRESHOLD:
				return formatThreshold(((DciValue)element).getActiveThreshold());
		}
		return null;
	}
	
	/**
	 * Format threshold
	 * 
	 * @param activeThreshold
	 * @return
	 */
	private String formatThreshold(Threshold threshold)
	{
		if (threshold == null)
			return "OK";
		return thresholdLabelProvider.getColumnText(threshold, Thresholds.COLUMN_OPERATION);
	}

	/**
	 * @param element
	 * @return
	 */
	private String getValue(DciValue element)
	{
		switch(element.getDataType())
		{
			case DataCollectionItem.DT_INT:
			case DataCollectionItem.DT_INT64:
			case DataCollectionItem.DT_UINT:
			case DataCollectionItem.DT_UINT64:
				try
				{
					long i = Long.parseLong(element.getValue());
					if ((i >= 10000000000L) || (i <= -10000000000L))
					{
						return Long.toString(i / 1000000000L) + " G";
					}
					if ((i >= 10000000) || (i <= -10000000))
					{
						return Long.toString(i / 1000000) + " M";
					}
					if ((i >= 10000) || (i <= -10000))
					{
						return Long.toString(i / 1000) + " K";
					}
				}
				catch(NumberFormatException e)
				{
				}
				return element.getValue();
			case DataCollectionItem.DT_FLOAT:
				try
				{
					double d = Double.parseDouble(element.getValue());
					NumberFormat nf = NumberFormat.getNumberInstance();
					nf.setMaximumFractionDigits(2);
					if ((d >= 10000000000.0) || (d <= -10000000000.0))
					{
						return nf.format(d / 1000000000.0) + " G";
					}
					if ((d >= 10000000) || (d <= -10000000))
					{
						return nf.format(d / 1000000) + " M";
					}
					if ((d >= 10000) || (d <= -10000))
					{
						return nf.format(d / 1000) + " K";
					}
				}
				catch(NumberFormatException e)
				{
				}
				return element.getValue();
			default:
				return element.getValue();
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		for(int i = 0; i < stateImages.length; i++)
			stateImages[i].dispose();
		thresholdLabelProvider.dispose();
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

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableColorProvider#getForeground(java.lang.Object, int)
	 */
	@Override
	public Color getForeground(Object element, int columnIndex)
	{
		return null;
		//return (((DciValue)element).getActiveThreshold() != null) ? StatusDisplayInfo.getStatusColor(Severity.MAJOR) : null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableColorProvider#getBackground(java.lang.Object, int)
	 */
	@Override
	public Color getBackground(Object element, int columnIndex)
	{
		return null;
	}
}

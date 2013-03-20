/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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

import java.util.HashMap;

import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.views.DataCollectionEditor;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for user manager
 */
public class DciLabelProvider implements ITableLabelProvider
{
	private static final long serialVersionUID = 1L;

	private NXCSession session;
	private Image statusImages[];
	private HashMap<Integer, String> originTexts = new HashMap<Integer, String>();
	private HashMap<Integer, String> dtTexts = new HashMap<Integer, String>();
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
		
		originTexts.put(DataCollectionItem.AGENT, Messages.DciLabelProvider_SourceAgent);
		originTexts.put(DataCollectionItem.SNMP, Messages.DciLabelProvider_SourceSNMP);
		originTexts.put(DataCollectionItem.CHECKPOINT_SNMP, Messages.DciLabelProvider_SourceCPSNMP);
		originTexts.put(DataCollectionItem.INTERNAL, Messages.DciLabelProvider_SourceInternal);
		originTexts.put(DataCollectionItem.PUSH, Messages.DciLabelProvider_SourcePush);
		originTexts.put(DataCollectionItem.WINPERF, "Windows Performance Counters");
		
		statusTexts.put(DataCollectionItem.ACTIVE, Messages.DciLabelProvider_Active);
		statusTexts.put(DataCollectionItem.DISABLED, Messages.DciLabelProvider_Disabled);
		statusTexts.put(DataCollectionItem.NOT_SUPPORTED, Messages.DciLabelProvider_NotSupported);

		dtTexts.put(DataCollectionItem.DT_INT, Messages.DciLabelProvider_DT_int32);
		dtTexts.put(DataCollectionItem.DT_UINT, Messages.DciLabelProvider_DT_uint32);
		dtTexts.put(DataCollectionItem.DT_INT64, Messages.DciLabelProvider_DT_int64);
		dtTexts.put(DataCollectionItem.DT_UINT64, Messages.DciLabelProvider_DT_uint64);
		dtTexts.put(DataCollectionItem.DT_FLOAT, Messages.DciLabelProvider_DT_float);
		dtTexts.put(DataCollectionItem.DT_STRING, Messages.DciLabelProvider_DT_string);
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
					return dtTexts.get(((DataCollectionItem)dci).getDataType());
				return Messages.DciLabelProvider_Table;
			case DataCollectionEditor.COLUMN_INTERVAL:
				if (dci.isUseAdvancedSchedule())
					return Messages.DciLabelProvider_CustomSchedule;
				return Integer.toString(dci.getPollingInterval());
			case DataCollectionEditor.COLUMN_RETENTION:
				int days = dci.getRetentionTime();
				return Integer.toString(days) + ((days == 1) ? Messages.DciLabelProvider_Day : Messages.DciLabelProvider_Days);
			case DataCollectionEditor.COLUMN_STATUS:
				return statusTexts.get(dci.getStatus());
			case DataCollectionEditor.COLUMN_TEMPLATE:
				if (dci.getTemplateId() == 0)
					return null;
				GenericObject object = session.findObjectById(dci.getTemplateId());
				return (object != null) ? object.getObjectName() : Messages.DciLabelProvider_Unknown;
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
}
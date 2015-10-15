/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.views.DataCollectionEditor;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for user manager
 */
public class DciLabelProvider implements ITableLabelProvider
{
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
		
		originTexts.put(DataCollectionItem.AGENT, Messages.get().DciLabelProvider_SourceAgent);
		originTexts.put(DataCollectionItem.SNMP, Messages.get().DciLabelProvider_SourceSNMP);
		originTexts.put(DataCollectionItem.CHECKPOINT_SNMP, Messages.get().DciLabelProvider_SourceCPSNMP);
		originTexts.put(DataCollectionItem.INTERNAL, Messages.get().DciLabelProvider_SourceInternal);
		originTexts.put(DataCollectionItem.PUSH, Messages.get().DciLabelProvider_SourcePush);
		originTexts.put(DataCollectionItem.WINPERF, Messages.get().DciLabelProvider_SourceWinPerf);
		originTexts.put(DataCollectionItem.ILO, Messages.get().DciLabelProvider_SourceILO);
      originTexts.put(DataCollectionItem.SCRIPT, Messages.get().DciLabelProvider_SourceScript);
		
		statusTexts.put(DataCollectionItem.ACTIVE, Messages.get().DciLabelProvider_Active);
		statusTexts.put(DataCollectionItem.DISABLED, Messages.get().DciLabelProvider_Disabled);
		statusTexts.put(DataCollectionItem.NOT_SUPPORTED, Messages.get().DciLabelProvider_NotSupported);

		dtTexts.put(DataCollectionItem.DT_INT, Messages.get().DciLabelProvider_DT_int32);
		dtTexts.put(DataCollectionItem.DT_UINT, Messages.get().DciLabelProvider_DT_uint32);
		dtTexts.put(DataCollectionItem.DT_INT64, Messages.get().DciLabelProvider_DT_int64);
		dtTexts.put(DataCollectionItem.DT_UINT64, Messages.get().DciLabelProvider_DT_uint64);
		dtTexts.put(DataCollectionItem.DT_FLOAT, Messages.get().DciLabelProvider_DT_float);
		dtTexts.put(DataCollectionItem.DT_STRING, Messages.get().DciLabelProvider_DT_string);
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
				return Messages.get().DciLabelProvider_Table;
			case DataCollectionEditor.COLUMN_INTERVAL:
				if (dci.isUseAdvancedSchedule())
					return Messages.get().DciLabelProvider_CustomSchedule;
				if (dci.getPollingInterval() <= 0)
				   return "default";
				return Integer.toString(dci.getPollingInterval());
			case DataCollectionEditor.COLUMN_RETENTION:
			   if ((dci.getFlags() & DataCollectionItem.DCF_NO_STORAGE) != 0)
			      return "none";
				int days = dci.getRetentionTime();
				if (days <= 0)
				   return "default";
				return Integer.toString(days) + ((days == 1) ? Messages.get().DciLabelProvider_Day : Messages.get().DciLabelProvider_Days);
			case DataCollectionEditor.COLUMN_STATUS:
				return statusTexts.get(dci.getStatus());
			case DataCollectionEditor.COLUMN_TEMPLATE:
				if (dci.getTemplateId() == 0)
					return null;
				AbstractObject object = session.findObjectById(dci.getTemplateId());
				return (object != null) ? object.getObjectName() : Messages.get().DciLabelProvider_Unknown;
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
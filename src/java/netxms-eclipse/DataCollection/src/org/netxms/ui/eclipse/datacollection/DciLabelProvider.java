package org.netxms.ui.eclipse.datacollection;

import java.util.HashMap;

import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCObject;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.ui.eclipse.datacollection.views.DataCollectionEditor;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

/**
 * Label provider for user manager
 * 
 * @author Victor
 * 
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
		session = NXMCSharedData.getInstance().getSession();
		
		statusImages = new Image[3];
		statusImages[DataCollectionItem.ACTIVE] = Activator.getImageDescriptor("icons/active.gif").createImage();
		statusImages[DataCollectionItem.DISABLED] = Activator.getImageDescriptor("icons/disabled.gif").createImage();
		statusImages[DataCollectionItem.NOT_SUPPORTED] = Activator.getImageDescriptor("icons/unsupported.gif").createImage();
		
		originTexts.put(DataCollectionItem.AGENT, "NetXMS Agent");
		originTexts.put(DataCollectionItem.SNMP, "SNMP");
		originTexts.put(DataCollectionItem.CHECKPOINT_SNMP, "Check Point SNMP");
		originTexts.put(DataCollectionItem.INTERNAL, "Internal");
		
		statusTexts.put(DataCollectionItem.ACTIVE, "Active");
		statusTexts.put(DataCollectionItem.DISABLED, "Disabled");
		statusTexts.put(DataCollectionItem.NOT_SUPPORTED, "Not supported");

		dtTexts.put(DataCollectionItem.DT_INT, "Integer");
		dtTexts.put(DataCollectionItem.DT_UINT, "Unsigned Integer");
		dtTexts.put(DataCollectionItem.DT_INT64, "Int64");
		dtTexts.put(DataCollectionItem.DT_UINT64, "Unsigned Int64");
		dtTexts.put(DataCollectionItem.DT_FLOAT, "Float");
		dtTexts.put(DataCollectionItem.DT_STRING, "String");
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		if (columnIndex != 0)
			return null;
		int status = ((DataCollectionItem)element).getStatus();
		return ((status >= 0) && (status < statusImages.length)) ? statusImages[status] : null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		DataCollectionItem dci = (DataCollectionItem)element;
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
				return dtTexts.get(dci.getDataType());
			case DataCollectionEditor.COLUMN_INTERVAL:
				if (dci.isUseAdvancedSchedule())
					return "advanced schedule";
				return Integer.toString(dci.getPollingInterval());
			case DataCollectionEditor.COLUMN_RETENTION:
				int days = dci.getRetentionTime();
				return Integer.toString(days) + ((days == 1) ? " day" : " days");
			case DataCollectionEditor.COLUMN_STATUS:
				return statusTexts.get(dci.getStatus());
			case DataCollectionEditor.COLUMN_TEMPLATE:
				if (dci.getTemplateId() == 0)
					return null;
				NXCObject object = session.findObjectById(dci.getTemplateId());
				return (object != null) ? object.getObjectName() : "<unknown>";
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
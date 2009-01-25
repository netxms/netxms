/**
 * 
 */
package org.netxms.nxmc.alarmviewer;

import java.text.DateFormat;

import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCAlarm;
import org.netxms.client.NXCObject;
import org.netxms.nxmc.core.extensionproviders.NXMCSharedData;


/**
 * @author Victor
 *
 */
public class AlarmListLabelProvider implements ITableLabelProvider
{
	// Constants
	private static final String[] severityText = { "Normal", "Warning", "Minor", "Major", "Critical" };
	private static final String[] stateText = { "Outstanding", "Acknowledged", "Terminated" };
	
	// Severity images
	private Image[] severityImages = new Image[5];
	private Image[] stateImages = new Image[3];
	
	/**
	 * Default constructor 
	 */
	public AlarmListLabelProvider()
	{
		super();

		severityImages[0] = Activator.getImageDescriptor("icons/normal.png").createImage();
		severityImages[1] = Activator.getImageDescriptor("icons/warning.png").createImage();
		severityImages[2] = Activator.getImageDescriptor("icons/minor.png").createImage();
		severityImages[3] = Activator.getImageDescriptor("icons/major.png").createImage();
		severityImages[4] = Activator.getImageDescriptor("icons/critical.png").createImage();

		stateImages[0] = Activator.getImageDescriptor("icons/outstanding.png").createImage();
		stateImages[1] = Activator.getImageDescriptor("icons/acknowledged.png").createImage();
		stateImages[2] = Activator.getImageDescriptor("icons/terminated.png").createImage();
	}

	
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case AlarmView.COLUMN_SEVERITY:
				return severityImages[((NXCAlarm)element).getCurrentSeverity()];
			case AlarmView.COLUMN_STATE:
				return stateImages[((NXCAlarm)element).getState()];
		}
		return null;
	}

	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case AlarmView.COLUMN_SEVERITY:
				return severityText[((NXCAlarm)element).getCurrentSeverity()];
			case AlarmView.COLUMN_STATE:
				return stateText[((NXCAlarm)element).getState()];
			case AlarmView.COLUMN_SOURCE:
				NXCObject object = NXMCSharedData.getSession().findObjectById(((NXCAlarm)element).getSourceObjectId());
				return (object != null) ? object.getObjectName() : null;
			case AlarmView.COLUMN_MESSAGE:
				return ((NXCAlarm)element).getMessage();
			case AlarmView.COLUMN_COUNT:
				return Integer.toString(((NXCAlarm)element).getRepeatCount());
			case AlarmView.COLUMN_CREATED:
				return DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT).format(((NXCAlarm)element).getCreationTime());
			case AlarmView.COLUMN_LASTCHANGE:
				return DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT).format(((NXCAlarm)element).getLastChangeTime());
		}
		return null;
	}

	@Override
	public void addListener(ILabelProviderListener listener)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void dispose()
	{
		for(int i = 0; i < 5; i++)
			severityImages[i].dispose();
	}

	@Override
	public boolean isLabelProperty(Object element, String property)
	{
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public void removeListener(ILabelProviderListener listener)
	{
		// TODO Auto-generated method stub
		
	}
}

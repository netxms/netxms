/**
 * 
 */
package org.netxms.ui.eclipse.alarmviewer;

import java.text.DateFormat;

import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCAlarm;
import org.netxms.client.NXCObject;
import org.netxms.client.constants.Severity;
import org.netxms.ui.eclipse.alarmviewer.widgets.AlarmList;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.shared.StatusDisplayInfo;


/**
 * @author Victor
 *
 */
public class AlarmListLabelProvider implements ITableLabelProvider
{
	// Constants
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

		severityImages[Severity.NORMAL] = StatusDisplayInfo.getStatusImageDescriptor(Severity.NORMAL).createImage();
		severityImages[Severity.WARNING] = StatusDisplayInfo.getStatusImageDescriptor(Severity.WARNING).createImage();
		severityImages[Severity.MINOR] = StatusDisplayInfo.getStatusImageDescriptor(Severity.MINOR).createImage();
		severityImages[Severity.MAJOR] = StatusDisplayInfo.getStatusImageDescriptor(Severity.MAJOR).createImage();
		severityImages[Severity.CRITICAL] = StatusDisplayInfo.getStatusImageDescriptor(Severity.CRITICAL).createImage();

		stateImages[0] = Activator.getImageDescriptor("icons/outstanding.png").createImage();
		stateImages[1] = Activator.getImageDescriptor("icons/acknowledged.png").createImage();
		stateImages[2] = Activator.getImageDescriptor("icons/terminated.png").createImage();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case AlarmList.COLUMN_SEVERITY:
				return severityImages[((NXCAlarm)element).getCurrentSeverity()];
			case AlarmList.COLUMN_STATE:
				return stateImages[((NXCAlarm)element).getState()];
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
			case AlarmList.COLUMN_SEVERITY:
				return StatusDisplayInfo.getStatusText(((NXCAlarm)element).getCurrentSeverity());
			case AlarmList.COLUMN_STATE:
				return stateText[((NXCAlarm)element).getState()];
			case AlarmList.COLUMN_SOURCE:
				NXCObject object = NXMCSharedData.getInstance().getSession().findObjectById(((NXCAlarm)element).getSourceObjectId());
				return (object != null) ? object.getObjectName() : null;
			case AlarmList.COLUMN_MESSAGE:
				return ((NXCAlarm)element).getMessage();
			case AlarmList.COLUMN_COUNT:
				return Integer.toString(((NXCAlarm)element).getRepeatCount());
			case AlarmList.COLUMN_CREATED:
				return DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT).format(((NXCAlarm)element).getCreationTime());
			case AlarmList.COLUMN_LASTCHANGE:
				return DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT).format(((NXCAlarm)element).getLastChangeTime());
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
		for(int i = 0; i < severityImages.length; i++)
			severityImages[i].dispose();

		for(int i = 0; i < stateImages.length; i++)
			stateImages[i].dispose();
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

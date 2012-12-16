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
package org.netxms.ui.eclipse.alarmviewer.widgets.helpers;

import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.api.client.users.AbstractUserObject;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.alarmviewer.Activator;
import org.netxms.ui.eclipse.alarmviewer.Messages;
import org.netxms.ui.eclipse.alarmviewer.widgets.AlarmList;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.console.tools.RegionalSettings;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;

/**
 * Label provider for alarm list
 */
public class AlarmListLabelProvider implements ITableLabelProvider
{
	private static final String[] stateText = { Messages.AlarmListLabelProvider_AlarmState_Outstanding, Messages.AlarmListLabelProvider_AlarmState_Acknowledged, Messages.AlarmListLabelProvider_AlarmState_Resolved, Messages.AlarmListLabelProvider_AlarmState_Terminated };
	
	private NXCSession session;
	private Image[] stateImages = new Image[5];
	private Image commentsImage;
	private boolean blinkState = true;
	
	/**
	 * Default constructor 
	 */
	public AlarmListLabelProvider()
	{
		session = (NXCSession)ConsoleSharedData.getSession();
		
		stateImages[0] = Activator.getImageDescriptor("icons/outstanding.png").createImage(); //$NON-NLS-1$
		stateImages[1] = Activator.getImageDescriptor("icons/acknowledged.png").createImage(); //$NON-NLS-1$
		stateImages[2] = Activator.getImageDescriptor("icons/resolved.png").createImage(); //$NON-NLS-1$
		stateImages[3] = Activator.getImageDescriptor("icons/terminated.png").createImage(); //$NON-NLS-1$
		stateImages[4] = Activator.getImageDescriptor("icons/acknowledged_sticky.png").createImage(); //$NON-NLS-1$
		
		commentsImage = Activator.getImageDescriptor("icons/comments.png").createImage(); //$NON-NLS-1$
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
				return StatusDisplayInfo.getStatusImage(((Alarm)element).getCurrentSeverity());
			case AlarmList.COLUMN_STATE:
				if (((Alarm)element).getState() == Alarm.STATE_OUTSTANDING)
					return blinkState ? stateImages[Alarm.STATE_OUTSTANDING] : SharedIcons.IMG_EMPTY;
				if ((((Alarm)element).getState() == Alarm.STATE_ACKNOWLEDGED) && ((Alarm)element).isSticky())
					return stateImages[4];
				return stateImages[((Alarm)element).getState()];
			case AlarmList.COLUMN_COMMENTS:
				return (((Alarm)element).getCommentsCount() > 0) ? commentsImage : null;
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
				return StatusDisplayInfo.getStatusText(((Alarm)element).getCurrentSeverity());
			case AlarmList.COLUMN_STATE:
				return stateText[((Alarm)element).getState()];
			case AlarmList.COLUMN_SOURCE:
				GenericObject object = session.findObjectById(((Alarm)element).getSourceObjectId());
				return (object != null) ? object.getObjectName() : ("[" + Long.toString(((Alarm)element).getSourceObjectId()) + "]"); //$NON-NLS-1$ //$NON-NLS-2$
			case AlarmList.COLUMN_MESSAGE:
				return ((Alarm)element).getMessage();
			case AlarmList.COLUMN_COUNT:
				return Integer.toString(((Alarm)element).getRepeatCount());
			case AlarmList.COLUMN_COMMENTS:
				return (((Alarm)element).getCommentsCount() > 0) ? Integer.toString(((Alarm)element).getCommentsCount()) : null;
			case AlarmList.COLUMN_ACK_BY:
				if (((Alarm)element).getState() == Alarm.STATE_OUTSTANDING)
					return null;
				long userId = (((Alarm)element).getState() == Alarm.STATE_ACKNOWLEDGED) ? ((Alarm)element).getAckByUser() : ((Alarm)element).getResolvedByUser();
				AbstractUserObject user = session.findUserDBObjectById(userId);
				return (user != null) ? user.getName() : ("[" + Long.toString(((Alarm)element).getAckByUser()) + "]"); //$NON-NLS-1$ //$NON-NLS-2$
			case AlarmList.COLUMN_CREATED:
				return RegionalSettings.getDateTimeFormat().format(((Alarm)element).getCreationTime());
			case AlarmList.COLUMN_LASTCHANGE:
				return RegionalSettings.getDateTimeFormat().format(((Alarm)element).getLastChangeTime());
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

	/**
	 * Toggle blink state
	 */
	public void toggleBlinkState()
	{
		blinkState = !blinkState;
	}
}

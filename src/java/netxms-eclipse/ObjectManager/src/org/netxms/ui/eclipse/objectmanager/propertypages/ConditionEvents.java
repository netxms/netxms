/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectmanager.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.Severity;
import org.netxms.client.objects.Condition;
import org.netxms.ui.eclipse.eventmanager.widgets.EventSelector;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.StatusSelector;

/**
 * "Events" property page for condition objects
 */
public class ConditionEvents extends PropertyPage
{
	private Condition object;
	private EventSelector activationEvent;
	private EventSelector deactivationEvent;
	private ObjectSelector sourceObject;
	private StatusSelector activeStatus;
	private StatusSelector inactiveStatus;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		object = (Condition)getElement().getAdapter(Condition.class);
		if (object == null)	// Paranoid check
			return dialogArea;
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 1;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
		dialogArea.setLayout(layout);
		
		/* event group */
		Group eventGroup = new Group(dialogArea, SWT.NONE);
		eventGroup.setText(Messages.get().ConditionEvents_Events);
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		eventGroup.setLayoutData(gd);

		layout = new GridLayout();
		eventGroup.setLayout(layout);
		
		activationEvent = new EventSelector(eventGroup, SWT.NONE);
		activationEvent.setLabel(Messages.get().ConditionEvents_ActivationEvent);
		activationEvent.setEventCode(object.getActivationEvent());
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		activationEvent.setLayoutData(gd);
		
		deactivationEvent = new EventSelector(eventGroup, SWT.NONE);
		deactivationEvent.setLabel(Messages.get().ConditionEvents_DeactivationEvent);
		deactivationEvent.setEventCode(object.getDeactivationEvent());
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		deactivationEvent.setLayoutData(gd);

		sourceObject = new ObjectSelector(eventGroup, SWT.NONE, true);
		sourceObject.setLabel(Messages.get().ConditionEvents_SourceObject);
		sourceObject.setEmptySelectionName(Messages.get().ConditionEvents_SelectionServer);
		sourceObject.setObjectId(object.getEventSourceObject());
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		sourceObject.setLayoutData(gd);
		
		/* status group */
		Group statusGroup = new Group(dialogArea, SWT.NONE);
		statusGroup.setText(Messages.get().ConditionEvents_Status);
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		statusGroup.setLayoutData(gd);

		layout = new GridLayout();
		layout.numColumns = 2;
		statusGroup.setLayout(layout);
		
		activeStatus = new StatusSelector(statusGroup, SWT.NONE, Severity.CRITICAL);
		activeStatus.setLabel(Messages.get().ConditionEvents_ActiveStatus);
		activeStatus.setSelection(object.getActiveStatus());
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		activeStatus.setLayoutData(gd);
		
		inactiveStatus = new StatusSelector(statusGroup, SWT.NONE, Severity.CRITICAL);
		inactiveStatus.setLabel(Messages.get().ConditionEvents_InactiveStatus);
		inactiveStatus.setSelection(object.getInactiveStatus());
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		inactiveStatus.setLayoutData(gd);
		
		return dialogArea;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		if (isApply)
			setValid(false);
		
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
		md.setActivationEvent((int)activationEvent.getEventCode());
		md.setDeactivationEvent((int)deactivationEvent.getEventCode());
		md.setSourceObject(sourceObject.getObjectId());
		md.setActiveStatus(activeStatus.getSelection());
		md.setInactiveStatus(inactiveStatus.getSelection());
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob(Messages.get().ConditionEvents_JobName, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().ConditionEvents_JobError;
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							ConditionEvents.this.setValid(true);
						}
					});
				}
			}
		}.start();
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}
}

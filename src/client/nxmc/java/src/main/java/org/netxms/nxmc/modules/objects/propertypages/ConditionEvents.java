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
package org.netxms.nxmc.modules.objects.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.Severity;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Condition;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.StatusSelector;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.widgets.EventSelector;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Events" property page for condition objects
 */
public class ConditionEvents extends ObjectPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(ConditionEvents.class);
   
	private Condition condition;
	private EventSelector activationEvent;
	private EventSelector deactivationEvent;
	private ObjectSelector sourceObject;
	private StatusSelector activeStatus;
	private StatusSelector inactiveStatus;

   /**
    * Create new page.
    *
    * @param condition object to edit
    */
   public ConditionEvents(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(ConditionEvents.class).tr("Events and Status"), object);
   }

   @Override
   public String getId()
   {
      return "conditionEvents";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 10;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return object instanceof Condition;
   }

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
      this.condition = (Condition)object;
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 1;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
		dialogArea.setLayout(layout);
		
		/* event group */
		Group eventGroup = new Group(dialogArea, SWT.NONE);
		eventGroup.setText(i18n.tr("Events"));
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		eventGroup.setLayoutData(gd);

		layout = new GridLayout();
		eventGroup.setLayout(layout);
		
		activationEvent = new EventSelector(eventGroup, SWT.NONE);
		activationEvent.setLabel(i18n.tr("Activation event"));
		activationEvent.setEventCode(condition.getActivationEvent());
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		activationEvent.setLayoutData(gd);
		
		deactivationEvent = new EventSelector(eventGroup, SWT.NONE);
		deactivationEvent.setLabel(i18n.tr("Deactivation event"));
		deactivationEvent.setEventCode(condition.getDeactivationEvent());
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		deactivationEvent.setLayoutData(gd);

		sourceObject = new ObjectSelector(eventGroup, SWT.NONE, true);
		sourceObject.setLabel(i18n.tr("Source object for events"));
		sourceObject.setEmptySelectionName(i18n.tr("<server>"));
		sourceObject.setObjectId(condition.getEventSourceObject());
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		sourceObject.setLayoutData(gd);
		
		/* status group */
		Group statusGroup = new Group(dialogArea, SWT.NONE);
		statusGroup.setText(i18n.tr("Status"));
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		statusGroup.setLayoutData(gd);

		layout = new GridLayout();
		layout.numColumns = 2;
		statusGroup.setLayout(layout);
		
		activeStatus = new StatusSelector(statusGroup, SWT.NONE, Severity.CRITICAL.getValue());
		activeStatus.setLabel(i18n.tr("Active status"));
		activeStatus.setSelection(condition.getActiveStatus());
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		activeStatus.setLayoutData(gd);
		
		inactiveStatus = new StatusSelector(statusGroup, SWT.NONE, Severity.CRITICAL.getValue());
		inactiveStatus.setLabel(i18n.tr("Inactive status"));
		inactiveStatus.setSelection(condition.getInactiveStatus());
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
	protected boolean applyChanges(final boolean isApply)
	{
		if (isApply)
			setValid(false);
		
		final NXCObjectModificationData md = new NXCObjectModificationData(condition.getObjectId());
		md.setActivationEvent((int)activationEvent.getEventCode());
		md.setDeactivationEvent((int)deactivationEvent.getEventCode());
		md.setSourceObject(sourceObject.getObjectId());
		md.setActiveStatus(activeStatus.getSelection());
		md.setInactiveStatus(inactiveStatus.getSelection());
		
		final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating condition events configuration"), null, messageArea) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot change condition events configuration");
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
		
      return true;
	}
}

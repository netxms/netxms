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
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Interface;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Polling" property page for NetMS interface objects 
 */
public class InterfacePolling extends PropertyPage
{
	private Spinner pollCount;
	private Combo expectedState;
	private Button checkExcludeFromTopology;
	private Interface object;
	private int currentPollCount;
	private int currentExpectedState;
	private boolean currentExcludeFlag;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		object = (Interface)getElement().getAdapter(Interface.class);
		if (object == null)	// Paranoid check
			return dialogArea;
		
		currentPollCount = object.getRequiredPollCount();
		currentExpectedState = object.getExpectedState();
		currentExcludeFlag = object.isExcludedFromTopology();
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
      dialogArea.setLayout(layout);

      pollCount = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, "Required poll count", 0, 1000, WidgetHelper.DEFAULT_LAYOUT_DATA);
      pollCount.setSelection(object.getRequiredPollCount());
      
      expectedState = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, "Expected state", WidgetHelper.DEFAULT_LAYOUT_DATA);
      expectedState.add("UP");
      expectedState.add("DOWN");
      expectedState.add("IGNORE");
      expectedState.select(object.getExpectedState());
      
      checkExcludeFromTopology = new Button(dialogArea, SWT.CHECK);
      checkExcludeFromTopology.setText("&Exclude this interface from network topology");
      checkExcludeFromTopology.setSelection(object.isExcludedFromTopology());
      checkExcludeFromTopology.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false, 2, 1));
      
		return dialogArea;
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		if ((expectedState.getSelectionIndex() == currentExpectedState) && 
			 (pollCount.getSelection() == currentPollCount) &&
			 (checkExcludeFromTopology.getSelection() == currentExcludeFlag))
			return;	// nothing to change 
		
		if (isApply)
			setValid(false);
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		final NXCObjectModificationData data = new NXCObjectModificationData(object.getObjectId());
		data.setExpectedState(expectedState.getSelectionIndex());
		data.setRequiredPolls(pollCount.getSelection());
		data.setObjectFlags(checkExcludeFromTopology.getSelection() ? Interface.IF_EXCLUDE_FROM_TOPOLOGY : 0);
		new ConsoleJob("Update interface polling configuration", null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(data);
			}

			@Override
			protected String getErrorMessage()
			{
				return String.format("Cannot modify interface object %s", object.getObjectName());
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
							currentExpectedState = data.getExpectedState();
							currentPollCount = data.getRequiredPolls();
							currentExcludeFlag = ((data.getObjectFlags() & Interface.IF_EXCLUDE_FROM_TOPOLOGY) != 0);
							InterfacePolling.this.setValid(true);
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

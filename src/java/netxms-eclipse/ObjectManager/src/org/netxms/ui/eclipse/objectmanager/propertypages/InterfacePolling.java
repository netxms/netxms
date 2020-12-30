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

import java.util.ArrayList;
import java.util.List;
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
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Polling" property page for NetMS interface objects 
 */
public class InterfacePolling extends PropertyPage
{
	private Spinner pollCount;
	private Combo expectedState;
	private Interface object;
	private int currentPollCount;
	private int currentExpectedState;
   private int currentFlags;
   private List<Button> flagButtons = new ArrayList<Button>();
   private List<Integer> flagValues = new ArrayList<Integer>();

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		object = (Interface)getElement().getAdapter(Interface.class);
		if (object == null)	// Paranoid check
			return dialogArea;
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
      dialogArea.setLayout(layout);

      pollCount = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, Messages.get().InterfacePolling_RequiredPollCount, 0, 1000, WidgetHelper.DEFAULT_LAYOUT_DATA);
      pollCount.setSelection(object.getRequiredPollCount());
      
      expectedState = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, Messages.get().InterfacePolling_ExpectedState, WidgetHelper.DEFAULT_LAYOUT_DATA);
      expectedState.add(Messages.get().InterfacePolling_StateUP);
      expectedState.add(Messages.get().InterfacePolling_StateDOWN);
      expectedState.add(Messages.get().InterfacePolling_StateIGNORE);
      expectedState.select(object.getExpectedState());

      addFlag(dialogArea, Interface.IF_EXCLUDE_FROM_TOPOLOGY, Messages.get().InterfacePolling_ExcludeFromTopology);
      addFlag(dialogArea, Interface.IF_INCLUDE_IN_ICMP_POLL, "&Collect ICMP response statistic for this interface");
      addFlag(dialogArea, Interface.IF_DISABLE_AGENT_STATUS_POLL, "Disable status polling with NetXMS &agent");
      addFlag(dialogArea, Interface.IF_DISABLE_SNMP_STATUS_POLL, "Disable status polling with &SNMP");
      addFlag(dialogArea, Interface.IF_DISABLE_ICMP_STATUS_POLL, "Disable status polling with &ICMP");
      
      currentPollCount = object.getRequiredPollCount();
      currentExpectedState = object.getExpectedState();
      currentFlags = object.getFlags() & collectFlagsMask();

		return dialogArea;
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
      int flags = collectFlags();
      int flagMask = collectFlagsMask();

		if ((expectedState.getSelectionIndex() == currentExpectedState) && 
			 (pollCount.getSelection() == currentPollCount) &&
            (flags == currentFlags))
			return;	// nothing to change 
		
		if (isApply)
			setValid(false);
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		final NXCObjectModificationData data = new NXCObjectModificationData(object.getObjectId());
		data.setExpectedState(expectedState.getSelectionIndex());
		data.setRequiredPolls(pollCount.getSelection());
      data.setObjectFlags(flags, flagMask);
		new ConsoleJob(Messages.get().InterfacePolling_JobName, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(data);
			}

			@Override
			protected String getErrorMessage()
			{
				return String.format(Messages.get().InterfacePolling_JobError, object.getObjectName());
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
                     currentFlags = collectFlags();
							InterfacePolling.this.setValid(true);
						}
					});
				}
			}
		}.start();
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}

   /**
    * Add checkbox for flag
    * 
    * @param value
    * @param name
    */
   private void addFlag(Composite parent, int value, String name)
   {
      final Button button = new Button(parent, SWT.CHECK);
      button.setText(name);
      GridData gd = new GridData();
      gd.horizontalSpan = 2;
      button.setLayoutData(gd);
      button.setSelection((object.getFlags() & value) != 0);
      flagButtons.add(button);
      flagValues.add(value);
   }

   /**
    * Collect new node flags from checkboxes
    * 
    * @return new node flags
    */
   private int collectFlags()
   {
      int flags = object.getFlags();
      for(int i = 0; i < flagButtons.size(); i++)
      {
         if (flagButtons.get(i).getSelection())
         {
            flags |= flagValues.get(i);
         }
         else
         {
            flags &= ~flagValues.get(i);
         }
      }
      return flags;
   }

   /**
    * Collect mask for flags being modified
    * 
    * @return
    */
   private int collectFlagsMask()
   {
      int mask = 0;
      for(int i = 0; i < flagButtons.size(); i++)
      {
         mask |= flagValues.get(i);
      }
      return mask;
   }
}

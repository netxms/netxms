/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Polling" property page for NetMS interface objects 
 */
public class InterfacePolling extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(InterfacePolling.class);

	private Spinner pollCount;
	private Combo expectedState;
   private Interface iface;
	private int currentPollCount;
	private int currentExpectedState;
   private int currentFlags;
   private List<Button> flagButtons = new ArrayList<Button>();
   private List<Integer> flagValues = new ArrayList<Integer>();

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public InterfacePolling(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(InterfacePolling.class).tr("Polling"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "interfacePolling";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 20;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return object instanceof Interface;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      iface = (Interface)object;

		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
      dialogArea.setLayout(layout);

      pollCount = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, i18n.tr("Required poll count"), 0, 1000, WidgetHelper.DEFAULT_LAYOUT_DATA);
      pollCount.setSelection(iface.getRequiredPollCount());
      
      expectedState = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, i18n.tr("Expected state"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      expectedState.add(i18n.tr("UP"));
      expectedState.add(i18n.tr("DOWN"));
      expectedState.add(i18n.tr("IGNORE"));
      expectedState.select(iface.getExpectedState());

      addFlag(dialogArea, Interface.IF_EXCLUDE_FROM_TOPOLOGY, i18n.tr("&Exclude this interface from network topology"));
      addFlag(dialogArea, Interface.IF_INCLUDE_IN_ICMP_POLL, "&Collect ICMP response statistic for this interface");
      addFlag(dialogArea, Interface.IF_DISABLE_AGENT_STATUS_POLL, "Disable status polling with NetXMS &agent");
      addFlag(dialogArea, Interface.IF_DISABLE_SNMP_STATUS_POLL, "Disable status polling with &SNMP");
      addFlag(dialogArea, Interface.IF_DISABLE_ICMP_STATUS_POLL, "Disable status polling with &ICMP");

      currentPollCount = iface.getRequiredPollCount();
      currentExpectedState = iface.getExpectedState();
      currentFlags = iface.getFlags() & collectFlagsMask();

		return dialogArea;
	}
	
   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
	{
      int flags = collectFlags();
      int flagMask = collectFlagsMask();

		if ((expectedState.getSelectionIndex() == currentExpectedState) && 
			 (pollCount.getSelection() == currentPollCount) &&
            (flags == currentFlags))
         return true; // nothing to change
		
		if (isApply)
			setValid(false);
		
      final NXCSession session = Registry.getSession();
		final NXCObjectModificationData data = new NXCObjectModificationData(object.getObjectId());
		data.setExpectedState(expectedState.getSelectionIndex());
		data.setRequiredPolls(pollCount.getSelection());
      data.setObjectFlags(flags, flagMask);
      new Job(i18n.tr("Updating interface polling configuration"), null, messageArea) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(data);
			}

			@Override
			protected String getErrorMessage()
			{
				return String.format(i18n.tr("Cannot modify interface object %s"), object.getObjectName());
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
		return true;
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
      button.setSelection((iface.getFlags() & value) != 0);
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
      int flags = 0;
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

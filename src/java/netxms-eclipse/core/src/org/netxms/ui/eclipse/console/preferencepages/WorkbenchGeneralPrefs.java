/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.console.preferencepages;

import org.eclipse.jface.action.ICoolBarManager;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.IWorkbenchPreferencePage;
import org.eclipse.ui.PlatformUI;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.console.ServerClockContributionItem;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Workbench" preference page 
 */
public class WorkbenchGeneralPrefs extends PreferencePage implements	IWorkbenchPreferencePage
{
	private Button cbShowHeapMonitor;
	private Button cbShowTrayIcon;
	private Button cbHideWhenMinimized;
	private Button cbShowHiddenAttributes;
   private Button cbShowServerClock;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		dialogArea.setLayout(layout);
		
		cbShowHeapMonitor = new Button(dialogArea, SWT.CHECK);
		cbShowHeapMonitor.setText(Messages.get().WorkbenchGeneralPrefs_show_heap);
		cbShowHeapMonitor.setSelection(getPreferenceStore().getBoolean("SHOW_MEMORY_MONITOR")); //$NON-NLS-1$
		
		cbShowTrayIcon = new Button(dialogArea, SWT.CHECK);
		cbShowTrayIcon.setText(Messages.get().WorkbenchGeneralPrefs_show_tray_icon);
		cbShowTrayIcon.setSelection(Activator.getDefault().getPreferenceStore().getBoolean("SHOW_TRAY_ICON")); //$NON-NLS-1$
		
		cbHideWhenMinimized = new Button(dialogArea, SWT.CHECK);
		cbHideWhenMinimized.setText(Messages.get().WorkbenchGeneralPrefs_HideWhenMinimized);
		cbHideWhenMinimized.setSelection(Activator.getDefault().getPreferenceStore().getBoolean("HIDE_WHEN_MINIMIZED")); //$NON-NLS-1$
		cbHideWhenMinimized.setEnabled(cbShowTrayIcon.getSelection());

		cbShowHiddenAttributes = new Button(dialogArea, SWT.CHECK);
		cbShowHiddenAttributes.setText(Messages.get().WorkbenchGeneralPrefs_ShowHiddenAttrs);
		cbShowHiddenAttributes.setSelection(Activator.getDefault().getPreferenceStore().getBoolean("SHOW_HIDDEN_ATTRIBUTES")); //$NON-NLS-1$
		
      cbShowServerClock = new Button(dialogArea, SWT.CHECK);
      cbShowServerClock.setText(Messages.get().WorkbenchGeneralPrefs_ShowServerClock);
      cbShowServerClock.setSelection(Activator.getDefault().getPreferenceStore().getBoolean("SHOW_SERVER_CLOCK")); //$NON-NLS-1$
      
		cbShowTrayIcon.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				if (cbShowTrayIcon.getSelection())
				{
					cbHideWhenMinimized.setEnabled(true);
				}
				else
				{
					cbHideWhenMinimized.setEnabled(false);
					cbHideWhenMinimized.setSelection(false);
				}
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IWorkbenchPreferencePage#init(org.eclipse.ui.IWorkbench)
	 */
	@Override
	public void init(IWorkbench workbench)
	{
		setPreferenceStore(PlatformUI.getPreferenceStore());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
	 */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		
		cbShowHeapMonitor.setSelection(true);
		cbShowTrayIcon.setSelection(true);
		cbHideWhenMinimized.setSelection(false);
		cbShowHiddenAttributes.setSelection(false);
		cbShowServerClock.setSelection(false);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		getPreferenceStore().setValue("SHOW_MEMORY_MONITOR", cbShowHeapMonitor.getSelection()); //$NON-NLS-1$
		Activator.getDefault().getPreferenceStore().setValue("SHOW_TRAY_ICON", cbShowTrayIcon.getSelection()); //$NON-NLS-1$
		Activator.getDefault().getPreferenceStore().setValue("HIDE_WHEN_MINIMIZED", cbHideWhenMinimized.getSelection() && cbShowTrayIcon.getSelection()); //$NON-NLS-1$
		Activator.getDefault().getPreferenceStore().setValue("SHOW_HIDDEN_ATTRIBUTES", cbShowHiddenAttributes.getSelection()); //$NON-NLS-1$
      Activator.getDefault().getPreferenceStore().setValue("SHOW_SERVER_CLOCK", cbShowServerClock.getSelection()); //$NON-NLS-1$
		
		if (cbShowTrayIcon.getSelection())
			Activator.showTrayIcon();
		else
			Activator.hideTrayIcon();
		
      ICoolBarManager coolBar = (ICoolBarManager)ConsoleSharedData.getProperty("CoolBarManager"); //$NON-NLS-1$
      coolBar.remove(ServerClockContributionItem.ID);
		if (cbShowServerClock.getSelection())
		{
		   coolBar.add(new ServerClockContributionItem());
	      coolBar.update(true);
	      PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell().layout(true, true);
		}
		else
		{
         coolBar.update(true);
		}
		
		return super.performOk();
	}
}

/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.perfview.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.preference.ColorSelector;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.dialogs.PropertyPage;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.perfview.Activator;
import org.netxms.ui.eclipse.perfview.PerfTabGraphSettings;
import org.netxms.ui.eclipse.tools.ColorConverter;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * DCI property page for Performance Tab settings
 *
 */
public class PerfTab extends PropertyPage
{
	private DataCollectionItem dci;
	private PerfTabGraphSettings settings;
	private Button checkShow;
	private LabeledText title;
	private ColorSelector color;
	private Button checkShowThresholds;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		dci = (DataCollectionItem)getElement().getAdapter(DataCollectionItem.class);
		try
		{
			settings = PerfTabGraphSettings.createFromXml(dci.getPerfTabSettings());
		}
		catch(Exception e)
		{
			settings = new PerfTabGraphSettings();	// Create default empty settings
		}
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
      dialogArea.setLayout(layout);
      
      checkShow = new Button(dialogArea, SWT.CHECK);
      checkShow.setText("&Show on performance tab");
      checkShow.setSelection(settings.isEnabled());
      GridData gd = new GridData();
      gd.horizontalSpan = 2;
      checkShow.setLayoutData(gd);
      
      title = new LabeledText(dialogArea, SWT.NONE);
      title.setLabel("Title");
      title.setText(settings.getTitle());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      title.setLayoutData(gd);
      
      Composite colors = new Composite(dialogArea, SWT.NONE);
      colors.setLayout(new RowLayout(SWT.VERTICAL));
      new Label(colors, SWT.NONE).setText("Color");
      color = new ColorSelector(colors);
      color.setColorValue(ColorConverter.rgbFromInt(settings.getColorAsInt()));

      checkShowThresholds = new Button(dialogArea, SWT.CHECK);
      checkShowThresholds.setText("&Show thresholds on graph");
      checkShowThresholds.setSelection(settings.isShowThresholds());
      gd = new GridData();
      gd.horizontalSpan = 2;
      checkShowThresholds.setLayoutData(gd);

      return dialogArea;
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if APPLY button was used
	 */
	private void applyChanges(final boolean isApply)
	{
		if (isApply)
			setValid(false);
		
		settings.setEnabled(checkShow.getSelection());
		settings.setTitle(title.getText());
		settings.setColor(ColorConverter.rgbToInt(color.getColorValue()));
		settings.setShowThresholds(checkShowThresholds.getSelection());
		
		try
		{
			dci.setPerfTabSettings(settings.createXml());
		}
		catch(Exception e)
		{
			dci.setPerfTabSettings(null);
		}
		
		ConsoleJob job = new ConsoleJob("Update performance tab settings for DCI " + dci.getId(), null, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return "Cannot update performance tab settings for DCI " + dci.getId();
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				dci.getOwner().modifyItem(dci);
			}

			/* (non-Javadoc)
			 * @see org.netxms.ui.eclipse.jobs.ConsoleJob#jobFinalize()
			 */
			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					new UIJob("Update \"Performance Tab\" property page") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							PerfTab.this.setValid(true);
							return Status.OK_STATUS;
						}
					}.schedule();
				}
			}
		};
		job.start();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
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
	 * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
	 */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		PerfTabGraphSettings defaults = new PerfTabGraphSettings();
		checkShow.setSelection(defaults.isEnabled());
		title.setText(defaults.getTitle());
		color.setColorValue(ColorConverter.rgbFromInt(defaults.getColorAsInt()));
	}
}

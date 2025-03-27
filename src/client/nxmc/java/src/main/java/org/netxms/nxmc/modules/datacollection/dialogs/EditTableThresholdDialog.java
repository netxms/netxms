/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.datacollection.TableThreshold;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.TableColumnEnumerator;
import org.netxms.nxmc.modules.datacollection.widgets.TableConditionsEditor;
import org.netxms.nxmc.modules.events.widgets.EventSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Table threshold editing dialog
 */
public class EditTableThresholdDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(EditTableThresholdDialog.class);
   
	private TableThreshold threshold;
	private EventSelector activationEvent;
	private EventSelector deactivationEvent;
	private LabeledSpinner sampleCount;
	private TableConditionsEditor conditionsEditor;
   private TableColumnEnumerator columnEnumerator;
	
	/**
	 * @param parentShell
	 * @param threshold
	 */
   public EditTableThresholdDialog(Shell parentShell, TableThreshold threshold, TableColumnEnumerator columnEnumerator)
	{
		super(parentShell);
		this.threshold = threshold;
      this.columnEnumerator = columnEnumerator;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(i18n.tr("Edit Table Threshold"));
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		dialogArea.setLayout(layout);
		
		activationEvent = new EventSelector(dialogArea, SWT.NONE);
		activationEvent.setLabel(i18n.tr("Activation event"));
		activationEvent.setEventCode(threshold.getActivationEvent());
		activationEvent.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
		
		deactivationEvent = new EventSelector(dialogArea, SWT.NONE);
		deactivationEvent.setLabel(i18n.tr("Deactivation event"));
		deactivationEvent.setEventCode(threshold.getDeactivationEvent());
		deactivationEvent.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
		
		sampleCount = new LabeledSpinner(dialogArea, SWT.NONE);
		sampleCount.setLabel("Sample count");
		sampleCount.setRange(1, 100000);
		sampleCount.setSelection(threshold.getSampleCount());
		
		new Label(dialogArea, SWT.NONE).setText(i18n.tr("Conditions"));
		
		conditionsEditor = new TableConditionsEditor(dialogArea, SWT.BORDER, columnEnumerator);
		GridData gd = new GridData(SWT.FILL, SWT.FILL, true, true);
		gd.widthHint = 700;
		gd.heightHint = 400;
		conditionsEditor.setLayoutData(gd);
		conditionsEditor.setConditions(threshold.getConditions());

		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		threshold.setActivationEvent((int)activationEvent.getEventCode());
		threshold.setDeactivationEvent((int)deactivationEvent.getEventCode());
		threshold.setSampleCount(sampleCount.getSelection());
		threshold.setConditions(conditionsEditor.getConditions());
		super.okPressed();
	}	
}

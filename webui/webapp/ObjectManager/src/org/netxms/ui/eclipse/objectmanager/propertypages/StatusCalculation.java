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
package org.netxms.ui.eclipse.objectmanager.propertypages;

import java.util.Arrays;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.Severity;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.NumericTextFieldValidator;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Status Calculation" property page
 */
public class StatusCalculation extends PropertyPage
{
	private GenericObject object;
	
	private NXCObjectModificationData currentState;
	private int calculationMethod;
	private int propagationMethod;
	
	private Button radioCalcDefault;
	private Button radioCalcMostCritical;
	private Button radioCalcSingle;
	private Button radioCalcMultiple;
	private Button radioPropDefault;
	private Button radioPropUnchanged;
	private Button radioPropFixed;
	private Button radioPropRelative;
	private Button radioPropTranslated;
	private Combo comboFixedStatus;
	private Combo[] comboTranslatedStatus = new Combo[4];
	private Text textRelativeStatus;
	private Text textSingleThreshold;
	private Text[] textThresholds = new Text[4];
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		object = (GenericObject)getElement().getAdapter(GenericObject.class);
		
		currentState = new NXCObjectModificationData(object.getObjectId());
		currentState.setStatusCalculationMethod(object.getStatusCalculationMethod());
		currentState.setStatusPropagationMethod(object.getStatusPropagationMethod());
		currentState.setFixedPropagatedStatus(object.getFixedPropagatedStatus());
		currentState.setStatusShift(object.getStatusShift());
		currentState.setStatusTransformation(object.getStatusTransformation());
		currentState.setStatusSingleThreshold(object.getStatusSingleThreshold());
		currentState.setStatusThresholds(object.getStatusThresholds());

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
		layout.makeColumnsEqualWidth = true;
      dialogArea.setLayout(layout);
      
      createLeftPanel(dialogArea);
      createRightPanel(dialogArea);
      
      changeCalculationMethod(object.getStatusCalculationMethod());
      changePropagationMethod(object.getStatusPropagationMethod());
      
      createCalcSelectionListener(radioCalcDefault, GenericObject.CALCULATE_DEFAULT);
      createCalcSelectionListener(radioCalcMostCritical, GenericObject.CALCULATE_MOST_CRITICAL);
      createCalcSelectionListener(radioCalcSingle, GenericObject.CALCULATE_SINGLE_THRESHOLD);
      createCalcSelectionListener(radioCalcMultiple, GenericObject.CALCULATE_MULTIPLE_THRESHOLDS);

      createPropSelectionListener(radioPropDefault, GenericObject.PROPAGATE_DEFAULT);
      createPropSelectionListener(radioPropUnchanged, GenericObject.PROPAGATE_UNCHANGED);
      createPropSelectionListener(radioPropFixed, GenericObject.PROPAGATE_FIXED);
      createPropSelectionListener(radioPropRelative, GenericObject.PROPAGATE_RELATIVE);
      createPropSelectionListener(radioPropTranslated, GenericObject.PROPAGATE_TRANSLATED);
      
      return dialogArea;
	}

	/**
	 * Create left (propagation) panel
	 * 
	 * @param parent
	 */
	private void createLeftPanel(Composite parent)
	{
		Group group = new Group(parent, SWT.NONE);
		group.setText("Propagate status as");
		
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		group.setLayoutData(gd);
		
		GridLayout layout = new GridLayout();
		group.setLayout(layout);
		
		radioPropDefault = new Button(group, SWT.RADIO);
		radioPropDefault.setText("&Default");
		radioPropDefault.setSelection(object.getStatusPropagationMethod() == GenericObject.PROPAGATE_DEFAULT);
		
		radioPropUnchanged = new Button(group, SWT.RADIO);
		radioPropUnchanged.setText("&Unchanged");
		radioPropUnchanged.setSelection(object.getStatusPropagationMethod() == GenericObject.PROPAGATE_UNCHANGED);
		
		radioPropFixed = new Button(group, SWT.RADIO);
		radioPropFixed.setText("&Fixed to value:");
		radioPropFixed.setSelection(object.getStatusPropagationMethod() == GenericObject.PROPAGATE_FIXED);
		
		comboFixedStatus = new Combo(group, SWT.BORDER | SWT.READ_ONLY);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalIndent = 20;
		comboFixedStatus.setLayoutData(gd);
		for(int i = 0; i < 5; i++)
			comboFixedStatus.add(StatusDisplayInfo.getStatusText(i));
		comboFixedStatus.select(object.getFixedPropagatedStatus());
		
		radioPropRelative = new Button(group, SWT.RADIO);
		radioPropRelative.setText("&Relative with offset:");
		radioPropRelative.setSelection(object.getStatusPropagationMethod() == GenericObject.PROPAGATE_RELATIVE);
		
		textRelativeStatus = new Text(group, SWT.BORDER);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalIndent = 20;
		textRelativeStatus.setLayoutData(gd);
		textRelativeStatus.setText(Integer.toString(object.getStatusShift()));

		radioPropTranslated = new Button(group, SWT.RADIO);
		radioPropTranslated.setText("&Severity based:");
		radioPropTranslated.setSelection(object.getStatusPropagationMethod() == GenericObject.PROPAGATE_TRANSLATED);

		Composite translationGroup = new Composite(group, SWT.NONE);
		layout = new GridLayout();
		layout.numColumns = 3;
		translationGroup.setLayout(layout);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalIndent = 20;
		translationGroup.setLayoutData(gd);
		for(int i = 0; i < 4; i++)
		{
			Label label = new Label(translationGroup, SWT.NONE);
			label.setText(StatusDisplayInfo.getStatusText(i + 1));
			gd = new GridData();
			gd.horizontalAlignment = SWT.RIGHT;
			label.setLayoutData(gd);
			
			label = new Label(translationGroup, SWT.NONE);
			label.setText("->");
			gd = new GridData();
			gd.horizontalAlignment = SWT.CENTER;
			label.setLayoutData(gd);
			
			comboTranslatedStatus[i] = new Combo(translationGroup, SWT.BORDER | SWT.READ_ONLY);
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			comboTranslatedStatus[i].setLayoutData(gd);
			for(int j = 0; j < 5; j++)
				comboTranslatedStatus[i].add(StatusDisplayInfo.getStatusText(j));
			comboTranslatedStatus[i].select(object.getStatusTransformation()[i]);
		}
	}

	/**
	 * Create right (calculation) panel
	 * 
	 * @param parent
	 */
	private void createRightPanel(Composite parent)
	{
		Group group = new Group(parent, SWT.NONE);
		group.setText("Calculate status as");
		
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		group.setLayoutData(gd);
		
		GridLayout layout = new GridLayout();
		group.setLayout(layout);
		
		radioCalcDefault = new Button(group, SWT.RADIO);
		radioCalcDefault.setText("D&efault");
		radioCalcDefault.setSelection(object.getStatusCalculationMethod() == GenericObject.CALCULATE_DEFAULT);
		
		radioCalcMostCritical = new Button(group, SWT.RADIO);
		radioCalcMostCritical.setText("&Most critical");
		radioCalcMostCritical.setSelection(object.getStatusCalculationMethod() == GenericObject.CALCULATE_MOST_CRITICAL);
		
		radioCalcSingle = new Button(group, SWT.RADIO);
		radioCalcSingle.setText("&Single threshold (%):");
		radioCalcSingle.setSelection(object.getStatusCalculationMethod() == GenericObject.CALCULATE_SINGLE_THRESHOLD);
		
		textSingleThreshold = new Text(group, SWT.BORDER);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalIndent = 20;
		textSingleThreshold.setLayoutData(gd);
		textSingleThreshold.setText(Integer.toString(object.getStatusSingleThreshold()));

		radioCalcMultiple = new Button(group, SWT.RADIO);
		radioCalcMultiple.setText("M&ultiple thresholds (%):");
		radioCalcMultiple.setSelection(object.getStatusCalculationMethod() == GenericObject.CALCULATE_MULTIPLE_THRESHOLDS);

		Composite thresholdGroup = new Composite(group, SWT.NONE);
		layout = new GridLayout();
		layout.numColumns = 2;
		thresholdGroup.setLayout(layout);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalIndent = 20;
		thresholdGroup.setLayoutData(gd);
		for(int i = 0; i < 4; i++)
		{
			final Label label = new Label(thresholdGroup, SWT.NONE);
			label.setText(StatusDisplayInfo.getStatusText(i + 1));
			gd = new GridData();
			gd.horizontalAlignment = SWT.RIGHT;
			label.setLayoutData(gd);
			
			textThresholds[i] = new Text(thresholdGroup, SWT.BORDER);
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			textThresholds[i].setLayoutData(gd);
			textThresholds[i].setText(Integer.toString(object.getStatusThresholds()[i]));
		}
	}

	/**
	 * @param method
	 */
	private void changeCalculationMethod(int method)
	{
		calculationMethod = method;
		textSingleThreshold.setEnabled(method == GenericObject.CALCULATE_SINGLE_THRESHOLD);
		for(int i = 0; i < 4; i++)
			textThresholds[i].setEnabled(method == GenericObject.CALCULATE_MULTIPLE_THRESHOLDS);
	}
	
	/**
	 * @param method
	 */
	private void changePropagationMethod(int method)
	{
		propagationMethod = method;
		comboFixedStatus.setEnabled(method == GenericObject.PROPAGATE_FIXED);
		textRelativeStatus.setEnabled(method == GenericObject.PROPAGATE_RELATIVE);
		for(int i = 0; i < 4; i++)
			comboTranslatedStatus[i].setEnabled(method == GenericObject.PROPAGATE_TRANSLATED);
	}
	
	/**
	 * @param button
	 * @param newMethod
	 */
	private void createCalcSelectionListener(Button button, final int newMethod)
	{
		button.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				changeCalculationMethod(newMethod);
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
	}
	
	/**
	 * @param button
	 * @param newMethod
	 */
	private void createPropSelectionListener(Button button, final int newMethod)
	{
		button.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				changePropagationMethod(newMethod);
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
	 */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		
		radioCalcDefault.setSelection(true);
		radioCalcMostCritical.setSelection(false);
		radioCalcMultiple.setSelection(false);
		radioCalcSingle.setSelection(false);
		
		radioPropDefault.setSelection(true);
		radioPropFixed.setSelection(false);
		radioPropRelative.setSelection(false);
		radioPropTranslated.setSelection(false);
		radioPropUnchanged.setSelection(false);
		
		changeCalculationMethod(GenericObject.CALCULATE_DEFAULT);
		changePropagationMethod(GenericObject.PROPAGATE_DEFAULT);
	}
	
	/**
	 * Check if something has been changed in status calculation/propagation
	 * 
	 * @param md
	 * @return
	 */
	private boolean hasChanges(NXCObjectModificationData md)
	{
		if ((currentState.getStatusCalculationMethod() != md.getStatusCalculationMethod()) ||
		    (currentState.getStatusPropagationMethod() != md.getStatusPropagationMethod()))
			return true;
		
		switch(md.getStatusCalculationMethod())
		{
			case GenericObject.CALCULATE_SINGLE_THRESHOLD:
				if (currentState.getStatusSingleThreshold() != md.getStatusSingleThreshold())
					return true;
				break;
			case GenericObject.CALCULATE_MULTIPLE_THRESHOLDS:
				if (!Arrays.equals(currentState.getStatusThresholds(), md.getStatusThresholds()))
					return true;
				break;
			default:
				break;
		}
		
		switch(md.getStatusPropagationMethod())
		{
			case GenericObject.PROPAGATE_FIXED:
				if (currentState.getFixedPropagatedStatus() != md.getFixedPropagatedStatus())
					return true;
				break;
			case GenericObject.PROPAGATE_RELATIVE:
				if (currentState.getStatusShift() != md.getStatusShift())
					return true;
				break;
			case GenericObject.PROPAGATE_TRANSLATED:
				if (!Arrays.equals(currentState.getStatusTransformation(), md.getStatusTransformation()))
					return true;
				break;
			default:
				break;
		}
		
		return false;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected boolean applyChanges(final boolean isApply)
	{
		if (!WidgetHelper.validateTextInput(textRelativeStatus, "Relative status", new NumericTextFieldValidator(-4, 4), this) ||
		    !WidgetHelper.validateTextInput(textSingleThreshold, "Single threshold", new NumericTextFieldValidator(0, 100), this) ||
		    !WidgetHelper.validateTextInput(textThresholds[0], "Threshold - " + StatusDisplayInfo.getStatusText(Severity.WARNING), new NumericTextFieldValidator(0, 100), this) ||
		    !WidgetHelper.validateTextInput(textThresholds[1], "Threshold - " + StatusDisplayInfo.getStatusText(Severity.MINOR), new NumericTextFieldValidator(0, 100), this) ||
		    !WidgetHelper.validateTextInput(textThresholds[2], "Threshold - " + StatusDisplayInfo.getStatusText(Severity.MAJOR), new NumericTextFieldValidator(0, 100), this) ||
		    !WidgetHelper.validateTextInput(textThresholds[3], "Threshold - " + StatusDisplayInfo.getStatusText(Severity.CRITICAL), new NumericTextFieldValidator(0, 100), this))
			return false;

		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
		md.setStatusCalculationMethod(calculationMethod);
		md.setStatusPropagationMethod(propagationMethod);
		md.setFixedPropagatedStatus(comboFixedStatus.getSelectionIndex());
		md.setStatusShift(Integer.parseInt(textRelativeStatus.getText()));
		int[] transformations = new int[4];
		for(int i = 0; i < 4; i++)
			transformations[i] = comboTranslatedStatus[i].getSelectionIndex();
		md.setStatusTransformation(transformations);
		md.setStatusSingleThreshold(Integer.parseInt(textSingleThreshold.getText()));
		int[] thresholds = new int[4];
		for(int i = 0; i < 4; i++)
			thresholds[i] = Integer.parseInt(textThresholds[i].getText());
		md.setStatusThresholds(thresholds);
		
		if (!hasChanges(md))
			return true;	// Nothing to apply
		currentState = md;

		if (isApply)
			setValid(false);
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob("Update status calculation for object " + object.getObjectName(), null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot change object's status calculation properties";
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
							StatusCalculation.this.setValid(true);
						}
					});
				}
			}
		}.start();
		
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		return applyChanges(false);
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

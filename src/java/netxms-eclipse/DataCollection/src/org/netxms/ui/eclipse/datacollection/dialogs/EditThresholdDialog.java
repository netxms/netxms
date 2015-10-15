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
package org.netxms.ui.eclipse.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.datacollection.Threshold;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.eventmanager.widgets.EventSelector;
import org.netxms.ui.eclipse.nxsl.dialogs.ScriptEditDialog;
import org.netxms.ui.eclipse.tools.NumericTextFieldValidator;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Threshold configuration dialog
 *
 */
public class EditThresholdDialog extends Dialog
{
	private Threshold threshold;
	private Combo function;
	private Combo operation;
	private Text samples;
	private Text value;
	private LabeledText script;
	private Group conditionGroup;
   private Composite scriptGroup;
	private EventSelector activationEvent;
	private EventSelector deactivationEvent;
	private Text repeatInterval;
	private Button repeatDefault;
	private Button repeatNever;
	private Button repeatCustom;
	private int selectedFunction;
	private String savedScript = null;
	private int savedOperation = -1;
	private String savedValue = null;
	
	/**
	 * @param parentShell
	 * @param threshold
	 */
	public EditThresholdDialog(Shell parentShell, Threshold threshold)
	{
		super(parentShell);
		this.threshold = threshold;
		selectedFunction = threshold.getFunction();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(final Composite parent)
	{
		final Composite dialogArea =  (Composite)super.createDialogArea(parent);
		
		GridLayout dialogLayout = new GridLayout();
		dialogArea.setLayout(dialogLayout);
		
		// Condition area
		conditionGroup = new Group(dialogArea, SWT.NONE);
		conditionGroup.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
		conditionGroup.setText(Messages.get().EditThresholdDialog_Condition);
		
		GridLayout condLayout = new GridLayout();
		condLayout.numColumns = 2;
		conditionGroup.setLayout(condLayout);
		
		function = WidgetHelper.createLabeledCombo(conditionGroup, SWT.BORDER | SWT.DROP_DOWN | SWT.READ_ONLY, Messages.get().EditThresholdDialog_Function, WidgetHelper.DEFAULT_LAYOUT_DATA);
		function.add(Messages.get().EditThresholdDialog_LastValue);
		function.add(Messages.get().EditThresholdDialog_AvgValue);
		function.add(Messages.get().EditThresholdDialog_MeanDeviation);
		function.add(Messages.get().EditThresholdDialog_Diff);
		function.add(Messages.get().EditThresholdDialog_DCError);
		function.add(Messages.get().EditThresholdDialog_Sum);
      function.add(Messages.get().EditThresholdDialog_Script);
		function.select(threshold.getFunction());
		function.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            int f = function.getSelectionIndex();
            if ((f != Threshold.F_SCRIPT) && (selectedFunction == Threshold.F_SCRIPT))
            {
               disposeScriptGroup();
               createOperGroup();
               parent.getParent().layout(true, true);
            }
            else if ((f == Threshold.F_SCRIPT) && (selectedFunction != Threshold.F_SCRIPT))
            {
               disposeOperGroup();
               createScriptGroup();
               parent.getParent().layout(true, true);
            }
            selectedFunction = f;
         }
      });
		
		samples = WidgetHelper.createLabeledText(conditionGroup, SWT.BORDER, 60, Messages.get().EditThresholdDialog_Samples, Integer.toString(threshold.getSampleCount()), WidgetHelper.DEFAULT_LAYOUT_DATA);
		samples.setTextLimit(5);
		
		if (threshold.getFunction() == Threshold.F_SCRIPT)
		   createScriptGroup();
		else
		   createOperGroup();
		
		// Event area
		Group eventGroup = new Group(dialogArea, SWT.NONE);
		eventGroup.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
		eventGroup.setText(Messages.get().EditThresholdDialog_Event);
		GridLayout eventLayout = new GridLayout();
		eventGroup.setLayout(eventLayout);
		
		activationEvent = new EventSelector(eventGroup, SWT.NONE);
		activationEvent.setLabel(Messages.get().EditThresholdDialog_ActEvent);
		activationEvent.setEventCode(threshold.getFireEvent());
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.widthHint = 550;
		activationEvent.setLayoutData(gd);
		
		deactivationEvent = new EventSelector(eventGroup, SWT.NONE);
		deactivationEvent.setLabel(Messages.get().EditThresholdDialog_DeactEvent);
		deactivationEvent.setEventCode(threshold.getRearmEvent());
		deactivationEvent.setLayoutData(gd);
		
		// Repeat area
		Group repeatGroup = new Group(dialogArea, SWT.NONE);
		repeatGroup.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
		repeatGroup.setText(Messages.get().EditThresholdDialog_RepeatEvent);
		GridLayout repeatLayout = new GridLayout();
		repeatLayout.numColumns = 3;
		repeatGroup.setLayout(repeatLayout);
		
		repeatDefault = new Button(repeatGroup, SWT.RADIO);
		repeatDefault.setText(Messages.get().EditThresholdDialog_UseDefault);
		gd = new GridData();
		gd.horizontalAlignment = SWT.LEFT;
		gd.horizontalSpan = 3;
		repeatDefault.setLayoutData(gd);
		repeatDefault.setSelection(threshold.getRepeatInterval() == -1);
		
		repeatNever = new Button(repeatGroup, SWT.RADIO);
		repeatNever.setText(Messages.get().EditThresholdDialog_Never);
		gd = new GridData();
		gd.horizontalAlignment = SWT.LEFT;
		gd.horizontalSpan = 3;
		repeatNever.setLayoutData(gd);
		repeatNever.setSelection(threshold.getRepeatInterval() == 0);
		
		repeatCustom = new Button(repeatGroup, SWT.RADIO);
		repeatCustom.setText(Messages.get().EditThresholdDialog_Every);
		repeatCustom.setSelection(threshold.getRepeatInterval() > 0);
		repeatCustom.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				repeatInterval.setEnabled(repeatCustom.getSelection());
			}
		});
		
		repeatInterval = new Text(repeatGroup, SWT.BORDER);
		repeatInterval.setTextLimit(5);
		repeatInterval.setText((threshold.getRepeatInterval() > 0) ? Integer.toString(threshold.getRepeatInterval()) : "3600"); //$NON-NLS-1$
		repeatInterval.setEnabled(threshold.getRepeatInterval() > 0);
		
		new Label(repeatGroup, SWT.NONE).setText(Messages.get().EditThresholdDialog_Seconds);
		
		return dialogArea;
	}
	
	/**
	 * Create "operation" group
	 */
	private void createOperGroup()
	{
      operation = WidgetHelper.createLabeledCombo(conditionGroup, SWT.BORDER | SWT.DROP_DOWN | SWT.READ_ONLY, Messages.get().EditThresholdDialog_Operation, WidgetHelper.DEFAULT_LAYOUT_DATA);
      operation.add(Messages.get().EditThresholdDialog_LT);
      operation.add(Messages.get().EditThresholdDialog_LE);
      operation.add(Messages.get().EditThresholdDialog_EQ);
      operation.add(Messages.get().EditThresholdDialog_GE);
      operation.add(Messages.get().EditThresholdDialog_GT);
      operation.add(Messages.get().EditThresholdDialog_NE);
      operation.add(Messages.get().EditThresholdDialog_LIKE);
      operation.add(Messages.get().EditThresholdDialog_NOTLIKE);
      operation.select((savedOperation != -1) ? savedOperation : threshold.getOperation());
      
      value = WidgetHelper.createLabeledText(conditionGroup, SWT.BORDER, 120, Messages.get().EditThresholdDialog_Value, 
            (savedValue != null) ? savedValue : threshold.getValue(), WidgetHelper.DEFAULT_LAYOUT_DATA);
	}
	
	/**
	 * Dispose "operation" group
	 */
	private void disposeOperGroup()
	{
	   savedOperation = operation.getSelectionIndex();
	   savedValue = value.getText();
	   operation.getParent().dispose();
	   value.getParent().dispose();
	}
	
	/**
	 * Create "script" group
	 */
	private void createScriptGroup()
	{
      scriptGroup = new Composite(conditionGroup, SWT.NONE);
      scriptGroup.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 1, 1));
      GridLayout scriptLayout = new GridLayout();
      scriptLayout.numColumns = 2;
      scriptLayout.marginHeight = 0;
      scriptLayout.marginWidth = 0;
      scriptLayout.horizontalSpacing = WidgetHelper.INNER_SPACING;
      scriptGroup.setLayout(scriptLayout);
      
      script = new LabeledText(scriptGroup, SWT.NONE);
      script.setLabel(Messages.get().EditThresholdDialog_Script);
      script.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      script.setText(savedScript != null ? savedScript : threshold.getScript());
      
      final Button editButton = new Button(scriptGroup, SWT.PUSH);
      editButton.setImage(SharedIcons.IMG_EDIT);
      GridData gd = new GridData();
      gd.verticalAlignment = SWT.BOTTOM;
      gd.horizontalAlignment = SWT.FILL;
      gd.heightHint = script.getTextControl().computeSize(SWT.DEFAULT, SWT.DEFAULT).y;
      editButton.setLayoutData(gd);
      editButton.setToolTipText(Messages.get().EditThresholdDialog_OpenScriptEditor);
      editButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            openScriptEditor();
         }
      });
      
      final Listener keyListener = new Listener() {
         private boolean disabled = false;
         
         @Override
         public void handleEvent(Event e)
         {
            if (((e.stateMask & SWT.MOD1) == SWT.CTRL) && (e.keyCode == 'e'))
            {
               if (disabled)
                  return;
               disabled = true;
               openScriptEditor();
               disabled = false;
            }
         }
      };
      editButton.getDisplay().addFilter(SWT.KeyDown, keyListener);
      editButton.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            editButton.getDisplay().removeFilter(SWT.KeyDown, keyListener);
         }
      });
      
      value = WidgetHelper.createLabeledText(conditionGroup, SWT.BORDER, 120, Messages.get().EditThresholdDialog_Value, 
            (savedValue != null) ? savedValue : threshold.getValue(), WidgetHelper.DEFAULT_LAYOUT_DATA);
	}
	
	/**
	 * 
	 */
	private void openScriptEditor()
	{
      ScriptEditDialog dlg = new ScriptEditDialog(getShell(), script.getText(), 
            "Variables:\r\n\t$1\t\t\tcurrent DCI value;\r\n\t$2\t\t\tthreshold value;\r\n\t$dci\t\t\tthis DCI object;\r\n\t$isCluster\ttrue if DCI is on cluster;\r\n\t$node\t\tcurrent node object (null if DCI is not on the node);\r\n\t$object\t\tcurrent object.\r\n\r\nReturn value: true if threshold violated.");
      if (dlg.open() == Window.OK)
      {
         script.setText(dlg.getScript());
      }
	}
	
	/**
	 * Dispose "script" group
	 */
	private void disposeScriptGroup()
	{
	   savedScript = script.getText();
	   scriptGroup.dispose();
	   value.getParent().dispose();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.get().EditThresholdDialog_Title);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		if (!WidgetHelper.validateTextInput(samples, Messages.get().EditThresholdDialog_Samples, new NumericTextFieldValidator(1, 1000), null))
			return;
		
		int rpt;
		if (repeatDefault.getSelection())
		{
			rpt = -1;
		}
		else if (repeatNever.getSelection())
		{
			rpt = 0;
		}
		else
		{
			if (!WidgetHelper.validateTextInput(repeatInterval, Messages.get().EditThresholdDialog_RepeatInterval, new NumericTextFieldValidator(1, 1000000), null))
				return;
			rpt = Integer.parseInt(repeatInterval.getText());
		}
		
		threshold.setFunction(function.getSelectionIndex());
		if (threshold.getFunction() == Threshold.F_SCRIPT)
		{
		   threshold.setScript(script.getText());
         threshold.setValue(value.getText());
		}
		else
		{
   		threshold.setOperation(operation.getSelectionIndex());
   		threshold.setValue(value.getText());
		}
		threshold.setSampleCount(Integer.parseInt(samples.getText()));
		threshold.setRepeatInterval(rpt);
		threshold.setFireEvent((int)activationEvent.getEventCode());
		threshold.setRearmEvent((int)deactivationEvent.getEventCode());
				
		super.okPressed();
	}
}

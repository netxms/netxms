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
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.widgets.EventSelector;
import org.netxms.nxmc.modules.nxsl.dialogs.ScriptEditDialog;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.NumericTextFieldValidator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Threshold configuration dialog
 */
public class EditThresholdDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(EditThresholdDialog.class);

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
   private Button checkDisabled;
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

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Edit Threshold"));
   }

   /**
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
      conditionGroup.setText(i18n.tr("Condition"));

		GridLayout condLayout = new GridLayout();
		condLayout.numColumns = 2;
      condLayout.makeColumnsEqualWidth = true;
		conditionGroup.setLayout(condLayout);

		function = WidgetHelper.createLabeledCombo(conditionGroup, SWT.BORDER | SWT.DROP_DOWN | SWT.READ_ONLY, i18n.tr("Function"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      function.add(i18n.tr("Last polled value"));
      function.add(i18n.tr("Average value"));
      function.add(i18n.tr("Mean deviation"));
      function.add(i18n.tr("Diff with previous value"));
      function.add(i18n.tr("Data collection error"));
      function.add(i18n.tr("Sum of values"));
      function.add(i18n.tr("Script"));
      function.add(i18n.tr("Absolute deviation"));
      function.add(i18n.tr("Anomaly"));
		function.select(threshold.getFunction());
		function.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            int f = function.getSelectionIndex();
            if ((f != Threshold.F_SCRIPT) && (selectedFunction == Threshold.F_SCRIPT))
            {
               disposeScriptGroup();
               createOperGroup(f);
               parent.getParent().layout(true, true);
            }
            else if ((f == Threshold.F_SCRIPT) && (selectedFunction != Threshold.F_SCRIPT))
            {
               disposeOperGroup();
               createScriptGroup();
               parent.getParent().layout(true, true);
            }
            if (((f == Threshold.F_ERROR) || (f == Threshold.F_ANOMALY)) && (selectedFunction != Threshold.F_ERROR) && (selectedFunction != Threshold.F_ANOMALY))
            {
               if (!operation.isDisposed())
                  operation.setEnabled(false);
               value.setEnabled(false);
            }
            else if ((f != Threshold.F_ERROR) && (f != Threshold.F_ANOMALY) && ((selectedFunction == Threshold.F_ERROR) || (selectedFunction == Threshold.F_ANOMALY)))
            {
               if (!operation.isDisposed())
                  operation.setEnabled(true);
               value.setEnabled(true);
            }
            selectedFunction = f;
         }
      });

		samples = WidgetHelper.createLabeledText(conditionGroup, SWT.BORDER, 60, i18n.tr("Samples"), Integer.toString(threshold.getSampleCount()), WidgetHelper.DEFAULT_LAYOUT_DATA);
		samples.setTextLimit(5);

		if (threshold.getFunction() == Threshold.F_SCRIPT)
		   createScriptGroup();
		else
         createOperGroup(threshold.getFunction());

		// Event area
		Group eventGroup = new Group(dialogArea, SWT.NONE);
		eventGroup.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      eventGroup.setText(i18n.tr("Event"));
		GridLayout eventLayout = new GridLayout();
		eventGroup.setLayout(eventLayout);

		activationEvent = new EventSelector(eventGroup, SWT.NONE);
      activationEvent.setLabel(i18n.tr("Activation event"));
		activationEvent.setEventCode(threshold.getFireEvent());
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.widthHint = 550;
		activationEvent.setLayoutData(gd);

      deactivationEvent = new EventSelector(eventGroup, SWT.NONE);
      deactivationEvent.setLabel(i18n.tr("Deactivation event"));
		deactivationEvent.setEventCode(threshold.getRearmEvent());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
		deactivationEvent.setLayoutData(gd);

		// Repeat area
		Group repeatGroup = new Group(dialogArea, SWT.NONE);
		repeatGroup.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      repeatGroup.setText(i18n.tr("Repeat event"));
		GridLayout repeatLayout = new GridLayout();
		repeatLayout.numColumns = 3;
		repeatGroup.setLayout(repeatLayout);

		repeatDefault = new Button(repeatGroup, SWT.RADIO);
      repeatDefault.setText(i18n.tr("Use &default settings"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.LEFT;
		gd.horizontalSpan = 3;
		repeatDefault.setLayoutData(gd);
		repeatDefault.setSelection(threshold.getRepeatInterval() == -1);

		repeatNever = new Button(repeatGroup, SWT.RADIO);
      repeatNever.setText(i18n.tr("&Never"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.LEFT;
		gd.horizontalSpan = 3;
		repeatNever.setLayoutData(gd);
		repeatNever.setSelection(threshold.getRepeatInterval() == 0);

		repeatCustom = new Button(repeatGroup, SWT.RADIO);
      repeatCustom.setText(i18n.tr("&Every"));
		repeatCustom.setSelection(threshold.getRepeatInterval() > 0);
		repeatCustom.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				repeatInterval.setEnabled(repeatCustom.getSelection());
			}
		});

		repeatInterval = new Text(repeatGroup, SWT.BORDER);
      repeatInterval.setTextLimit(10);
      repeatInterval.setText((threshold.getRepeatInterval() > 0) ? Integer.toString(threshold.getRepeatInterval()) : "3600");
      repeatInterval.setEnabled(threshold.getRepeatInterval() > 0);
      gd = new GridData();
      gd.verticalAlignment = SWT.CENTER;
      gd.widthHint = WidgetHelper.getTextWidth(repeatInterval, "0000000000");
      repeatInterval.setLayoutData(gd);

      new Label(repeatGroup, SWT.NONE).setText(i18n.tr("seconds"));

      checkDisabled = new Button(dialogArea, SWT.CHECK);
      checkDisabled.setText(i18n.tr("This threshold is &disabled"));
      checkDisabled.setSelection(threshold.isDisabled());

      dialogArea.layout(true, true); // Fix for NX-2493

		return dialogArea;
	}

	/**
    * Create "operation" group
    * 
    * @param function selected function
    */
   private void createOperGroup(int function)
	{
      operation = WidgetHelper.createLabeledCombo(conditionGroup, SWT.BORDER | SWT.DROP_DOWN | SWT.READ_ONLY, i18n.tr("Operation"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      operation.add(i18n.tr("<  : less than"));
      operation.add(i18n.tr("<= : less than or equal to"));
      operation.add(i18n.tr("== : equal to"));
      operation.add(i18n.tr(">= : greater than or equal to"));
      operation.add(i18n.tr(">  : greater than"));
      operation.add(i18n.tr("!= : not equal to"));
      operation.add(i18n.tr("like"));
      operation.add(i18n.tr("not like"));
      operation.add(i18n.tr("like (ignore case)"));
      operation.add(i18n.tr("not like (ignore case)"));
      operation.select((savedOperation != -1) ? savedOperation : threshold.getOperation());
      operation.setEnabled((function != Threshold.F_ERROR) && (function != Threshold.F_ANOMALY));

      value = WidgetHelper.createLabeledText(conditionGroup, SWT.BORDER, 120, i18n.tr("Value"), 
            (savedValue != null) ? savedValue : threshold.getValue(), WidgetHelper.DEFAULT_LAYOUT_DATA);
      value.setEnabled((function != Threshold.F_ERROR) && (function != Threshold.F_ANOMALY));
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
      script.setLabel(i18n.tr("Script"));
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.widthHint = 400;  // Prevent excessive field growth if script is big
      script.setLayoutData(gd);
      script.setText(savedScript != null ? savedScript : threshold.getScript());
      
      final Button editButton = new Button(scriptGroup, SWT.PUSH);
      editButton.setImage(SharedIcons.IMG_EDIT);
      gd = new GridData();
      gd.verticalAlignment = SWT.BOTTOM;
      gd.horizontalAlignment = SWT.FILL;
      gd.heightHint = script.getTextControl().computeSize(SWT.DEFAULT, SWT.DEFAULT).y;
      editButton.setLayoutData(gd);
      editButton.setToolTipText(i18n.tr("Open script editor (Ctrl+E)"));
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

      value = WidgetHelper.createLabeledText(conditionGroup, SWT.BORDER, 120, i18n.tr("Value"),
            (savedValue != null) ? savedValue : threshold.getValue(), WidgetHelper.DEFAULT_LAYOUT_DATA);
	}

	/**
    * Open script editor
    */
	private void openScriptEditor()
	{
      ScriptEditDialog dlg = new ScriptEditDialog(getShell(), script.getText(), 
            "Variables:\n\t$1\t\t\tcurrent DCI value;\n\t$2\t\t\tthreshold value;\n\t$dci\t\t\tthis DCI object;\n\t$isCluster\ttrue if DCI is on cluster;\n\t$node\t\tcurrent node object (null if DCI is not on the node);\n\t$object\t\tcurrent object.\n\nReturn value: true if threshold violated.");
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

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
      if (!WidgetHelper.validateTextInput(samples, new NumericTextFieldValidator(1, 1000)))
      {
         WidgetHelper.adjustWindowSize(this);
			return;
      }

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
         if (!WidgetHelper.validateTextInput(repeatInterval, new NumericTextFieldValidator(1, 1000000)))
         {
            WidgetHelper.adjustWindowSize(this);
				return;
         }
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
      threshold.setDisabled(checkDisabled.getSelection());

		super.okPressed();
	}
}

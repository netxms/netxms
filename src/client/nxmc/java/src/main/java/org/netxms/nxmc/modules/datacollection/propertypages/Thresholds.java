/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.propertypages;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.Iterator;
import java.util.List;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.constants.AnomalyDetectionMode;
import org.netxms.client.constants.AnomalyDirection;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.Threshold;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.DateTimeSelector;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.DataCollectionObjectEditor;
import org.netxms.nxmc.modules.datacollection.dialogs.EditThresholdDialog;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.ThresholdLabelProvider;
import org.netxms.nxmc.modules.events.widgets.EventSelector;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.NumericTextFieldValidator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Thresholds" page for data collection item
 */
public class Thresholds extends AbstractDCIPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(Thresholds.class);

   public static final int COLUMN_OPERATION = 0;
	public static final int COLUMN_EVENT = 1;
   public static final int COLUMN_DEACTIVATION_EVENT = 2;

	private DataCollectionItem dci;
	private List<Threshold> thresholds;
	private LabeledText instance;
   private Button checkDetectAnomalies;
   private Button checkAnomalyTuning;
   private LabeledText anomalySensitivity;
   private LabeledCombo anomalyDirection;
   private LabeledCombo anomalyMode;
   private Composite anomalyWarningArea;
   private Label anomalyWarningText;
	private Button checkAllThresholds;
   private Button disableProcessing;
   private Button disableUntil;
   private DateTimeSelector disableUntilTimeSelector;
   private SortableTableViewer thresholdList;
	private Button addButton;
	private Button modifyButton;
	private Button deleteButton;
	private Button upButton;
	private Button downButton;
   private Button duplicateButton;
   private EventSelector rearmEventSelector;

   /**
    * Create property page.
    *
    * @param editor data collection editor
    */
   public Thresholds(DataCollectionObjectEditor editor)
   {
      super(LocalizationHelper.getI18n(Thresholds.class).tr("Thresholds"), editor);
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.propertypages.AbstractDCIPropertyPage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = (Composite)super.createContents(parent);
		dci = editor.getObjectAsItem();

		thresholds = new ArrayList<Threshold>(dci.getThresholds().size());
		for(Threshold t : dci.getThresholds())
			thresholds.add(new Threshold(t));

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		dialogArea.setLayout(layout);

		instance = new LabeledText(dialogArea, SWT.NONE);
      instance.setLabel(i18n.tr("Instance name"));
		instance.setText(dci.getInstanceName());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		instance.setLayoutData(gd);
		if (dci.getTemplateId() == dci.getNodeId())	// DCI created by instance discovery
			instance.getTextControl().setEditable(false);

      checkDetectAnomalies = new Button(dialogArea, SWT.CHECK);
      checkDetectAnomalies.setText(i18n.tr("Detect anomalies"));
      checkDetectAnomalies.setSelection(dci.isAnomalyDetectionEnabled());
      checkDetectAnomalies.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            updateAnomalyControls();
         }
      });

      checkAnomalyTuning = new Button(dialogArea, SWT.CHECK);
      checkAnomalyTuning.setText(i18n.tr("Tune anomaly detection settings using AI"));
      checkAnomalyTuning.setSelection(dci.isAnomalyTuningEnabled());

      anomalyWarningArea = new Composite(dialogArea, SWT.BORDER);
      anomalyWarningArea.setBackground(ThemeEngine.getBackgroundColor("MessageBar"));
      anomalyWarningArea.setLayout(new GridLayout(2, false));
      anomalyWarningArea.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      final Label anomalyWarningImage = new Label(anomalyWarningArea, SWT.NONE);
      anomalyWarningImage.setBackground(anomalyWarningArea.getBackground());
      anomalyWarningImage.setImage(ResourceManager.getImage("icons/warning.png"));
      anomalyWarningImage.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, false, false));
      anomalyWarningImage.addDisposeListener((e) -> anomalyWarningImage.getImage().dispose());

      anomalyWarningText = new Label(anomalyWarningArea, SWT.WRAP);
      anomalyWarningText.setBackground(anomalyWarningArea.getBackground());
      anomalyWarningText.setForeground(ThemeEngine.getForegroundColor("MessageBar"));
      anomalyWarningText.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Composite anomalySettingsArea = new Composite(dialogArea, SWT.NONE);
      GridLayout anomalySettingsLayout = new GridLayout();
      anomalySettingsLayout.numColumns = 3;
      anomalySettingsLayout.makeColumnsEqualWidth = true;
      anomalySettingsLayout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      anomalySettingsLayout.marginWidth = 0;
      anomalySettingsLayout.marginHeight = 0;
      anomalySettingsArea.setLayout(anomalySettingsLayout);
      anomalySettingsArea.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      anomalySensitivity = new LabeledText(anomalySettingsArea, SWT.NONE);
      anomalySensitivity.setLabel(i18n.tr("Anomaly sensitivity"));
      anomalySensitivity.setText(Double.toString(dci.getAnomalySensitivity()));
      anomalySensitivity.getTextControl().setToolTipText(i18n.tr("Maximum deviation from expected value, in standard deviations, that is still considered normal. Lower value makes detection more sensitive."));
      anomalySensitivity.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      anomalyDirection = new LabeledCombo(anomalySettingsArea, SWT.NONE, SWT.BORDER | SWT.READ_ONLY);
      anomalyDirection.setLabel(i18n.tr("Report deviations"));
      anomalyDirection.add(i18n.tr("In both directions"));
      anomalyDirection.add(i18n.tr("Up only"));
      anomalyDirection.add(i18n.tr("Down only"));
      anomalyDirection.select(dci.getAnomalyDirection().getValue());
      anomalyDirection.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      anomalyMode = new LabeledCombo(anomalySettingsArea, SWT.NONE, SWT.BORDER | SWT.READ_ONLY);
      anomalyMode.setLabel(i18n.tr("Analyze"));
      anomalyMode.add(i18n.tr("Automatic selection"));
      anomalyMode.add(i18n.tr("Value"));
      anomalyMode.add(i18n.tr("Rate of change"));
      anomalyMode.select(dci.getAnomalyDetectionMode().getValue());
      anomalyMode.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      updateAnomalyControls();

		checkAllThresholds = new Button(dialogArea, SWT.CHECK);
      checkAllThresholds.setText(i18n.tr("Process all thresholds"));
		checkAllThresholds.setSelection(dci.isProcessAllThresholds());

      disableProcessing = new Button(dialogArea, SWT.CHECK);
      disableProcessing.setText(i18n.tr("Disable threshold processing"));
      disableProcessing.setSelection(dci.getThresholdDisableEndTime() != 0);
      disableProcessing.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            disableUntil.setEnabled(disableProcessing.getSelection());
            disableUntilTimeSelector.setEnabled(disableProcessing.getSelection() && disableUntil.getSelection());
         }
      });

      disableUntil = new Button(dialogArea, SWT.CHECK);
      disableUntil.setText(i18n.tr("Re-enable threshold processing at given time"));
      disableUntil.setSelection(dci.getThresholdDisableEndTime() > 0);
      disableUntil.setEnabled(disableProcessing.getSelection());
      disableUntil.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            disableUntilTimeSelector.setEnabled(disableProcessing.getSelection() && disableUntil.getSelection());
         }
      });

      disableUntilTimeSelector = new DateTimeSelector(dialogArea, SWT.NONE);
      disableUntilTimeSelector.setEnabled(disableProcessing.getSelection() && disableUntil.getSelection());
      if (dci.getThresholdDisableEndTime() > 0)
         disableUntilTimeSelector.setValue(new Date(dci.getThresholdDisableEndTime() * 1000L));

		Composite thresholdArea = new Composite(dialogArea, SWT.NONE);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.horizontalSpan = 2;
		thresholdArea.setLayoutData(gd);
		layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
		thresholdArea.setLayout(layout);

      new Label(thresholdArea, SWT.NONE).setText(i18n.tr("Thresholds"));

      final String[] names = { i18n.tr("Expression"), i18n.tr("Activation event"), i18n.tr("Deactivation event") };
      final int[] widths = { 300, 150, 150 };
      thresholdList = new SortableTableViewer(thresholdArea, names, widths, 0, -1, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION | SWT.H_SCROLL | SWT.V_SCROLL);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.horizontalSpan = 2;
		thresholdList.getControl().setLayoutData(gd);
      thresholdList.setContentProvider(new ArrayContentProvider());
      thresholdList.setLabelProvider(new ThresholdLabelProvider());
      thresholdList.disableSorting();
		thresholdList.setInput(thresholds.toArray());
      thresholdList.packColumns();

		Composite leftButtons = new Composite(thresholdArea, SWT.NONE);
		gd = new GridData();
		gd.horizontalAlignment = SWT.LEFT;
		leftButtons.setLayoutData(gd);
		RowLayout buttonsLayout = new RowLayout(SWT.HORIZONTAL);
		buttonsLayout.marginBottom = 0;
		buttonsLayout.marginLeft = 0;
		buttonsLayout.marginRight = 0;
		buttonsLayout.marginTop = 0;
		buttonsLayout.spacing = WidgetHelper.OUTER_SPACING;
		buttonsLayout.fill = true;
		buttonsLayout.pack = false;
		leftButtons.setLayout(buttonsLayout);

		upButton = new Button(leftButtons, SWT.PUSH);
      upButton.setText(i18n.tr("&Up"));
		upButton.setEnabled(false);
      upButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				moveUp();
			}
		});

		downButton = new Button(leftButtons, SWT.PUSH);
      downButton.setText(i18n.tr("&Down"));
		downButton.setEnabled(false);
      downButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				moveDown();
			}
		});

		Composite buttons = new Composite(thresholdArea, SWT.NONE);
		gd = new GridData();
		gd.horizontalAlignment = SWT.RIGHT;
		buttons.setLayoutData(gd);
		buttonsLayout = new RowLayout(SWT.HORIZONTAL);
		buttonsLayout.marginBottom = 0;
		buttonsLayout.marginLeft = 0;
		buttonsLayout.marginRight = 0;
		buttonsLayout.marginTop = 0;
		buttonsLayout.spacing = WidgetHelper.OUTER_SPACING;
		buttonsLayout.fill = true;
		buttonsLayout.pack = false;
		buttons.setLayout(buttonsLayout);

		addButton = new Button(buttons, SWT.PUSH);
      addButton.setText(i18n.tr("&Add..."));
		RowData rd = new RowData();
		rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
		addButton.setLayoutData(rd);
      addButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				addThreshold();
			}
		});
		
		duplicateButton = new Button(buttons, SWT.PUSH);
      duplicateButton.setText(i18n.tr("Du&plicate"));
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      duplicateButton.setLayoutData(rd);
      duplicateButton.setEnabled(false);
      duplicateButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            duplicateThreshold();
         }
      });

		modifyButton = new Button(buttons, SWT.PUSH);
      modifyButton.setText(i18n.tr("&Edit..."));
		rd = new RowData();
		rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
		modifyButton.setLayoutData(rd);
		modifyButton.setEnabled(false);
      modifyButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				editThreshold();
			}
		});

		deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(i18n.tr("De&lete"));
		rd = new RowData();
		rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
		deleteButton.setLayoutData(rd);
		deleteButton.setEnabled(false);
      deleteButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				deleteThresholds();
			}
		});

      rearmEventSelector = new EventSelector(dialogArea, SWT.NONE);
      rearmEventSelector.setLabel(i18n.tr("Generate event when all thresholds are deactivated"));
      rearmEventSelector.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      rearmEventSelector.setEventCode(dci.getAllThresholdsRearmEvent());

		/*** Selection change listener for thresholds list ***/
      thresholdList.addSelectionChangedListener((event) -> {
         IStructuredSelection selection = (IStructuredSelection)event.getSelection();
         int index = thresholds.indexOf(selection.getFirstElement());
         upButton.setEnabled((selection.size() == 1) && (index > 0));
         downButton.setEnabled((selection.size() == 1) && (index >= 0) && (index < thresholds.size() - 1));
         modifyButton.setEnabled(selection.size() == 1);
         deleteButton.setEnabled(selection.size() > 0);
         duplicateButton.setEnabled(selection.size() > 0);
		});

      thresholdList.addDoubleClickListener((e) -> editThreshold());

		return dialogArea;
	}

	/**
	 * Delete selected thresholds
	 */
	private void deleteThresholds()
	{
      final IStructuredSelection selection = thresholdList.getStructuredSelection();
		if (!selection.isEmpty())
		{
			Iterator<?> it = selection.iterator();
			while(it.hasNext())
			{
				thresholds.remove(it.next());
			}
			thresholdList.setInput(thresholds.toArray());
         thresholdList.packColumns();
		}
	}

	/**
	 * Edit selected threshold
	 */
	private void editThreshold()
	{
      final IStructuredSelection selection = thresholdList.getStructuredSelection();
		if (selection.size() == 1)
		{
			final Threshold threshold = (Threshold)selection.getFirstElement();
			EditThresholdDialog dlg = new EditThresholdDialog(getShell(), threshold);
			if (dlg.open() == Window.OK)
			{
				thresholdList.update(threshold, null);
            thresholdList.packColumns();
			}
		}
	}

	/**
	 * Add new threshold
	 */
	private void addThreshold()
	{
		Threshold threshold = new Threshold();
		EditThresholdDialog dlg = new EditThresholdDialog(getShell(), threshold);
		if (dlg.open() == Window.OK)
		{
			thresholds.add(threshold);
			thresholdList.setInput(thresholds.toArray());
         thresholdList.packColumns();
			thresholdList.setSelection(new StructuredSelection(threshold));
		}
	}

	/**
	 * Duplicate selected threshold
	 */
	@SuppressWarnings("unchecked")
   private void duplicateThreshold()
	{
      final IStructuredSelection selection = thresholdList.getStructuredSelection();
      if (selection.size() > 0)
      {
         List<Threshold> list = selection.toList();
         for(Threshold t : list)
         {               
            thresholds.add(thresholds.indexOf(t) + 1, t.duplicate());
            thresholdList.setInput(thresholds.toArray());
         }
      }
	}

	/**
	 * Move currently selected threshold up
	 */
	private void moveUp()
	{
      final IStructuredSelection selection = thresholdList.getStructuredSelection();
		if (selection.size() == 1)
		{
			final Threshold threshold = (Threshold)selection.getFirstElement();

			int index = thresholds.indexOf(threshold);
			if (index > 0)
			{
				Collections.swap(thresholds, index - 1, index);
				thresholdList.setInput(thresholds.toArray());
				thresholdList.setSelection(new StructuredSelection(threshold));
			}
		}
	}

	/**
	 * Move currently selected threshold down
	 */
	private void moveDown()
	{
      final IStructuredSelection selection = thresholdList.getStructuredSelection();
		if (selection.size() == 1)
		{
			final Threshold threshold = (Threshold)selection.getFirstElement();

			int index = thresholds.indexOf(threshold);
			if ((index < thresholds.size() - 1) && (index >= 0))
			{
				Collections.swap(thresholds, index + 1, index);
				thresholdList.setInput(thresholds.toArray());
				thresholdList.setSelection(new StructuredSelection(threshold));
			}
		}
	}

   /**
    * Enable or disable anomaly detection controls depending on anomaly detection being enabled
    */
   private void updateAnomalyControls()
   {
      boolean enabled = checkDetectAnomalies.getSelection();
      checkAnomalyTuning.setEnabled(enabled);
      anomalySensitivity.setEnabled(enabled);
      anomalyDirection.setEnabled(enabled);
      anomalyMode.setEnabled(enabled);
      updateAnomalyWarning();
   }

   /**
    * Show or hide warning about storage settings that do not let anomaly detection work correctly. Storage settings are
    * taken as currently set on "General" page.
    */
   private void updateAnomalyWarning()
   {
      String message = null;
      if (checkDetectAnomalies.getSelection())
      {
         if (editor.getRetentionType() == DataCollectionObject.RETENTION_NONE)
         {
            message = i18n.tr("Collected values are not stored, so anomaly detection will not be able to learn.");
         }
         else
         {
            StringBuilder sb = new StringBuilder();
            int retentionTime = getRetentionTime();
            if ((retentionTime > 0) && (retentionTime < DataCollectionItem.ANOMALY_MIN_RETENTION_TIME))
            {
               sb.append(i18n.tr("History retention period ({0} days) is shorter than {1} days used by anomaly detection for learning. Detection will be less accurate.",
                     Integer.toString(retentionTime), Integer.toString(DataCollectionItem.ANOMALY_MIN_RETENTION_TIME)));
            }
            if (editor.getSampleSaveInterval() > 1)
            {
               if (sb.length() > 0)
                  sb.append('\n');
               sb.append(i18n.tr("Not every collected value is stored. Anomaly detection will be less accurate."));
            }
            if (sb.length() > 0)
               message = sb.toString();
         }
      }

      boolean visible = (message != null);
      if (visible)
         anomalyWarningText.setText(message);
      if ((anomalyWarningArea.getVisible() != visible) || visible)
      {
         anomalyWarningArea.setVisible(visible);
         ((GridData)anomalyWarningArea.getLayoutData()).exclude = !visible;
         anomalyWarningArea.getParent().layout(true, true);
      }
   }

   /**
    * Get history retention time (in days) as currently set on "General" page.
    *
    * @return history retention time in days, or 0 if it cannot be determined (custom retention time is not a number)
    */
   private int getRetentionTime()
   {
      if (editor.getRetentionType() == DataCollectionObject.RETENTION_DEFAULT)
         return Registry.getSession().getDefaultDciRetentionTime();
      try
      {
         return Math.max(Integer.parseInt(editor.getRetentionTime().trim()), 1);
      }
      catch(Exception e)
      {
         return 0; // empty or contains macros
      }
   }

   /**
    * @see org.eclipse.jface.dialogs.DialogPage#setVisible(boolean)
    */
   @Override
   public void setVisible(boolean visible)
   {
      super.setVisible(visible);
      if (visible && (anomalyWarningArea != null) && !anomalyWarningArea.isDisposed())
         updateAnomalyWarning();
   }

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected boolean applyChanges(final boolean isApply)
	{
      if (checkDetectAnomalies.getSelection())
      {
         if (!WidgetHelper.validateTextInput(anomalySensitivity, new NumericTextFieldValidator(0.1, 100.0)))
            return false;
         dci.setAnomalySensitivity(Double.parseDouble(anomalySensitivity.getText()));
         dci.setAnomalyDirection(AnomalyDirection.getByValue(anomalyDirection.getSelectionIndex()));
         dci.setAnomalyDetectionMode(AnomalyDetectionMode.getByValue(anomalyMode.getSelectionIndex()));
      }

		dci.setInstanceName(instance.getText());
      dci.setAnomalyDetectionEnabled(checkDetectAnomalies.getSelection(), checkAnomalyTuning.getSelection());
      dci.setProcessAllThresholds(checkAllThresholds.getSelection());
		dci.getThresholds().clear();
		dci.getThresholds().addAll(thresholds);
      dci.setAllThresholdsRearmEvent(rearmEventSelector.getEventCode());

      long disableEndTime;
      if (disableProcessing.getSelection())
      {
         if (disableUntil.getSelection())
         {
            disableEndTime = disableUntilTimeSelector.getValue().getTime() / 1000;
         }
         else
         {
            disableEndTime = -1; // permanently disabled
         }
      }
      else
      {
         disableEndTime = 0; // enabled
      }
      dci.setThresholdDisableEndTime(disableEndTime);

		editor.modify();
		return true;
	}
}

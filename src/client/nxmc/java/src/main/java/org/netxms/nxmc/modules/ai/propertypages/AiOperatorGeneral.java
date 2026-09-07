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
package org.netxms.nxmc.modules.ai.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.ai.AiOperator;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.LabeledDurationInput;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "General" property page for AI operator instance
 */
public class AiOperatorGeneral extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(AiOperatorGeneral.class);

   private AiOperator operator;
   private LabeledText textName;
   private LabeledText textDescription;
   private LabeledText textScopeFilter;
   private LabeledText textModelSlot;
   private LabeledDurationInput minInterval;
   private LabeledDurationInput maxInterval;
   private LabeledSpinner spinnerTokenBudget;
   private LabeledSpinner spinnerRetentionDays;
   private LabeledSpinner spinnerMaxRecords;
   private LabeledText textPersonaPrompt;
   private Button checkEnabled;

   /**
    * Create page.
    *
    * @param operator operator instance to edit
    */
   public AiOperatorGeneral(AiOperator operator)
   {
      super(LocalizationHelper.getI18n(AiOperatorGeneral.class).tr("General"));
      noDefaultAndApplyButton();
      this.operator = operator;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      textName = new LabeledText(dialogArea, SWT.NONE);
      textName.setLabel(i18n.tr("Name"));
      textName.getTextControl().setTextLimit(63);
      textName.setText(operator.getName());
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.widthHint = 300;
      textName.setLayoutData(gd);

      textModelSlot = new LabeledText(dialogArea, SWT.NONE);
      textModelSlot.setLabel(i18n.tr("Model slot (empty to use default)"));
      textModelSlot.getTextControl().setTextLimit(63);
      textModelSlot.setText(operator.getModelSlot());
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.widthHint = 200;
      textModelSlot.setLayoutData(gd);

      textDescription = new LabeledText(dialogArea, SWT.NONE);
      textDescription.setLabel(i18n.tr("Description"));
      textDescription.setText(operator.getDescription());
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      textDescription.setLayoutData(gd);

      textScopeFilter = new LabeledText(dialogArea, SWT.NONE);
      textScopeFilter.setLabel(i18n.tr("Scope filter (object names or IDs, empty for all accessible objects)"));
      textScopeFilter.setText(operator.getScopeFilter());
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      textScopeFilter.setLayoutData(gd);

      textPersonaPrompt = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI | SWT.V_SCROLL | SWT.WRAP);
      textPersonaPrompt.setLabel(i18n.tr("Persona instructions"));
      textPersonaPrompt.setText(operator.getPersonaPrompt());
      gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.horizontalSpan = 2;
      gd.heightHint = 150;
      gd.widthHint = 500;
      textPersonaPrompt.setLayoutData(gd);

      minInterval = new LabeledDurationInput(dialogArea, SWT.NONE);
      minInterval.setLabel(i18n.tr("Minimum interval"));
      minInterval.setRange(60, 604800);
      minInterval.setValue(operator.getMinInterval());
      minInterval.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      maxInterval = new LabeledDurationInput(dialogArea, SWT.NONE);
      maxInterval.setLabel(i18n.tr("Maximum interval"));
      maxInterval.setRange(60, 604800);
      maxInterval.setValue(operator.getMaxInterval());
      maxInterval.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      spinnerTokenBudget = new LabeledSpinner(dialogArea, SWT.NONE);
      spinnerTokenBudget.setLabel(i18n.tr("Daily token budget (0 = unlimited)"));
      spinnerTokenBudget.setRange(0, 1000000000);
      spinnerTokenBudget.setSelection(operator.getDailyTokenBudget());
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      spinnerTokenBudget.setLayoutData(gd);

      spinnerRetentionDays = new LabeledSpinner(dialogArea, SWT.NONE);
      spinnerRetentionDays.setLabel(i18n.tr("Observation retention (days, 0 = server default)"));
      spinnerRetentionDays.setRange(0, 3650);
      spinnerRetentionDays.setSelection(operator.getObservationRetentionDays());
      spinnerRetentionDays.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      spinnerMaxRecords = new LabeledSpinner(dialogArea, SWT.NONE);
      spinnerMaxRecords.setLabel(i18n.tr("Observation record limit (0 = server default)"));
      spinnerMaxRecords.setRange(0, 1000000);
      spinnerMaxRecords.setSelection(operator.getObservationMaxRecords());
      spinnerMaxRecords.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      checkEnabled = new Button(dialogArea, SWT.CHECK);
      checkEnabled.setText(i18n.tr("&Enabled"));
      checkEnabled.setSelection(operator.isEnabled());
      gd = new GridData(SWT.LEFT, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      checkEnabled.setLayoutData(gd);

      if (operator.getId() == 0)
         textName.setFocus();

      return dialogArea;
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
   {
      String name = textName.getText().trim();
      if (name.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Instance name cannot be empty"));
         return false;
      }

      if (!minInterval.validate() || !maxInterval.validate())
         return false;

      if (maxInterval.getValue() < minInterval.getValue())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Maximum interval cannot be less than minimum interval"));
         return false;
      }

      operator.setName(name);
      operator.setDescription(textDescription.getText().trim());
      operator.setScopeFilter(textScopeFilter.getText().trim());
      operator.setModelSlot(textModelSlot.getText().trim());
      operator.setMinInterval(minInterval.getValue());
      operator.setMaxInterval(maxInterval.getValue());
      operator.setDailyTokenBudget(spinnerTokenBudget.getSelection());
      operator.setPersonaPrompt(textPersonaPrompt.getText().trim());
      operator.setObservationRetentionDays(spinnerRetentionDays.getSelection());
      operator.setObservationMaxRecords(spinnerMaxRecords.getSelection());
      operator.setEnabled(checkEnabled.getSelection());
      return true;
   }
}

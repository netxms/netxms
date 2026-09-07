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
package org.netxms.nxmc.modules.ai.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.ai.AiOperatorCheck;
import org.netxms.client.constants.AiCheckAction;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledDurationInput;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptEditor;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * AI operator standing check edit dialog
 */
public class AiOperatorCheckEditDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(AiOperatorCheckEditDialog.class);

   private AiOperatorCheck check;
   private LabeledText textName;
   private LabeledText textDescription;
   private ObjectSelector objectSelector;
   private LabeledCombo comboAction;
   private LabeledDurationInput interval;
   private LabeledDurationInput cooldown;
   private LabeledDurationInput renotifyInterval;
   private ScriptEditor scriptEditor;
   private Button checkEnabled;
   private Button checkLocked;

   /**
    * Create check edit dialog.
    *
    * @param parentShell parent shell
    * @param check check to edit (ID 0 for new check)
    */
   public AiOperatorCheckEditDialog(Shell parentShell, AiOperatorCheck check)
   {
      super(parentShell);
      this.check = check;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText((check.getId() == 0) ? i18n.tr("Create Standing Check") : i18n.tr("Edit Standing Check"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#isResizable()
    */
   @Override
   protected boolean isResizable()
   {
      return true;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.numColumns = 3;
      dialogArea.setLayout(layout);

      textName = new LabeledText(dialogArea, SWT.NONE);
      textName.setLabel(i18n.tr("Name"));
      textName.getTextControl().setTextLimit(63);
      textName.setText(check.getName());
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 3;
      gd.widthHint = 600;
      textName.setLayoutData(gd);

      textDescription = new LabeledText(dialogArea, SWT.NONE);
      textDescription.setLabel(i18n.tr("Description (what the check watches and why)"));
      textDescription.getTextControl().setTextLimit(255);
      textDescription.setText(check.getDescription());
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 3;
      textDescription.setLayoutData(gd);

      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, true);
      objectSelector.setLabel(i18n.tr("Object bound as $object"));
      objectSelector.setObjectClass(AbstractObject.class);
      objectSelector.setObjectId(check.getObjectId());
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      objectSelector.setLayoutData(gd);

      comboAction = new LabeledCombo(dialogArea, SWT.NONE);
      comboAction.setLabel(i18n.tr("Action when fired"));
      comboAction.add(i18n.tr("Wake operator"));
      comboAction.add(i18n.tr("Record observation"));
      comboAction.select((check.getAction() == AiCheckAction.OBSERVE) ? 1 : 0);
      comboAction.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      interval = new LabeledDurationInput(dialogArea, SWT.NONE);
      interval.setLabel(i18n.tr("Run interval"));
      interval.setRange(30, 604800);
      interval.setValue(check.getInterval());
      interval.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      cooldown = new LabeledDurationInput(dialogArea, SWT.NONE);
      cooldown.setLabel(i18n.tr("Cooldown (0 = none)"));
      cooldown.setRange(0, 604800);
      cooldown.setValue(check.getCooldown());
      cooldown.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      renotifyInterval = new LabeledDurationInput(dialogArea, SWT.NONE);
      renotifyInterval.setLabel(i18n.tr("Renotify interval (0 = edge only)"));
      renotifyInterval.setRange(0, 604800);
      renotifyInterval.setValue(check.getRenotifyInterval());
      renotifyInterval.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      String hints = i18n.tr("Variables:\n\t$object\t\tbound object (null if none)\n\nReturn value:\n\tnull or false - quiet\n\tstring - fired, string used as title\n\thash with title, severity, details - fired\n\tanything else - check error");
      scriptEditor = new ScriptEditor(dialogArea, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL, hints);
      scriptEditor.setText(check.getSource());
      gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.horizontalSpan = 3;
      gd.heightHint = 300;
      scriptEditor.setLayoutData(gd);

      checkEnabled = new Button(dialogArea, SWT.CHECK);
      checkEnabled.setText(i18n.tr("&Enabled"));
      checkEnabled.setSelection(check.isEnabled());
      checkEnabled.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, true, false));

      checkLocked = new Button(dialogArea, SWT.CHECK);
      checkLocked.setText(i18n.tr("&Locked (operator cannot modify or delete)"));
      checkLocked.setSelection(check.isLocked());
      gd = new GridData(SWT.LEFT, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      checkLocked.setLayoutData(gd);

      if (check.getId() != 0)
      {
         LabeledText textState = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI | SWT.WRAP | SWT.READ_ONLY);
         textState.setLabel(i18n.tr("Run state"));
         StringBuilder sb = new StringBuilder();
         sb.append(i18n.tr("Runs: ")).append(check.getRunCount());
         sb.append(i18n.tr(", last run: ")).append(formatTime(check.getLastRun()));
         sb.append(i18n.tr(", last fire: ")).append(formatTime(check.getLastFire()));
         sb.append(i18n.tr(", consecutive errors: ")).append(check.getConsecutiveErrors());
         if (!check.getLastPayload().isEmpty())
            sb.append(i18n.tr("\nLast payload: ")).append(check.getLastPayload());
         if (!check.getCompileError().isEmpty())
            sb.append(i18n.tr("\nCompilation error: ")).append(check.getCompileError());
         textState.setText(sb.toString());
         gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
         gd.horizontalSpan = 3;
         textState.setLayoutData(gd);
      }

      if (check.getId() == 0)
         textName.setFocus();

      return dialogArea;
   }

   /**
    * Format timestamp for display
    *
    * @param time timestamp or null
    * @return formatted time or "never"
    */
   private String formatTime(java.util.Date time)
   {
      return (time != null) ? DateFormatFactory.getDateTimeFormat().format(time) : i18n.tr("never");
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      String name = textName.getText().trim();
      if (name.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Check name cannot be empty"));
         return;
      }

      String source = scriptEditor.getText().trim();
      if (source.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Check script cannot be empty"));
         return;
      }

      if (!interval.validate() || !cooldown.validate() || !renotifyInterval.validate())
         return;

      check.setName(name);
      check.setDescription(textDescription.getText().trim());
      check.setObjectId(objectSelector.getObjectId());
      check.setAction((comboAction.getSelectionIndex() == 1) ? AiCheckAction.OBSERVE : AiCheckAction.WAKE);
      check.setInterval(interval.getValue());
      check.setCooldown(cooldown.getValue());
      check.setRenotifyInterval(renotifyInterval.getValue());
      check.setSource(source);
      check.setEnabled(checkEnabled.getSelection());
      check.setLocked(checkLocked.getSelection());
      super.okPressed();
   }

   /**
    * Get edited check.
    *
    * @return edited check
    */
   public AiOperatorCheck getCheck()
   {
      return check;
   }
}

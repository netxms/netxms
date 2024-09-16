/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.events.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.events.ActionExecutionConfiguration;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Edit action execution configuration
 */
public class ActionExecutionConfigurationDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(ActionExecutionConfigurationDialog.class);

   private ActionExecutionConfiguration configuration;
   private LabeledText timerDelay;
   private LabeledText timerKey;
   private LabeledText snoozeTime;
   private LabeledText blockingTimerKey;
   private Button checkActive;

   /**
    * @param parentShell
    */
   public ActionExecutionConfigurationDialog(Shell parentShell, ActionExecutionConfiguration configuration)
   {
      super(parentShell);
      this.configuration = configuration;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Action Execution Configuration"));
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
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);


      Group activeGroup = new Group(dialogArea, SWT.NONE);
      activeGroup.setLayout(new RowLayout(SWT.VERTICAL));
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      activeGroup.setLayoutData(gd);
      
      checkActive = new Button(activeGroup, SWT.CHECK);
      checkActive.setText("Active");
      checkActive.setSelection(configuration.isActive());
      
      timerDelay = new LabeledText(dialogArea, SWT.NONE);
      timerDelay.setLabel(i18n.tr("Delay"));
      timerDelay.setText(configuration.getTimerDelay());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.widthHint = 200;
      timerDelay.setLayoutData(gd);
      
      timerKey = new LabeledText(dialogArea, SWT.NONE);
      timerKey.setLabel(i18n.tr("Delay timer key"));
      timerKey.setText(configuration.getTimerKey());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 400;
      timerKey.setLayoutData(gd);
      
      snoozeTime = new LabeledText(dialogArea, SWT.NONE);
      snoozeTime.setLabel(i18n.tr("Snooze time"));
      snoozeTime.setText(configuration.getSnoozeTime());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      snoozeTime.setLayoutData(gd);
      
      blockingTimerKey = new LabeledText(dialogArea, SWT.NONE);
      blockingTimerKey.setLabel(i18n.tr("Snooze/blocking timer key (Do not run action if this timer exists)"));
      blockingTimerKey.setText(configuration.getBlockingTimerKey());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 400;
      blockingTimerKey.setLayoutData(gd);      
      
      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      configuration.setTimerDelay(timerDelay.getText().trim());
      configuration.setTimerKey(timerKey.getText().trim());
      configuration.setSnoozeTime(snoozeTime.getText().trim());
      configuration.setBlockingTimerKey(blockingTimerKey.getText().trim());
      configuration.setActive(checkActive.getSelection());
      super.okPressed();
   }
}

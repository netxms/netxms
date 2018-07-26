/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.epp.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.events.ActionExecutionConfiguration;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Edit action execution configuration
 */
public class ActionExecutionConfigurationDialog extends Dialog
{
   private ActionExecutionConfiguration configuration;
   private LabeledSpinner timerDelay;
   private LabeledText timerKey;

   /**
    * @param parentShell
    */
   public ActionExecutionConfigurationDialog(Shell parentShell, ActionExecutionConfiguration configuration)
   {
      super(parentShell);
      this.configuration = configuration;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Action Execution Configuration");
   }

   /* (non-Javadoc)
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
      
      timerDelay = new LabeledSpinner(dialogArea, SWT.NONE);
      timerDelay.setLabel("Delay");
      timerDelay.setRange(0, 100000000);
      timerDelay.setSelection(configuration.getTimerDelay());
      
      timerKey = new LabeledText(dialogArea, SWT.NONE);
      timerKey.setLabel("Timer key");
      timerKey.setText(configuration.getTimerKey());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 400;
      timerKey.setLayoutData(gd);
      
      return dialogArea;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      configuration.setTimerDelay(timerDelay.getSelection());
      configuration.setTimerKey(timerKey.getText().trim());
      super.okPressed();
   }
}

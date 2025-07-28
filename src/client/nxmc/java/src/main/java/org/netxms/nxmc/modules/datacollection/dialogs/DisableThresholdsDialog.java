/**
 * NetXMS - open source network management system
 * Copyright (C) 2025 Raden Solutions
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

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.base.NXCPCodes;
import org.netxms.client.datacollection.BulkDciUpdateElement;
import org.netxms.nxmc.base.widgets.DateTimeSelector;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.dialogs.helpers.BulkDciThresholdUpdateElement;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for disabling thresholds for selected DCIs
 */
public class DisableThresholdsDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(DisableThresholdsDialog.class);

   private Button disableProcessing;
   private Button disableUntil;
   private DateTimeSelector disableUntilTimeSelector;
   private long disableEndTime;

   /**
    * @param shell
    */
   public DisableThresholdsDialog(Shell shell)
   {
      super(shell);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Disable DCI Threshold processing");
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      dialogArea.setLayout(layout);
      
      disableProcessing = new Button(dialogArea, SWT.RADIO);
      disableProcessing.setText(i18n.tr("Disable threshold processing indefinitely"));
      disableProcessing.setSelection(true);
      disableProcessing.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            disableUntilTimeSelector.setEnabled(false);
         }
      });

      disableUntil = new Button(dialogArea, SWT.RADIO);
      disableUntil.setText(i18n.tr("Disable threshold processing until"));
      disableUntil.setEnabled(disableProcessing.getSelection());
      disableUntil.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            disableUntilTimeSelector.setEnabled(true);
         }
      });      

      disableUntilTimeSelector = new DateTimeSelector(dialogArea, SWT.NONE);
      disableUntilTimeSelector.setEnabled(false);
      
      return dialogArea;
   }
   
   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if (disableProcessing.getSelection())
      {
         disableEndTime = -1; // permanently disabled
      }
      else
      {
         disableEndTime = disableUntilTimeSelector.getValue().getTime() / 1000;
      }
      
      super.okPressed();
   }

   /**
    * Get bulk update elements
    * 
    * @return list of bulk update elements
    */
   public List<BulkDciUpdateElement> getBulkUpdateElements()
   {      
      return new ArrayList<BulkDciUpdateElement>(Arrays.asList(new BulkDciThresholdUpdateElement(NXCPCodes.VID_THRESHOLD_ENABLE_TIME, disableEndTime)));
   }
}

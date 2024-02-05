/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.users.dialogs;

import java.util.Date;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.users.AuthenticationToken;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.DateTimeSelector;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Issue authentication token
 */
public class IssueTokenDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(IssueTokenDialog.class);

   private int userId;
   private DateTimeSelector expirationTime;
   private LabeledText description;
   private LabeledText token;

   /**
    * Dialog for issuing authentication tokens.
    *
    * @param parentShell parent shell
    * @param userId user ID
    */
   public IssueTokenDialog(Shell parentShell, int userId)
   {
      super(parentShell);
      this.userId = userId;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Issue Authentication Token"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      final GridLayout layout = new GridLayout();

      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      expirationTime = new DateTimeSelector(dialogArea, SWT.NONE);
      expirationTime.setValue(new Date(System.currentTimeMillis() + 86400000L * 365L));
      expirationTime.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

      description = new LabeledText(dialogArea, SWT.NONE);
      description.setLabel(i18n.tr("Description"));
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.widthHint = 400;
      gd.horizontalSpan = 2;
      description.setLayoutData(gd);

      token = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.READ_ONLY);
      token.setLabel(i18n.tr("Issued token"));
      token.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Button copyButton = new Button(dialogArea, SWT.PUSH);
      copyButton.setBackground(dialogArea.getBackground());
      copyButton.setImage(SharedIcons.IMG_COPY_TO_CLIPBOARD);
      copyButton.setToolTipText(i18n.tr("Copy token to clipboard"));
      gd = new GridData();
      gd.verticalAlignment = SWT.BOTTOM;
      copyButton.setLayoutData(gd);
      copyButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            WidgetHelper.copyToClipboard(token.getText());
         }
      });

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
      createButton(parent, IDialogConstants.OK_ID, i18n.tr("Issue"), true);
      createButton(parent, IDialogConstants.CANCEL_ID, i18n.tr("Cancel"), false);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      final NXCSession session = Registry.getSession();
      final String d = description.getText().trim();
      final int v = (int)((expirationTime.getValue().getTime() - System.currentTimeMillis()) / 1000);
      new Job("Issuing authentication token", null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            try
            {
               final AuthenticationToken t = session.requestAuthenticationToken(true, v, d, userId);
               runInUIThread(() -> {
                  token.setText(t.getValue());
                  getButton(IDialogConstants.OK_ID).dispose();
                  getButton(IDialogConstants.CANCEL_ID).setText(i18n.tr("Close"));
                  Composite buttonBar = getButton(IDialogConstants.CANCEL_ID).getParent();
                  ((GridLayout)buttonBar.getLayout()).numColumns--;
                  buttonBar.getParent().layout(true, true);
               });
            }
            catch(Exception e)
            {
               runInUIThread(() -> MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Cannot issue authentication token ({0})", e.getLocalizedMessage())));
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return null;
         }
      }.start();
   }
}

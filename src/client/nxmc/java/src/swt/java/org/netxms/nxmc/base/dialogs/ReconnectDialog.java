/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.base.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.ProgressBar;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Client reconnect dialog
 */
public class ReconnectDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(ReconnectDialog.class);

   private Label statusMessage;

   /**
    * @param parentShell
    */
   public ReconnectDialog(Shell parentShell)
   {
      super(parentShell);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell shell)
   {
      super.configureShell(shell);
      shell.setText(i18n.tr("Reconnect"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
      createButton(parent, IDialogConstants.CANCEL_ID, i18n.tr("E&xit"), false);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      final Composite container = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING * 2;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      container.setLayout(layout);

      Label message = new Label(container, SWT.NONE);
      message.setFont(JFaceResources.getBannerFont());
      message.setText(i18n.tr("Connection with the server is lost. Trying to reconnect..."));
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      message.setLayoutData(gd);

      statusMessage = new Label(container, SWT.NONE);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      statusMessage.setLayoutData(gd);

      ProgressBar progress = new ProgressBar(container, SWT.INDETERMINATE);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 400;
      progress.setLayoutData(gd);

      return container;
   }

   /**
    * Update status message.
    *
    * @param text new message text
    */
   public void updateStatusMessage(String text)
   {
      statusMessage.setText(text);
   }
}

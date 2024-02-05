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
package org.netxms.nxmc.modules.objects.dialogs;

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
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog to show public access information for network map
 */
public class NetworkMapPublicAccessDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(NetworkMapPublicAccessDialog.class);

   private String token;
   private String url;

   /**
    * Create dialog.
    * 
    * @param parentShell parent shell
    * @param token access token
    * @param url direct access URL
    */
   public NetworkMapPublicAccessDialog(Shell parentShell, String token, String url)
   {
      super(parentShell);
      this.token = token;
      this.url = url;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Map Public Access Details"));
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
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      LabeledText tokenText = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.READ_ONLY);
      tokenText.setLabel("Access token");
      tokenText.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      tokenText.setText(token);

      Button copyButton = new Button(dialogArea, SWT.PUSH);
      copyButton.setBackground(dialogArea.getBackground());
      copyButton.setImage(SharedIcons.IMG_COPY_TO_CLIPBOARD);
      copyButton.setToolTipText(i18n.tr("Copy token to clipboard"));
      GridData gd = new GridData();
      gd.verticalAlignment = SWT.BOTTOM;
      copyButton.setLayoutData(gd);
      copyButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            WidgetHelper.copyToClipboard(token);
         }
      });

      LabeledText urlText = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.READ_ONLY);
      urlText.setLabel(i18n.tr("Direct access URL"));
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.widthHint = 600;
      urlText.setLayoutData(gd);
      urlText.setText(url);

      Button copyUrlButton = new Button(dialogArea, SWT.PUSH);
      copyUrlButton.setBackground(dialogArea.getBackground());
      copyUrlButton.setImage(SharedIcons.IMG_COPY_TO_CLIPBOARD);
      copyUrlButton.setToolTipText(i18n.tr("Copy URL to clipboard"));
      gd = new GridData();
      gd.verticalAlignment = SWT.BOTTOM;
      copyUrlButton.setLayoutData(gd);
      copyUrlButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            WidgetHelper.copyToClipboard(url);
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
      createButton(parent, IDialogConstants.CANCEL_ID, i18n.tr("Close"), false);
   }
}

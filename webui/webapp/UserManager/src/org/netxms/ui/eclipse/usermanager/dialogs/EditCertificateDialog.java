/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Raden Solutions
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
package org.netxms.ui.eclipse.usermanager.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.api.client.users.AuthCertificate;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.usermanager.Messages;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "Create new tool" dialog
 */
public class EditCertificateDialog extends Dialog
{
   protected AuthCertificate cert;

   private LabeledText textId;
   private LabeledText textSubject;
   private LabeledText textComments;
   private Combo comboType;

   /**
    * Constructor for edit form
    * 
    * @param parentShell
    * @param id Id of certificate, should be set to 0 if new certificate is created.
    * @param type Type of certificate mapping. Can be USER_MAP_CERT_BY_SUBJECT or USER_MAP_CERT_BY_PUBKEY.
    * @param certData Certificate in PEM format.
    * @param subjec Subject of the certificate
    * @param comments Comments for certificate
    */
   public EditCertificateDialog(Shell parentShell, AuthCertificate cert)
   {
      super(parentShell);
      this.cert = cert;
   }

   /**
    * 
    * 
    * @param parentShell
    */
   public EditCertificateDialog(Shell parentShell)
   {
      super(parentShell);
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);

      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      
      textId = new LabeledText(dialogArea, SWT.NONE);
      textId.setEditable(false);
      textId.setLabel(Messages.get().EditCertificateDialog_CertIDLabel);
      textId.setText(Long.toString(cert.getId()));
      textId.setLayoutData(gd);

      comboType = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, Messages.get().EditCertificateDialog_CertTypeLabel, WidgetHelper.DEFAULT_LAYOUT_DATA);
      String options[] = { Messages.get().EditCertificateDialog_CertType_AC, Messages.get().EditCertificateDialog_CertType_UC };
      comboType.add(options[cert.getType()]);
      comboType.select(0);

      textSubject = new LabeledText(dialogArea, SWT.NONE);
      textSubject.setEditable(false);
      textSubject.setLabel(Messages.get().EditCertificateDialog_CertSubjectLabel);
      textSubject.setText(cert.getSubject());
      textSubject.setLayoutData(gd);

      textComments = new LabeledText(dialogArea, SWT.NONE);
      textComments.setLabel(Messages.get().EditCertificateDialog_CertCommentLabel);
      textComments.setText(cert.getComments());
      textComments.setLayoutData(gd);

      return dialogArea;
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      cert.setComments(textComments.getText());
      super.okPressed();
   }

   /**
    * @return the changed comment
    */
   public String getComments()
   {
      return cert.getComments();
   }
}

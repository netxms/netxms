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

import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.rap.rwt.widgets.FileUpload;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.usermanager.Messages;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "Create new tool" dialog
 */
public class CreateNewCertificateDialog extends Dialog
{
   protected String comment;
   protected byte [] fileContent;
	private Text textFileName;
	private LabeledText textComments;
	private Button browseButton;
	
	/**
	 * Standard constructor
	 * 
    * @param parentShell
    */
	public CreateNewCertificateDialog(Shell parentShell)
   {
      super(parentShell);
   }
	
	@Override
   protected void configureShell(Shell newShell)
   {
      // TODO Auto-generated method stub
      super.configureShell(newShell);
      newShell.setText(Messages.get().CreateNewCertificateDialog_CreateNewCertificateDialogTitle);
   }

	/* (non-Javadoc)
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
		
		textFileName = WidgetHelper.createLabeledText(dialogArea, SWT.BORDER, 300, Messages.get().CreateNewCertificateDialog_FileNameLabel, null, WidgetHelper.DEFAULT_LAYOUT_DATA);
      
      browseButton = new Button(dialogArea, SWT.PUSH);
      browseButton.setText(Messages.get().CreateNewCertificateDialog_BrowseLabel);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      browseButton.setLayoutData(gd);
      browseButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
        	FileDialog fd = new FileDialog(getShell(), SWT.OPEN);
            fd.setText(Messages.get().CreateNewCertificateDialog_SelectFileHeader);
            String selected = fd.open();
            if (selected != null)
               textFileName.setText(selected);
         }
      });
      
      textComments = new LabeledText(dialogArea, SWT.MULTI);
      textComments.setLabel(Messages.get().CreateNewCertificateDialog_CertificateCommentLabel);
      GridData gdText = new GridData();
      gdText.horizontalAlignment = SWT.FILL;
      gdText.horizontalSpan = 2;
      gdText.grabExcessHorizontalSpace = true;
      textComments.setLayoutData(gdText);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
	   comment = textComments.getText();
	   String fName = textFileName.getText();
	   try
      {
         fileContent = readFile(fName);
      }
      catch(CertificateException e)
      {
         e.printStackTrace();
      }
      catch(FileNotFoundException e)
      {
         e.printStackTrace();
      }
		super.okPressed();
	}
	
	 public byte[] readFile(String filename) throws CertificateException, FileNotFoundException
    {
	    FileInputStream fis = new FileInputStream(filename);
	    BufferedInputStream bis = new BufferedInputStream(fis);

	    CertificateFactory cf = CertificateFactory.getInstance("X.509"); //$NON-NLS-1$

	    Certificate cert = (Certificate)cf.generateCertificate(bis);
       return cert.getEncoded();
    }
	
	/**
	 * @return the changed comment
	 */
	public String getComment()
   {
      return comment; 
   }
	
	/**
    * @return the changed comment
    */
   public byte[] getFileContent()
   {
      return fileContent; 
   }
}

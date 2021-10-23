/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Victor Kirhenshtein
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

import java.net.MalformedURLException;
import java.net.URL;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Object's associated URL edit dialog
 */
public class ObjectUrlEditDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(ObjectUrlEditDialog.class);
	private LabeledText textUrl;
	private LabeledText textDescription;
	private URL url;
	private String description;
	
	/**
	 * @param parentShell
	 */
	public ObjectUrlEditDialog(Shell parentShell, URL url, String description)
	{
		super(parentShell);
		this.url = url;
		this.description = description;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText((url == null) ? i18n.tr("Add URL") : i18n.tr("Edit URL"));
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
      dialogArea.setLayout(layout);
		
      textUrl = new LabeledText(dialogArea, SWT.NONE);
      textUrl.setLabel("URL");
      textUrl.getTextControl().setTextLimit(2000);
      if (url != null)
      	textUrl.setText(url.toExternalForm());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textUrl.setLayoutData(gd);
      
      textDescription = new LabeledText(dialogArea, SWT.NONE);
      textDescription.setLabel("Description");
      textDescription.getTextControl().setTextLimit(2000);
      if (description != null)
      	textDescription.setText(description);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 500;
      textDescription.setLayoutData(gd);
      
		return dialogArea;
	}

	/**
	 * Get URL
	 */
	public URL getUrl()
	{
		return url;
	}

	/**
	 * Get description
	 */
	public String getDescription()
	{
		return description;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		try
      {
         url = new URL(textUrl.getText().trim());
      }
      catch(MalformedURLException e)
      {
         MessageDialogHelper.openWarning(getShell(), "Warning", "Entered URL is invalid. Please enter valid URL.");
         return;
      }
		description = textDescription.getText();
		super.okPressed();
	}
}

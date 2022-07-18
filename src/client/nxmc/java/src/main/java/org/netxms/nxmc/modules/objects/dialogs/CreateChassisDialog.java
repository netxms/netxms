/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for creating new chassis
 */
public class CreateChassisDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(CreateChassisDialog.class);

	private LabeledText objectNameField;
   private LabeledText objectAliasField;
	private ObjectSelector controllerSelector;
	private Button checkCreateAnother;
	
	private String objectName;
   private String objectAlias;
	private long controllerId = 0;
	private boolean showAgain = false;
	
	/**
	 * @param parentShell
	 */
	public CreateChassisDialog(Shell parentShell, CreateChassisDialog prev)
	{
		super(parentShell);
		if (prev != null)
		{
		   controllerId = prev.controllerId;
		   showAgain = prev.showAgain;
		}
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(i18n.tr("Create Chassis"));
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
		objectNameField = new LabeledText(dialogArea, SWT.NONE);
      objectNameField.setLabel(i18n.tr("Name"));
		objectNameField.getTextControl().setTextLimit(255);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 300;
		gd.horizontalSpan = 2;
		objectNameField.setLayoutData(gd);
		
      objectAliasField = new LabeledText(dialogArea, SWT.NONE);
      objectAliasField.setLabel(i18n.tr("Alias"));
      objectAliasField.getTextControl().setTextLimit(255);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      objectAliasField.setLayoutData(gd);

		final Composite ipAddrGroup = new Composite(dialogArea, SWT.NONE);
		layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.numColumns = 2;
		ipAddrGroup.setLayout(layout);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		ipAddrGroup.setLayoutData(gd);
		
		controllerSelector = new ObjectSelector(dialogArea, SWT.NONE, true);
      controllerSelector.setLabel(i18n.tr("Controller"));
		controllerSelector.setObjectClass(Node.class);
		controllerSelector.setObjectId(controllerId);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		controllerSelector.setLayoutData(gd);
		
		checkCreateAnother = new Button(dialogArea, SWT.CHECK);
      checkCreateAnother.setText(i18n.tr("Show this dialog again to &create another chassis"));
		checkCreateAnother.setSelection(showAgain);
		
		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		objectName = objectNameField.getText().trim();
      if (objectName.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Object name cannot be empty"));
         return;
      }
      
		controllerId = controllerSelector.getObjectId();
		showAgain = checkCreateAnother.getSelection();
		super.okPressed();
	}

	/**
	 * @return the name
	 */
	public String getObjectName()
	{
		return objectName;
	}

   /**
    * @return the alias
    */
   public String getObjectAlias()
   {
      return objectAlias;
   }

   /**
    * @return the controllerId
    */
   public long getControllerId()
   {
      return controllerId;
   }

   /**
    * @return the showAgain
    */
   public boolean isShowAgain()
   {
      return showAgain;
   }
}

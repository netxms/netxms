/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
import org.netxms.client.NXCSession;
import org.netxms.client.objects.configs.CustomAttribute;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Object's custom attribute edit dialog
 */
public class AttributeEditDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(AttributeEditDialog.class);
	private LabeledText textName;
	private LabeledText textValue;
	private Button checkInherite;
	private String name;
	private String value;
	private long flags;
   private boolean inherited;
   private long source;
   
   /**
    * @param parentShell
    * @param source
    */
   public AttributeEditDialog(Shell parentShell, String name, String value, long flags, long source)
   {
      super(parentShell);
      this.name = name;
      this.value = value;
      this.flags = flags;
      this.inherited = (source != 0);
      this.source = source;
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
		
      textName = new LabeledText(dialogArea, SWT.NONE);
      textName.setLabel(i18n.tr("Name"));
      textName.getTextControl().setTextLimit(127);
      if (name != null)
      {
      	textName.setText(name);
      	textName.setEditable(false);
      }
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textName.setLayoutData(gd);
      
      textValue = new LabeledText(dialogArea, SWT.NONE);
      textValue.setLabel(i18n.tr("Value"));
      if (value != null)
      	textValue.setText(value);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      textValue.setLayoutData(gd);
      
      if (name != null)
      	textValue.setFocus();
      
      checkInherite = new Button(dialogArea, SWT.CHECK);
      if (inherited)
      {
         NXCSession session = Registry.getSession();
         checkInherite.setText(String.format(i18n.tr("Inheritable (enforced by %s [%d])"), 
               session.getObjectName(source), source));  
      }
      else
      {
         checkInherite.setText(i18n.tr("Inheritable"));
      }
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      checkInherite.setLayoutData(gd);
      
      checkInherite.setSelection(inherited || (flags & CustomAttribute.INHERITABLE) > 0);
      checkInherite.setEnabled(!inherited);
      
		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText((name == null) ? i18n.tr("Add Attribute") : i18n.tr("Modify Attribute"));
	}
	
	/**
	 * Get variable name
	 */
	public String getName()
	{
		return name;
	}
	
	/**
	 * Get variable value
	 */
	public String getValue()
	{
		return value;
	}
	
	/**
	 * Get variable flags
	 */
	public long getFlags()
	{
	   return flags;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      name = textName.getText().trim();
      value = textValue.getText();
      flags = 0;
      if (checkInherite.getSelection())
         flags |= CustomAttribute.INHERITABLE;
      if (inherited)
         flags |= CustomAttribute.REDEFINED | CustomAttribute.INHERITABLE;
      super.okPressed();
   }
}

/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
package org.netxms.ui.eclipse.objectmanager.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.constants.RackElementType;
import org.netxms.client.constants.RackOrientation;
import org.netxms.client.objects.configs.PassiveRackElement;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Rack attribute edit dialog
 */
public class RackPassiveElementEditDialog extends Dialog
{
   private final static String[] ORIENTATION = { "Fill", "Front", "Rear" };
   private final static String[] TYPE = { "Patch panel", "Filler panel", "Organiser" };
   
   private PassiveRackElement element;
   private LabeledText name;
   private Combo type;
   private Combo orientation;
   private LabeledSpinner position;
   private String title;
   
   /**
    * @param parentShell
    */
   public RackPassiveElementEditDialog(Shell parentShell, PassiveRackElement element)
   {
      super(parentShell);
      this.element = (element != null) ? element : new PassiveRackElement();
      title = (element == null) ? "Add Element" : "Edit Element";
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(title);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);
      
      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.numColumns = 3;
      dialogArea.setLayout(layout);
      
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 3;
      gd.widthHint = 400;
      name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel("Name");
      name.setText(element.getName());
      name.setLayoutData(gd);
      
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      type = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Type", gd);
      type.setItems(TYPE);
      type.setText(TYPE[element.getType().getValue()]);
      
      position = new LabeledSpinner(dialogArea, SWT.NONE);
      position.setLabel(Messages.get().RackPlacement_Position);
      position.setRange(1, 50);
      position.setSelection(element.getPosition());
      
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      position.setLayoutData(gd);
      
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      orientation = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Orientation", gd);
      orientation.setItems(ORIENTATION);
      orientation.setText(ORIENTATION[element.getOrientation().getValue()]);
      
      return dialogArea;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {      
      element.setPosition(position.getSelection());
      element.setOrientation(RackOrientation.getByValue(orientation.getSelectionIndex()));
      element.setName(name.getText());
      element.setType(RackElementType.getByValue(type.getSelectionIndex()));
      super.okPressed();
   }
   
   /**
    * Get attribute config entry
    * 
    * @return attribute
    */
   public PassiveRackElement getElement()
   {
      return element;
   }
}

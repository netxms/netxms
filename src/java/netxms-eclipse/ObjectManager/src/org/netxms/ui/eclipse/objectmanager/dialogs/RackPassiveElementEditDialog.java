/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.constants.PassiveRackElementType;
import org.netxms.client.constants.RackOrientation;
import org.netxms.client.objects.configs.PassiveRackElement;
import org.netxms.ui.eclipse.imagelibrary.widgets.ImageSelector;
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
   private final static String[] TYPE = { "Patch panel", "Filler panel", "Organiser", "PDU" };

   private PassiveRackElement element;
   private LabeledText name;
   private Combo type;
   private Combo orientation;
   private LabeledSpinner position;
   private LabeledSpinner height;
   private String title;
   private ImageSelector frontImage;
   private ImageSelector rearImage;

   /**
    * @param parentShell
    */
   public RackPassiveElementEditDialog(Shell parentShell, PassiveRackElement element)
   {
      super(parentShell);
      this.element = (element != null) ? element : new PassiveRackElement();
      title = (element == null) ? "Add Element" : "Edit Element";
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(title);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.numColumns = 4;
      dialogArea.setLayout(layout);

      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = layout.numColumns;
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

      height = new LabeledSpinner(dialogArea, SWT.NONE);
      height.setLabel("Height");
      height.setRange(1, 50);
      height.setSelection(element.getHeight());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      height.setLayoutData(gd);

      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      orientation = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Orientation", gd);
      orientation.setItems(ORIENTATION);
      orientation.setText(ORIENTATION[element.getOrientation().getValue()]);
      orientation.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            RackOrientation o = RackOrientation.getByValue(orientation.getSelectionIndex());
            frontImage.setEnabled(o == RackOrientation.FRONT || o == RackOrientation.FILL);
            rearImage.setEnabled(o == RackOrientation.REAR || o == RackOrientation.FILL);
         }
      });

      frontImage = new ImageSelector(dialogArea, SWT.NONE);
      frontImage.setLabel("Front image");
      frontImage.setImageGuid(element.getFrontImage(), true);
      frontImage.setEnabled(element.getOrientation() == RackOrientation.FRONT || element.getOrientation() == RackOrientation.FILL);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = layout.numColumns;
      frontImage.setLayoutData(gd);

      rearImage = new ImageSelector(dialogArea, SWT.NONE);
      rearImage.setLabel("Rear image");
      rearImage.setImageGuid(element.getRearImage(), true);
      rearImage.setEnabled(element.getOrientation() == RackOrientation.REAR || element.getOrientation() == RackOrientation.FILL);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = layout.numColumns;
      rearImage.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {      
      element.setPosition(position.getSelection());
      element.setHeight(height.getSelection());
      element.setOrientation(RackOrientation.getByValue(orientation.getSelectionIndex()));
      element.setName(name.getText());
      element.setType(PassiveRackElementType.getByValue(type.getSelectionIndex()));
      element.setFrontImage(frontImage.getImageGuid());
      element.setRearImage(rearImage.getImageGuid());
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

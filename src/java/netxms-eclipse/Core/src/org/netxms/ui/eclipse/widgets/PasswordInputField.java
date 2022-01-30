/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Password input field - combines text input with "reveal" button.
 */
public class PasswordInputField extends LabeledControl
{
   private Composite composite;
   private Text text;
   private Button buttonShow;

   /**
    * Create new password input field.
    *
    * @param parent
    * @param style
    */
   public PasswordInputField(Composite parent, int style)
   {
      super(parent, style);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.LabeledControl#createControl(int)
    */
   @Override
   protected Control createControl(int controlStyle)
   {
      composite = new Composite(this, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
      composite.setLayout(layout);

      text = new Text(composite, controlStyle);
      text.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      buttonShow = new Button(composite, SWT.PUSH);
      buttonShow.setImage(SharedIcons.IMG_SHOW);
      buttonShow.setToolTipText("Show");
      GridData gd = new GridData(SWT.CENTER, SWT.CENTER, false, false);
      gd.heightHint = text.computeSize(SWT.DEFAULT, SWT.DEFAULT).y;
      buttonShow.setLayoutData(gd);
      buttonShow.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            togglePasswordVisibility();
         }
      });

      return composite;
   }

   /**
    * @see org.netxms.nxmc.base.widgets.LabeledControl#setText(java.lang.String)
    */
   @Override
   public void setText(String newText)
   {
      text.setText(newText);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.LabeledControl#getText()
    */
   @Override
   public String getText()
   {
      return text.getText();
   }

   /**
    * @see org.netxms.ui.eclipse.widgets.LabeledControl#getDefaultControlStyle()
    */
   @Override
   protected int getDefaultControlStyle()
   {
      return SWT.BORDER | SWT.SINGLE | SWT.PASSWORD;
   }

   /**
    * Toggle password visibility
    */
   private void togglePasswordVisibility()
   {
      int style = text.getStyle();
      String content = text.getText();
      text.dispose();
      if ((style & SWT.PASSWORD) != 0)
      {
         text = new Text(composite, style & ~SWT.PASSWORD);
         buttonShow.setImage(SharedIcons.IMG_HIDE);
         buttonShow.setToolTipText("Hide");
      }
      else
      {
         text = new Text(composite, style | SWT.PASSWORD);
         buttonShow.setImage(SharedIcons.IMG_SHOW);
         buttonShow.setToolTipText("Show");
      }
      text.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      text.moveAbove(null);
      text.setText(content);
      composite.layout(true, true);
   }

   /**
    * Sets the editable state.
    * 
    * @param editable the new editable state
    */
   public void setEditable(boolean editable)
   {
      text.setEditable(editable);
   }

   /**
    * Returns the editable state.
    * 
    * @return whether or not the receiver is editable
    */
   public boolean isEditable()
   {
      return text.getEditable();
   }
}

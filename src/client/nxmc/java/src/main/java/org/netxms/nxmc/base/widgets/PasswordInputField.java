/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.base.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Password input field - combines text input with "reveal" button.
 */
public class PasswordInputField extends LabeledControl
{
   private I18n i18n = LocalizationHelper.getI18n(PasswordInputField.class);
   private Composite composite;
   private Text text;
   private Button buttonShow;
   private Button buttonCopyToClipboard;
   private boolean enabled = true;

   /**
    * Create new password input field.
    *
    * @param parent parent composite
    * @param style control style
    */
   public PasswordInputField(Composite parent, int style, boolean showCopyOption)
   {
      super(parent, style, SWT.BORDER | SWT.SINGLE | SWT.PASSWORD, SWT.DEFAULT, Boolean.valueOf(showCopyOption));
      buttonShow.setToolTipText(i18n.tr("Show")); // Should be done here because createControl() called by superclass constructor
      if (buttonCopyToClipboard != null)
         buttonCopyToClipboard.setToolTipText(i18n.tr("Copy to clipboard"));
   }

   /**
    * Create new password input field.
    *
    * @param parent parent composite
    * @param style control style
    */
   public PasswordInputField(Composite parent, int style)
   {
      this(parent, style, false);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.LabeledControl#createControl(int, java.lang.Object)
    */
   @Override
   protected Control createControl(int controlStyle, Object parameters)
   {
      boolean showCopyOption = (Boolean)parameters;

      composite = new Composite(this, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = showCopyOption ? 3 : 2;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
      composite.setLayout(layout);

      text = new Text(composite, controlStyle);
      text.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      buttonShow = new Button(composite, SWT.PUSH);
      buttonShow.setImage(SharedIcons.IMG_SHOW);
      GridData gd = new GridData(SWT.CENTER, SWT.CENTER, false, false);
      int heightHint = text.computeSize(SWT.DEFAULT, SWT.DEFAULT).y;
      gd.heightHint = heightHint;
      buttonShow.setLayoutData(gd);
      buttonShow.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            togglePasswordVisibility();
         }
      });

      if (showCopyOption)
      {
         buttonCopyToClipboard = new Button(composite, SWT.PUSH);
         buttonCopyToClipboard.setImage(SharedIcons.IMG_COPY_TO_CLIPBOARD);
         gd = new GridData(SWT.CENTER, SWT.CENTER, false, false);
         gd.heightHint = heightHint;
         buttonCopyToClipboard.setLayoutData(gd);
         buttonCopyToClipboard.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               WidgetHelper.copyToClipboard(text.getText());
            }
         });
      }

      return composite;
   }

   /**
    * @see org.netxms.nxmc.base.widgets.LabeledControl#setText(java.lang.String)
    */
   @Override
   public void setText(String newText)
   {
      text.setText((newText != null) ? newText : "");
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
         buttonShow.setToolTipText(i18n.tr("Hide"));
      }
      else
      {
         text = new Text(composite, style | SWT.PASSWORD);
         buttonShow.setImage(SharedIcons.IMG_SHOW);
         buttonShow.setToolTipText(i18n.tr("Show"));
      }
      text.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      text.moveAbove(null);
      text.setText(content);
      text.setEnabled(enabled);
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

   /**
    * Enable or disable input controls.
    *
    * @param enabled true to enable input controls
    */
   public void setInputControlsEnabled(boolean enabled)
   {
      this.enabled = enabled;
      text.setEnabled(enabled);
      buttonShow.setEnabled(enabled);
   }
}

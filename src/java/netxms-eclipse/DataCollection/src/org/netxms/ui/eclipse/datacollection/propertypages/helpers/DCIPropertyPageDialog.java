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
package org.netxms.ui.eclipse.datacollection.propertypages.helpers;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.api.DataCollectionObjectEditor;

/**
 * Property page for DCI`s
 */
public class DCIPropertyPageDialog extends PropertyPage
{
   protected DataCollectionObjectEditor editor;

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      editor = (DataCollectionObjectEditor)getElement().getAdapter(DataCollectionObjectEditor.class);
      String message = DataCollectionObjectEditor.createModificationWarningMessage(editor.getObject());
      if (message != null)
      {
         Composite messageArea = new Composite(parent, SWT.BORDER);
         messageArea.setBackground(SharedColors.getColor(SharedColors.MESSAGE_BAR_BACKGROUND, messageArea.getDisplay()));
         GridLayout layout = new GridLayout(2, false);
         messageArea.setLayout(layout);
         GridData gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.verticalAlignment = SWT.TOP;
         messageArea.setLayoutData(gd);
         
         Label imageLabel = new Label(messageArea, SWT.NONE);
         imageLabel.setBackground(messageArea.getBackground());
         imageLabel.setImage(Activator.getImageDescriptor("icons/warning.png").createImage());
         gd = new GridData();
         gd.horizontalAlignment = SWT.LEFT;
         gd.verticalAlignment = SWT.FILL;
         imageLabel.setLayoutData(gd);
         
         Label messageLabel = new Label(messageArea, SWT.WRAP);
         messageLabel.setBackground(messageArea.getBackground());
         messageLabel.setForeground(SharedColors.getColor(SharedColors.MESSAGE_BAR_TEXT, messageArea.getDisplay()));
         messageLabel.setText(message);
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.verticalAlignment = SWT.CENTER;
         messageLabel.setLayoutData(gd);
      }
      
      Composite content = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      content.setLayout(layout);
      return content;
   }
}

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
package org.netxms.nxmc.modules.datacollection.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.modules.datacollection.DataCollectionObjectEditor;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.ThemeEngine;

/**
 * Property page for DCI`s
 */
public abstract class AbstractDCIPropertyPage extends PropertyPage
{
   protected DataCollectionObjectEditor editor;

   /**
    * Create DCI property page
    *
    * @param title page title
    * @param editor data collection editor
    */
   public AbstractDCIPropertyPage(String title, DataCollectionObjectEditor editor)
   {
      super(title);
      this.editor = editor;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      String message = DataCollectionObjectEditor.createModificationWarningMessage(editor.getObject());
      if (message != null)
      {
         Composite messageArea = new Composite(parent, SWT.BORDER);
         messageArea.setBackground(ThemeEngine.getBackgroundColor("MessageBar"));
         GridLayout layout = new GridLayout(2, false);
         messageArea.setLayout(layout);
         GridData gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.verticalAlignment = SWT.TOP;
         messageArea.setLayoutData(gd);

         Label imageLabel = new Label(messageArea, SWT.NONE);
         imageLabel.setBackground(messageArea.getBackground());
         imageLabel.setImage(ResourceManager.getImage("icons/warning.png"));
         gd = new GridData();
         gd.horizontalAlignment = SWT.LEFT;
         gd.verticalAlignment = SWT.FILL;
         imageLabel.setLayoutData(gd);
         imageLabel.addDisposeListener(new DisposeListener() {
            @Override
            public void widgetDisposed(DisposeEvent e)
            {
               imageLabel.getImage().dispose();
            }
         });

         Label messageLabel = new Label(messageArea, SWT.WRAP);
         messageLabel.setBackground(messageArea.getBackground());
         messageLabel.setForeground(ThemeEngine.getForegroundColor("MessageBar"));
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

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
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.api.DataCollectionObjectEditor;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

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
      if (editor.getObject().getTemplateId() != 0)
      {
         String message;
         if (editor.getObject().getTemplateId() == editor.getObject().getNodeId())
         {
            message = "This DCI was added by Instance Discovery, all local changes will be overwritten by the parent DCI";
         }
         else
         {
            AbstractObject object = ((NXCSession)ConsoleSharedData.getSession()).findObjectById(editor.getObject().getTemplateId());
            if (object != null)
            {
               message = String.format("This DCI was added by %s %s, all local changes will be overwritten once it has been modified!",
                                          object.getObjectName(), (object.getObjectClass() == AbstractObject.OBJECT_CLUSTER) ? " cluster" : " template");
            }
            else
            {
               message = String.format("This DCI was added by object with ID: %d", editor.getObject().getTemplateId());
            }
         }
         
         Composite messageArea = new Composite(parent, SWT.NONE);
         GridLayout layout = new GridLayout(2, false);
         layout.marginWidth = 0;
         layout.marginHeight = 0;
         messageArea.setLayout(layout);
         
         GridData gd = new GridData();
         gd.horizontalAlignment = SWT.LEFT;
         gd.verticalAlignment = SWT.FILL;
         Label imageLabel = new Label(messageArea, SWT.NONE);
         imageLabel.setImage(Activator.getImageDescriptor("icons/warning_obj.gif").createImage());
         imageLabel.setLayoutData(gd);
         
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.verticalAlignment = SWT.FILL;
         Label messageLabel = new Label(messageArea, SWT.WRAP);
         messageLabel.setText(message);
      }
      return new Composite(parent, SWT.NONE);
   }

}

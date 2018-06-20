/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ObjectDetailsConfig;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;

/**
 * Properties for "Object details" dashboard element ("Query" page)
 */
public class ObjectDetailsQuery extends PropertyPage
{
   private ObjectDetailsConfig config;
   private ScriptEditor query;
   private LabeledSpinner refreshInterval;
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      config = (ObjectDetailsConfig)getElement().getAdapter(ObjectDetailsConfig.class);
      
      Composite dialogArea = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      dialogArea.setLayout(layout);

      Label label = new Label(dialogArea, SWT.NONE);
      label.setText("Query");

      query = new ScriptEditor(dialogArea, SWT.BORDER, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL, true);
      query.setText(config.getQuery());
      
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.widthHint = 300;
      gd.heightHint = 300;
      query.setLayoutData(gd);

      refreshInterval = new LabeledSpinner(dialogArea, SWT.NONE);
      refreshInterval.setLabel("Refresh interval (seconds)");
      refreshInterval.setRange(1, 100000);
      refreshInterval.setSelection(config.getRefreshRate());
      
      return dialogArea;
   }


   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      config.setQuery(query.getText());
      config.setRefreshRate(refreshInterval.getSelection());
      return true;
   }
}

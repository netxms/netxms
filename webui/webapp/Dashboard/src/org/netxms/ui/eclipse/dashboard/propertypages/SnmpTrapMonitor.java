/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2022 RadenSolutions
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
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.dashboard.widgets.TitleConfigurator;
import org.netxms.ui.eclipse.dashboard.widgets.internal.SnmpTrapMonitorConfig;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;

public class SnmpTrapMonitor extends PropertyPage
{
   private SnmpTrapMonitorConfig config;
   private ObjectSelector objectSelector;
   private TitleConfigurator title;

   @Override
   protected Control createContents(Composite parent)
   {
      config = (SnmpTrapMonitorConfig)getElement().getAdapter(SnmpTrapMonitorConfig.class);

      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      dialogArea.setLayout(layout);

      title = new TitleConfigurator(dialogArea, config);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      title.setLayoutData(gd);

      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, true, true);
      objectSelector.setLabel("Root object");
      objectSelector.setObjectClass(AbstractObject.class);
      objectSelector.setObjectId(config.getObjectId());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      objectSelector.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      title.updateConfiguration(config);
      config.setObjectId(objectSelector.getObjectId());
      return true;
   }
}

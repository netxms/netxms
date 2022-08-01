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
package org.netxms.nxmc.modules.users.dialogs.helpers;

import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Custom 2FA method binding configurator
 */
public class CustomMethodBindingConfigurator extends AbstractMethodBindingConfigurator
{
   private I18n i18n = LocalizationHelper.getI18n(CustomMethodBindingConfigurator.class);
   private LabeledText configuration;

   /**
    * @param parent
    */
   public CustomMethodBindingConfigurator(Composite parent)
   {
      super(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      setLayout(layout);

      configuration = new LabeledText(this, SWT.NONE, SWT.BORDER | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
      configuration.setLabel(i18n.tr("Configuration"));
      GridData gd = new GridData();
      gd.heightHint = 400;
      gd.widthHint = 600;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      configuration.setLayoutData(gd);
   }

   /**
    * @see org.netxms.ui.eclipse.usermanager.dialogs.helpers.AbstractMethodBindingConfigurator#setConfiguration(java.util.Map)
    */
   @Override
   public void setConfiguration(Map<String, String> configuration)
   {
      StringBuilder sb = new StringBuilder();
      for(Entry<String, String> e : configuration.entrySet())
      {
         sb.append(e.getKey());
         sb.append(" = ");
         sb.append(e.getValue());
         sb.append('\n');
      }
      this.configuration.setText(sb.toString());
   }

   /**
    * @see org.netxms.ui.eclipse.usermanager.dialogs.helpers.AbstractMethodBindingConfigurator#getConfiguration()
    */
   @Override
   public Map<String, String> getConfiguration()
   {
      Map<String, String> config = new HashMap<String, String>();
      String[] lines = configuration.getText().split("\n");
      for(String line : lines)
      {
         String[] parts = line.split("=", 2);
         if (parts.length == 2)
         {
            config.put(parts[0].trim(), parts[1].trim());
         }
      }
      return config;
   }
}

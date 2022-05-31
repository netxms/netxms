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
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * 2FA method binding configurator for "TOTP" driver
 */
public class TOTPMethodBindingConfigurator extends AbstractMethodBindingConfigurator
{
   private I18n i18n = LocalizationHelper.getI18n(TOTPMethodBindingConfigurator.class);
   private boolean initialized = false;
   private LabeledText status;
   private Button resetButton;

   /**
    * Create configurator.
    *
    * @param parent parent composite
    */
   public TOTPMethodBindingConfigurator(Composite parent)
   {
      super(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 2;
      setLayout(layout);

      status = new LabeledText(this, SWT.NONE, SWT.BORDER | SWT.READ_ONLY);
      status.setLabel(i18n.tr("Status"));
      status.setText(i18n.tr("Not initialized"));
      status.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));

      resetButton = new Button(this, SWT.PUSH);
      resetButton.setText(i18n.tr("&Reset"));
      resetButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            initialized = false;
            status.setText(i18n.tr("Not initialized"));
         }
      });
      GridData gd = new GridData();
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      gd.verticalAlignment = SWT.BOTTOM;
      resetButton.setLayoutData(gd);
   }

   /**
    * @see org.netxms.ui.eclipse.usermanager.dialogs.helpers.AbstractMethodBindingConfigurator#setConfiguration(java.util.Map)
    */
   @Override
   public void setConfiguration(Map<String, String> configuration)
   {
      initialized = Boolean.parseBoolean(configuration.get("Initialized"));
      status.setText(initialized ? i18n.tr("Initialized") : i18n.tr("Not initialized"));
   }

   /**
    * @see org.netxms.ui.eclipse.usermanager.dialogs.helpers.AbstractMethodBindingConfigurator#getConfiguration()
    */
   @Override
   public Map<String, String> getConfiguration()
   {
      Map<String, String> config = new HashMap<String, String>();
      config.put("Initialized", Boolean.toString(initialized));
      return config;
   }
}

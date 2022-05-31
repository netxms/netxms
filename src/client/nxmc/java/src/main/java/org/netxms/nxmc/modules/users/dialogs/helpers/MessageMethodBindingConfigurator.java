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
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * 2FA method binding configurator for "Message" driver
 */
public class MessageMethodBindingConfigurator extends AbstractMethodBindingConfigurator
{
   private I18n i18n = LocalizationHelper.getI18n(MessageMethodBindingConfigurator.class);
   private LabeledText subject;
   private LabeledText recipient;

   /**
    * Create configurator.
    *
    * @param parent parent composite
    */
   public MessageMethodBindingConfigurator(Composite parent)
   {
      super(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      setLayout(layout);

      recipient = new LabeledText(this, SWT.NONE);
      recipient.setLabel(i18n.tr("Recipient"));
      recipient.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));

      subject = new LabeledText(this, SWT.NONE);
      subject.setLabel(i18n.tr("Subject"));
      subject.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));
   }

   /**
    * @see org.netxms.ui.eclipse.usermanager.dialogs.helpers.AbstractMethodBindingConfigurator#setConfiguration(java.util.Map)
    */
   @Override
   public void setConfiguration(Map<String, String> configuration)
   {
      String value = configuration.get("Subject");
      if (value != null)
         subject.setText(value);
      value = configuration.get("Recipient");
      if (value != null)
         recipient.setText(value);
   }

   /**
    * @see org.netxms.ui.eclipse.usermanager.dialogs.helpers.AbstractMethodBindingConfigurator#getConfiguration()
    */
   @Override
   public Map<String, String> getConfiguration()
   {
      Map<String, String> config = new HashMap<String, String>();
      config.put("Subject", subject.getText().trim());
      config.put("Recipient", recipient.getText().trim());
      return config;
   }
}

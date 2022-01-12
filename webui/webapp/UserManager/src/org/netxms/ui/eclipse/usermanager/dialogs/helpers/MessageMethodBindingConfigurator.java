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
package org.netxms.ui.eclipse.usermanager.dialogs.helpers;

import java.io.StringWriter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.usermanager.Activator;
import org.netxms.ui.eclipse.widgets.LabeledText;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Path;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * 2FA method binding configurator for "Message" driver
 */
public class MessageMethodBindingConfigurator extends AbstractMethodBindingConfigurator
{
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
      recipient.setLabel("Recipient");
      recipient.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));

      subject = new LabeledText(this, SWT.NONE);
      subject.setLabel("Subject");
      subject.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));
   }

   /**
    * @see org.netxms.ui.eclipse.usermanager.dialogs.helpers.AbstractMethodBindingConfigurator#setConfiguration(java.lang.String)
    */
   @Override
   public void setConfiguration(String configuration)
   {
      Serializer serializer = new Persister();
      try
      {
         Configuration config = serializer.read(Configuration.class, configuration);
         subject.setText(config.subject);
         recipient.setText(config.recipient);
      }
      catch(Exception e)
      {
         Activator.logError("Cannot parse 2FA method binding configuration", e);
      }
   }

   /**
    * @see org.netxms.ui.eclipse.usermanager.dialogs.helpers.AbstractMethodBindingConfigurator#getConfiguration()
    */
   @Override
   public String getConfiguration()
   {
      Configuration config = new Configuration();
      config.subject = subject.getText().trim();
      config.recipient = recipient.getText().trim();
      Serializer serializer = new Persister();
      StringWriter writer = new StringWriter();
      try
      {
         serializer.write(config, writer);
         return writer.toString();
      }
      catch(Exception e)
      {
         Activator.logError("Cannot serialize 2FA method binding configuration", e);
         return "";
      }
   }

   /**
    * Configuration for method binding
    */
   @Root(name = "config", strict = false)
   private static class Configuration
   {
      @Element(name = "Recipient", required = false)
      @Path("MethodBinding")
      String recipient;

      @Element(name = "Subject", required = false)
      @Path("MethodBinding")
      String subject;
   }
}

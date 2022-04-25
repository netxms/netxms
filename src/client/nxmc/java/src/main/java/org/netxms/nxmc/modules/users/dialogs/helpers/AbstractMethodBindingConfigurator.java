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

import java.util.Map;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;

/**
 * Abstract 2FA method bindig configurator
 */
public abstract class AbstractMethodBindingConfigurator extends Composite
{
   /**
    * Create configurator.
    *
    * @param parent parent composite
    */
   public AbstractMethodBindingConfigurator(Composite parent)
   {
      super(parent, SWT.NONE);
   }

   /**
    * Set configuration to widgets.
    *
    * @param configuration method binding configuration
    */
   public abstract void setConfiguration(Map<String, String> configuration);

   /**
    * Get configuration from widgets.
    *
    * @return configuration from widgets
    */
   public abstract Map<String, String> getConfiguration();
}

/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
package org.netxms.nxmc.base.views;

import org.eclipse.swt.SWT;
import org.netxms.nxmc.keyboard.KeyStroke;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * Pinboard perspective
 */
public class PinboardPerspective extends Perspective
{
   /**
    * @param name
    * @param image
    */
   public PinboardPerspective()
   {
      super("Pinboard", LocalizationHelper.getI18n(PinboardPerspective.class).tr("Pinboard"), ResourceManager.getImage("icons/perspective-pinboard.png"));
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configurePerspective(org.netxms.nxmc.base.views.PerspectiveConfiguration)
    */
   @Override
   protected void configurePerspective(PerspectiveConfiguration configuration)
   {
      super.configurePerspective(configuration);
      configuration.hasNavigationArea = false;
      configuration.hasSupplementalArea = false;
      configuration.multiViewMainArea = true;
      configuration.enableViewPinning = false;
      configuration.allViewsAreCloseable = true;
      configuration.useGlobalViewId = true;
      configuration.ignoreViewContext = true;
      configuration.enableViewHide = false;
      configuration.keyboardShortcut = new KeyStroke(SWT.F12);
   }
}

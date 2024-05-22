/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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

import org.netxms.nxmc.keyboard.KeyStroke;

/**
 * Configuration for perspective
 */
public class PerspectiveConfiguration
{
   public boolean hasNavigationArea = true;
   public boolean multiViewNavigationArea = true;
   public boolean enableNavigationHistory = false;
   public boolean multiViewMainArea = true;
   public boolean hasHeaderArea = false;
   public boolean hasSupplementalArea = false;
   public boolean multiViewSupplementalArea = false;
   public boolean enableViewPinning = true;
   public boolean enableViewExtraction = true;
   public boolean allViewsAreCloseable = false;
   public boolean useGlobalViewId = false;
   public boolean ignoreViewContext = false;
   public boolean enableViewHide = true;
   public int priority = 255;
   public KeyStroke keyboardShortcut = null;
   public String requiredComponentId = null;

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "PerspectiveConfiguration [hasNavigationArea=" + hasNavigationArea + ", multiViewNavigationArea=" + multiViewNavigationArea + ", enableNavigationHistory=" + enableNavigationHistory +
            ", multiViewMainArea=" + multiViewMainArea + ", hasHeaderArea=" + hasHeaderArea + ", hasSupplementalArea=" + hasSupplementalArea + ", multiViewSupplementalArea=" +
            multiViewSupplementalArea + ", enableViewPinning=" + enableViewPinning + ", enableViewExtraction=" + enableViewExtraction + ", allViewsAreCloseable=" + allViewsAreCloseable +
            ", useGlobalViewId=" + useGlobalViewId + ", ignoreViewContext=" + ignoreViewContext + ", enableViewHide=" + enableViewHide + ", priority=" + priority + ", keyboardShortcut=" +
            keyboardShortcut + ", requiredComponentId=" + requiredComponentId + "]";
   }
}

/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Raden Solutions
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
package org.netxms.ui.eclipse.console.themes.classic;

import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.internal.presentations.defaultpresentation.DefaultTabFolder;
import org.eclipse.ui.internal.presentations.util.TabbedStackPresentation;
import org.eclipse.ui.presentations.IStackPresentationSite;
import org.eclipse.ui.presentations.StackPresentation;
import org.eclipse.ui.presentations.WorkbenchPresentationFactory;

/**
 * Presentation factory for "Classic" theme
 */
@SuppressWarnings("restriction")
public class ClassicPresentationFactory extends WorkbenchPresentationFactory
{
   /* (non-Javadoc)
    * @see org.eclipse.ui.presentations.WorkbenchPresentationFactory#createViewPresentation(org.eclipse.swt.widgets.Composite, org.eclipse.ui.presentations.IStackPresentationSite)
    */
   @Override
   public StackPresentation createViewPresentation(Composite parent, IStackPresentationSite site)
   {
      TabbedStackPresentation p = (TabbedStackPresentation)super.createViewPresentation(parent, site);
      ((DefaultTabFolder)p.getTabFolder()).setUnselectedCloseVisible(true);
      return p;
   }
}

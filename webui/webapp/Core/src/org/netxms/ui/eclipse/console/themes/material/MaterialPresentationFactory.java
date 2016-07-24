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
package org.netxms.ui.eclipse.console.themes.material;

import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.internal.provisional.action.CoolBarManager2;
import org.eclipse.jface.internal.provisional.action.ICoolBarManager2;
import org.eclipse.jface.internal.provisional.action.IToolBarContributionItem;
import org.eclipse.jface.internal.provisional.action.IToolBarManager2;
import org.eclipse.jface.internal.provisional.action.ToolBarContributionItem2;
import org.eclipse.jface.internal.provisional.action.ToolBarManager2;
import org.eclipse.rap.ui.interactiondesign.IWindowComposer;
import org.eclipse.rap.ui.interactiondesign.PresentationFactory;
import org.netxms.ui.eclipse.console.themes.material.managers.MenuBarManager;

/**
 * Presentation factory for "Material" theme
 */
public class MaterialPresentationFactory extends PresentationFactory
{
   /* (non-Javadoc)
    * @see org.eclipse.rap.ui.interactiondesign.PresentationFactory#createCoolBarManager()
    */
   @Override
   public ICoolBarManager2 createCoolBarManager()
   {
      return new CoolBarManager2();
   }

   /* (non-Javadoc)
    * @see org.eclipse.rap.ui.interactiondesign.PresentationFactory#createMenuBarManager()
    */
   @Override
   public MenuManager createMenuBarManager()
   {
      return new MenuBarManager();
   }

   /* (non-Javadoc)
    * @see org.eclipse.rap.ui.interactiondesign.PresentationFactory#createPartMenuManager()
    */
   @Override
   public MenuManager createPartMenuManager()
   {
      return new MenuManager();
   }

   /* (non-Javadoc)
    * @see org.eclipse.rap.ui.interactiondesign.PresentationFactory#createToolBarContributionItem(org.eclipse.jface.action.IToolBarManager, java.lang.String)
    */
   @Override
   public IToolBarContributionItem createToolBarContributionItem(IToolBarManager toolBarManager, String id)
   {
      return new ToolBarContributionItem2(toolBarManager, id);
   }

   /* (non-Javadoc)
    * @see org.eclipse.rap.ui.interactiondesign.PresentationFactory#createToolBarManager()
    */
   @Override
   public IToolBarManager2 createToolBarManager()
   {
      return new ToolBarManager2();
   }

   /* (non-Javadoc)
    * @see org.eclipse.rap.ui.interactiondesign.PresentationFactory#createViewToolBarManager()
    */
   @Override
   public IToolBarManager2 createViewToolBarManager()
   {
      return new ToolBarManager2();
   }

   /* (non-Javadoc)
    * @see org.eclipse.rap.ui.interactiondesign.PresentationFactory#createWindowComposer()
    */
   @Override
   public IWindowComposer createWindowComposer()
   {
      return new MaterialWindowComposer();
   }
}

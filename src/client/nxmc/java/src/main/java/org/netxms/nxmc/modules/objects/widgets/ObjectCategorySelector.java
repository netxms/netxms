/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.ObjectCategory;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.AbstractSelector;
import org.netxms.nxmc.base.widgets.helpers.SelectorConfigurator;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.ObjectCategorySelectionDialog;
import org.xnap.commons.i18n.I18n;

/**
 * Selector for object categories
 */
public class ObjectCategorySelector extends AbstractSelector
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectCategorySelector.class);

   int categoryId = 0;

   /**
    * Create new category selector.
    *
    * @param parent parent composite
    * @param style widget style
    * @param options selector options
    */
   public ObjectCategorySelector(Composite parent, int style)
   {
      super(parent, style, new SelectorConfigurator().setShowClearButton(true));
      setText(i18n.tr("<none>"));
   }

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
    */
   @Override
   protected void selectionButtonHandler()
   {
      ObjectCategorySelectionDialog dlg = new ObjectCategorySelectionDialog(getShell());
      if (dlg.open() == Window.OK)
      {
         ObjectCategory category = Registry.getSession().getObjectCategory(dlg.getCategoryId());
         if (category != null)
         {
            categoryId = category.getId();
            setText(category.getName());
         }
      }
   }

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractSelector#clearButtonHandler()
    */
   @Override
   protected void clearButtonHandler()
   {
      categoryId = 0;
      setText(i18n.tr("<none>"));
   }

   /**
    * Get selected category.
    *
    * @return selected category ID
    */
   public int getCategoryId()
   {
      return categoryId;
   }

   /**
    * Set selected category.
    *
    * @param categoryId selected category ID
    */
   public void setCategoryId(int categoryId)
   {
      if (categoryId == 0)
      {
         this.categoryId = 0;
         setText(i18n.tr("<none>"));
         return;
      }

      ObjectCategory category = Registry.getSession().getObjectCategory(categoryId);
      if (category != null)
      {
         this.categoryId = category.getId();
         setText(category.getName());
      }
      else
      {
         this.categoryId = 0;
         setText(i18n.tr("<none>"));
      }
   }
}

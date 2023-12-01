/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2022 RadenSolutions
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
package org.netxms.nxmc.modules.alarms.widgets;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.events.AlarmCategory;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.AbstractSelector;
import org.netxms.nxmc.base.widgets.helpers.SelectorConfigurator;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.alarms.dialogs.AlarmCategorySelectionDialog;
import org.xnap.commons.i18n.I18n;

/**
 * Alarm category selector widget.
 */
public class AlarmCategorySelector extends AbstractSelector
{
   private final I18n i18n = LocalizationHelper.getI18n(AlarmCategorySelector.class);

   private List<Long> categoryId;

   /**
    * @param parent
    * @param style
    * @param options
    */
   public AlarmCategorySelector(Composite parent, int style)
   {
      super(parent, style, new SelectorConfigurator()
            .setSelectionButtonToolTip(LocalizationHelper.getI18n(AlarmCategorySelector.class).tr("Select category")));
      setText(i18n.tr("<none>"));
      categoryId = new ArrayList<Long>(0);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.AbstractSelector#selectionButtonHandler()
    */
   @Override
   protected void selectionButtonHandler()
   {
      AlarmCategorySelectionDialog dlg = new AlarmCategorySelectionDialog(getShell());
      if (dlg.open() == Window.OK)
      {
         categoryId.clear();
         AlarmCategory categories[] = dlg.getSelectedCategories();
         if ((categories != null) && (categories.length > 0))
         {
            StringBuilder sb = new StringBuilder();
            for(int i = 0; i < categories.length; i++)
            {
               categoryId.add(categories[i].getId());
               sb.append(categories[i].getName() + "; ");
            }
            setText(sb.toString());
         }
         else
         {
            setText(i18n.tr("<none>"));
         }
      }
   }

   /**
    * Get Id of selected category
    * 
    * @return selected category`s Id
    */
   public List<Long> getCategoryId()
   {
      return categoryId;
   }

   /**
    * Set category Id
    * 
    * @param categoryId
    */
   public void setCategoryId(List<Long> categoryId)
   {
      if (this.categoryId.equals(categoryId))
         return; // Nothing to change
      
      if (categoryId != null && !categoryId.isEmpty())
      {
         if (categoryId.get(0) == 0)
         {
            setText(i18n.tr("<none>"));
         }
         else
         {
            List<AlarmCategory> categories = Registry.getSession().findMultipleAlarmCategories(categoryId);
            if (categories != null)
            {
               StringBuilder sb = new StringBuilder();
               this.categoryId.clear();
               for(AlarmCategory c : categories)
               {
                  sb.append(c.getName() + ";");
                  this.categoryId.add(c.getId());
               }
               setText(sb.toString());
            }
            else
            {
               setText(i18n.tr("<unknown>"));
            }
         }
      }
      else
      {
         setText(i18n.tr("<none>"));
      }
      fireModifyListeners();
   }

   /**
    * @see org.netxms.nxmc.base.widgets.AbstractSelector#clearButtonHandler()
    */
   @Override
   protected void clearButtonHandler()
   {
      if (categoryId == null || categoryId.isEmpty())
         return;

      setText(i18n.tr("<none>"));
      fireModifyListeners();
   }
}

/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.widgets;

import java.util.List;
import java.util.regex.Pattern;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.datacollection.DciValue;
import org.netxms.nxmc.base.widgets.AbstractSelector;
import org.netxms.nxmc.base.widgets.helpers.SelectorConfigurator;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.dialogs.SelectDciDialog;

/**
 * Selector for template DCIs
 */
public class TemplateDciSelector extends AbstractSelector
{
   private boolean selectDescription = false;
   private boolean noValueObject = false;

   /**
    * @param parent
    * @param style
    * @param options
    */
   public TemplateDciSelector(Composite parent, int style)
   {
      super(parent, style, new SelectorConfigurator()
            .setEditableText(true)
            .setSelectionButtonToolTip(LocalizationHelper.getI18n(TemplateDciSelector.class).tr("Pick from existing DCI")));
   }

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
    */
   @Override
   protected void selectionButtonHandler()
   {
      SelectDciDialog dlg = new SelectDciDialog(getShell(), 0);
      dlg.setSingleSelection(true);
      dlg.setAllowNoValueObjects(true);
      if (dlg.open() == Window.OK)
      {
         List<DciValue> dci = dlg.getSelection();
         if (dci != null && dci.size() == 1)
         {
            String value = selectDescription ? dci.get(0).getDescription() : dci.get(0).getName();
            if (dci.get(0).isNoValueObject())
            {
               noValueObject = true;
               value = Pattern.quote(value);
               value = value.replace("{instance}", "\\E(.*)\\Q").replace("{instance-name}", "\\E(.*)\\Q");
            } 
            setText(value);
         }
         fireModifyListeners();
      }
   }

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractSelector#setText(java.lang.String)
    */
   @Override
   public void setText(String newText)
   {
      super.setText(newText);
   }

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractSelector#getText()
    */
   @Override
   public String getText()
   {
      return super.getText();
   }

   /**
    * Check if description will be selected instead of name.
    *
    * @return true if description will be selected instead of name
    */
   public boolean isSelectDescription()
   {
      return selectDescription;
   }

   /**
    * Set if description should be selected instead of name.
    *
    * @param selectDescription true if description should be selected instead of name
    */
   public void setSelectDescription(boolean selectDescription)
   {
      this.selectDescription = selectDescription;
   }

   /**
    * @return the noValueObject
    */
   public boolean isNoValueObject()
   {
      return noValueObject;
   }
}

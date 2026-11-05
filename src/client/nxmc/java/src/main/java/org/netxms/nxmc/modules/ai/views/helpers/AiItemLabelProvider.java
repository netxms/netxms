/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.ai.views.helpers;

import org.eclipse.jface.viewers.ITableColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.ai.views.AiSkillsManager;
import org.netxms.nxmc.modules.ai.views.AiSkillsManager.AiItemEntry;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for AI skills and tools list
 */
public class AiItemLabelProvider extends LabelProvider implements ITableLabelProvider, ITableColorProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(AiItemLabelProvider.class);
   private final Color disabledColor = WidgetHelper.getSystemColor(WidgetHelper.COLOR_WIDGET_DISABLED_FOREGROUND);
   private final Image imageTool;
   private final Image imageSkill;
   private final Image imageUnknownElement;

   /**
    * Constructor
    */
   public AiItemLabelProvider()
   {
      imageTool = ResourceManager.getImage("icons/ai/function.png");
      imageSkill = ResourceManager.getImage("icons/ai/skill.png");
      imageUnknownElement = ResourceManager.getImage("icons/ai/unknown-element.png");
   }

   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      imageTool.dispose();
      imageSkill.dispose();
      imageUnknownElement.dispose();
      super.dispose();
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      if (columnIndex != 0)
         return null;

      if (!((AiItemEntry)element).isRegistered())
         return imageUnknownElement;

      switch(((AiItemEntry)element).getTypeChar())
      {
         case 'F':
            return imageTool;
         case 'S':
            return imageSkill;
      }

      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      AiItemEntry entry = (AiItemEntry)element;
      switch(columnIndex)
      {
         case AiSkillsManager.COLUMN_TYPE:
            return entry.getTypeName();
         case AiSkillsManager.COLUMN_NAME:
            return entry.getName() + (entry.isRegistered() ? "" : " *");
         case AiSkillsManager.COLUMN_DESCRIPTION:
            return entry.getDescription();
         case AiSkillsManager.COLUMN_STATUS:
            return entry.isDisabled() ? i18n.tr("Disabled") : i18n.tr("Enabled");
         default:
            return null;
      }
   }

   /**
    * @see org.eclipse.jface.viewers.ITableColorProvider#getForeground(java.lang.Object, int)
    */
   @Override
   public Color getForeground(Object element, int columnIndex)
   {
      if (((AiItemEntry)element).isDisabled())
         return disabledColor;
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableColorProvider#getBackground(java.lang.Object, int)
    */
   @Override
   public Color getBackground(Object element, int columnIndex)
   {
      return null;
   }
}

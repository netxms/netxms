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
package org.netxms.nxmc.base.widgets.helpers;

import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Configurator for AbstractSelector. Could be used by subclasses to tweak base class during construction.
 */
public class SelectorConfigurator
{
   private final I18n i18n = LocalizationHelper.getI18n(SelectorConfigurator.class);

   private boolean useHyperlink = false;
   private boolean showLabel = true;
   private boolean showContextButton = false;
   private boolean showClearButton = false;
   private boolean editableText = false;
   private String selectionButtonToolTip = i18n.tr("Select");
   private String clearButtonToolTip = i18n.tr("Clear selection");
   private String contextButtonToolTip = i18n.tr("Context");
   private String textToolTip = null;

   /**
    * @return the useHyperlink
    */
   public boolean isUseHyperlink()
   {
      return useHyperlink;
   }

   /**
    * @param useHyperlink the useHyperlink to set
    */
   public SelectorConfigurator setUseHyperlink(boolean useHyperlink)
   {
      this.useHyperlink = useHyperlink;
      return this;
   }

   /**
    * @return the showLabel
    */
   public boolean isShowLabel()
   {
      return showLabel;
   }

   /**
    * @param showLabel the showLabel to set
    */
   public SelectorConfigurator setShowLabel(boolean showLabel)
   {
      this.showLabel = showLabel;
      return this;
   }

   /**
    * @return the showContextButton
    */
   public boolean isShowContextButton()
   {
      return showContextButton;
   }

   /**
    * @param showContextButton the showContextButton to set
    */
   public SelectorConfigurator setShowContextButton(boolean showContextButton)
   {
      this.showContextButton = showContextButton;
      return this;
   }

   /**
    * @return the showClearButton
    */
   public boolean isShowClearButton()
   {
      return showClearButton;
   }

   /**
    * @param showClearButton the showClearButton to set
    */
   public SelectorConfigurator setShowClearButton(boolean showClearButton)
   {
      this.showClearButton = showClearButton;
      return this;
   }

   /**
    * @return the editableText
    */
   public boolean isEditableText()
   {
      return editableText;
   }

   /**
    * @param editableText the editableText to set
    */
   public SelectorConfigurator setEditableText(boolean editableText)
   {
      this.editableText = editableText;
      return this;
   }

   /**
    * @param selectionButtonToolTip the selectionButtonToolTip to set
    */
   public SelectorConfigurator setSelectionButtonToolTip(String selectionButtonToolTip)
   {
      this.selectionButtonToolTip = selectionButtonToolTip;
      return this;
   }

   /**
    * @param clearButtonToolTip the clearButtonToolTip to set
    */
   public SelectorConfigurator setClearButtonToolTip(String clearButtonToolTip)
   {
      this.clearButtonToolTip = clearButtonToolTip;
      return this;
   }

   /**
    * @param contextButtonToolTip the contextButtonToolTip to set
    */
   public SelectorConfigurator setContextButtonToolTip(String contextButtonToolTip)
   {
      this.contextButtonToolTip = contextButtonToolTip;
      return this;
   }

   /**
    * @param textToolTip the textToolTip to set
    */
   public SelectorConfigurator setTextToolTip(String textToolTip)
   {
      this.textToolTip = textToolTip;
      return this;
   }

   /**
    * Returns tooltip text for selection button.
    * 
    * @return tooltip text for selection button
    */
   public String getSelectionButtonToolTip()
   {
      return selectionButtonToolTip;
   }

   /**
    * Returns tooltip text for clear button.
    * 
    * @return tooltip text for clear button
    */
   public String getClearButtonToolTip()
   {
      return clearButtonToolTip;
   }

   /**
    * Returns tooltip text for context button.
    * 
    * @return tooltip text for context button
    */
   public String getContextButtonToolTip()
   {
      return contextButtonToolTip;
   }

   /**
    * Returns tooltip text for text area.
    * 
    * @return tooltip text for selection button
    */
   public String getTextToolTip()
   {
      return textToolTip;
   }
}

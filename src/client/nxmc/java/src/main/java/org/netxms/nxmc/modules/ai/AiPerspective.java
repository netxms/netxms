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
package org.netxms.nxmc.modules.ai;

import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.PerspectiveConfiguration;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.ai.views.AiAssistantChatView;
import org.netxms.nxmc.modules.ai.views.AiMessagesView;
import org.xnap.commons.i18n.I18n;

/**
 * AI perspective - home for AI assistant and related interactive views. Only shown when the server has the AI subsystem
 * registered.
 */
public class AiPerspective extends Perspective
{
   private final I18n i18n = LocalizationHelper.getI18n(AiPerspective.class);

   /**
    * The constructor.
    */
   public AiPerspective()
   {
      super("ai", LocalizationHelper.getI18n(AiPerspective.class).tr("AI"), "icons/perspectives/ai.svg");
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#getSectionName()
    */
   @Override
   public String getSectionName()
   {
      return i18n.tr("Administration");
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configurePerspective(org.netxms.nxmc.base.views.PerspectiveConfiguration)
    */
   @Override
   protected void configurePerspective(PerspectiveConfiguration configuration)
   {
      super.configurePerspective(configuration);

      configuration.hasNavigationArea = false;
      configuration.multiViewMainArea = true;
      configuration.hasSupplementalArea = false;
      configuration.useGlobalViewId = true;
      configuration.ignoreViewContext = true;
      configuration.priority = 200;
      configuration.requiredComponentId = "AI-ASSISTANT";
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configureViews()
    */
   @Override
   protected void configureViews()
   {
      addMainView(new AiAssistantChatView());
      addMainView(new AiMessagesView());
   }
}

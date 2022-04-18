/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc.modules.events.propertypages;

import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.modules.events.widgets.RuleEditor;

/**
 * Base property page for EPP rule
 */
public abstract class RuleBasePropertyPage extends PropertyPage
{
   protected RuleEditor editor;
   protected EventProcessingPolicyRule rule;

   /**
    * @param title
    */
   public RuleBasePropertyPage(RuleEditor editor, String title)
   {
      super(title);
      this.editor = editor;
      this.rule = editor.getRule();
   }
}

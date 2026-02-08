/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.widgets.RuleEditor;

/**
 * "AI Agent Instructions" property page
 */
public class RuleAiAgentInstructions extends RuleBasePropertyPage
{
	private Text instructions;

   /**
    * Create property page.
    *
    * @param editor rule editor
    */
   public RuleAiAgentInstructions(RuleEditor editor)
   {
      super(editor, LocalizationHelper.getI18n(RuleAiAgentInstructions.class).tr("AI Agent Instructions"));
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		dialogArea.setLayout(new FillLayout());

		instructions = new Text(dialogArea, SWT.BORDER | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
      instructions.setText(rule.getAiAgentInstructions());

		return dialogArea;
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
	{
      rule.setAiAgentInstructions(instructions.getText().trim());
		editor.setModified(true);
      return true;
	}
}

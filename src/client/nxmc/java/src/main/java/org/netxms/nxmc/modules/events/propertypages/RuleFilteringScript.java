/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.widgets.RuleEditor;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptEditor;
import org.xnap.commons.i18n.I18n;

/**
 * "Filtering Script" property page
 */
public class RuleFilteringScript extends RuleBasePropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(RuleFilteringScript.class);

	private ScriptEditor scriptEditor;

   /**
    * Create property page.
    *
    * @param editor rule editor
    */
   public RuleFilteringScript(RuleEditor editor)
   {
      super(editor, LocalizationHelper.getI18n(RuleFilteringScript.class).tr("Filtering Script"));
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		dialogArea.setLayout(new FillLayout());

      scriptEditor = new ScriptEditor(dialogArea, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL,  
            i18n.tr("Variables:\n\t$object\tevent source object\n\t$node\tevent source object if it's class is Node\n\t$event\tevent being processed\n\t$dci\tDCI associated with this event\n\nReturn value: true to pass event through rule filter"));
		scriptEditor.setText(rule.getFilterScript());

		return dialogArea;
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
	{
		rule.setFilterScript(scriptEditor.getText());
		editor.setModified(true);
      return true;
	}
}

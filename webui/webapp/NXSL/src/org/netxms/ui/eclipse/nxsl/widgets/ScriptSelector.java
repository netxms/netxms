/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.nxsl.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.nxsl.dialogs.SelectScriptDialog;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

/**
 * Script selector
 */
public class ScriptSelector extends AbstractSelector
{
	private static final long serialVersionUID = 1L;

	private String scriptName = "";
	
	/**
	 * @param parent
	 * @param style
	 * @param useHyperlink
	 */
	public ScriptSelector(Composite parent, int style, boolean useHyperlink, boolean showLabel)
	{
		super(parent, style, USE_TEXT | (useHyperlink ? USE_HYPERLINK : 0) | (showLabel ? 0 : HIDE_LABEL));
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
	 */
	@Override
	protected void selectionButtonHandler()
	{
		SelectScriptDialog dlg = new SelectScriptDialog(getShell());
		if (dlg.open() == Window.OK)
		{
			scriptName = dlg.getScript().getName();
			setText(scriptName);
			fireModifyListeners();
		}
	}

	/**
	 * @return the scriptName
	 */
	public final String getScriptName()
	{
		return scriptName;
	}

	/**
	 * @param scriptName the scriptName to set
	 */
	public final void setScriptName(String scriptName)
	{
		this.scriptName = scriptName;
		setText(scriptName);
	}
}

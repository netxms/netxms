/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.epp.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.situations.Situation;
import org.netxms.ui.eclipse.epp.Messages;
import org.netxms.ui.eclipse.epp.SituationCache;
import org.netxms.ui.eclipse.epp.dialogs.SituationSelectionDialog;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

/**
 * Situation selector widget. Provides uniform way to display selected
 * situation object and change selection.
 */
public class SituationSelector extends AbstractSelector
{
	private long situationId = 0;
	
	/**
	 * @param parent
	 * @param style
	 */
	public SituationSelector(Composite parent, int style)
	{
		super(parent, style, USE_TEXT | SHOW_CLEAR_BUTTON);
		setText(Messages.SituationSelector_None);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
	 */
	@Override
	protected void selectionButtonHandler()
	{
		SituationSelectionDialog dlg = new SituationSelectionDialog(getShell());
		dlg.enableMultiSelection(false);
		if (dlg.open() == Window.OK)
		{
			Situation[] situations = dlg.getSelectedSituations();
			if (situations.length > 0)
			{
				situationId = situations[0].getId();
				setText(situations[0].getName());
				getTextControl().setToolTipText(situations[0].getComments());
			}
			else
			{
				situationId = 0;
				setText(Messages.SituationSelector_None);
				getTextControl().setToolTipText(null);
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#clearButtonHandler()
	 */
	@Override
	protected void clearButtonHandler()
	{
		situationId = 0;
		setText(Messages.SituationSelector_None);
		getTextControl().setToolTipText(null);
	}

	/**
	 * Set situation ID
	 * @param situationId
	 */
	public void setSituationId(long situationId)
	{
		this.situationId = situationId;
		if (situationId != 0)
		{
			Situation s = SituationCache.findSituation(situationId);
			if (s != null)
			{
				setText(s.getName());
				getTextControl().setToolTipText(s.getComments());
			}
			else
			{
				setText(Messages.SituationSelector_Unknown);
				getTextControl().setToolTipText(null);
			}
		}
		else
		{
			setText(Messages.SituationSelector_None);
			getTextControl().setToolTipText(null);
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#getSelectionButtonToolTip()
	 */
	@Override
	protected String getSelectionButtonToolTip()
	{
		return Messages.SituationSelector_Tooltip;
	}

	/**
	 * @return the situationId
	 */
	public long getSituationId()
	{
		return situationId;
	}
}

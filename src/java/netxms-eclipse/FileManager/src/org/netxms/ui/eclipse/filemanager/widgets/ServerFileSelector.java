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
package org.netxms.ui.eclipse.filemanager.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.ServerFile;
import org.netxms.ui.eclipse.filemanager.Messages;
import org.netxms.ui.eclipse.filemanager.dialogs.SelectServerFileDialog;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

/**
 * Selector for server files
 *
 */
public class ServerFileSelector extends AbstractSelector
{
	private ServerFile file = null;
	private WorkbenchLabelProvider labelProvider;
	
	/**
	 * 
	 * @param parent
	 * @param style
	 */
	public ServerFileSelector(Composite parent, int style)
	{
		super(parent, style, 0);
		labelProvider = new WorkbenchLabelProvider();
		setText(Messages.ServerFileSelector_None);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
	 */
	@Override
	protected void selectionButtonHandler()
	{
		SelectServerFileDialog dlg = new SelectServerFileDialog(getShell());
		if (dlg.open() == Window.OK)
		{
			setFile(dlg.getSelectedFiles()[0]);
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#getButtonToolTip()
	 */
	@Override
	protected String getButtonToolTip()
	{
		return Messages.ServerFileSelector_Tooltip;
	}

	/**
	 * @return the file
	 */
	public ServerFile getFile()
	{
		return file;
	}

	/**
	 * @param file the file to set
	 */
	public void setFile(ServerFile file)
	{
		this.file = file;
		if (file != null)
		{
			setImage(labelProvider.getImage(file));
			setText(labelProvider.getText(file));
		}
		else
		{
			setImage(null);
			setText(Messages.ServerFileSelector_None);
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Widget#dispose()
	 */
	@Override
	public void dispose()
	{
		labelProvider.dispose();
		super.dispose();
	}
}

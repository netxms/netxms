/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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

import java.io.File;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.netxms.ui.eclipse.filemanager.Messages;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

/**
 * Local file selector
 */
public class LocalFileSelector extends AbstractSelector
{
	private File file = null;
	private String[] filterExtensions = { "*.*" }; //$NON-NLS-1$
	private String[] filterNames = { Messages.LocalFileSelector_AllFiles };
	
	/**
	 * @param parent
	 * @param style
	 */
	public LocalFileSelector(Composite parent, int style, boolean useHyperlink)
	{
		super(parent, style, USE_TEXT | (useHyperlink ? USE_HYPERLINK : 0));

		setText(Messages.LocalFileSelector_None);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
	 */
	@Override
	protected void selectionButtonHandler()
	{
		FileDialog fd = new FileDialog(getShell(), SWT.OPEN);
		fd.setText(Messages.LocalFileSelector_SelectFile);
		fd.setFilterExtensions(filterExtensions);
		fd.setFilterNames(filterNames);
		String selected = fd.open();
		if (selected != null)
			setFile(new File(selected));
		else
			setFile(null);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#getButtonToolTip()
	 */
	@Override
	protected String getSelectionButtonToolTip()
	{
		return Messages.LocalFileSelector_Tooltip;
	}

	/**
	 * @return the file
	 */
	public File getFile()
	{
		return file;
	}

	/**
	 * @param file the file to set
	 */
	public void setFile(File file)
	{
		this.file = file;
		if (file != null)
		{
			setText(file.getAbsolutePath());
		}
		else
		{
			setText(Messages.LocalFileSelector_None);
		}
	}

	/**
	 * @return the filterExtensions
	 */
	public String[] getFilterExtensions()
	{
		return filterExtensions;
	}

	/**
	 * @param filterExtensions the filterExtensions to set
	 */
	public void setFilterExtensions(String[] filterExtensions)
	{
		this.filterExtensions = filterExtensions;
	}

	/**
	 * @return the filterNames
	 */
	public String[] getFilterNames()
	{
		return filterNames;
	}

	/**
	 * @param filterNames the filterNames to set
	 */
	public void setFilterNames(String[] filterNames)
	{
		this.filterNames = filterNames;
	}
}

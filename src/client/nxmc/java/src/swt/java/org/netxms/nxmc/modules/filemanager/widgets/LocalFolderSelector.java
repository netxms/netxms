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
package org.netxms.nxmc.modules.filemanager.widgets;

import java.io.File;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.DirectoryDialog;
import org.netxms.nxmc.base.widgets.AbstractSelector;
import org.netxms.nxmc.base.widgets.helpers.SelectorConfigurator;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Local file selector
 */
public class LocalFolderSelector extends AbstractSelector
{
   private final I18n i18n = LocalizationHelper.getI18n(LocalFolderSelector.class);
   
	private File file = null;
	private int selectorType;

	/**
	 * @param parent
	 * @param style
	 * @param selectorType Selector type: SWT.OPEN or SWT.SAVE
	 */
	public LocalFolderSelector(Composite parent, int style, boolean useHyperlink, int selectorType)
	{
      super(parent, style, new SelectorConfigurator()
            .setUseHyperlink(useHyperlink)
            .setSelectionButtonToolTip(LocalizationHelper.getI18n(LocalFolderSelector.class).tr("Select folder")));

		this.selectorType = selectorType;

		setText(i18n.tr("<none>"));
	}

   /**
    * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
    */
	@Override
	protected void selectionButtonHandler()
	{
	   DirectoryDialog fd = new DirectoryDialog(getShell(), selectorType);
      fd.setText(i18n.tr("Select File"));
      String selected = fd.open();
      if (selected != null)
         setFolder(new File(selected));
      else
         setFolder(null);
	}

	/**
	 * @return the file
	 */
	public File getFolder()
	{
		return file;
	}

   /**
    * @param folder the folder to set
    */
   public void setFolder(File file)
   {
      this.file = file;
      if (file != null)
      {
         setText(file.getAbsolutePath());
      }
      else
      {
         setText(i18n.tr("<none>"));
      }
   }
}

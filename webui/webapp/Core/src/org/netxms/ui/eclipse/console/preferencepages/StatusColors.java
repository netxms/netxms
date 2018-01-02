/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.console.preferencepages;

import org.eclipse.jface.preference.ColorFieldEditor;
import org.eclipse.jface.preference.FieldEditorPreferencePage;
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.IWorkbenchPreferencePage;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;

/**
 * Chart colors preference page
 *
 */
public class StatusColors extends FieldEditorPreferencePage implements IWorkbenchPreferencePage
{
	public StatusColors()
	{
		super(FieldEditorPreferencePage.GRID);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IWorkbenchPreferencePage#init(org.eclipse.ui.IWorkbench)
	 */
	@Override
	public void init(IWorkbench workbench)
	{
		setPreferenceStore(Activator.getDefault().getPreferenceStore());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.FieldEditorPreferencePage#createFieldEditors()
	 */
	@Override
	protected void createFieldEditors()
	{
		addField(new ColorFieldEditor("Status.Colors.Normal", "Normal", getFieldEditorParent())); //$NON-NLS-1$
      addField(new ColorFieldEditor("Status.Colors.Warning", "Warning", getFieldEditorParent())); //$NON-NLS-1$
      addField(new ColorFieldEditor("Status.Colors.Minor", "Minor", getFieldEditorParent())); //$NON-NLS-1$
      addField(new ColorFieldEditor("Status.Colors.Major", "Major", getFieldEditorParent())); //$NON-NLS-1$
      addField(new ColorFieldEditor("Status.Colors.Critical", "Critical", getFieldEditorParent())); //$NON-NLS-1$
      addField(new ColorFieldEditor("Status.Colors.Unknown", "Unknown", getFieldEditorParent())); //$NON-NLS-1$
      addField(new ColorFieldEditor("Status.Colors.Unmanaged", "Unmanaged", getFieldEditorParent())); //$NON-NLS-1$
      addField(new ColorFieldEditor("Status.Colors.Disabled", "Disabled", getFieldEditorParent())); //$NON-NLS-1$
      addField(new ColorFieldEditor("Status.Colors.Testing", "Testing", getFieldEditorParent())); //$NON-NLS-1$
	}

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.FieldEditorPreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      if (!super.performOk())
         return false;
      
      StatusDisplayInfo.updateStatusColors();
      return true;
   }
}

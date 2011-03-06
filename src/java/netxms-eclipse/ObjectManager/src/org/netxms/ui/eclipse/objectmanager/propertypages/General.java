/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectmanager.propertypages;

import java.util.UUID;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.dialogs.PropertyPage;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.imagelibrary.widgets.ImageSelector;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "General" property page for NetMS objects 
 *
 */
public class General extends PropertyPage
{
	private Text textName;
	private String initialName;
	private GenericObject object;
	private ImageSelector image;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		object = (GenericObject)getElement().getAdapter(GenericObject.class);
		if (object == null)	// Paranoid check
			return dialogArea;
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      
      // Object ID
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT, "Object ID",
                                     Long.toString(object.getObjectId()), WidgetHelper.DEFAULT_LAYOUT_DATA);
      
		// Object class
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT, "Object class",
                                     object.getObjectClassName(), WidgetHelper.DEFAULT_LAYOUT_DATA);
		
		// Object name
      initialName = new String(object.getObjectName());
      textName = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, "Object name",
      		                                    initialName, WidgetHelper.DEFAULT_LAYOUT_DATA);
      
      // Image
      image = new ImageSelector(dialogArea, SWT.NONE);
      image.setLabel("Presentation image");
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      image.setLayoutData(gd);
      image.setImageGuid(object.getImage(), false);
		
		return dialogArea;
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		if (isApply)
			setValid(false);
		
		final String newName = new String(textName.getText());
		final UUID newImage = image.getImageGuid();
		new ConsoleJob("Rename object", null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				NXCObjectModificationData data = new NXCObjectModificationData(object.getObjectId());
				data.setName(newName);
				data.setImage(newImage);
				((NXCSession)ConsoleSharedData.getSession()).modifyObject(data);
				initialName = newName;
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot modify object";
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					new UIJob("Update \"General\" property page") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							General.this.setValid(true);
							return Status.OK_STATUS;
						}
					}.schedule();
				}
			}
		}.schedule();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}
}

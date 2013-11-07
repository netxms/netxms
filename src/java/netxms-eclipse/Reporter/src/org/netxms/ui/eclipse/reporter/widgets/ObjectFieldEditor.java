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
package org.netxms.ui.eclipse.reporter.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.api.client.reporting.ReportParameter;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Field editor for "object" type field
 */
public class ObjectFieldEditor extends FieldEditor
{
	private static final String EMPTY_SELECTION_TEXT = "<none>";
	
	private CLabel text;
	private long objectId = 0;
	private WorkbenchLabelProvider labelProvider;
	
	/**
	 * @param parameter
	 * @param toolkit
	 * @param parent
	 */
	public ObjectFieldEditor(ReportParameter parameter, FormToolkit toolkit, Composite parent)
	{
		super(parameter, toolkit, parent);
		labelProvider = new WorkbenchLabelProvider();
		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				labelProvider.dispose();
			}
		});
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.reporter.widgets.FieldEditor#createContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createContent(Composite parent)
	{
		Composite content = toolkit.createComposite(parent, SWT.BORDER);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.TOP;
		content.setLayoutData(gd);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		content.setLayout(layout);
		
		text = new CLabel(content, SWT.NONE);
		toolkit.adapt(text);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.TOP;
		text.setLayoutData(gd);
		text.setText(EMPTY_SELECTION_TEXT);
		
		final ImageHyperlink selectionLink = toolkit.createImageHyperlink(content, SWT.NONE);
		selectionLink.setImage(SharedIcons.IMG_FIND);
		selectionLink.addHyperlinkListener(new HyperlinkAdapter() {
			/* (non-Javadoc)
			 * @see org.eclipse.ui.forms.events.HyperlinkAdapter#linkActivated(org.eclipse.ui.forms.events.HyperlinkEvent)
			 */
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				selectObject();
			}
		});
	}
	
	/**
	 * Select object
	 */
	private void selectObject()
	{
		ObjectSelectionDialog dlg = new ObjectSelectionDialog(getShell(), null, null);
		dlg.enableMultiSelection(false);
		if (dlg.open() == Window.OK)
		{
			AbstractObject[] objects = dlg.getSelectedObjects(AbstractObject.class);
			if (objects.length > 0)
			{
				objectId = objects[0].getObjectId();
				text.setText(objects[0].getObjectName());
				text.setImage(labelProvider.getImage(objects[0]));
			}
			else
			{
				objectId = 0;
				text.setText(EMPTY_SELECTION_TEXT);
				text.setImage(null);
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.reporter.widgets.FieldEditor#getValue()
	 */
	@Override
	public String getValue()
	{
		return Long.toString(objectId);
	}
}

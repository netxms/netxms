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
package org.netxms.ui.eclipse.logviewer.widgets;

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
import org.netxms.client.log.ColumnFilter;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.logviewer.Messages;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Condition editor for object columns
 */
public class ObjectConditionEditor extends ConditionEditor
{
	private static final String[] OPERATIONS = { Messages.ObjectConditionEditor_Is, Messages.ObjectConditionEditor_IsNot, Messages.ObjectConditionEditor_Within, Messages.ObjectConditionEditor_NotWithin };
	private static final String EMPTY_SELECTION_TEXT = Messages.ObjectConditionEditor_None;
	
	private WorkbenchLabelProvider labelProvider;
	private long objectId = 0;
	private CLabel objectName;
	
	/**
	 * @param parent
	 * @param toolkit
	 * @param parentElement
	 */
	public ObjectConditionEditor(Composite parent, FormToolkit toolkit)
	{
		super(parent, toolkit);

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
	 * @see org.netxms.ui.eclipse.logviewer.widgets.ConditionEditor#getOperations()
	 */
	@Override
	protected String[] getOperations()
	{
		return OPERATIONS;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.logviewer.widgets.ConditionEditor#createContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createContent(Composite parent)
	{
		Composite group = new Composite(this, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
		layout.numColumns = 2;
		group.setLayout(layout);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		group.setLayoutData(gd);

		objectName = new CLabel(group, SWT.NONE);
		toolkit.adapt(objectName);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.TOP;
		objectName.setLayoutData(gd);
		objectName.setText(EMPTY_SELECTION_TEXT);
		
		final ImageHyperlink selectionLink = toolkit.createImageHyperlink(group, SWT.NONE);
		selectionLink.setImage(SharedIcons.IMG_FIND);
		selectionLink.addHyperlinkListener(new HyperlinkAdapter() {
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
				objectName.setText(objects[0].getObjectName());
				objectName.setImage(labelProvider.getImage(objects[0]));
			}
			else
			{
				objectId = 0;
				objectName.setText(EMPTY_SELECTION_TEXT);
				objectName.setImage(null);
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.logviewer.widgets.ConditionEditor#createFilter()
	 */
	@Override
	public ColumnFilter createFilter()
	{
		int op = getSelectedOperation();
		ColumnFilter filter = new ColumnFilter(((op == 2) || (op == 3)) ? ColumnFilter.CHILDOF : ColumnFilter.EQUALS, objectId);
		filter.setNegated((op == 1) || (op == 3));
		return filter;
	}
}

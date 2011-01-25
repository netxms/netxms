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
package org.netxms.ui.eclipse.objectview.objecttabs;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Layout;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.Capabilities;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.Commands;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.Comments;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.GeneralInfo;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement;

/**
 * Object overview tab
 *
 */
public class ObjectOverview extends ObjectTab
{
	private static final Color BACKGROUND_COLOR = new Color(Display.getDefault(), 255, 255, 255);
	
	private Set<OverviewPageElement> elements = new HashSet<OverviewPageElement>();
	private Composite leftColumn;
	private Composite rightColumn;
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createTabContent(Composite parent)
	{
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		parent.setLayout(layout);
		parent.setBackground(BACKGROUND_COLOR);
		
		leftColumn = new Composite(parent, SWT.NONE);
		leftColumn.setLayout(createColumnLayout());
		leftColumn.setBackground(BACKGROUND_COLOR);
		GridData gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		//gd.horizontalAlignment = SWT.FILL;
		leftColumn.setLayoutData(gd);
		
		rightColumn = new Composite(parent, SWT.NONE);
		rightColumn.setLayout(createColumnLayout());
		rightColumn.setBackground(BACKGROUND_COLOR);
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		//gd.horizontalAlignment = SWT.FILL;
		rightColumn.setLayoutData(gd);

		addElement(new GeneralInfo(leftColumn, getObject()));
		addElement(new Commands(leftColumn, getObject()));
		addElement(new Comments(leftColumn, getObject()));
		addElement(new Capabilities(rightColumn, getObject()));
	}
	
	/**
	 * Create layout for column
	 * 
	 * @return
	 */
	private Layout createColumnLayout()
	{
		RowLayout layout = new RowLayout();
		layout.fill = true;
		layout.marginBottom = 0;
		layout.marginTop = 0;
		layout.marginLeft = 0;
		layout.marginRight = 0;
		layout.spacing = 5;
		layout.type = SWT.VERTICAL;
		return layout;
	}
	
	/**
	 * Add page element
	 * 
	 * @param element
	 * @param span
	 * @param grabExcessSpace
	 */
	private void addElement(OverviewPageElement element)
	{
		RowData rd = new RowData();
		rd.exclude = false;
		element.setLayoutData(rd);
		elements.add(element);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.GenericObject)
	 */
	@Override
	public void objectChanged(GenericObject object)
	{
		for(OverviewPageElement element : elements)
		{
			element.setVisible(element.isApplicableForObject(object));
			((RowData)element.getLayoutData()).exclude = !element.isVisible();
			element.setObject(object);
		}
		getClientArea().layout();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#dispose()
	 */
	@Override
	public void dispose()
	{
		super.dispose();
	}
}

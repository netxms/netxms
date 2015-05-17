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
package org.netxms.ui.eclipse.objectview.objecttabs;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Layout;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.AvailabilityChart;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.Capabilities;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.Commands;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.Comments;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.Connection;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.GeneralInfo;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.LastValues;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement;

/**
 * Object overview tab
 */
public class ObjectOverview extends ObjectTab
{
	private Set<OverviewPageElement> elements = new HashSet<OverviewPageElement>();
	private ScrolledComposite scroller;
	private Composite viewArea;
	private Composite leftColumn;
	private Composite rightColumn;
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createTabContent(Composite parent)
	{
		scroller = new ScrolledComposite(parent, SWT.V_SCROLL);
		scroller.setExpandVertical(true);
		scroller.setExpandHorizontal(true);
		scroller.getVerticalBar().setIncrement(20);
		scroller.addControlListener(new ControlAdapter() {
			public void controlResized(ControlEvent e)
			{
				Rectangle r = scroller.getClientArea();
				scroller.setMinSize(viewArea.computeSize(r.width, SWT.DEFAULT));
				objectChanged(getObject());
			}
		});
		
		viewArea = new Composite(scroller, SWT.NONE);
		viewArea.setBackground(SharedColors.getColor(SharedColors.OBJECT_TAB_BACKGROUND, parent.getDisplay()));
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		viewArea.setLayout(layout);
		scroller.setContent(viewArea);
		
		leftColumn = new Composite(viewArea, SWT.NONE);
		leftColumn.setLayout(createColumnLayout());
		leftColumn.setBackground(SharedColors.getColor(SharedColors.OBJECT_TAB_BACKGROUND, parent.getDisplay()));
		GridData gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		leftColumn.setLayoutData(gd);
		
		rightColumn = new Composite(viewArea, SWT.NONE);
		rightColumn.setLayout(createColumnLayout());
		rightColumn.setBackground(SharedColors.getColor(SharedColors.OBJECT_TAB_BACKGROUND, parent.getDisplay()));
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		gd.horizontalAlignment = SWT.LEFT;
		gd.grabExcessHorizontalSpace = true;
		gd.minimumWidth = SWT.DEFAULT;
		rightColumn.setLayoutData(gd);

		OverviewPageElement e = new GeneralInfo(leftColumn, null, this);
		elements.add(e);
      e = new LastValues(leftColumn, e, this);
      elements.add(e);
		e = new Commands(leftColumn, e, this);
		elements.add(e);
		e = new AvailabilityChart(leftColumn, e, this);
		elements.add(e);
		e = new Comments(leftColumn, e, this);
		elements.add(e);
		e = new Capabilities(rightColumn, null, this);
		elements.add(e);
		e = new Connection(rightColumn, e, this);
		elements.add(e);
	}
	
	/**
	 * Create layout for column
	 * 
	 * @return
	 */
	private Layout createColumnLayout()
	{
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.verticalSpacing = 5;
		return layout;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#currentObjectUpdated(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public void currentObjectUpdated(AbstractObject object)
	{
		changeObject(object);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public void objectChanged(AbstractObject object)
	{
		viewArea.setRedraw(false);
		for(OverviewPageElement element : elements)
		{
			if (element.isApplicableForObject(object))
			{
				element.setObject(object);
			}
			else
			{
				element.dispose();
			}
		}
		for(OverviewPageElement element : elements)
			element.fixPlacement();
		viewArea.layout(true, true);
		viewArea.setRedraw(true);
		
		Rectangle r = scroller.getClientArea();
		scroller.setMinSize(viewArea.computeSize(r.width, SWT.DEFAULT));
		
		Point s = viewArea.getSize();
		viewArea.redraw(0, 0, s.x, s.y, true);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
	 */
	@Override
	public void refresh()
	{
		objectChanged(getObject());
	}
}

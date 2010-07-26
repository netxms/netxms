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
package org.netxms.ui.eclipse.epp.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Cursor;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.ui.eclipse.epp.Activator;

/**
 * @author victor
 *
 */
public class RuleHeader extends Composite
{
	private Rule rule;
	private Label collapseButton;
	private CLabel number;
	private Image collapsedIcon;
	private Image expandedIcon;
	
	/**
	 * @param parent
	 * @param style
	 */
	public RuleHeader(Rule rule)
	{
		super(rule, SWT.NONE);
		this.rule = rule;
		
		collapsedIcon = Activator.getImageDescriptor("icons/collapsed.png").createImage();
		expandedIcon = Activator.getImageDescriptor("icons/expanded.png").createImage();
		
		setBackground(new Color(getDisplay(), 225, 233, 241));
		GridLayout layout = new GridLayout();
		layout.horizontalSpacing = 2;
		layout.marginHeight = 3;
		layout.marginWidth = 3;
		layout.numColumns = 2;
		setLayout(layout);
		
		collapseButton = new Label(this, SWT.NONE);
		collapseButton.setImage(rule.isCollapsed() ? collapsedIcon : expandedIcon);
		collapseButton.setCursor(new Cursor(getDisplay(), SWT.CURSOR_HAND));
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.LEFT;
		gd.verticalAlignment = SWT.TOP;
		collapseButton.setLayoutData(gd);
		collapseButton.addMouseListener(new MouseListener() {
			@Override
			public void mouseDoubleClick(MouseEvent e)
			{
			}

			@Override
			public void mouseDown(MouseEvent e)
			{
				toggleCollapsedStatus();
			}

			@Override
			public void mouseUp(MouseEvent e)
			{
			}
		});
		
		number = new CLabel(this, SWT.LEFT);
		number.setText(Integer.toString(rule.getId()));
		number.setBackground(getBackground());
		gd = new GridData();
		gd.horizontalAlignment = SWT.LEFT;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		number.setLayoutData(gd);
	}

	/**
	 * Toggle rule's collapsed status
	 */
	private void toggleCollapsedStatus()
	{
		rule.setCollapsed(!rule.isCollapsed());
		collapseButton.setImage(rule.isCollapsed() ? collapsedIcon : expandedIcon);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Widget#dispose()
	 */
	@Override
	public void dispose()
	{
		collapsedIcon.dispose();
		expandedIcon.dispose();
		super.dispose();
	}
}

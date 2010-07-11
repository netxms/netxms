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

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.objects.GenericObject;

/**
 * Object overview tab
 *
 */
public class ObjectOverview extends ObjectTab
{
	private Font headerFont;
	private Text comments;
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createTabContent(Composite parent)
	{
		GridLayout layout = new GridLayout();
		//layout.marginHeight = 0;
		//layout.marginWidth = 0;
		parent.setLayout(layout);
		parent.setBackground(new Color(parent.getDisplay(), 255, 255, 255));
		
		headerFont = new Font(parent.getDisplay(), "Verdana", 8, SWT.BOLD);
		
		Label label = new Label(parent, SWT.NONE);
		label.setFont(headerFont);
		label.setText("Attributes");
		label.setBackground(parent.getBackground());
		
		label = new Label(parent, SWT.NONE);
		label.setFont(headerFont);
		label.setText("Comments");
		label.setBackground(parent.getBackground());
		
		comments = new Text(parent, SWT.MULTI | SWT.READ_ONLY);
		comments.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		comments.setBackground(parent.getBackground());
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.GenericObject)
	 */
	@Override
	public void objectChanged(GenericObject object)
	{
		comments.setText(object.getComments());
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#dispose()
	 */
	@Override
	public void dispose()
	{
		headerFont.dispose();
		super.dispose();
	}
}

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
package org.netxms.ui.eclipse.widgets;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.graphics.Cursor;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.ui.eclipse.console.resources.SharedColors;

/**
 * Implements command box - vertical list of hyperlinks to given actions
 *
 */
public class CommandBox extends Composite implements DisposeListener
{
	private static final long serialVersionUID = 1L;

	private List<Action> actions = new ArrayList<Action>();
	private Map<ImageDescriptor, Image> imageCache = new HashMap<ImageDescriptor, Image>();
	private Cursor cursor;
	private Font font;
	
	/**
	 * @param parent
	 * @param style
	 */
	public CommandBox(Composite parent, int style)
	{
		super(parent, style);
		
		setBackground(SharedColors.getColor(SharedColors.COMMAND_BOX_BACKGROUND, getDisplay()));
		
		cursor = new Cursor(getDisplay(), SWT.CURSOR_HAND);
		font = new Font(getDisplay(), "Verdana", 9, SWT.NORMAL); //$NON-NLS-1$
		
		RowLayout layout = new RowLayout();
		layout.type = SWT.VERTICAL;
		setLayout(layout);
		
		addDisposeListener(this);
	}

	/**
	 * Rebuild command box
	 */
	public void rebuild()
	{
		for(final Action a : actions)
		{
			CLabel label = new CLabel(this, SWT.LEFT);
			label.setText(a.getText());
			label.setImage(getImage(a));
			label.setCursor(cursor);
			label.setForeground(SharedColors.getColor(SharedColors.COMMAND_BOX_TEXT, getDisplay()));
			label.setBackground(SharedColors.getColor(SharedColors.COMMAND_BOX_BACKGROUND, getDisplay()));
			//label.setFont(font);
			label.addMouseListener(new MouseListener() {
				private static final long serialVersionUID = 1L;

				@Override
				public void mouseDoubleClick(MouseEvent e)
				{
				}

				@Override
				public void mouseDown(MouseEvent e)
				{
				}

				@Override
				public void mouseUp(MouseEvent e)
				{
					if (e.button == 1)
						a.run();
				}
			});
		}
		layout();
	}
	
	/**
	 * Get (possibly cached) image for given action
	 * 
	 * @param action action
	 * @return image for given action
	 */
	private Image getImage(Action action)
	{
		ImageDescriptor d = action.getImageDescriptor();
		Image img = null;
		if (d != null)
		{
			img = imageCache.get(d);
			if (img == null)
			{
				img = d.createImage();
				imageCache.put(d, img);
			}
		}
		else
		{
		}
		return img;
	}
	
	/**
	 * Add new action
	 * 
	 * @param action action
	 * @param doRebuild if set to true, rebuild() will be called
	 */
	public void add(Action action, boolean doRebuild)
	{
		actions.add(action);
		if (doRebuild)
			rebuild();
	}
	
	/**
	 * Delete all actions
	 * 
	 * @param doRebuild if set to true, rebuild() will be called
	 */
	public void deleteAll(boolean doRebuild)
	{
		actions.clear();
		for(Control c : getChildren())
			c.dispose();
		if (doRebuild)
			rebuild();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.DisposeListener#widgetDisposed(org.eclipse.swt.events.DisposeEvent)
	 */
	@Override
	public void widgetDisposed(DisposeEvent e)
	{
		for(Image i : imageCache.values())
			i.dispose();
		cursor.dispose();
		font.dispose();
	}
}

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
package org.netxms.ui.eclipse.agentmanager.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Widget for editing agent's config
 */
public class AgentConfigEditor extends Composite
{
	private static final long serialVersionUID = -3664402982525808493L;
	
	private Text editor;

	/**
	 * @param parent
	 * @param style
	 * @param editorStyle
	 */
	public AgentConfigEditor(Composite parent, int style, int editorStyle)
	{
		super(parent, style);
		
		setLayout(new FillLayout());
		editor = new Text(this, SWT.MULTI | editorStyle);
		
		final Font font = new Font(getDisplay(), "Courier New", WidgetHelper.fontPixelsToPoints(getDisplay(), 16), SWT.NORMAL);
		editor.setFont(font);
		addDisposeListener(new DisposeListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetDisposed(DisposeEvent event)
			{
				font.dispose();
			}
		});
	}

	/**
	 * Get underlying text widget
	 * @return text widget
	 */
	public Text getTextWidget()
	{
		return editor;
	}
	
	/**
	 * Set text for editing
	 * @param text
	 */
	public void setText(String text)
	{
		editor.setText(text);
	}
	
	/**
	 * Get editor's content
	 * @return
	 */
	public String getText()
	{
		return editor.getText();
	}
}

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
import java.util.List;

import org.eclipse.jface.action.Action;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.netxms.ui.eclipse.shared.SharedFonts;
import org.netxms.ui.eclipse.widgets.helpers.DashboardElementButton;

/**
 * Dashboard element. Provides all basic functionality - border, buttons, etc.
 *
 */
public abstract class DashboardElement extends Canvas
{
	private static final long serialVersionUID = -9080487483187052859L;
	
	private static final Color DEFAULT_BORDER_COLOR = new Color(Display.getCurrent(), 153, 180, 209);
	private static final Color DEFAULT_TITLE_COLOR = new Color(Display.getCurrent(), 0, 0, 0);
	private static final int BORDER_WIDTH = 3;
	private static final int HEADER_HEIGHT = 22;
	
	private String text;
	private Composite headerArea;
	private Control clientArea;
	private Color borderColor;
	private Color titleColor;
	private Action doubleClickAction = null;
	private List<DashboardElementButton> buttons = new ArrayList<DashboardElementButton>(0);
	
	/**
	 * @param parent
	 * @param style
	 */
	public DashboardElement(Composite parent, String text)
	{
		super(parent, SWT.NONE);
		this.text = text;
		
		borderColor = DEFAULT_BORDER_COLOR;
		titleColor = DEFAULT_TITLE_COLOR;
		
		headerArea = new Composite(this, SWT.NONE);
		headerArea.setBackground(borderColor);
		RowLayout headerLayout = new RowLayout(SWT.HORIZONTAL);
		headerLayout.marginBottom = 0;
		headerLayout.marginTop = 0;
		headerArea.setLayout(headerLayout);
		
		clientArea = createClientAreaInternal();
		
		setFont(SharedFonts.ELEMENT_TITLE);
		
		addPaintListener(new PaintListener() {
			private static final long serialVersionUID = -5503147029441108914L;

			@Override
			public void paintControl(PaintEvent e)
			{
				doPaint(e.gc);
			}
		});
		
		addMouseListener(new MouseListener() {
			private static final long serialVersionUID = 7576373820712020023L;

			@Override
			public void mouseUp(MouseEvent e)
			{
			}
			
			@Override
			public void mouseDown(MouseEvent e)
			{
			}
			
			@Override
			public void mouseDoubleClick(MouseEvent e)
			{
				if (doubleClickAction != null)
					doubleClickAction.run();
			}
		});
		
		GridLayout layout = new GridLayout();
		layout.marginWidth = BORDER_WIDTH;
		layout.marginHeight = BORDER_WIDTH;
		layout.verticalSpacing = 3;
		//layout.marginTop = HEADER_HEIGHT;
		setLayout(layout);
		
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.RIGHT;
		gd.verticalAlignment = SWT.FILL;
		gd.heightHint = HEADER_HEIGHT - layout.verticalSpacing;
		headerArea.setLayoutData(gd);
	}
	
	/**
	 * Create client area control and do necessary configuration
	 * 
	 * @return
	 */
	private Control createClientAreaInternal()
	{
		Control ca = createClientArea(this);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		ca.setLayoutData(gd);
		return ca;
	}
	
	/**
	 * Paint border and other elements
	 * @param gc graphics context
	 */
	private void doPaint(GC gc)
	{
		gc.setForeground(borderColor);
		gc.setLineWidth(BORDER_WIDTH);
		Rectangle rect = getClientArea();
		rect.x += BORDER_WIDTH / 2;
		rect.y += BORDER_WIDTH / 2;
		rect.width -= BORDER_WIDTH;
		rect.height -= BORDER_WIDTH;
		gc.drawRectangle(rect);
		
		rect.height = BORDER_WIDTH / 2 + HEADER_HEIGHT;
		gc.setBackground(borderColor);
		gc.fillRectangle(rect);
		
		gc.setForeground(titleColor);
		gc.drawText(text, 5, 5);
	}

	/**
	 * Create client area for dashboard element.
	 * 
	 * @param parent parent composite
	 * @return client area control
	 */
	abstract protected Control createClientArea(Composite parent);

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#setFocus()
	 */
	@Override
	public boolean setFocus()
	{
		return clientArea.setFocus();
	}

	/**
	 * @return the text
	 */
	public String getText()
	{
		return text;
	}

	/**
	 * @param text the text to set
	 */
	public void setText(String text)
	{
		this.text = text;
	}

	/**
	 * @return the borderColor
	 */
	protected Color getBorderColor()
	{
		return borderColor;
	}

	/**
	 * @param borderColor the borderColor to set
	 */
	protected void setBorderColor(Color borderColor)
	{
		this.borderColor = borderColor;
		headerArea.setBackground(borderColor);
		for(DashboardElementButton b : buttons)
			b.getControl().setBackground(borderColor);
	}

	/**
	 * @return the titleColor
	 */
	protected Color getTitleColor()
	{
		return titleColor;
	}

	/**
	 * @param titleColor the titleColor to set
	 */
	protected void setTitleColor(Color titleColor)
	{
		this.titleColor = titleColor;
	}
	
	/**
	 * Add button
	 * 
	 * @param button
	 */
	public void addButton(final DashboardElementButton button)
	{
		final Label l = new Label(headerArea, SWT.NONE);
		l.setBackground(getBorderColor());
		l.setImage(button.getImage());
		l.setToolTipText(button.getName());
		l.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
		l.addMouseListener(new MouseListener() {
			private static final long serialVersionUID = -12169841952812734L;

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
				button.getAction().run();
			}
		});
		button.setControl(l);
		buttons.add(button);
	}

	/**
	 * @return the doubleClickAction
	 */
	public Action getDoubleClickAction()
	{
		return doubleClickAction;
	}

	/**
	 * @param doubleClickAction the doubleClickAction to set
	 */
	public void setDoubleClickAction(Action doubleClickAction)
	{
		this.doubleClickAction = doubleClickAction;
	}
	
	/**
	 * Dispose current client area and create new one by calling createClientArea
	 */
	public void replaceClientArea()
	{
		if (clientArea != null)
			clientArea.dispose();
		clientArea = createClientAreaInternal();
		layout();
	}
}

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
package org.netxms.ui.eclipse.widgets;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.helpers.DashboardElementButton;

/**
 * Instances of this class provide a custom-drawn border with an optional title bar and buttons.
 */
public abstract class CGroup extends Canvas
{
	private static final int BORDER_WIDTH = 3;
	
	private String text;
	private Control clientArea;
	private Font titleFont;
	private Color borderColor;
	private Color titleColor;
	private Point headerSize;
	private Action doubleClickAction = null;
	private List<DashboardElementButton> buttons = new ArrayList<DashboardElementButton>(0);
	
	/**
	 * @param parent
	 * @param style
	 */
	public CGroup(Composite parent, String text)
	{
		super(parent, SWT.NONE);
		this.text = text;

		setBackground(getParent().getBackground());
		
		borderColor = SharedColors.getColor(SharedColors.CGROUP_BORDER, getDisplay());
		titleColor = SharedColors.getColor(SharedColors.CGROUP_TITLE, getDisplay());
		
		FontData fd = JFaceResources.getDefaultFont().getFontData()[0];
		fd.setStyle(SWT.BOLD);
		titleFont = new Font(getDisplay(), fd);
		setFont(titleFont);
		
		headerSize = WidgetHelper.getTextExtent(this, text);
		headerSize.y += 5;
		
		addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            titleFont.dispose();
         }
      });
		
      GridLayout layout = new GridLayout();
      layout.marginWidth = BORDER_WIDTH;
      layout.marginHeight = BORDER_WIDTH;
      layout.verticalSpacing = 3;
      layout.marginTop = headerSize.y + 2;
      layout.marginBottom = 2;
      setLayout(layout);

      clientArea = createClientAreaInternal();
		
		addPaintListener(new PaintListener() {
			@Override
			public void paintControl(PaintEvent e)
			{
				doPaint(e.gc);
			}
		});
		
		addMouseListener(new MouseListener() {
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
		
		addControlListener(new ControlListener() {
         @Override
         public void controlResized(ControlEvent e)
         {
            layoutButtons();
         }
         
         @Override
         public void controlMoved(ControlEvent e)
         {
         }
      });
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
      Rectangle rect = getClientArea();
      rect.x += BORDER_WIDTH / 2;
      rect.y += BORDER_WIDTH / 2;
      rect.width -= BORDER_WIDTH;
      rect.height -= BORDER_WIDTH;
      
	   gc.setAntialias(SWT.ON);

	   gc.setAlpha(127);
      gc.setBackground(borderColor);
      gc.fillRoundRectangle(rect.x, rect.y, rect.width, headerSize.y, 4, 4);
      gc.setAlpha(255);
	   
	   gc.setForeground(borderColor);
		gc.setLineWidth(BORDER_WIDTH);
		gc.drawRoundRectangle(rect.x, rect.y, rect.width, rect.height, 4, 4);
		gc.setLineWidth(1);
		gc.drawLine(rect.x, headerSize.y, rect.x + rect.width, headerSize.y);

		gc.setForeground(titleColor);
		gc.drawText(text, 10, BORDER_WIDTH, SWT.DRAW_TRANSPARENT);
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
		final Label l = new Label(this, SWT.NONE);
		l.setBackground(getBorderColor());
		l.setImage(button.getImage());
		l.setToolTipText(button.getName());
		l.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
		l.addMouseListener(new MouseListener() {
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
		
		GridData gd = new GridData();
		gd.exclude = true;
		l.setLayoutData(gd);
		
		button.setControl(l);
		buttons.add(button);
		layoutButtons();
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
	
	/**
	 * Layout buttons
	 */
	private void layoutButtons()
	{
	   int pos = getSize().x - 12;
	   for(int i = buttons.size() - 1; i >= 0; i--)
	   {
	      Control c = buttons.get(i).getControl();
	      c.setSize(c.computeSize(SWT.DEFAULT, SWT.DEFAULT));
	      Point cs = c.getSize();
	      c.setLocation(pos - cs.x, headerSize.y / 2 + BORDER_WIDTH - cs.y / 2 - 1);
	      pos -= cs.x + 4;
	   }
	}
}

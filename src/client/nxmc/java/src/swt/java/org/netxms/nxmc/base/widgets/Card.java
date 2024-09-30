/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.base.widgets;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.action.Action;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.MouseAdapter;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.nxmc.base.widgets.helpers.DashboardElementButton;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.FontTools;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Instances of this class provide a custom-drawn border with an optional title bar and buttons.
 */
public abstract class Card extends DashboardComposite
{
   private static final int HEADER_MARGIN_HEIGHT = 6;
   private static final int HEADER_MARGIN_WIDTH = 8;
   private static final int BOTTOM_MARGIN = 2;

	private String text;
	private Control clientArea;
	private Font titleFont;
   private Color titleBackground = null;
	private Color titleColor;
	private Point headerSize;
	private Action doubleClickAction = null;
	private List<DashboardElementButton> buttons = new ArrayList<DashboardElementButton>(0);

	/**
	 * @param parent
	 * @param style
	 */
	public Card(Composite parent, String text)
	{
      super(parent, SWT.BORDER);
		this.text = text;

      titleColor = ThemeEngine.getForegroundColor("Card.Title");

      titleFont = FontTools.createTitleFont();
		setFont(titleFont);

		headerSize = WidgetHelper.getTextExtent(this, text);
      headerSize.y += HEADER_MARGIN_HEIGHT * 2 + getBorderWidth();

      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = 3;
      layout.marginTop = headerSize.y;
      layout.marginBottom = BOTTOM_MARGIN;
      setLayout(layout);

      clientArea = createClientAreaInternal();

      final PaintListener paintListener = new PaintListener() {
			@Override
			public void paintControl(PaintEvent e)
			{
				doPaint(e.gc);
			}
      };
      addPaintListener(paintListener);

      addMouseListener(new MouseAdapter() {
			@Override
			public void mouseDoubleClick(MouseEvent e)
			{
				if (doubleClickAction != null)
					doubleClickAction.run();
			}
		});

      addControlListener(new ControlAdapter() {
         @Override
         public void controlResized(ControlEvent e)
         {
            layoutButtons();
         }
      });

      addDisposeListener((e) -> {
         removePaintListener(paintListener);
         titleFont.dispose();
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
    * Show/hide client area
    */
   protected void showClientArea(boolean show)
   {
      clientArea.setVisible(show);
      ((GridData)clientArea.getLayoutData()).exclude = !show;
      layout(true, true);
   }

   /**
    * @see org.eclipse.swt.widgets.Control#computeSize(int, int, boolean)
    */
   @Override
   public Point computeSize(int wHint, int hHint, boolean changed)
   {
      if (!clientArea.isVisible())
         return new Point(wHint == SWT.DEFAULT ? headerSize.x + 1 : wHint, hHint == SWT.DEFAULT ? headerSize.y + 1 : hHint);
      return super.computeSize(wHint, hHint, changed);
   }

   /**
    * Paint header and footer
    * 
    * @param gc graphics context
    */
	private void doPaint(GC gc)
	{
      Rectangle rect = getFullClientArea();
	   gc.setAntialias(SWT.ON);

      if (clientArea.isVisible())
      {
         gc.setForeground(getBorderOuterColor());
         gc.drawLine(rect.x, headerSize.y, rect.x + rect.width, headerSize.y);
      }

      if (titleBackground != null)
      {
         rect = getClientArea();
         gc.setBackground(titleBackground);
         gc.fillRectangle(rect.x, rect.y, rect.width, headerSize.y - getBorderWidth() - 1);
      }

		gc.setForeground(titleColor);
      gc.drawText(text, HEADER_MARGIN_WIDTH, HEADER_MARGIN_HEIGHT + getBorderWidth(), SWT.DRAW_TRANSPARENT);
	}

	/**
    * Create client area for card.
    * 
    * @param parent parent composite
    * @return client area control
    */
	abstract protected Control createClientArea(Composite parent);

   /**
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
    * @return title background color
    */
	protected Color getTitleBackground()
	{
		return titleBackground;
	}

	/**
    * Set title background color
    * 
    * @param backgroundColor new title background color
    */
	protected void setTitleBackground(Color backgroundColor)
	{
		this.titleBackground = backgroundColor;
		for(DashboardElementButton b : buttons)
			b.getControl().setBackground(backgroundColor);
	}

	/**
	 * @return the titleColor
	 */
	protected Color getTitleColor()
	{
		return titleColor;
	}

	/**
    * Set title text color
    * 
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
		l.setBackground(getTitleBackground());
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
    * Update buttons representation after changes in button objects
    */
   public void updateButtons()
   {
      for(DashboardElementButton b : buttons)
      {
         Label l = (Label)b.getControl();
         l.setImage(b.getImage());
         l.setToolTipText(b.getName());
      }
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
      int pos = getSize().x - HEADER_MARGIN_WIDTH - getBorderWidth();
	   for(int i = buttons.size() - 1; i >= 0; i--)
	   {
	      Control c = buttons.get(i).getControl();
	      c.setSize(c.computeSize(SWT.DEFAULT, SWT.DEFAULT));
	      Point cs = c.getSize();
         c.setLocation(pos - cs.x, headerSize.y / 2 - cs.y / 2);
	      pos -= cs.x + 4;
	   }
	}
}

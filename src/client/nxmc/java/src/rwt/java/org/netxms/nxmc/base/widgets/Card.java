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
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.nxmc.base.widgets.helpers.DashboardElementButton;

/**
 * Instances of this class provide a custom-drawn border with an optional title bar and buttons.
 */
public abstract class Card extends DashboardComposite
{
   private static final int HEADER_MARGIN_HEIGHT = 6;
   private static final int HEADER_MARGIN_WIDTH = 8;

   private Composite header;
	private Label headerText;
	private Control clientArea;
	private Action doubleClickAction = null;
	private List<DashboardElementButton> buttons = new ArrayList<DashboardElementButton>(0);

	/**
	 * @param parent
	 * @param style
	 */
	public Card(Composite parent, String text)
	{
      super(parent, SWT.BORDER);
		
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.marginTop = 0;
      layout.marginBottom = 0;
      layout.verticalSpacing = 0;
      setLayout(layout);

      header = new Composite(this, SWT.NONE);
      header.setData(RWT.CUSTOM_VARIANT, "CardHeader");
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      header.setLayoutData(gd);
      layout = new GridLayout();
      layout.marginHeight = HEADER_MARGIN_HEIGHT;
      layout.marginWidth = HEADER_MARGIN_WIDTH;
      layout.horizontalSpacing = 4;
      header.setLayout(layout);

      headerText = new Label(header, SWT.NONE);
      headerText.setData(RWT.CUSTOM_VARIANT, "CardHeader");
      headerText.setText(text);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.CENTER;
      headerText.setLayoutData(gd);

      clientArea = createClientAreaInternal();

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
		return headerText.getText();
	}

	/**
	 * @param text the text to set
	 */
	public void setText(String text)
	{
		headerText.setText(text);
	}

	/**
    * @return title background color
    */
	protected Color getTitleBackground()
	{
		return header.getBackground();
	}

	/**
    * Set title background color
    * 
    * @param backgroundColor new title background color
    */
	protected void setTitleBackground(Color backgroundColor)
	{
	   header.setBackground(backgroundColor);
	   headerText.setBackground(backgroundColor);
		for(DashboardElementButton b : buttons)
			b.getControl().setBackground(backgroundColor);
	}

	/**
	 * @return the titleColor
	 */
	protected Color getTitleColor()
	{
		return headerText.getForeground();
	}

	/**
    * Set title text color
    * 
    * @param titleColor the titleColor to set
    */
	protected void setTitleColor(Color titleColor)
	{
	   headerText.setForeground(titleColor);
	}

	/**
	 * Add button
	 * 
	 * @param button
	 */
	public void addButton(final DashboardElementButton button)
	{
	   ((GridLayout)header.getLayout()).numColumns++;

		final Label l = new Label(header, SWT.NONE);
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
		gd.horizontalAlignment = SWT.CENTER;
		gd.verticalAlignment = SWT.CENTER;
		l.setLayoutData(gd);

		button.setControl(l);
		buttons.add(button);
		layout(true, true);
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
}

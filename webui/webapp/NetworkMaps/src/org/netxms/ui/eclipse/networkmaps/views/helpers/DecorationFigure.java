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
package org.netxms.ui.eclipse.networkmaps.views.helpers;

import java.util.UUID;
import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.MouseEvent;
import org.eclipse.draw2d.MouseListener;
import org.eclipse.draw2d.MouseMotionListener;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.maps.elements.NetworkMapDecoration;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.imagelibrary.shared.ImageProvider;
import org.netxms.ui.eclipse.tools.ColorConverter;

/**
 * Map decoration figure
 */
public class DecorationFigure extends Figure implements MouseListener, MouseMotionListener
{
	private static final int MARGIN_X = 4;
	private static final int MARGIN_Y = 4;
	private static final int LABEL_MARGIN = 5;
	private static final int TITLE_OFFSET = MARGIN_X + 10;
	
	private static final int TOP_LEFT = 0;
	private static final int TOP_RIGHT = 1;
	private static final int BOTTOM_LEFT = 2;
	private static final int BOTTOM_RIGHT = 3;
	
	private NetworkMapDecoration decoration;
	private MapLabelProvider labelProvider;
	private ExtendedGraphViewer viewer;
	private Label label;
	private boolean drag = false;
	private boolean resize = true;
	private boolean selected = false;
	private int lastX;
	private int lastY;
	
	/**
	 * @param decoration
	 * @param labelProvider
	 */
	public DecorationFigure(NetworkMapDecoration decoration, MapLabelProvider labelProvider, ExtendedGraphViewer viewer)
	{
		this.decoration = decoration;
		this.labelProvider = labelProvider;
		this.viewer = viewer;
		
		if (decoration.getDecorationType() == NetworkMapDecoration.GROUP_BOX)
		{
			setSize(decoration.getWidth(), decoration.getHeight());
	
			label = new Label(decoration.getTitle());
			label.setFont(labelProvider.getTitleFont());
			add(label);
			
			Dimension d = label.getPreferredSize();
			label.setSize(d.width + LABEL_MARGIN * 2, d.height + 2);
			label.setLocation(new Point(TITLE_OFFSET, 0));
			label.setBackgroundColor(labelProvider.getColors().create(ColorConverter.rgbFromInt(decoration.getColor())));
			label.setForegroundColor(SharedColors.getColor(SharedColors.MAP_GROUP_BOX_TITLE, Display.getCurrent()));
			
			createResizeHandle(BOTTOM_RIGHT);
		}
		else	// Image
		{
			try
			{
				UUID guid = UUID.fromString(decoration.getTitle());
				org.eclipse.swt.graphics.Rectangle bounds = ImageProvider.getInstance(Display.getCurrent()).getImage(guid).getBounds();
				setSize(bounds.width, bounds.height);
			}
			catch(IllegalArgumentException e)
			{			
				setSize(decoration.getWidth(), decoration.getHeight());
			}
		}
		
		addMouseListener(this);
	}
	
	/**
	 * Create resize handle
	 */
	private void createResizeHandle(int pos)
	{
		final Figure handle = new Figure();
		add(handle);
		Dimension size = getSize();
		handle.setSize(8, 8);
		
		switch(pos)
		{
			case TOP_LEFT:
				handle.setLocation(new Point(-1, -1));
				handle.setCursor(Display.getCurrent().getSystemCursor(SWT.CURSOR_SIZENWSE));
				break;
			case TOP_RIGHT:
				handle.setLocation(new Point(size.width - 7, -1));
				handle.setCursor(Display.getCurrent().getSystemCursor(SWT.CURSOR_SIZENESW));
				break;
			case BOTTOM_LEFT:
				handle.setLocation(new Point(-1, size.height - 7));
				handle.setCursor(Display.getCurrent().getSystemCursor(SWT.CURSOR_SIZENESW));
				break;
			case BOTTOM_RIGHT:
				handle.setLocation(new Point(size.width - 7, size.height - 7));
				handle.setCursor(Display.getCurrent().getSystemCursor(SWT.CURSOR_SIZENWSE));
				break;
		}
		
		handle.addMouseListener(new MouseListener() {
			@Override
			public void mouseReleased(MouseEvent me)
			{
				if (resize)
				{
					resize = false;
					Dimension size = getSize();
					decoration.setSize(size.width, size.height);
				}
			}
			
			@Override
			public void mousePressed(MouseEvent me)
			{
				if (me.button == 1)
				{
					resize = true;
					lastX = me.x;
					lastY = me.y;
					me.consume();
				}
			}
			
			@Override
			public void mouseDoubleClicked(MouseEvent me)
			{
			}
		});
		handle.addMouseMotionListener(new MouseMotionListener() {
			@Override
			public void mouseMoved(MouseEvent me)
			{
			}
			
			@Override
			public void mouseHover(MouseEvent me)
			{
			}
			
			@Override
			public void mouseExited(MouseEvent me)
			{
			}
			
			@Override
			public void mouseEntered(MouseEvent me)
			{
			}
			
			@Override
			public void mouseDragged(MouseEvent me)
			{
				if (resize)
				{
					Dimension size = getSize();
					
					int dx = me.x - lastX;
					int dy = me.y - lastY;
					
					if ((dx < 0) && (size.width <= 40))
						dx = 0;
					if ((dy < 0) && (size.height <= 20))
						dy = 0;
					
					size.width += dx;
					size.height += dy;
					setSize(size);
					
					Point p = handle.getLocation();
					p.performTranslate(dx, dy);
					handle.setLocation(p);
					
					lastX = me.x;
					lastY = me.y;
					me.consume();
				}
			}
		});
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
	 */
	@Override
	protected void paintFigure(Graphics gc)
	{
		switch(decoration.getDecorationType())
		{
			case NetworkMapDecoration.GROUP_BOX:
				drawGroupBox(gc);
				break;
			case NetworkMapDecoration.IMAGE:
				drawImage(gc);
				break;
			default:
				break;
		}
	}
	
	/**
	 * Draw "group box" decoration
	 */
	private void drawGroupBox(Graphics gc)
	{
		gc.setAntialias(SWT.ON);
		Rectangle rect = new Rectangle(getBounds());
		
		int topMargin = label.getSize().height / 2;
		rect.x += MARGIN_X;
		rect.y += topMargin;
		rect.width -= MARGIN_X * 2;
		rect.height -= MARGIN_Y + topMargin + 1;
		
		final Color color = labelProvider.getColors().create(ColorConverter.rgbFromInt(decoration.getColor()));
		
		gc.setBackgroundColor(color);
		gc.setAlpha(16);
		gc.fillRoundRectangle(rect, 8, 8);
		gc.setAlpha(255);
		
		gc.setForegroundColor(color);
		gc.setLineWidth(3);
		gc.setLineStyle(selected ? Graphics.LINE_DOT : Graphics.LINE_SOLID);
		gc.drawRoundRectangle(rect, 8, 8);
		
		gc.setBackgroundColor(color);
		gc.fillRoundRectangle(label.getBounds(), 8, 8);
	}
	
	/**
	 * Draw "image" decoration
	 */
	private void drawImage(Graphics gc)
	{
		try
		{
			UUID guid = UUID.fromString(decoration.getTitle());
			Image image = ImageProvider.getInstance(Display.getCurrent()).getImage(guid);
			Rectangle rect = new Rectangle(getBounds());
			gc.drawImage(image, rect.x, rect.y);
		}
		catch(IllegalArgumentException e)
		{			
		}
	}

	/**
	 * Stop dragging
	 */
	private void stopDragging()
	{
		removeMouseMotionListener(this);
		drag = false;
		Point loc = getLocation();
		decoration.setLocation(loc.x, loc.y);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.MouseListener#mousePressed(org.eclipse.draw2d.MouseEvent)
	 */
	@Override
	public void mousePressed(MouseEvent me)
	{
		if (!selected)
		{
			viewer.setDecorationSelection(decoration, (me.button == 1) && ((me.getState() & SWT.MOD1) != 0));
			setSelected(true);
		}
		if ((me.button == 1) && ((me.getState() & SWT.MOD1) == 0))
		{
			addMouseMotionListener(this);
			drag = true;
			lastX = me.x;
			lastY = me.y;
			me.consume();
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.MouseListener#mouseReleased(org.eclipse.draw2d.MouseEvent)
	 */
	@Override
	public void mouseReleased(MouseEvent me)
	{
		if ((me.button == 1) && drag)
			stopDragging();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.MouseListener#mouseDoubleClicked(org.eclipse.draw2d.MouseEvent)
	 */
	@Override
	public void mouseDoubleClicked(MouseEvent me)
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.MouseMotionListener#mouseDragged(org.eclipse.draw2d.MouseEvent)
	 */
	@Override
	public void mouseDragged(MouseEvent me)
	{
		if (drag)
		{
			Point p = getLocation();
			p.performTranslate(me.x - lastX, me.y - lastY);
			setLocation(p);
			lastX = me.x;
			lastY = me.y;
			me.consume();
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.MouseMotionListener#mouseEntered(org.eclipse.draw2d.MouseEvent)
	 */
	@Override
	public void mouseEntered(MouseEvent me)
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.MouseMotionListener#mouseExited(org.eclipse.draw2d.MouseEvent)
	 */
	@Override
	public void mouseExited(MouseEvent me)
	{
		if (drag)
			stopDragging();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.MouseMotionListener#mouseHover(org.eclipse.draw2d.MouseEvent)
	 */
	@Override
	public void mouseHover(MouseEvent me)
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.MouseMotionListener#mouseMoved(org.eclipse.draw2d.MouseEvent)
	 */
	@Override
	public void mouseMoved(MouseEvent me)
	{
	}

	/**
	 * @return the selected
	 */
	public final boolean isSelected()
	{
		return selected;
	}

	/**
	 * @param selected the selected to set
	 */
	public final void setSelected(boolean selected)
	{
		this.selected = selected;
		repaint();
	}
	
	/**
	 * Refresh figure
	 */
	public void refresh()
	{
		if (decoration.getDecorationType() == NetworkMapDecoration.GROUP_BOX)
		{
			setSize(decoration.getWidth(), decoration.getHeight());

			label.setText(decoration.getTitle());
			Dimension d = label.getPreferredSize();
			label.setSize(d.width + LABEL_MARGIN * 2, d.height + 2);
			label.setBackgroundColor(labelProvider.getColors().create(ColorConverter.rgbFromInt(decoration.getColor())));
		}
		else	// Image
		{
			try
			{
				UUID guid = UUID.fromString(decoration.getTitle());
				org.eclipse.swt.graphics.Rectangle bounds = ImageProvider.getInstance(Display.getCurrent()).getImage(guid).getBounds();
				setSize(bounds.width, bounds.height);
			}
			catch(IllegalArgumentException e)
			{			
				setSize(decoration.getWidth(), decoration.getHeight());
			}
		}
		repaint();
	}
}

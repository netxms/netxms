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

import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.MouseEvent;
import org.eclipse.draw2d.MouseListener;
import org.eclipse.draw2d.MouseMotionListener;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.swt.SWT;
import org.netxms.client.maps.elements.NetworkMapElement;

/**
 * Map decoration figure
 */
public abstract class DecorationLayerAbstractFigure extends Figure implements MouseListener, MouseMotionListener
{
	
	private NetworkMapElement decoration;
	private ExtendedGraphViewer viewer;
	private boolean drag = false;
	private boolean selected = false;
	private int lastX;
	private int lastY;
	
	/**
	 * @param decoration
	 * @param labelProvider
	 */
	public DecorationLayerAbstractFigure(NetworkMapElement decoration, ExtendedGraphViewer viewer)
	{
	   this.decoration = decoration;
		this.viewer = viewer;
		
		addMouseListener(this);
	}
	

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
	 */
	@Override
	protected abstract void paintFigure(Graphics gc);
	

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
	public abstract void refresh();
}

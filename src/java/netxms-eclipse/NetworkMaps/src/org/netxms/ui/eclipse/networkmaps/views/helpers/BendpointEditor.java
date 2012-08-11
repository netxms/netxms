/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.draw2d.AbsoluteBendpoint;
import org.eclipse.draw2d.Connection;
import org.eclipse.draw2d.ConnectionRouter;
import org.eclipse.draw2d.MouseEvent;
import org.eclipse.draw2d.MouseListener;
import org.eclipse.draw2d.MouseMotionListener;
import org.eclipse.draw2d.RoundedRectangle;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Display;
import org.eclipse.zest.core.widgets.GraphConnection;
import org.netxms.client.maps.NetworkMapLink;

/**
 * Bendpoint editor
 */
public class BendpointEditor
{
	private static final int HANDLE_SIZE = 12;
	
	private NetworkMapLink linkObject;
	private GraphConnection connection;
	private Connection connectionFigure;
	private ExtendedGraphViewer viewer;
	private MouseListener mouseListener;
	private List<AbsoluteBendpoint> bendpoints = new ArrayList<AbsoluteBendpoint>(16);
	private Map<AbsoluteBendpoint, BendpointHandle> handles = new HashMap<AbsoluteBendpoint, BendpointHandle>(16);
	
	/**
	 * @param linkObject
	 * @param connectionFigure
	 */
	public BendpointEditor(NetworkMapLink linkObject, GraphConnection connection, ExtendedGraphViewer viewer)
	{
		this.linkObject = linkObject;
		this.connection = connection;
		this.connectionFigure = connection.getConnectionFigure();
		this.viewer = viewer;
		
		long[] points = linkObject.getBendPoints();
		if (points != null)
		{
			for(int i = 0; i < points.length; i += 2)
			{
				if (points[i] == 0x7FFFFFFF)
					break;
				AbsoluteBendpoint bp = new AbsoluteBendpoint((int)points[i], (int)points[i + 1]);
				bendpoints.add(bp);
				handles.put(bp, new BendpointHandle(bp));
			}
			setRoutingConstraints();
		}
		
		mouseListener = new MouseListener() {
			@Override
			public void mousePressed(MouseEvent me)
			{
			}

			@Override
			public void mouseReleased(MouseEvent me)
			{
			}

			@Override
			public void mouseDoubleClicked(MouseEvent me)
			{
				addBendPoint(me.x, me.y);
			}
		};
		connectionFigure.addMouseListener(mouseListener);
	}
	
	/**
	 * Stop link editing
	 */
	public void stop()
	{
		connectionFigure.removeMouseListener(mouseListener);
		
		saveBendPoints();
		setRoutingConstraints();
		
		for(BendpointHandle h : handles.values())
			h.getParent().remove(h);
	}
	
	/**
	 * Save bend points into map link object
	 */
	private void saveBendPoints()
	{
		long[] points = new long[handles.size() * 2 + 2];
		int pos = 0;
		for(AbsoluteBendpoint bp : bendpoints)
		{
			points[pos++] = bp.x;
			points[pos++] = bp.y;
		}
		points[pos++] = 0x7FFFFFFF;
		points[pos++] = 0x7FFFFFFF;
		linkObject.setBendPoints(points);
	}
	
	/**
	 * Set routing constraint on connection figure
	 */
	private void setRoutingConstraints()
	{
		if (connection.isDisposed())
			return;
		
		ConnectionRouter router = (ConnectionRouter)connection.getData("ROUTER");
		router.setConstraint(connectionFigure, bendpoints);
		connectionFigure.setRoutingConstraint(bendpoints);
	}
	
	/**
	 * Add new bendpoint
	 * 
	 * @param x
	 * @param y
	 */
	private void addBendPoint(int x, int y)
	{
		if (bendpoints.size() >= 16)
			return;
		
		AbsoluteBendpoint bp = new AbsoluteBendpoint(x, y);
		bendpoints.add(bp);
		setRoutingConstraints();

		handles.put(bp, new BendpointHandle(bp));
	}
	
	/**
	 * Bendpoint handle figure
	 */
	private class BendpointHandle extends RoundedRectangle implements MouseListener, MouseMotionListener
	{
		private AbsoluteBendpoint bp;
		private boolean drag = false;
		
		/**
		 * Create new handle figure at given location
		 * 
		 * @param bp
		 * @param x
		 * @param y
		 */
		BendpointHandle(AbsoluteBendpoint bp)
		{
			this.bp = bp;
			setSize(HANDLE_SIZE, HANDLE_SIZE);
			connectionFigure.getParent().add(this);
			setLocation(new Point(bp.x - HANDLE_SIZE / 2, bp.y - HANDLE_SIZE / 2));
			addMouseListener(this);
			setCursor(Display.getCurrent().getSystemCursor(SWT.CURSOR_CROSS));
		}

		/* (non-Javadoc)
		 * @see org.eclipse.draw2d.MouseListener#mousePressed(org.eclipse.draw2d.MouseEvent)
		 */
		@Override
		public void mousePressed(MouseEvent me)
		{
			if (me.button == 1)
			{
				if ((me.getState() & SWT.CONTROL) != 0)
				{
					bendpoints.remove(bp);
					handles.remove(bp);
					getParent().remove(this);
					setRoutingConstraints();
					saveBendPoints();
				}
				else
				{
					addMouseMotionListener(this);
					drag = true;
					viewer.showCrosshair(me.x, me.y);
				}
			}
		}

		/* (non-Javadoc)
		 * @see org.eclipse.draw2d.MouseListener#mouseReleased(org.eclipse.draw2d.MouseEvent)
		 */
		@Override
		public void mouseReleased(MouseEvent me)
		{
			if ((me.button == 1) && drag)
			{
				viewer.hideCrosshair();
				removeMouseMotionListener(this);
				drag = false;
				bp.x = me.x;
				bp.y = me.y;
				setRoutingConstraints();
				saveBendPoints();
			}
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
				viewer.showCrosshair(me.x, me.y);
				setLocation(new Point(me.x - HANDLE_SIZE / 2, me.y - HANDLE_SIZE / 2));
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
	}
}

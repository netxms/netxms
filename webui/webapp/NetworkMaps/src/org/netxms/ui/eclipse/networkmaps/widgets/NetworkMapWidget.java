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
package org.netxms.ui.eclipse.networkmaps.widgets;

import java.util.Comparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.zest.layouts.LayoutAlgorithm;
import org.eclipse.zest.layouts.LayoutStyles;
import org.eclipse.zest.layouts.algorithms.CompositeLayoutAlgorithm;
import org.eclipse.zest.layouts.algorithms.GridLayoutAlgorithm;
import org.eclipse.zest.layouts.algorithms.HorizontalTreeLayoutAlgorithm;
import org.eclipse.zest.layouts.algorithms.RadialLayoutAlgorithm;
import org.eclipse.zest.layouts.algorithms.SpringLayoutAlgorithm;
import org.eclipse.zest.layouts.algorithms.TreeLayoutAlgorithm;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.ui.eclipse.networkmaps.algorithms.ManualLayout;
import org.netxms.ui.eclipse.networkmaps.algorithms.SparseTree;
import org.netxms.ui.eclipse.networkmaps.views.helpers.ExtendedGraphViewer;
import org.netxms.ui.eclipse.networkmaps.views.helpers.GraphLayoutFilter;
import org.netxms.ui.eclipse.networkmaps.views.helpers.MapContentProvider;
import org.netxms.ui.eclipse.networkmaps.views.helpers.MapLabelProvider;

/**
 * Widget for embedding network map into dashboards
 */
public class NetworkMapWidget extends Composite
{
	private static final long serialVersionUID = 1L;

	protected static final int LAYOUT_SPRING = 0;
	protected static final int LAYOUT_RADIAL = 1;
	protected static final int LAYOUT_HTREE = 2;
	protected static final int LAYOUT_VTREE = 3;
	protected static final int LAYOUT_SPARSE_VTREE = 4;
	
	protected ExtendedGraphViewer viewer;
	protected MapLabelProvider labelProvider;

	public NetworkMapWidget(Composite parent, int style)
	{
		super(parent, style);
		
		setLayout(new FillLayout());

		viewer = new ExtendedGraphViewer(this, SWT.NONE);
		viewer.setContentProvider(new MapContentProvider());
		labelProvider = new MapLabelProvider(viewer);
		viewer.setLabelProvider(labelProvider);
	}
	
	/**
	 * @param page
	 */
	public void setContent(NetworkMapPage page)
	{
		viewer.setInput(page);
	}
	
	/**
	 * @param layout
	 */
	public void setMapLayout(int layout)
	{
		if (layout == org.netxms.client.objects.NetworkMap.LAYOUT_MANUAL)
		{
			viewer.setLayoutAlgorithm(new ManualLayout(LayoutStyles.NO_LAYOUT_NODE_RESIZING));
		}
		else
		{
			setLayoutAlgorithm(layout);
		}
	}
	
	/**
	 * Set layout algorithm for map
	 * @param alg
	 */
	@SuppressWarnings("rawtypes")
	private void setLayoutAlgorithm(int alg)
	{
		LayoutAlgorithm algorithm;
		
		switch(alg)
		{
			case LAYOUT_SPRING:
				algorithm = new SpringLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING);
				break;
			case LAYOUT_RADIAL:
				algorithm = new RadialLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING);
				break;
			case LAYOUT_HTREE:
				algorithm = new HorizontalTreeLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING);
				break;
			case LAYOUT_VTREE:
				algorithm = new TreeLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING);
				break;
			case LAYOUT_SPARSE_VTREE:
				TreeLayoutAlgorithm mainLayoutAlgorithm = new TreeLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING);
				mainLayoutAlgorithm.setComparator(new Comparator() {
					@Override
					public int compare(Object arg0, Object arg1)
					{
						return arg0.toString().compareToIgnoreCase(arg1.toString());
					}
				});
				algorithm = new CompositeLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING, 
						new LayoutAlgorithm[] { mainLayoutAlgorithm,
						                        new SparseTree(LayoutStyles.NO_LAYOUT_NODE_RESIZING) });
				break;
			default:
				algorithm = new GridLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING);
				break;
		}
		algorithm.setFilter(new GraphLayoutFilter(true));
		ManualLayout decorationLayoutAlgorithm = new ManualLayout(LayoutStyles.NO_LAYOUT_NODE_RESIZING);
		decorationLayoutAlgorithm.setFilter(new GraphLayoutFilter(false));
		viewer.setLayoutAlgorithm(new CompositeLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING, 
				new LayoutAlgorithm[] { algorithm, decorationLayoutAlgorithm }), true);
	}
}

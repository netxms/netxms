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
package org.netxms.ui.eclipse.serviceview.widgets;

import java.util.Comparator;
import org.eclipse.core.runtime.Platform;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.zest.core.viewers.GraphViewer;
import org.eclipse.zest.layouts.LayoutAlgorithm;
import org.eclipse.zest.layouts.LayoutStyles;
import org.eclipse.zest.layouts.algorithms.CompositeLayoutAlgorithm;
import org.eclipse.zest.layouts.algorithms.TreeLayoutAlgorithm;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.networkmaps.algorithms.SparseTree;
import org.netxms.ui.eclipse.serviceview.widgets.helpers.IServiceFigureListener;
import org.netxms.ui.eclipse.serviceview.widgets.helpers.ServiceTreeContentProvider;
import org.netxms.ui.eclipse.serviceview.widgets.helpers.ServiceTreeElement;
import org.netxms.ui.eclipse.serviceview.widgets.helpers.ServiceTreeLabelProvider;
import org.netxms.ui.eclipse.serviceview.widgets.helpers.ServiceTreeModel;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Service tree widget
 *
 */
public class ServiceTree extends Composite implements IServiceFigureListener
{
	private static final long serialVersionUID = 1L;

	private GraphViewer viewer;
	private ServiceTreeModel model;
	
	/**
	 * @param parent
	 * @param style
	 */
	@SuppressWarnings("rawtypes")
	public ServiceTree(Composite parent, int style, GenericObject rootObject)
	{
		super(parent, style);
		setLayout(new FillLayout());
		
		// Initiate loading of object manager plugin if it was not loaded before
		try
		{
			Platform.getAdapterManager().loadAdapter(((NXCSession)ConsoleSharedData.getSession()).getTopLevelObjects()[0], "org.eclipse.ui.model.IWorkbenchAdapter");
		}
		catch(Exception e)
		{
		}
		
		viewer = new GraphViewer(this, SWT.NONE);
		viewer.setContentProvider(new ServiceTreeContentProvider());
		viewer.setLabelProvider(new ServiceTreeLabelProvider(viewer, this));
		TreeLayoutAlgorithm mainLayoutAlgorithm = new TreeLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING);
		mainLayoutAlgorithm.setComparator(new Comparator() {
			@Override
			public int compare(Object arg0, Object arg1)
			{
				return arg0.toString().compareToIgnoreCase(arg1.toString());
			}
		});
		viewer.setLayoutAlgorithm(new CompositeLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING, 
				new LayoutAlgorithm[] { mainLayoutAlgorithm,
				                        new SparseTree(LayoutStyles.NO_LAYOUT_NODE_RESIZING) }));
		
		model = new ServiceTreeModel(rootObject);
		viewer.setInput(model);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.serviceview.widgets.helpers.IServiceFigureListener#expandButtonPressed(org.netxms.ui.eclipse.serviceview.widgets.helpers.ServiceTreeElement)
	 */
	@Override
	public void expandButtonPressed(ServiceTreeElement element)
	{
		element.setExpanded(!element.isExpanded());
		viewer.refresh();
		viewer.applyLayout();
	}
}

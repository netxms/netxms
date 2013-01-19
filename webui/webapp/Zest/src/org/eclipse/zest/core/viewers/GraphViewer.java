/*******************************************************************************
 * Copyright 2005-2010, CHISEL Group, University of Victoria, Victoria, BC, Canada.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License v1.0 which
 * accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors: The Chisel Group, University of Victoria
 ******************************************************************************/
package org.eclipse.zest.core.viewers;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IContentProvider;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Widget;
import org.eclipse.zest.core.viewers.internal.GraphModelEntityFactory;
import org.eclipse.zest.core.viewers.internal.GraphModelEntityRelationshipFactory;
import org.eclipse.zest.core.viewers.internal.GraphModelFactory;
import org.eclipse.zest.core.viewers.internal.IStylingGraphModelFactory;
import org.eclipse.zest.core.viewers.internal.TreeModelEntityFactory;
import org.eclipse.zest.core.widgets.Graph;
import org.eclipse.zest.core.widgets.GraphItem;
import org.eclipse.zest.core.widgets.ZestStyles;
import org.eclipse.zest.core.widgets.zooming.ZoomManager;
import org.eclipse.zest.layouts.LayoutAlgorithm;

/**
 * This view is used to represent a static graph. Static graphs can be layed
 * out, but do not continually update their layout locations.
 * 
 * @author Ian Bull
 * 
 * @author Chris Callendar
 * 
 * @noextend This class is not intended to be subclassed by clients.
 */
public class GraphViewer extends AbstractStructuredGraphViewer implements
		ISelectionProvider {

	protected Graph graph = null;
	private IStylingGraphModelFactory modelFactory = null;
	private List selectionChangedListeners = null;

	/**
	 * Initializes the viewer.
	 * 
	 * @param composite
	 * @param style
	 *            the style for the viewer and for the layout algorithm
	 * @see ZestStyles#LAYOUT_GRID
	 * @see ZestStyles#LAYOUT_TREE
	 * @see ZestStyles#LAYOUT_RADIAL
	 * @see ZestStyles#LAYOUT_SPRING
	 * @see ZestStyles#NO_OVERLAPPING_NODES
	 * @see ZestStyles#NODES_HIGHLIGHT_ADJACENT
	 * @see SWT#V_SCROLL
	 * @see SWT#H_SCROLL
	 */
	public GraphViewer(Composite composite, int style) {
		super(style);
		this.graph = new Graph(composite, style);
		hookControl(this.graph);
	}

	/**
	 * Initializes the viewer.
	 * 
	 * @param graph
	 *            The initial graph widget
	 */
	public GraphViewer(Graph graph) {
		super(graph.getStyle());
		this.graph = graph;
		hookControl(this.graph);
	}

	protected void hookControl(Control control) {
		super.hookControl(control);

		selectionChangedListeners = new ArrayList();
		getGraphControl().addSelectionListener(new SelectionAdapter() {

			public void widgetSelected(SelectionEvent e) {
				Iterator iterator = selectionChangedListeners.iterator();

				ISelection structuredSelection = getSelection();
				SelectionChangedEvent event = new SelectionChangedEvent(
						GraphViewer.this, structuredSelection);

				while (iterator.hasNext()) {
					ISelectionChangedListener listener = (ISelectionChangedListener) iterator
							.next();
					listener.selectionChanged(event);
				}
				firePostSelectionChanged(event);
			}

		});

		control.addMouseListener(new MouseListener() {

			public void mouseDoubleClick(MouseEvent e) {
				DoubleClickEvent doubleClickEvent = new DoubleClickEvent(
						GraphViewer.this, getSelection());
				fireDoubleClick(doubleClickEvent);
			}

			public void mouseDown(MouseEvent e) {

			}

			public void mouseUp(MouseEvent e) {

			}

		});
	}

	protected void inputChanged(Object input, Object oldInput) {
		graph.setDynamicLayout(false);
		super.inputChanged(input, oldInput);
		graph.setDynamicLayout(true);
		graph.applyLayout();
	}

	/**
	 * Gets the styles for this structuredViewer
	 * 
	 * @return
	 */
	public int getStyle() {
		return this.graph.getStyle();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @seeorg.eclipse.zest.core.viewer.internal.AbstractStructuredGraphViewer#
	 * getGraphControl()
	 */
	public Graph getGraphControl() {
		return super.getGraphControl();
	};

	/**
	 * Sets the layout algorithm to use for this viewer.
	 * 
	 * @param algorithm
	 *            the algorithm to layout the nodes
	 * @param runLayout
	 *            if the layout should be run
	 */
	public void setLayoutAlgorithm(LayoutAlgorithm algorithm, boolean runLayout) {
		graph.setLayoutAlgorithm(algorithm, runLayout);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @seeorg.eclipse.zest.core.viewer.internal.AbstractStructuredGraphViewer#
	 * setLayoutAlgorithm(org.eclipse.zest.layouts.LayoutAlgorithm)
	 */
	public void setLayoutAlgorithm(LayoutAlgorithm algorithm) {
		super.setLayoutAlgorithm(algorithm);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.zest.core.viewers.AbstractStructuredGraphViewer#setNodeStyle
	 * (int)
	 */
	public void setNodeStyle(int nodeStyle) {
		super.setNodeStyle(nodeStyle);
		this.graph.setNodeStyle(nodeStyle);
	}

	public void setContentProvider(IContentProvider contentProvider) {
		if (contentProvider instanceof IGraphContentProvider) {
			super.setContentProvider(contentProvider);
		} else if (contentProvider instanceof IGraphEntityContentProvider) {
			super.setContentProvider(contentProvider);
		} else if (contentProvider instanceof IGraphEntityRelationshipContentProvider) {
			super.setContentProvider(contentProvider);
		} else if (contentProvider instanceof ITreeContentProvider) {
			super.setContentProvider(contentProvider);
		} else {
			throw new IllegalArgumentException(
					"Invalid content provider, only IGraphContentProvider, IGraphEntityContentProvider, IGraphEntityRelationshipContentProvider and ITreeContentProvider are supported.");
		}
	}

	/**
	 * Finds the graph widget item for a given user model item.
	 * 
	 * Note: This method returns an internal interface (GraphItem). You should
	 * be able to cast this to either a IGraphModelNode or IGraphModelConnection
	 * (which are also internal). These are internal because this API is not
	 * stable. If use this method (to access internal nodes and edges), your
	 * code may not compile between versions.
	 * 
	 * @param The
	 *            user model node.
	 * @return An IGraphItem. This should be either a IGraphModelNode or
	 *         IGraphModelConnection
	 */
	public GraphItem findGraphItem(Object element) {
		Widget[] result = findItems(element);
		return (result.length == 0 || !(result[0] instanceof GraphItem)) ? null
				: (GraphItem) result[0];
	}

	/**
	 * Applys the current layout to the viewer
	 */
	public void applyLayout() {
		graph.applyLayout();
	}

	protected void setSelectionToWidget(List l, boolean reveal) {
		GraphItem[] listOfItems = findItems(l);
		graph.setSelection(listOfItems);
	}

	public Control getControl() {
		return graph;
	}

	public Object[] getNodeElements() {
		return super.getNodeElements();
	}

	public Object[] getConnectionElements() {
		return super.getConnectionElements();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.zest.core.viewer.internal.AbstractStructuredGraphViewer#reveal
	 * (java.lang.Object)
	 */
	public void reveal(Object element) {
		super.reveal(element);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @seeorg.eclipse.zest.core.viewer.internal.AbstractStructuredGraphViewer#
	 * setConnectionStyle(int)
	 */
	public void setConnectionStyle(int connectionStyle) {
		super.setConnectionStyle(connectionStyle);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.zest.core.viewer.internal.AbstractStructuredGraphViewer#unReveal
	 * (java.lang.Object)
	 */
	public void unReveal(Object element) {
		super.unReveal(element);
	}

	public void addSelectionChangedListener(ISelectionChangedListener listener) {
		if (!selectionChangedListeners.contains(listener)) {
			selectionChangedListeners.add(listener);
		}
	}

	public void removeSelectionChangedListener(
			ISelectionChangedListener listener) {
		if (selectionChangedListeners.contains(listener)) {
			selectionChangedListeners.remove(listener);
		}
	}

	/**
	 * {@inheritDoc}
	 * 
	 * NOTE: If a layout algorithm is set in the receiver, layout is performed
	 * after the refresh.
	 */
	public void refresh(Object element) {
		boolean dynamicLayoutEnabled = graph.isDynamicLayoutEnabled();
		graph.setDynamicLayout(false);
		super.refresh(element);
		graph.setDynamicLayout(dynamicLayoutEnabled);
	}

	/**
	 * {@inheritDoc}
	 * 
	 * NOTE: If a layout algorithm is set in the receiver, layout is performed
	 * after the refresh.
	 */
	public void refresh(Object element, boolean updateLabels) {
		boolean dynamicLayoutEnabled = graph.isDynamicLayoutEnabled();
		graph.setDynamicLayout(false);
		super.refresh(element, updateLabels);
		graph.setDynamicLayout(dynamicLayoutEnabled);
	}

	/**
	 * {@inheritDoc}
	 * 
	 * NOTE: If a layout algorithm is set in the receiver, layout is performed
	 * after the update.
	 */
	public void update(Object element, String[] properties) {
		boolean dynamicLayoutEnabled = graph.isDynamicLayoutEnabled();
		graph.setDynamicLayout(false);
		super.update(element, properties);
		graph.setDynamicLayout(dynamicLayoutEnabled);
	}

	/**
	 * {@inheritDoc}
	 * 
	 * NOTE: If a layout algorithm is set in the receiver, layout is performed
	 * after the update.
	 */
	public void update(Object[] elements, String[] properties) {
		boolean dynamicLayoutEnabled = graph.isDynamicLayoutEnabled();
		graph.setDynamicLayout(false);
		super.update(elements, properties);
		graph.setDynamicLayout(dynamicLayoutEnabled);
	}

	// @tag zest.bug.156286-Zooming.fix.experimental : expose the zoom manager
	// for new actions.
	protected ZoomManager getZoomManager() {
		return getGraphControl().getZoomManager();
	}

	/**
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.zest.core.viewers.AbstractStructuredGraphViewer#getFactory()
	 * @noreference This method is not intended to be referenced by clients.
	 * @nooverride This method is not intended to be re-implemented or extended
	 *             by clients.
	 */
	protected IStylingGraphModelFactory getFactory() {
		if (modelFactory == null) {
			if (getContentProvider() instanceof IGraphContentProvider) {
				modelFactory = new GraphModelFactory(this);
			} else if (getContentProvider() instanceof IGraphEntityContentProvider) {
				modelFactory = new GraphModelEntityFactory(this);
			} else if (getContentProvider() instanceof IGraphEntityRelationshipContentProvider) {
				modelFactory = new GraphModelEntityRelationshipFactory(this);
			} else if (getContentProvider() instanceof ITreeContentProvider) {
				modelFactory = new TreeModelEntityFactory(this);
			}
		}
		return modelFactory;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @seeorg.eclipse.zest.core.viewers.AbstractStructuredGraphViewer#
	 * getLayoutAlgorithm()
	 */
	protected LayoutAlgorithm getLayoutAlgorithm() {
		return graph.getLayoutAlgorithm();
	}

}

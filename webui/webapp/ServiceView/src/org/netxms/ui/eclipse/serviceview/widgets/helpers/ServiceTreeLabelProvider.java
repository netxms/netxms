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
package org.netxms.ui.eclipse.serviceview.widgets.helpers;

import org.eclipse.draw2d.IFigure;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.eclipse.zest.core.viewers.GraphViewer;
import org.eclipse.zest.core.viewers.IFigureProvider;
import org.netxms.ui.eclipse.serviceview.Activator;

/**
 * Label provider for service tree
 *
 */
public class ServiceTreeLabelProvider extends LabelProvider implements IFigureProvider
{
	private static final long serialVersionUID = 1L;

	private GraphViewer viewer;
	private IServiceFigureListener figureListener;
	private ILabelProvider workbenchLabelProvider;
	private Font font;
	private Image collapsedIcon;
	private Image expandedIcon;
	
	/**
	 * The constructor
	 * 
	 * @param figureListener
	 */
	public ServiceTreeLabelProvider(GraphViewer viewer, IServiceFigureListener figureListener)
	{
		this.viewer = viewer;
		this.figureListener = figureListener;
		workbenchLabelProvider = WorkbenchLabelProvider.getDecoratingWorkbenchLabelProvider();
		font = new Font(Display.getDefault(), "Verdana", 7, SWT.NORMAL);
		collapsedIcon = Activator.getImageDescriptor("icons/collapsed.png").createImage();
		expandedIcon = Activator.getImageDescriptor("icons/expanded.png").createImage();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
	 */
	@Override
	public String getText(Object element)
	{
		if (element instanceof ServiceTreeElement)
			return ((ServiceTreeElement)element).getObject().getObjectName();
		
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
	 */
	@Override
	public Image getImage(Object element)
	{
		if (element instanceof ServiceTreeElement)
			return workbenchLabelProvider.getImage(((ServiceTreeElement)element).getObject());
		return null;
	}

	/**
	 * @return the font
	 */
	public Font getFont()
	{
		return font;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.zest.core.viewers.IFigureProvider#getFigure(java.lang.Object)
	 */
	@Override
	public IFigure getFigure(Object element)
	{
		return new ServiceFigure((ServiceTreeElement)element, this, figureListener);
	}
	
	/**
	 * Get icon representing collapsed/expanded state of a service
	 * 
	 * @param element service object
	 * @return icon
	 */
	public Image getExpansionStatusIcon(ServiceTreeElement element)
	{
		return element.hasChildren() ? (element.isExpanded() ? expandedIcon : collapsedIcon) : null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		font.dispose();
		collapsedIcon.dispose();
		expandedIcon.dispose();
		workbenchLabelProvider.dispose();
		super.dispose();
	}

	/**
	 * Check if given service tree element selected in the viewer
	 * 
	 * @param element element to test
	 * @return true if given element is selected
	 */
	public boolean isElementSelected(ServiceTreeElement element)
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		return (selection != null) ? selection.toList().contains(element) : false;
	}
}

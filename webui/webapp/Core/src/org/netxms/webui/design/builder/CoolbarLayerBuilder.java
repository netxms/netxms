/*******************************************************************************
 * Copyright (c) 2009, 2011 EclipseSource and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    EclipseSource - initial API and implementation
 ******************************************************************************/
package org.netxms.webui.design.builder;

import org.eclipse.rap.ui.interactiondesign.layout.ElementBuilder;
import org.eclipse.rwt.lifecycle.WidgetUtil;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.webui.design.ILayoutSetConstants;
import org.netxms.webui.design.managers.CoolBarManager;

public class CoolbarLayerBuilder extends ElementBuilder
{

	private Image bg;
	private Image right;
	private Image left;
	private Composite layer;

	public CoolbarLayerBuilder(Composite parent, String layoutSetId)
	{
		super(parent, layoutSetId);
		bg = getImage(ILayoutSetConstants.OVERFLOW_BG);
		right = getImage(ILayoutSetConstants.OVERFLOW_RIGHT);
		left = getImage(ILayoutSetConstants.OVERFLOW_LEFT);
	}

	public void addControl(Control control, Object layoutData)
	{
	}

	public void addControl(Control control, String positionId)
	{
	}

	public void addImage(Image image, Object layoutData)
	{
	}

	public void addImage(Image image, String positionId)
	{
	}

	public void build()
	{
		Composite parent = getParent();
		if (left != null)
		{
			parent = getParent().getParent();
		}
		Composite layerParent = new Composite(parent, SWT.NONE);
		layerParent.setBackgroundMode(SWT.INHERIT_FORCE);
		layerParent.setData(WidgetUtil.CUSTOM_VARIANT, "compTrans"); //$NON-NLS-1$
		layerParent.setLayout(new FormLayout());
		FormData fdLayerParent = new FormData();
		layerParent.setLayoutData(fdLayerParent);
		fdLayerParent.top = new FormAttachment(0, 32);
		fdLayerParent.height = bg.getBounds().height;

		Label rightLabel = new Label(layerParent, SWT.NONE);
		rightLabel.setImage(right);
		FormData fdRightLabel = new FormData();
		rightLabel.setLayoutData(fdRightLabel);
		fdRightLabel.top = new FormAttachment(0);
		fdRightLabel.left = new FormAttachment(100, -right.getBounds().width);
		fdRightLabel.height = right.getBounds().height;
		fdRightLabel.width = right.getBounds().width;

		layer = new Composite(layerParent, SWT.NONE);
		layer.setLayout(new FormLayout());
		layer.setBackgroundImage(bg);
		FormData fdLayer = new FormData();
		layer.setLayoutData(fdLayer);
		fdLayer.top = new FormAttachment(0);
		fdLayer.right = new FormAttachment(rightLabel);
		fdLayer.height = bg.getBounds().height;

		if (left != null)
		{
			Label leftLabel = new Label(layerParent, SWT.NONE);
			leftLabel.setImage(left);
			FormData fdLeftLabel = new FormData();
			leftLabel.setLayoutData(fdLeftLabel);
			fdLeftLabel.left = new FormAttachment(0);
			fdLeftLabel.top = new FormAttachment(0);
			fdLeftLabel.height = left.getBounds().height;
			fdLeftLabel.width = left.getBounds().width;
			fdLayer.left = new FormAttachment(leftLabel);
		}
		else
		{
			fdLayer.left = new FormAttachment(0);
		}

	}

	public void dispose()
	{
	}

	public Control getControl()
	{
		return layer;
	}

	public Point getSize()
	{
		return layer.getSize();
	}

	@SuppressWarnings("rawtypes")
	public Object getAdapter(final Class adapter)
	{
		Object result = null;
		if (adapter == CoolBarManager.class)
		{
			result = left;
		}
		return result;
	}
}

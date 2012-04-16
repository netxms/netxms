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
package org.netxms.webui.design.stacks;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.rwt.SessionSingletonBase;
import org.eclipse.swt.widgets.Control;

/**
 * This class acts as a registry for ViewStackPresentations. This is necessary
 * because the same view can be in different parts. If a toolbar for one part
 * change the others should be notified.
 */
public class ViewToolBarRegistry
{
	@SuppressWarnings("rawtypes")
	private List presentationList = new ArrayList();

	private ViewToolBarRegistry()
	{

	}

	public static ViewToolBarRegistry getInstance()
	{
		Object instance = SessionSingletonBase.getInstance(ViewToolBarRegistry.class);
		return (ViewToolBarRegistry)instance;
	}

	@SuppressWarnings("unchecked")
	public void addViewPartPresentation(final ViewStackPresentation presentation)
	{
		presentationList.add(presentation);
	}

	public void removeViewPartPresentation(final ViewStackPresentation presentation)
	{
		presentationList.remove(presentation);
	}

	public void fireToolBarChanged()
	{
		for(int i = 0; i < presentationList.size(); i++)
		{
			if (presentationList.get(i) != null)
			{
				ViewStackPresentation presentation = (ViewStackPresentation)presentationList.get(i);
				presentation.catchToolbarChange();
			}
		}
	}

	public void moveAllToolbarsBellow(final Control control)
	{
		for(int i = 0; i < presentationList.size(); i++)
		{
			if (presentationList.get(i) != null)
			{
				ViewStackPresentation presentation = (ViewStackPresentation)presentationList.get(i);
				presentation.hideAllToolBars(control);
			}
		}
	}

}

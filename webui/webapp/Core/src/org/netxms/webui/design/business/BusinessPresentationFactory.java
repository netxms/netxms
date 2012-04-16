/*******************************************************************************
 * Copyright (c) 2008, 2011 EclipseSource and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    EclipseSource - initial API and implementation
 ******************************************************************************/
package org.netxms.webui.design.business;

import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.internal.provisional.action.ICoolBarManager2;
import org.eclipse.jface.internal.provisional.action.IToolBarContributionItem;
import org.eclipse.jface.internal.provisional.action.IToolBarManager2;
import org.eclipse.rap.ui.interactiondesign.IWindowComposer;
import org.eclipse.rap.ui.interactiondesign.PresentationFactory;
import org.netxms.webui.design.managers.ContribItem;
import org.netxms.webui.design.managers.CoolBarManager;
import org.netxms.webui.design.managers.MenuBarManager;
import org.netxms.webui.design.managers.PartMenuManager;
import org.netxms.webui.design.managers.ToolBarManager;
import org.netxms.webui.design.managers.ViewToolBarManager;

@SuppressWarnings("restriction")
public class BusinessPresentationFactory extends PresentationFactory
{
	public ICoolBarManager2 createCoolBarManager()
	{
		return new CoolBarManager();
	}

	public MenuManager createMenuBarManager()
	{
		return new MenuBarManager();
	}

	public MenuManager createPartMenuManager()
	{
		return new PartMenuManager();
	}

	public IToolBarContributionItem createToolBarContributionItem(final IToolBarManager toolBarManager, final String id)
	{
		return new ContribItem(toolBarManager, id);
	}

	public IToolBarManager2 createToolBarManager()
	{
		return new ToolBarManager();
	}

	public IToolBarManager2 createViewToolBarManager()
	{
		return new ViewToolBarManager();
	}

	public IWindowComposer createWindowComposer()
	{
		return new BusinessWindowComposer();
	}

}

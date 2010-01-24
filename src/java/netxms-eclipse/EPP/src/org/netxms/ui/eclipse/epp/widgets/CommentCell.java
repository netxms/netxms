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
package org.netxms.ui.eclipse.epp.widgets;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.ui.eclipse.epp.dialogs.EditCommentsDlg;

/**
 * Cell holding rule comments
 *
 */
public class CommentCell extends Cell
{
	private Label comments;
	private EventProcessingPolicyRule eppRule;
	private Action actionEdit;
	
	public CommentCell(Rule rule, Object data)
	{
		super(rule, data);
		eppRule = (EventProcessingPolicyRule)data;

		comments = new Label(this, SWT.WRAP);
		comments.setText(eppRule.getComments());
		comments.setBackground(PolicyEditor.COLOR_BACKGROUND);
		comments.addMouseListener(new MouseListener() {
			@Override
			public void mouseDoubleClick(MouseEvent e)
			{
				actionEdit.run();
			}

			@Override
			public void mouseDown(MouseEvent e)
			{
			}

			@Override
			public void mouseUp(MouseEvent e)
			{
			}
		});
	
		createActions();
		createContextMenu();
		
		rule.setRuleName(eppRule.getComments());
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionEdit = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				EditCommentsDlg dlg = new EditCommentsDlg(getShell(), eppRule.getComments());
				if (dlg.open() == Window.OK)
				{
					eppRule.setComments(dlg.getText());
					comments.setText(dlg.getText());
					contentChanged();
				}
			}
		};
		actionEdit.setText("&Edit...");
	}
	
	/**
	 * Create context menu for cell 
	 */
	private void createContextMenu()
	{
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener()
		{
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(comments);
		comments.setMenu(menu);
	}

	/**
	 * Fill cell's context menu
	 * 
	 * @param mgr Menu manager
	 */
	private void fillContextMenu(IMenuManager mgr)
	{
		mgr.add(actionEdit);
	}
}

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
package org.netxms.ui.eclipse.widgets;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.dnd.Clipboard;
import org.eclipse.swt.dnd.TextTransfer;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.netxms.ui.eclipse.library.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Abstract selector widget
 *
 */
public class AbstractSelector extends Composite
{
	private Label label;
	private CLabel text;
	private Button button;
	private Action actionCopy;
	private Image scaledImage = null;
	
	/**
	 * Create abstract selector.
	 * 
	 * @param parent
	 * @param style
	 */
	public AbstractSelector(Composite parent, int style)
	{
		super(parent, style);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.INNER_SPACING;
		layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginTop = 0;
		layout.marginBottom = 0;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
		setLayout(layout);
		
		label = new Label(this, SWT.NONE);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.horizontalSpan = 2;
		label.setLayoutData(gd);
		
		text = new CLabel(this, SWT.BORDER);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.TOP;
		text.setLayoutData(gd);
		
		button = new Button(this, SWT.PUSH);
		gd = new GridData();
		gd.heightHint = text.computeSize(SWT.DEFAULT, SWT.DEFAULT).y;
		button.setLayoutData(gd);
		button.setText("..."); //$NON-NLS-1$
		button.setToolTipText(getButtonToolTip());
		button.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				selectionButtonHandler();
			}
		});
		
		createActions();
		createContextMenu();
		
		text.setToolTipText(getTextToolTip());
		
		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				if (scaledImage != null)
					scaledImage.dispose();
			}
		});
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionCopy = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				final Clipboard cb = new Clipboard(getDisplay());
				cb.setContents(new Object[] { text.getText() }, new Transfer[] { TextTransfer.getInstance() });
			}
		};
		actionCopy.setText(Messages.AbstractSelector_CopyToClipboard);
	}
	
	/**
	 * Create context menu for text area 
	 */
	private void createContextMenu()
	{
		// Create menu manager.
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
		Menu menu = menuMgr.createContextMenu(text);
		text.setMenu(menu);
	}
	
	/**
	 * Fill context menu. Can be overridden by subclasses to add
	 * and/or override context menu items.
	 * 
	 * @param mgr menu manager
	 */
	protected void fillContextMenu(IMenuManager mgr)
	{
		mgr.add(actionCopy);
	}
	
	/**
	 * Handler for selection button. This method intended to be overriden by subclasses.
	 * Default implementation does nothing.
	 */
	protected void selectionButtonHandler()
	{
	}
	
	/**
	 * Returns tooltip text for selection button. Can be overridden by subclasses.
	 * @return tooltip text for selection button
	 */
	protected String getButtonToolTip()
	{
		return Messages.AbstractSelector_Select;
	}

	/**
	 * Returns tooltip text for text area. Can be overridden by subclasses.
	 * @return tooltip text for selection button
	 */
	protected String getTextToolTip()
	{
		return null;
	}

	/**
	 * Set selector's label
	 * 
	 * @param newLabel New label
	 */
	public void setLabel(final String newLabel)
	{
		label.setText(newLabel);
	}
	
	/**
	 * Get selector's label
	 * 
	 * @return Current selector's label
	 */
	public String getLabel()
	{
		return label.getText();
	}
	
	/**
	 * Set selector's text
	 * 
	 * @param newText
	 */
	public void setText(final String newText)
	{
		text.setText(newText);
	}

	/**
	 * Get selector's text
	 * 
	 * @return Selector's text
	 */
	public String getText()
	{
		return text.getText();
	}

	/**
	 * Set selector's image
	 * 
	 * @param newText
	 */
	public void setImage(final Image image)
	{
		if (scaledImage != null)
		{
			scaledImage.dispose();
			scaledImage = null;
		}

		if (image != null)
		{
			Rectangle size = image.getBounds();
			if ((size.width > 64) || (size.height > 64))
			{
				scaledImage = new Image(getDisplay(), image.getImageData().scaledTo(64, 64));
				text.setImage(scaledImage);
			}
			else
			{
				text.setImage(image);
			}
		}
		else
		{
			text.setImage(null);
		}
	}

	/**
	 * Get selector's text
	 * 
	 * @return Selector's text
	 */
	public Image getImage()
	{
		return text.getImage();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Control#setEnabled(boolean)
	 */
	@Override
	public void setEnabled(boolean enabled)
	{
		text.setEnabled(enabled);
		button.setEnabled(enabled);
		super.setEnabled(enabled);
	}
	
	/**
	 * Get text control
	 * @return text control
	 */
	protected Control getTextControl()
	{
		return text;
	}
}

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
package org.netxms.ui.eclipse.widgets;

import java.util.HashSet;
import java.util.Set;
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
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Abstract selector widget
 */
public class AbstractSelector extends Composite
{
	public static final int USE_HYPERLINK = 0x0001;
	public static final int HIDE_LABEL = 0x0002;
	public static final int USE_TEXT = 0x0004;
	public static final int SHOW_CLEAR_BUTTON = 0x0008;
	
	private Label label;
	private Control text;
	private Button selectionButton;
	private Button clearingButton;
	private ImageHyperlink selectionLink;
	private ImageHyperlink clearingLink;
	private Action actionCopy;
	private Image scaledImage = null;
	private Set<ModifyListener> modifyListeners = new HashSet<ModifyListener>(0);
	
	/**
	 * Create abstract selector.
	 * 
	 * @param parent
	 * @param style
	 * @param options
	 */
	public AbstractSelector(Composite parent, int style, int options)
	{
		super(parent, style);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.INNER_SPACING;
		layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginTop = 0;
		layout.marginBottom = 0;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = ((options & SHOW_CLEAR_BUTTON) != 0) ? 3 : 2;
		setLayout(layout);
		
		if ((options & HIDE_LABEL) == 0)
		{
			label = new Label(this, SWT.NONE);
			GridData gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.horizontalSpan = layout.numColumns;
			label.setLayoutData(gd);
		}
		
		text = ((options & USE_TEXT) != 0) ? new Text(this, SWT.BORDER | SWT.READ_ONLY) : new CLabel(this, SWT.BORDER);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.TOP;
		text.setLayoutData(gd);
		
		if ((options & USE_HYPERLINK) != 0)
		{
			selectionLink = new ImageHyperlink(this, SWT.NONE);
			gd = new GridData();
			gd.heightHint = text.computeSize(SWT.DEFAULT, SWT.DEFAULT).y;
			selectionLink.setLayoutData(gd);
			selectionLink.setImage(SharedIcons.IMG_FIND);
			selectionLink.setToolTipText(getSelectionButtonToolTip());
			selectionLink.addHyperlinkListener(new HyperlinkAdapter() {
				@Override
				public void linkActivated(HyperlinkEvent e)
				{
					selectionButtonHandler();
				}
			});
			
			if ((options & SHOW_CLEAR_BUTTON) != 0)
			{
				clearingLink = new ImageHyperlink(this, SWT.NONE);
				gd = new GridData();
				gd.heightHint = text.computeSize(SWT.DEFAULT, SWT.DEFAULT).y;
				clearingLink.setLayoutData(gd);
				clearingLink.setImage(SharedIcons.IMG_CLEAR);
				clearingLink.setToolTipText(getClearingButtonToolTip());
				clearingLink.addHyperlinkListener(new HyperlinkAdapter() {
					@Override
					public void linkActivated(HyperlinkEvent e)
					{
						clearButtonHandler();
					}
				});
			}			
		}
		else
		{
			selectionButton = new Button(this, SWT.PUSH);
			gd = new GridData();
			gd.heightHint = text.computeSize(SWT.DEFAULT, SWT.DEFAULT).y;
			selectionButton.setLayoutData(gd);
			selectionButton.setImage(SharedIcons.IMG_FIND);
			selectionButton.setToolTipText(getSelectionButtonToolTip());
			selectionButton.addSelectionListener(new SelectionListener() {
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
			
			if ((options & SHOW_CLEAR_BUTTON) != 0)
			{
				clearingButton = new Button(this, SWT.PUSH);
				gd = new GridData();
				gd.heightHint = text.computeSize(SWT.DEFAULT, SWT.DEFAULT).y;
				clearingButton.setLayoutData(gd);
				clearingButton.setImage(SharedIcons.IMG_CLEAR);
				clearingButton.setToolTipText(getClearingButtonToolTip());
				clearingButton.addSelectionListener(new SelectionListener() {
					@Override
					public void widgetDefaultSelected(SelectionEvent e)
					{
						widgetSelected(e);
					}
		
					@Override
					public void widgetSelected(SelectionEvent e)
					{
						clearButtonHandler();
					}
				});
			}
		}
		
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
			@Override
			public void run()
			{
				final Clipboard cb = new Clipboard(getDisplay());
				cb.setContents(new Object[] { AbstractSelector.this.getText() }, new Transfer[] { TextTransfer.getInstance() });
			}
		};
		actionCopy.setText(Messages.get().AbstractSelector_CopyToClipboard);
	}
	
	/**
	 * Create context menu for text area 
	 */
	private void createContextMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
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
	 * Handler for clear button. This method intended to be overriden by subclasses.
	 * Default implementation does nothing.
	 */
	protected void clearButtonHandler()
	{
	}
	
	/**
	 * Returns tooltip text for selection button. Can be overridden by subclasses.
	 * @return tooltip text for selection button
	 */
	protected String getSelectionButtonToolTip()
	{
		return Messages.get().AbstractSelector_Select;
	}

	/**
	 * Returns tooltip text for clear button. Can be overridden by subclasses.
	 * @return tooltip text for clear button
	 */
	protected String getClearingButtonToolTip()
	{
		return Messages.get().AbstractSelector_ClearSelection;
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
	protected void setText(final String newText)
	{
		if (text instanceof CLabel)
			((CLabel)text).setText(newText);
		else
			((Text)text).setText(newText);
	}

	/**
	 * Get selector's text
	 * 
	 * @return Selector's text
	 */
	protected String getText()
	{
		if (text instanceof CLabel)
			return ((CLabel)text).getText();
		else
			return ((Text)text).getText();
	}

	/**
	 * Set selector's image
	 * 
	 * @param newText
	 */
	protected void setImage(final Image image)
	{
		if (!(text instanceof CLabel))
			return;
		
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
				((CLabel)text).setImage(scaledImage);
			}
			else
			{
				((CLabel)text).setImage(image);
			}
		}
		else
		{
			((CLabel)text).setImage(null);
		}
	}

	/**
	 * Get selector's text
	 * 
	 * @return Selector's text
	 */
	protected Image getImage()
	{
		if (text instanceof CLabel)
			return ((CLabel)text).getImage();
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Control#setEnabled(boolean)
	 */
	@Override
	public void setEnabled(boolean enabled)
	{
		text.setEnabled(enabled);
		if (selectionButton != null)
			selectionButton.setEnabled(enabled);
		if (selectionLink != null)
			selectionLink.setEnabled(enabled);
		if (clearingButton != null)
			clearingButton.setEnabled(enabled);
		if (clearingLink != null)
			clearingLink.setEnabled(enabled);
		super.setEnabled(enabled);
	}
	
	/**
	 * Get text control
	 * @return text control
	 */
	public Control getTextControl()
	{
		return text;
	}
	
	/**
	 * @param listener
	 */
	public void addModifyListener(ModifyListener listener)
	{
		modifyListeners.add(listener);
	}
	
	/**
	 * @param listener
	 */
	public void removeModifyListener(ModifyListener listener)
	{
		modifyListeners.remove(listener);
	}
	
	/**
	 * Call all registered modify listeners
	 */
	protected void fireModifyListeners()
	{
		if (modifyListeners.isEmpty())
			return;
		
		Event e = new Event();
		e.display = getDisplay();
		e.doit = true;
		e.widget = this;
		ModifyEvent me = new ModifyEvent(e);
		for(ModifyListener l : modifyListeners)
			l.modifyText(me);
	}
}

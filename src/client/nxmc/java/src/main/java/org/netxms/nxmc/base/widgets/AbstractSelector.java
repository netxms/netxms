/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.base.widgets;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Label;
import org.netxms.nxmc.base.widgets.events.HyperlinkAdapter;
import org.netxms.nxmc.base.widgets.events.HyperlinkEvent;
import org.netxms.nxmc.base.widgets.helpers.SelectorConfigurator;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Abstract selector widget
 */
public class AbstractSelector extends Composite
{
   private final I18n i18n = LocalizationHelper.getI18n(AbstractSelector.class);

	private Label label;
   private CText text;
   private CLabel errorMessage;
	private Button buttonSelect;
   private Button buttonContext;
	private Button buttonClear;
	private ImageHyperlink linkSelect;
   private ImageHyperlink linkContext;
	private ImageHyperlink linkClear;
	private Action actionCopy;
   private Image imageContext = null;
	private Image scaledImage = null;
   private Image errorIcon = null;
	private Set<ModifyListener> modifyListeners = new HashSet<ModifyListener>(0);

   /**
    * Create abstract selector with default configuration.
    * 
    * @param parent parent composite
    * @param style widget style
    * @param configurator configurator
    */
   public AbstractSelector(Composite parent, int style)
   {
      this(parent, style, new SelectorConfigurator());
   }

	/**
    * Create abstract selector.
    * 
    * @param parent parent composite
    * @param style widget style
    * @param configurator configurator
    */
   public AbstractSelector(Composite parent, int style, SelectorConfigurator configurator)
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
      if (configurator.isShowContextButton())
         layout.numColumns++;
      if (configurator.isShowClearButton())
         layout.numColumns++;
		setLayout(layout);

      if (configurator.isShowLabel())
		{
			label = new Label(this, SWT.NONE);
			GridData gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.horizontalSpan = layout.numColumns;
			label.setLayoutData(gd);
		}

      text = new CText(this, SWT.BORDER | (configurator.isEditableText() ? 0 : SWT.READ_ONLY));
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
		text.setLayoutData(gd);

      if (configurator.isUseHyperlink())
		{
			linkSelect = new ImageHyperlink(this, SWT.NONE);
			gd = new GridData();
			gd.heightHint = text.computeSize(SWT.DEFAULT, SWT.DEFAULT).y;
			linkSelect.setLayoutData(gd);
			linkSelect.setImage(SharedIcons.IMG_FIND);
         linkSelect.setToolTipText(configurator.getSelectionButtonToolTip());
			linkSelect.addHyperlinkListener(new HyperlinkAdapter() {
				@Override
				public void linkActivated(HyperlinkEvent e)
				{
					selectionButtonHandler();
				}
			});

         if (configurator.isShowContextButton())
			{
            imageContext = ResourceManager.getImage("icons/context.png");

            linkContext = new ImageHyperlink(this, SWT.NONE);
				gd = new GridData();
				gd.heightHint = text.computeSize(SWT.DEFAULT, SWT.DEFAULT).y;
            linkContext.setLayoutData(gd);
            linkContext.setImage(imageContext);
            linkContext.setToolTipText(configurator.getContextButtonToolTip());
            linkContext.addHyperlinkListener(new HyperlinkAdapter() {
					@Override
					public void linkActivated(HyperlinkEvent e)
					{
                  contextButtonHandler();
					}
				});
			}			

         if (configurator.isShowClearButton())
         {
            linkClear = new ImageHyperlink(this, SWT.NONE);
            gd = new GridData();
            gd.heightHint = text.computeSize(SWT.DEFAULT, SWT.DEFAULT).y;
            linkClear.setLayoutData(gd);
            linkClear.setImage(SharedIcons.IMG_CLEAR);
            linkClear.setToolTipText(configurator.getClearButtonToolTip());
            linkClear.addHyperlinkListener(new HyperlinkAdapter() {
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
			buttonSelect = new Button(this, SWT.PUSH);
			gd = new GridData();
			gd.heightHint = text.computeSize(SWT.DEFAULT, SWT.DEFAULT).y;
			buttonSelect.setLayoutData(gd);
			buttonSelect.setImage(SharedIcons.IMG_FIND);
         buttonSelect.setToolTipText(configurator.getSelectionButtonToolTip());
         buttonSelect.addSelectionListener(new SelectionAdapter() {
				@Override
				public void widgetSelected(SelectionEvent e)
				{
					selectionButtonHandler();
				}
			});

         if (configurator.isShowContextButton())
			{
            imageContext = ResourceManager.getImage("icons/context.png");

            buttonContext = new Button(this, SWT.PUSH);
				gd = new GridData();
				gd.heightHint = text.computeSize(SWT.DEFAULT, SWT.DEFAULT).y;
            buttonContext.setLayoutData(gd);
            buttonContext.setImage(imageContext);
            buttonContext.setToolTipText(configurator.getContextButtonToolTip());
            buttonContext.addSelectionListener(new SelectionAdapter() {
					@Override
					public void widgetSelected(SelectionEvent e)
					{
                  contextButtonHandler();
					}
				});
			}

         if (configurator.isShowClearButton())
         {
            buttonClear = new Button(this, SWT.PUSH);
            gd = new GridData();
            gd.heightHint = text.computeSize(SWT.DEFAULT, SWT.DEFAULT).y;
            buttonClear.setLayoutData(gd);
            buttonClear.setImage(SharedIcons.IMG_CLEAR);
            buttonClear.setToolTipText(configurator.getClearButtonToolTip());
            buttonClear.addSelectionListener(new SelectionAdapter() {
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

      text.setToolTipText(configurator.getTextToolTip());

      addDisposeListener((e) -> {
         if (scaledImage != null)
            scaledImage.dispose();
         if (imageContext != null)
            imageContext.dispose();
         if (errorIcon != null)
            errorIcon.dispose();
		});
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
      actionCopy = new Action(i18n.tr("&Copy to clipboard")) {
			@Override
			public void run()
			{
            WidgetHelper.copyToClipboard(AbstractSelector.this.getText());
			}
		};
	}

	/**
	 * Create context menu for text area 
	 */
	private void createContextMenu()
	{
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> fillContextMenu(m));
      text.setMenu(menuMgr.createContextMenu(text));
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
    * Handler for context button. This method intended to be overriden by subclasses. Default implementation does nothing.
    */
   protected void contextButtonHandler()
   {
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
      text.setText((newText != null) ? newText : "");
	}

	/**
	 * Get selector's text
	 * 
	 * @return Selector's text
	 */
	protected String getText()
	{
      return text.getText();
	}

	/**
    * Set selector's image
    * 
    * @param image new image (can be null)
    */
	protected void setImage(final Image image)
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
    * Get selector's image
    * 
    * @return selector's image
    */
	protected Image getImage()
	{
      return text.getImage();
	}

   /**
    * @see org.eclipse.swt.widgets.Control#setBackground(org.eclipse.swt.graphics.Color)
    */
   @Override
   public void setBackground(Color color)
   {
      super.setBackground(color);
      for(Control c : getChildren())
         c.setBackground(color);
   }

   /**
    * @see org.eclipse.swt.widgets.Control#setEnabled(boolean)
    */
	@Override
	public void setEnabled(boolean enabled)
	{
		text.setEnabled(enabled);
		if (buttonSelect != null)
			buttonSelect.setEnabled(enabled);
		if (linkSelect != null)
			linkSelect.setEnabled(enabled);
      if (buttonContext != null)
         buttonContext.setEnabled(enabled);
      if (linkContext != null)
         linkContext.setEnabled(enabled);
      if (buttonClear != null)
         buttonClear.setEnabled(enabled);
		if (linkClear != null)
			linkClear.setEnabled(enabled);
		super.setEnabled(enabled);
	}
   
   /**
    * If selection button should be enabled 
    * @param enabled if selection button should be enabled
    */
   public void setSelectioEnabled(boolean enabled)
   {
      if (buttonSelect != null)
         buttonSelect.setEnabled(enabled);
      if (linkSelect != null)
         linkSelect.setEnabled(enabled);
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

   /**
    * Set error message inside control.
    *
    * @param message message to show or null to hide existing message
    */
   public void setErrorMessage(String message)
   {
      if (message == null)
      {
         if (errorMessage != null)
         {
            errorMessage.dispose();
            errorMessage = null;
            layout(true, true);
         }
         return;
      }

      if (errorMessage == null)
      {
         if (errorIcon == null)
            errorIcon = ResourceManager.getImage("icons/labeled-control-alert.png");

         errorMessage = new CLabel(this, SWT.NONE);
         errorMessage.setForeground(ThemeEngine.getForegroundColor("List.Error"));
         errorMessage.setText(message);
         errorMessage.setImage(errorIcon);
         layout(true, true);
      }
   }
}

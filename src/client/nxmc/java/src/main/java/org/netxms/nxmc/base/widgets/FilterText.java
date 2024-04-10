/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.fieldassist.ContentProposalAdapter;
import org.eclipse.jface.fieldassist.IContentProposal;
import org.eclipse.jface.fieldassist.IContentProposalListener;
import org.eclipse.jface.fieldassist.IContentProposalProvider;
import org.eclipse.jface.fieldassist.TextContentAdapter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.KeyListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Filter text widgets. Shows labelled text entry field, filtering 
 * attribute selection buttons, and close button.
 */
public class FilterText extends Composite
{
   private I18n i18n = LocalizationHelper.getI18n(FilterText.class);
	private Text text;
	private Composite buttonArea;
	private Composite textArea;
	private List<Button> attrButtons = new ArrayList<Button>(4);
   private Label tooltipIcon;
	private Label closeButton;
	private Label clearButton;
	private Action closeAction = null;
   private Runnable closeCallback = null;
   private boolean autoApply = true;
	private int delay = 300;
	private int minLength = 1;
	private Set<ModifyListener> modifyListeners = new HashSet<ModifyListener>();
	private ModifyEvent lastModifyEvent = null;
   private ContentProposalAdapter proposalAdapter = null;
   private Color defaultBackground;

	public FilterText(Composite parent, int style)
   {
      this(parent, style, null, true, true, null);
   }

   public FilterText(Composite parent, int style, String tooltip, boolean showFilterCloseButton)
   {
      this(parent, style, tooltip, showFilterCloseButton, true, null);
   }

   public FilterText(Composite parent, int style, String tooltip, boolean showFilterCloseButton, boolean showFilterLabel)
   {
      this(parent, style, tooltip, showFilterCloseButton, showFilterLabel, null);
   }

   public FilterText(Composite parent, int style, String tooltip, boolean showFilterCloseButton, boolean showFilterLabel, IContentProposalProvider proposalProvider)
	{
		super(parent, style);		
		GridLayout layout = new GridLayout();
      layout.numColumns = showFilterLabel ? 5 : 4;
         
		setLayout(layout);
		
      if (showFilterLabel)
      {
         final Label label = new Label(this, SWT.NONE);
         label.setText(i18n.tr("Filter:"));
         GridData gd = new GridData();
         gd.verticalAlignment = SWT.CENTER;
         label.setLayoutData(gd);
      }

		textArea = new Composite(this, SWT.BORDER);
		GridLayout textLayout = new GridLayout();
		textLayout.numColumns = 2;
		textLayout.marginBottom = 0;
		textLayout.marginTop = 0;
		textLayout.marginLeft = 0;
      textLayout.marginRight = 0;
      textArea.setLayout(textLayout);
      GridData gd = new GridData();
      gd.verticalAlignment = SWT.CENTER;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.CENTER;
      textArea.setLayoutData(gd);

		text = new Text(textArea, SWT.SINGLE);
		text.setTextLimit(64);
      text.setMessage(i18n.tr("Filter is empty"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.CENTER;
		text.setLayoutData(gd);
		text.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(final ModifyEvent e)
         {
            lastModifyEvent = e;
            if (!autoApply)
               return;
            if (minLength > 1)
            {
               int l = text.getText().length();
               if ((l < minLength) && (l > 0))
                  return;
            }
            if (delay > 0)
            {
               getDisplay().timerExec(delay, new Runnable() {
                  @Override
                  public void run()
                  {
                     if (text.isDisposed())
                        return;
                     if (lastModifyEvent == e)
                        callModifyListeners(e);
                  }
               });
            }
            else
            {
               callModifyListeners(e);
            }

            if (text.getText().length() > 0)
            {
               textArea.setBackground(ThemeEngine.getBackgroundColor("MessageArea.Warning"));
               text.setBackground(textArea.getBackground());
            }
            else
            {
               textArea.setBackground(defaultBackground);
               text.setBackground(textArea.getBackground());               
            }
         }
      });
		text.addKeyListener(new KeyListener() {
         @Override
         public void keyReleased(KeyEvent e)
         {
            if (e.keyCode == '\r')
               callModifyListeners(lastModifyEvent);
         }

         @Override
         public void keyPressed(KeyEvent e)
         {
         }
      });

		if (tooltip != null) 
		{
         tooltipIcon = new Label(textArea, SWT.NONE);
         tooltipIcon.setImage(SharedIcons.IMG_INFORMATION);
         gd = new GridData();
         gd.verticalAlignment = SWT.CENTER;
         tooltipIcon.setLayoutData(gd);
         tooltipIcon.setToolTipText(tooltip);
         tooltipIcon.setBackground(text.getBackground());
		}

      defaultBackground = text.getBackground();
		textArea.setBackground(text.getBackground());

      enableAutoComplete(proposalProvider);

		buttonArea = new Composite(this, SWT.NONE);
		RowLayout buttonLayout = new RowLayout();
		buttonLayout.type = SWT.HORIZONTAL;
		buttonLayout.wrap = true;
		buttonLayout.marginBottom = 0;
		buttonLayout.marginTop = 0;
		buttonLayout.marginLeft = 0;
		buttonLayout.marginRight = 0;
		buttonLayout.spacing = WidgetHelper.OUTER_SPACING;
		buttonLayout.pack = false;
		buttonArea.setLayout(buttonLayout);
		gd = new GridData();
		gd.verticalAlignment = SWT.CENTER;
		buttonArea.setLayoutData(gd);
		
		clearButton = new Label(this, SWT.NONE);
		clearButton.setBackground(getBackground());
		clearButton.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
		clearButton.setImage(SharedIcons.IMG_CLEAR);
      clearButton.setToolTipText(i18n.tr("Clear"));
		gd = new GridData();
		gd.verticalAlignment = SWT.CENTER;
		clearButton.setLayoutData(gd);
		clearButton.addMouseListener(new MouseListener() {
		   private boolean doAction = false;
		   
		   @Override
		   public void mouseDoubleClick(MouseEvent e)
		   {
		      if (e.button == 1)
		         doAction = false;
		   }
		   
		   @Override
		   public void mouseDown(MouseEvent e)
		   {
		      if (e.button == 1)
		         doAction = true;
		   }
		   
		   @Override
		   public void mouseUp(MouseEvent e)
		   {
		      if ((e.button == 1) && doAction)
		         clearFilter();
		   }
		});		

		if (showFilterCloseButton)
		{
   		closeButton = new Label(this, SWT.NONE);
   		closeButton.setBackground(getBackground());
   		closeButton.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
   		closeButton.setImage(SharedIcons.IMG_CLOSE);
         closeButton.setToolTipText(i18n.tr("Close filter"));
   		gd = new GridData();
   		gd.verticalAlignment = SWT.CENTER;
   		closeButton.setLayoutData(gd);
   		closeButton.addMouseListener(new MouseListener() {
   			private boolean doAction = false;
   			
   			@Override
   			public void mouseDoubleClick(MouseEvent e)
   			{
   				if (e.button == 1)
   					doAction = false;
   			}
   
   			@Override
   			public void mouseDown(MouseEvent e)
   			{
   				if (e.button == 1)
   					doAction = true;
   			}
   
   			@Override
   			public void mouseUp(MouseEvent e)
   			{
   				if ((e.button == 1) && doAction)
   					closeFilter();
   			}
   		});
		}
	}

   /**
    * Enable text auto completion
    */
   public void enableAutoComplete(IContentProposalProvider proposalProvider)
   {
      if (proposalProvider == null)
         return;

      proposalAdapter = new ContentProposalAdapter(text, new TextContentAdapter(), proposalProvider, null, null);
      proposalAdapter.setPropagateKeys(true);
      proposalAdapter.setProposalAcceptanceStyle(ContentProposalAdapter.PROPOSAL_IGNORE);
      proposalAdapter.addContentProposalListener(new IContentProposalListener() {
         @Override
         public void proposalAccepted(IContentProposal proposal)
         {
            String content = text.getText();
            if (content.isEmpty())
            {
               text.append(proposal.getContent());
               return;
            }
            int pos = text.getCaretPosition() - 1;
            if (pos == -1)
               pos = 0;
            while((pos > 0) && !isStopCharacter(content.charAt(pos)))
               pos--;
            text.setSelection(isStopCharacter(content.charAt(pos)) ? pos + 1 : pos, text.getCaretPosition());
            text.insert(proposal.getContent());
         }
      });
   }

   /**
    * Check if given character is a stop character for autocomplete element search.
    *
    * @param ch character to test
    * @return true if given character is a stop character for autocomplete element search
    */
   private static boolean isStopCharacter(char ch)
   {
      return (ch == ':') || (ch == ',') || Character.isWhitespace(ch);
   }

	/**
	 * Clear filter
	 */
	private void clearFilter()
   {
      if (!text.getText().equals(""))
         text.setText("");
	   if (!autoApply)
	      callModifyListeners(lastModifyEvent);
   }

	/**
	 * Close filter widget
	 */
	private void closeFilter()
	{
		if (closeAction != null)
			closeAction.run();
		if (closeCallback != null)
		   closeCallback.run();
	}
	
	/**
	 * Set filtering attribute list
	 * 
	 * @param attributes
	 */
	public void setAttributeList(String[] attributes)
	{
		for(Button b : attrButtons)
		{
			b.dispose();
		}
		attrButtons.clear();
		
		for(String attr : attributes)
		{
			final Button b = new Button(buttonArea, SWT.TOGGLE);
			b.setText(attr);
			attrButtons.add(b);
			b.addSelectionListener(new SelectionListener () {
				@Override
				public void widgetSelected(SelectionEvent e)
				{
					onAttrButtonSelection(b);
				}

				@Override
				public void widgetDefaultSelected(SelectionEvent e)
				{
					widgetSelected(e);
				}
			});
		}
		
		layout(true, true);
	}
	
	/**
	 * Handler for attribute button selection
	 * 
	 * @param b button object
	 */
	private void onAttrButtonSelection(Button b)
	{
	}
	
	/**
	 * Add text modify listener
	 * 
	 * @param listener
	 */
	public void addModifyListener(ModifyListener listener)
	{
	   modifyListeners.add(listener);
	}
	
	/**
	 * Remove text modify listener
	 * 
	 * @param listener
	 */
	public void removeModifyListener(ModifyListener listener)
	{
	   modifyListeners.remove(listener);
	}
	
	/**
	 * @param e
	 */
	private void callModifyListeners(ModifyEvent e)
	{
	   for(ModifyListener l : modifyListeners)
	      l.modifyText(e);
	}
	
	/**
	 * Get current filter text
	 * 
	 * @return current filter text
	 */
	public String getText()
	{
		return text.getText();
	}
	
	/**
	 * Set filter text
	 * 
	 * @param value new filter text
	 */
	public void setText(String value)
	{
		text.setText(value);
	}

	/**
	 * @return the closeAction
	 */
	public Action getCloseAction()
	{
		return closeAction;
	}

	/**
	 * @param closeAction the closeAction to set
	 */
	public void setCloseAction(Action closeAction)
	{
		this.closeAction = closeAction;
	}

	/**
    * @param closeCallback the closeCallback to set
    */
   public void setCloseCallback(Runnable closeCallback)
   {
      this.closeCallback = closeCallback;
   }

   /**
    * @see org.eclipse.swt.widgets.Composite#setFocus()
    */
	@Override
	public boolean setFocus()
	{
	   if (text.isDisposed())
	      return false;
		return text.setFocus();
	}

   /**
    * @return the delay
    */
   public int getDelay()
   {
      return delay;
   }

   /**
    * @param delay the delay to set
    */
   public void setDelay(int delay)
   {
      this.delay = delay;
   }

   /**
    * @return the autoApply
    */
   public boolean isAutoApply()
   {
      return autoApply;
   }

   /**
    * @param autoApply the autoApply to set
    */
   public void setAutoApply(boolean autoApply)
   {
      this.autoApply = autoApply;
   }

   /**
    * @return the minLength
    */
   public int getMinLength()
   {
      return minLength;
   }

   /**
    * @param minLength the minLength to set
    */
   public void setMinLength(int minLength)
   {
      this.minLength = minLength;
   }

   /**
    * Set tooltip to be shown on "information" icon (if present).
    *
    * @param tooltip new information tooltip text
    */
   public void setTooltip(String tooltip)
   {
      if (tooltipIcon != null)
         tooltipIcon.setToolTipText(tooltip);
   }

   /**
    * @see org.eclipse.swt.widgets.Control#setBackground(org.eclipse.swt.graphics.Color)
    */
   @Override
   public void setBackground(Color color)
   {
      super.setBackground(color);
      if (closeButton != null)
         closeButton.setBackground(color);
      if (clearButton != null)
         clearButton.setBackground(color);
   }
}

/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.jface.action.Action;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Filter text widgets. Shows labelled text entry field, filtering 
 * attribute selection buttons, and close button.
 */
public class FilterText extends Composite
{
	private Text text;
	private Composite buttonArea;
	private List<Button> attrButtons = new ArrayList<Button>(4);
	private Label closeButton;
	private Action closeAction = null;
	private int delay = 300;
	private Set<ModifyListener> modifyListeners = new HashSet<ModifyListener>();
	private ModifyEvent lastModifyEvent = null;
	
	/**
	 * @param parent
	 * @param style
	 */
	public FilterText(Composite parent, int style)
	{
		super(parent, style);
		GridLayout layout = new GridLayout();
		layout.numColumns = 4;
		setLayout(layout);
		
		final Label label = new Label(this, SWT.NONE);
		label.setText(Messages.get().FilterText_Filter);
		GridData gd = new GridData();
		gd.verticalAlignment = SWT.CENTER;
		label.setLayoutData(gd);
		
		text = new Text(this, SWT.BORDER);
		text.setTextLimit(64);
		text.setMessage(Messages.get().FilterText_FilterIsEmpty);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.CENTER;
		text.setLayoutData(gd);
		text.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(final ModifyEvent e)
         {
            if (delay > 0)
            {
               lastModifyEvent = e;
               getDisplay().timerExec(delay, new Runnable() {
                  @Override
                  public void run()
                  {
                     if (lastModifyEvent == e)
                        callModifyListeners(e);
                  }
               });
            }
            else
            {
               callModifyListeners(e);
            }
         }
      });
		
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
		gd.widthHint = 1;
		gd.heightHint = 1;
		buttonArea.setLayoutData(gd);
		
		closeButton = new Label(this, SWT.NONE);
		closeButton.setBackground(getBackground());
		closeButton.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
		closeButton.setImage(SharedIcons.IMG_CLOSE);
		closeButton.setToolTipText(Messages.get().FilterText_CloseFilter);
		gd = new GridData();
		gd.verticalAlignment = SWT.CENTER;
		gd.horizontalAlignment = SWT.CENTER;
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
	
	/**
	 * Close filter widget
	 */
	private void closeFilter()
	{
		if (closeAction != null)
			closeAction.run();
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
		
		((GridData)buttonArea.getLayoutData()).widthHint = (attrButtons.size() > 0) ? SWT.DEFAULT : 1;
		((GridData)buttonArea.getLayoutData()).heightHint = (attrButtons.size() > 0) ? SWT.DEFAULT : 1;
		
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

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#setFocus()
	 */
	@Override
	public boolean setFocus()
	{
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
}

/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.nxsl.widgets;

import java.util.Collection;
import java.util.Set;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.MouseAdapter;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.console.resources.SharedIcons;

/**
 * NXSL script editor
 *
 */
public class ScriptEditor extends Composite
{
	private Text editor;
	private String hintText;
	private Composite hintArea;
	private Text hintTextControl = null;
	private Label hintsExpandButton = null;
	
   /**
    * @param parent
    * @param style
    * @param editorStyle
    */
   public ScriptEditor(Composite parent, int style, int editorStyle)
   {
      this(parent, style, editorStyle, true, null);
   }

   /**
    * @param parent
    * @param style
    * @param editorStyle
    * @param showLineNumbers
    */
   public ScriptEditor(Composite parent, int style, int editorStyle, boolean showLineNumbers)
   {
      this(parent, style, editorStyle, showLineNumbers, null);
   }
   
	/**
	 * @param parent
	 * @param style
	 * @param editorStyle
	 * @param showLineNumbers
	 * @param hints
	 */
	public ScriptEditor(Composite parent, int style, int editorStyle, boolean showLineNumbers, String hints)
	{
		super(parent, style);
		
      hintText = hints;

      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = 0;
      setLayout(layout);
      
      if (hints != null)
      {
         createHintsArea();
      }
		
		editor = new Text(this, editorStyle);
		editor.setData(RWT.CUSTOM_VARIANT, "monospace");
		editor.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
	}
	
   /**
    * Create hints area
    */
   private void createHintsArea()
   {
      hintArea = new Composite(this, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = 0;
      layout.numColumns = 2;
      hintArea.setLayout(layout);
      hintArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
      hintArea.setBackground(SharedColors.getColor(SharedColors.MESSAGE_BAR_BACKGROUND, getDisplay()));
      
      CLabel hintsTitle = new CLabel(hintArea, SWT.NONE);
      hintsTitle.setBackground(SharedColors.getColor(SharedColors.MESSAGE_BAR_BACKGROUND, getDisplay()));
      hintsTitle.setForeground(SharedColors.getColor(SharedColors.MESSAGE_BAR_TEXT, getDisplay()));
      hintsTitle.setImage(SharedIcons.IMG_INFORMATION);
      hintsTitle.setText("Hints");
      hintsTitle.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
      hintsTitle.addMouseListener(new MouseAdapter() {
         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
            if (e.button == 1)
               toggleHints();
         }
      });
      
      hintsExpandButton = new Label(hintArea, SWT.NONE);
      hintsExpandButton.setBackground(hintArea.getBackground());
      hintsExpandButton.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
      hintsExpandButton.setImage(SharedIcons.IMG_EXPAND);
      hintsExpandButton.setToolTipText("Hide message");
      GridData gd = new GridData();
      gd.verticalAlignment = SWT.CENTER;
      hintsExpandButton.setLayoutData(gd);
      hintsExpandButton.addMouseListener(new MouseListener() {
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
               toggleHints();
         }
      });
      
      Label separator = new Label(this, SWT.SEPARATOR | SWT.HORIZONTAL);
      separator.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
   }
	
	/**
	 * Get underlying text widget
	 * @return text widget
	 */
	public Text getTextWidget()
	{
		return editor;
	}
	
	/**
	 * Set text for editing
	 * @param text
	 */
	public void setText(String text)
	{
		editor.setText((text != null) ? text : "");
	}
	
	/**
	 * Get editor's content
	 * @return
	 */
	public String getText()
	{
		return editor.getText();
	}

	/**
	 * @param functions the functions to set
	 */
	public void setFunctions(Set<String> functions)
	{
	}
	
	/**
	 * Add functions
	 * 
	 * @param fc
	 */
	public void addFunctions(Collection<String> fc)
	{
	}

	/**
	 * @param variables the variables to set
	 */
	public void setVariables(Set<String> variables)
	{
	}

	/**
	 * Add variables
	 * 
	 * @param vc
	 */
	public void addVariables(Collection<String> vc)
	{
	}

	/**
	 * @return the functionsCache
	 */
	public String[] getFunctions()
	{
		return new String[0];
	}

	/**
	 * @return the variablesCache
	 */
	public String[] getVariables()
	{
		return new String[0];
	}

	/**
	 * Get icon for given autocompletion proposal type. Proposal types defined in NXSLProposalProcessor.
	 * 
	 * @param type proposal type
	 * @return image for given proposal type or null
	 */
	public Image getProposalIcon(int type)
	{
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
	 */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		Point p = editor.computeSize(wHint, hHint, changed);
		p.y += 4;
		return p;
	}
   
   /**
    * Toggle hints area
    */
   private void toggleHints()
   {
      if (hintTextControl != null)
      {
         hintTextControl.dispose();
         hintTextControl = null;
         hintsExpandButton.setImage(SharedIcons.IMG_EXPAND);
      }
      else
      {
         hintTextControl = new Text(hintArea, SWT.MULTI | SWT.WRAP);
         hintTextControl.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false, 2, 1));
         hintTextControl.setEditable(false);
         hintTextControl.setText(hintText);
         hintTextControl.setBackground(SharedColors.getColor(SharedColors.MESSAGE_BAR_BACKGROUND, getDisplay()));
         hintTextControl.setForeground(SharedColors.getColor(SharedColors.MESSAGE_BAR_TEXT, getDisplay()));
         hintsExpandButton.setImage(SharedIcons.IMG_COLLAPSE);
      }
      layout(true, true);
   }
}

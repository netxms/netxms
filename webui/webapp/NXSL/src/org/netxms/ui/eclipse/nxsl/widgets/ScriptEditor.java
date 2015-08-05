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
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Text;

/**
 * NXSL script editor
 *
 */
public class ScriptEditor extends Composite
{
	private Text editor;

   /**
    * @param parent
    * @param style
    * @param editorStyle
    */
   public ScriptEditor(Composite parent, int style, int editorStyle)
   {
      this(parent, style, editorStyle, true);
   }
   
	/**
	 * @param parent
	 * @param style
	 * @param editorStyle
	 * @param showLineNumbers
	 */
	public ScriptEditor(Composite parent, int style, int editorStyle, boolean showLineNumbers)
	{
		super(parent, style);
		
		setLayout(new FillLayout());
		
		editor = new Text(this, editorStyle);
		editor.setData(RWT.CUSTOM_VARIANT, "monospace");
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
}

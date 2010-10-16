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
package org.netxms.ui.eclipse.nxsl.widgets;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.jface.text.source.ISourceViewer;
import org.eclipse.jface.text.source.SourceViewer;
import org.eclipse.jface.text.source.VerticalRuler;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.StyledText;
import org.eclipse.swt.custom.VerifyKeyListener;
import org.eclipse.swt.events.VerifyEvent;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.nxsl.widgets.internal.NXSLDocument;
import org.netxms.ui.eclipse.nxsl.widgets.internal.NXSLSourceViewerConfiguration;

/**
 * NXSL script editor
 *
 */
public class ScriptEditor extends Composite
{
	private SourceViewer editor;
	private Font editorFont;
	private boolean modified;
	private List<String> functions = new ArrayList<String>(0);
	private List<String> variables = new ArrayList<String>(0);

	/**
	 * @param parent
	 * @param style
	 */
	public ScriptEditor(Composite parent, int style)
	{
		super(parent, style);
		
		editorFont = new Font(getShell().getDisplay(), "Courier New", 10, SWT.NORMAL);
		
		setLayout(new FillLayout());
		editor = new SourceViewer(this, new VerticalRuler(10), SWT.NONE);
		editor.configure(new NXSLSourceViewerConfiguration());
		editor.prependVerifyKeyListener(new VerifyKeyListener() {
			@Override
			public void verifyKey(VerifyEvent event)
			{
				if (event.doit && (event.stateMask == SWT.MOD1) && event.character == ' ')
				{
					editor.doOperation(ISourceViewer.CONTENTASSIST_PROPOSALS);
					event.doit = false;
				}
			}
		});

		StyledText control = editor.getTextWidget();
		control.setFont(editorFont);
		control.setWordWrap(false);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Widget#dispose()
	 */
	@Override
	public void dispose()
	{
		editorFont.dispose();
		super.dispose();
	}
	
	/**
	 * Get underlying text widget
	 * @return text widget
	 */
	public StyledText getTextWidget()
	{
		return editor.getTextWidget();
	}
	
	/**
	 * Set text for editing
	 * @param text
	 */
	public void setText(String text)
	{
		editor.setDocument(new NXSLDocument(text));
	}
	
	/**
	 * Get editor's content
	 * @return
	 */
	public String getText()
	{
		return editor.getDocument().get();
	}

	/**
	 * @return the modified
	 */
	public boolean isModified()
	{
		return modified;
	}

	/**
	 * @param modified the modified to set
	 */
	public void setModified(boolean modified)
	{
		this.modified = modified;
	}

	/**
	 * @return the functions
	 */
	public List<String> getFunctions()
	{
		return functions;
	}

	/**
	 * @param functions the functions to set
	 */
	public void setFunctions(List<String> functions)
	{
		this.functions = functions;
	}

	/**
	 * @return the variables
	 */
	public List<String> getVariables()
	{
		return variables;
	}

	/**
	 * @param variables the variables to set
	 */
	public void setVariables(List<String> variables)
	{
		this.variables = variables;
	}
}

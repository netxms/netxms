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

import java.util.Collection;
import java.util.HashSet;
import java.util.Set;

import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.text.IFindReplaceTarget;
import org.eclipse.jface.text.ITextOperationTarget;
import org.eclipse.jface.text.TextViewerUndoManager;
import org.eclipse.jface.text.source.ISourceViewer;
import org.eclipse.jface.text.source.SourceViewer;
import org.eclipse.jface.text.source.VerticalRuler;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.StyledText;
import org.eclipse.swt.custom.VerifyKeyListener;
import org.eclipse.swt.events.VerifyEvent;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.nxsl.Activator;
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
	private Set<String> functions = new HashSet<String>(0);
	private Set<String> variables = new HashSet<String>(0);
	private String[] functionsCache = new String[0];
	private String[] variablesCache = new String[0];
	private Image[] proposalIcons = new Image[4];

   /**
    * @param parent
    * @param style
    * @param editorStyle
    */
   public ScriptEditor(Composite parent, int style, int editorStyle)
   {
      this(parent, style, editorStyle, 20);
   }
   
	/**
	 * @param parent
	 * @param style
	 * @param editorStyle
	 * @param rulerWidth
	 */
	public ScriptEditor(Composite parent, int style, int editorStyle, int rulerWidth)
	{
		super(parent, style);
		
		//editorFont = new Font(getShell().getDisplay(), "Courier New", 10, SWT.NORMAL);
		editorFont = JFaceResources.getTextFont();
		
		proposalIcons[0] = Activator.getImageDescriptor("icons/function.gif").createImage(); //$NON-NLS-1$
		proposalIcons[1] = Activator.getImageDescriptor("icons/var_global.gif").createImage(); //$NON-NLS-1$
		proposalIcons[2] = Activator.getImageDescriptor("icons/var_local.gif").createImage(); //$NON-NLS-1$
		proposalIcons[3] = Activator.getImageDescriptor("icons/constant.gif").createImage(); //$NON-NLS-1$
		
		setLayout(new FillLayout());
		editor = new SourceViewer(this, (rulerWidth > 0) ? new VerticalRuler(rulerWidth) : null, editorStyle);
		editor.configure(new NXSLSourceViewerConfiguration(this));

		final TextViewerUndoManager undoManager = new TextViewerUndoManager(50);
		editor.setUndoManager(undoManager);
		undoManager.connect(editor);
		
		editor.getFindReplaceTarget();

		editor.prependVerifyKeyListener(new VerifyKeyListener() {
			@Override
			public void verifyKey(VerifyEvent event)
			{
				if (!event.doit)
					return;
				
				if (event.stateMask == SWT.MOD1)
				{
					switch(event.character)
					{
						case ' ':
							editor.doOperation(ISourceViewer.CONTENTASSIST_PROPOSALS);
							event.doit = false;
							break;
						case 0x19:	// Ctrl+Y
							undoManager.redo();
							event.doit = false;
							break;
						case 0x1A:	// Ctrl+Z
							undoManager.undo();
							event.doit = false;
							break;
					}
				}
				else if (event.stateMask == SWT.NONE)
				{
					if (event.character == 0x09)
					{
						if (editor.getSelectedRange().y > 0)
						{
							editor.doOperation(ITextOperationTarget.SHIFT_RIGHT);
							event.doit = false;
						}
					}
				}
				else if (event.stateMask == SWT.SHIFT)
				{
					if (event.character == 0x09)
					{
						if (editor.getSelectedRange().y > 0)
						{
							editor.doOperation(ITextOperationTarget.SHIFT_LEFT);
							event.doit = false;
						}
					}
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
		for(int i = 0; i < proposalIcons.length; i++)
			proposalIcons[i].dispose();
		//editorFont.dispose();
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
	 * @param functions the functions to set
	 */
	public void setFunctions(Set<String> functions)
	{
		this.functions = functions;
		functionsCache = functions.toArray(new String[functions.size()]);
	}
	
	/**
	 * Add functions
	 * 
	 * @param fc
	 */
	public void addFunctions(Collection<String> fc)
	{
		functions.addAll(fc);
		functionsCache = functions.toArray(new String[functions.size()]);
	}

	/**
	 * @param variables the variables to set
	 */
	public void setVariables(Set<String> variables)
	{
		this.variables = variables;
		variablesCache = variables.toArray(new String[variables.size()]);
	}

	/**
	 * Add variables
	 * 
	 * @param vc
	 */
	public void addVariables(Collection<String> vc)
	{
		variables.addAll(vc);
		variablesCache = variables.toArray(new String[variables.size()]);
	}

	/**
	 * @return the functionsCache
	 */
	public String[] getFunctions()
	{
		return functionsCache;
	}

	/**
	 * @return the variablesCache
	 */
	public String[] getVariables()
	{
		return variablesCache;
	}

	/**
	 * Get icon for given autocompletion proposal type. Proposal types defined in NXSLProposalProcessor.
	 * 
	 * @param type proposal type
	 * @return image for given proposal type or null
	 */
	public Image getProposalIcon(int type)
	{
		try
		{
			return proposalIcons[type];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return null;
		}
	}

	/**
	 * Returns the find/replace target of underlying source viewer
	 * @return the find/replace target of underlying source viewer
	 */
	public IFindReplaceTarget getFindReplaceTarget()
	{
		return editor.getFindReplaceTarget();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
	 */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		Point p = editor.getTextWidget().computeSize(wHint, hHint, changed);
		p.y += 4;
		return p;
	}
}

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

import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.text.ITextOperationTarget;
import org.eclipse.jface.text.TextViewerUndoManager;
import org.eclipse.jface.text.source.ISourceViewer;
import org.eclipse.jface.text.source.SourceViewer;
import org.eclipse.jface.text.source.VerticalRuler;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.StyledText;
import org.eclipse.swt.custom.VerifyKeyListener;
import org.eclipse.swt.events.VerifyEvent;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.widgets.internal.AgentConfigDocument;
import org.netxms.ui.eclipse.widgets.internal.AgentConfigSourceViewerConfiguration;

/**
 * Widget for editing agent's config
 */
public class AgentConfigEditor extends Composite
{
	private SourceViewer editor;

	/**
	 * @param parent
	 * @param style
	 * @param editorStyle
	 */
   public AgentConfigEditor(Composite parent, int style, int editorStyle)
   {
      this(parent, style, editorStyle, 20);
   }
   
	/**
	 * @param parent
	 * @param style
	 * @param editorStyle
	 * @param rulerWidth
	 */
	public AgentConfigEditor(Composite parent, int style, int editorStyle, int rulerWidth)
	{
		super(parent, style);
		
		setLayout(new FillLayout());
		editor = new SourceViewer(this, (rulerWidth > 0) ? new VerticalRuler(rulerWidth) : null, editorStyle);
		editor.configure(new AgentConfigSourceViewerConfiguration());

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
		control.setFont(JFaceResources.getTextFont());
		control.setWordWrap(false);
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
		editor.setDocument(new AgentConfigDocument(text));
	}
	
	/**
	 * Get editor's content
	 * @return
	 */
	public String getText()
	{
		return editor.getDocument().get();
	}
}

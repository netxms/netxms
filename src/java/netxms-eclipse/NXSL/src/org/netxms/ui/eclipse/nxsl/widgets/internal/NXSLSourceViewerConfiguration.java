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
package org.netxms.ui.eclipse.nxsl.widgets.internal;

import org.eclipse.jface.text.IDocument;
import org.eclipse.jface.text.contentassist.ContentAssistant;
import org.eclipse.jface.text.contentassist.IContentAssistant;
import org.eclipse.jface.text.presentation.IPresentationReconciler;
import org.eclipse.jface.text.presentation.PresentationReconciler;
import org.eclipse.jface.text.rules.DefaultDamagerRepairer;
import org.eclipse.jface.text.rules.IRule;
import org.eclipse.jface.text.rules.IWordDetector;
import org.eclipse.jface.text.rules.PatternRule;
import org.eclipse.jface.text.rules.RuleBasedScanner;
import org.eclipse.jface.text.source.ISourceViewer;
import org.eclipse.jface.text.source.SourceViewerConfiguration;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;

/**
 * Source viewer configuration for NXSL
 *
 */
public class NXSLSourceViewerConfiguration extends SourceViewerConfiguration
{
	private ScriptEditor scriptEditor;
	
	private static final IWordDetector nxslWordDetector = new IWordDetector() {
		@Override
		public boolean isWordPart(char c)
		{
			return Character.isLetterOrDigit(c) || (c == '$') || (c == '_');
		}

		@Override
		public boolean isWordStart(char c)
		{
			return Character.isLetter(c) || (c == '$') || (c == '_');
		}
	};
	
	private static final String[] nxslKeywords = { 
		"array", "break", "case", "const", "classof", "continue", "default", "do", 
		"else", "exit", "FALSE", "false", "for", "foreach", "global", "if", "ilike",
      "imatch", "int32", "int64", "like", "match", "NULL", "null", "print", "println",
      "real", "return", "string", "sub", "switch", "TRUE", "true", "typeof", "uint32",
      "uint64", "use", "while" };
	
	private static final IRule[] codeRules = { 
		new KeywordRule(nxslWordDetector, NXSLTextAttributeProvider.getTextAttributeToken(NXSLTextAttributeProvider.KEYWORD),
				NXSLTextAttributeProvider.getTextAttributeToken(NXSLTextAttributeProvider.DEFAULT), nxslKeywords)
	};
	
	/**
	 * Creates source viewer configuration for NXSL
	 */
	public NXSLSourceViewerConfiguration(ScriptEditor scriptEditor)
	{
		super();
		this.scriptEditor = scriptEditor;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.text.source.SourceViewerConfiguration#getContentAssistant(org.eclipse.jface.text.source.ISourceViewer)
	 */
	@Override
	public IContentAssistant getContentAssistant(ISourceViewer sourceViewer)
	{
		NXSLProposalProcessor processor = new NXSLProposalProcessor(scriptEditor);
		
		ContentAssistant ca = new ContentAssistant();
		ca.enableAutoActivation(true);
		ca.setContentAssistProcessor(processor, IDocument.DEFAULT_CONTENT_TYPE);
		ca.setInformationControlCreator(getInformationControlCreator(sourceViewer));
		return ca;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.text.source.SourceViewerConfiguration#getTabWidth(org.eclipse.jface.text.source.ISourceViewer)
	 */
	@Override
	public int getTabWidth(ISourceViewer sourceViewer)
	{
		return 3;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.text.source.SourceViewerConfiguration#getConfiguredContentTypes(org.eclipse.jface.text.source.ISourceViewer)
	 */
	@Override
	public String[] getConfiguredContentTypes(ISourceViewer sourceViewer)
	{
		return NXSLDocument.NXSL_CONTENT_TYPES;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.text.source.SourceViewerConfiguration#getPresentationReconciler(org.eclipse.jface.text.source.ISourceViewer)
	 */
	@Override
	public IPresentationReconciler getPresentationReconciler(ISourceViewer sourceViewer)
	{
		PresentationReconciler reconciler = new PresentationReconciler();

		RuleBasedScanner scanner = new RuleBasedScanner();
		scanner.setRules(codeRules);
		scanner.setDefaultReturnToken(NXSLTextAttributeProvider.getTextAttributeToken(NXSLTextAttributeProvider.DEFAULT));
		DefaultDamagerRepairer dr = new DefaultDamagerRepairer(scanner);
		reconciler.setDamager(dr, IDocument.DEFAULT_CONTENT_TYPE);
		reconciler.setRepairer(dr, IDocument.DEFAULT_CONTENT_TYPE);
	
		dr = new DefaultDamagerRepairer(new SingleTokenScanner(NXSLTextAttributeProvider.getTextAttribute(NXSLTextAttributeProvider.COMMENT)));
		reconciler.setDamager(dr, NXSLDocument.CONTENT_COMMENTS);
		reconciler.setRepairer(dr, NXSLDocument.CONTENT_COMMENTS);
	
		scanner = new RuleBasedScanner();
		scanner.setRules(new IRule[] { new PatternRule("\"", "\n", NXSLTextAttributeProvider.getTextAttributeToken(NXSLTextAttributeProvider.ERROR), (char)0, false) });
		scanner.setDefaultReturnToken(NXSLTextAttributeProvider.getTextAttributeToken(NXSLTextAttributeProvider.STRING));
		dr = new DefaultDamagerRepairer(scanner);
		reconciler.setDamager(dr, NXSLDocument.CONTENT_STRING);
		reconciler.setRepairer(dr, NXSLDocument.CONTENT_STRING);
	
		return reconciler;
	}
}

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
package org.netxms.ui.eclipse.widgets.internal;

import org.eclipse.jface.text.IDocument;
import org.eclipse.jface.text.presentation.IPresentationReconciler;
import org.eclipse.jface.text.presentation.PresentationReconciler;
import org.eclipse.jface.text.rules.DefaultDamagerRepairer;
import org.eclipse.jface.text.rules.IRule;
import org.eclipse.jface.text.rules.IWordDetector;
import org.eclipse.jface.text.rules.PatternRule;
import org.eclipse.jface.text.rules.RuleBasedScanner;
import org.eclipse.jface.text.source.ISourceViewer;
import org.eclipse.jface.text.source.SourceViewerConfiguration;

/**
 * Source viewer configuration for agent config
 */
public class AgentConfigSourceViewerConfiguration extends SourceViewerConfiguration
{
	private static final IWordDetector configWordDetector = new IWordDetector() {
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
	
	private static final String[] configKeywords = { 
		"Action",  //$NON-NLS-1$
		"ActionShellExec",  //$NON-NLS-1$
      "AppAgent",  //$NON-NLS-1$
      "BackgroundLogWriter",  //$NON-NLS-1$
		"CodePage",  //$NON-NLS-1$
		"ConfigIncludeDir",  //$NON-NLS-1$
		"ControlServers",  //$NON-NLS-1$
		"CreateCrashDumps",  //$NON-NLS-1$
      "DataCollectionThreadPoolSize",  //$NON-NLS-1$
      "DataDirectory",  //$NON-NLS-1$
      "DataReconciliationBlockSize",  //$NON-NLS-1$
      "DataReconciliationTimeout",  //$NON-NLS-1$
      "DailyLogFileSuffix",  //$NON-NLS-1$
      "DebugLevel",  //$NON-NLS-1$
      "DisableIPv4",  //$NON-NLS-1$
      "DisableIPv6",  //$NON-NLS-1$
		"DumpDirectory",  //$NON-NLS-1$
		"EnableActions",  //$NON-NLS-1$
		"EnabledCiphers", //$NON-NLS-1$
		"EnableProxy", //$NON-NLS-1$
		"EnableSNMPProxy", //$NON-NLS-1$
		"EnableSubagentAutoload", //$NON-NLS-1$
		"EnableWatchdog", //$NON-NLS-1$
		"ExecTimeout", //$NON-NLS-1$
      "ExternalList", //$NON-NLS-1$
      "ExternalMasterAgent", //$NON-NLS-1$
		"ExternalParameter", //$NON-NLS-1$
		"ExternalParameterShellExec", //$NON-NLS-1$
      "ExternalSubAgent", //$NON-NLS-1$
		"FileStore", //$NON-NLS-1$
      "GroupId", //$NON-NLS-1$
		"ListenAddress", //$NON-NLS-1$
		"ListenPort", //$NON-NLS-1$
		"LogFile", //$NON-NLS-1$
		"LogHistorySize", //$NON-NLS-1$
		"LogRotationMode",  //$NON-NLS-1$
		"LogUnresolvedSymbols", //$NON-NLS-1$
      "LongRunningQueryThreshold", //$NON-NLS-1$
		"MasterServers", //$NON-NLS-1$
		"MaxLogSize", //$NON-NLS-1$
		"MaxSessions", //$NON-NLS-1$
		"PlatformSuffix", //$NON-NLS-1$
		"RequireAuthentication", //$NON-NLS-1$
		"RequireEncryption", //$NON-NLS-1$
		"Servers", //$NON-NLS-1$
		"SessionIdleTimeout", //$NON-NLS-1$
		"SharedSecret", //$NON-NLS-1$
		"SNMPTimeout", //$NON-NLS-1$
      "SNMPTrapListenAddress", //$NON-NLS-1$
      "SNMPTrapPort", //$NON-NLS-1$
		"StartupDelay", //$NON-NLS-1$
		"SubAgent", //$NON-NLS-1$
      "SyslogListenPort", //$NON-NLS-1$
      "UserId", //$NON-NLS-1$
		"WaitForProcess", //$NON-NLS-1$
      "ZoneId" //$NON-NLS-1$
	};
	
	private static final IRule[] codeRules = { 
		new KeywordRule(configWordDetector, AgentConfigTextAttributeProvider.getTextAttributeToken(AgentConfigTextAttributeProvider.KEYWORD),
				AgentConfigTextAttributeProvider.getTextAttributeToken(AgentConfigTextAttributeProvider.DEFAULT), configKeywords)
	};

	/**
	 * Creates source viewer configuration for agent config
	 */
	public AgentConfigSourceViewerConfiguration()
	{
		super();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.text.source.SourceViewerConfiguration#getTabWidth(org.eclipse.jface.text.source.ISourceViewer)
	 */
	@Override
	public int getTabWidth(ISourceViewer sourceViewer)
	{
		return 8;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.text.source.SourceViewerConfiguration#getConfiguredContentTypes(org.eclipse.jface.text.source.ISourceViewer)
	 */
	@Override
	public String[] getConfiguredContentTypes(ISourceViewer sourceViewer)
	{
		return AgentConfigDocument.CONTENT_TYPES;
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
		scanner.setDefaultReturnToken(AgentConfigTextAttributeProvider.getTextAttributeToken(AgentConfigTextAttributeProvider.DEFAULT));
		DefaultDamagerRepairer dr = new DefaultDamagerRepairer(scanner);
		reconciler.setDamager(dr, IDocument.DEFAULT_CONTENT_TYPE);
		reconciler.setRepairer(dr, IDocument.DEFAULT_CONTENT_TYPE);
	
		dr = new DefaultDamagerRepairer(new SingleTokenScanner(AgentConfigTextAttributeProvider.getTextAttribute(AgentConfigTextAttributeProvider.COMMENT)));
		reconciler.setDamager(dr, AgentConfigDocument.CONTENT_COMMENTS);
		reconciler.setRepairer(dr, AgentConfigDocument.CONTENT_COMMENTS);
	
		dr = new DefaultDamagerRepairer(new SingleTokenScanner(AgentConfigTextAttributeProvider.getTextAttribute(AgentConfigTextAttributeProvider.SECTION)));
		reconciler.setDamager(dr, AgentConfigDocument.CONTENT_SECTION);
		reconciler.setRepairer(dr, AgentConfigDocument.CONTENT_SECTION);
	
		scanner = new RuleBasedScanner();
		scanner.setRules(new IRule[] { new PatternRule("\"", "\n", AgentConfigTextAttributeProvider.getTextAttributeToken(AgentConfigTextAttributeProvider.ERROR), (char)0, false) }); //$NON-NLS-1$ //$NON-NLS-2$
		scanner.setDefaultReturnToken(AgentConfigTextAttributeProvider.getTextAttributeToken(AgentConfigTextAttributeProvider.STRING));
		dr = new DefaultDamagerRepairer(scanner);
		reconciler.setDamager(dr, AgentConfigDocument.CONTENT_STRING);
		reconciler.setRepairer(dr, AgentConfigDocument.CONTENT_STRING);
	
		return reconciler;
	}
}

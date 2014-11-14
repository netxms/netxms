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

import java.util.ArrayList;
import java.util.List;

import org.eclipse.jface.text.ITextViewer;
import org.eclipse.jface.text.contentassist.CompletionProposal;
import org.eclipse.jface.text.contentassist.ICompletionProposal;
import org.eclipse.jface.text.contentassist.IContentAssistProcessor;
import org.eclipse.jface.text.contentassist.IContextInformation;
import org.eclipse.jface.text.contentassist.IContextInformationValidator;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;

/**
 * NXSL proposal processor
 *
 */
public class NXSLProposalProcessor implements IContentAssistProcessor
{
	private static final int FUNCTION = 0;
	private static final int GLOBAL_VARIABLE = 1;
	//private static final int LOCAL_VARIABLE = 2;
	private static final int CONSTANT = 3;
	
	private static final String[] BUILTIN_SYSTEM_VARIABLES = { "$1", "$2", "$3", "$4", "$5", "$6", "$7", "$8", "$9" };
	private static final String[] BUILTIN_FUNCTIONS = { "abs", "ceil", "classof", "d2x", "exit", "exp", "floor",
		"format", "gmtime", "index", "left", "length", "localtime", "log", "log10", "lower", "ltrim", "map", "max", "min",
		"pow", "right", "rindex", "round", "rtrim", "sleep", "strftime", "substr", "time", "trace", "trim", "typeof", "upper",
		"AddrInRange", "AddrInSubnet", "BindObject", "CreateContainer", "CreateSNMPTransport", "FindDCIByDescription",
		"FindDCIByName", "FindNodeObject", "FindObject", "FindSituation", "GetAvgDCIValue", "GetConfigurationVariable",
		"GetCustomAttribute", "GetDCIObject", "GetDCIValue", "GetDCIValueByDescription", "GetDCIValueByName", "GetEventParameter",
		"GetInterfaceName", "GetInterfaceObject", "GetMaxDCIValue", "GetMinDCIValue", "GetNodeInterfaces", "GetNodeParents", "GetNodeTemplates",
		"GetObjectChildren", "GetObjectParents", "GetSituationAttribute", "PostEvent", "RemoveContainer", "RenameObject",
		"SetCustomAttribute", "SetEventParameter", "SecondsToUptime", "SNMPGet", "SNMPGetValue", "SNMPSet", "SNMPWalk", "UnbindObject" };
	private static final String[] BUILTIN_CONSTANTS = { "null", "true", "false" };
	
	private ScriptEditor scriptEditor;
	
	/**
	 * Create proposal processor for NXSL
	 * 
	 * @param scriptEditor script editor this proposal processor associated with
	 */
	public NXSLProposalProcessor(ScriptEditor scriptEditor)
	{
		super();
		this.scriptEditor = scriptEditor;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.text.contentassist.IContentAssistProcessor#computeCompletionProposals(org.eclipse.jface.text.ITextViewer, int)
	 */
	@Override
	public ICompletionProposal[] computeCompletionProposals(ITextViewer viewer, int offset)
	{
		String content = viewer.getTextWidget().getText(Math.max(0, offset - 64), offset - 1);
		StringBuilder sb = new StringBuilder();
		for(int i = content.length() - 1; i >= 0; i--)
		{
			char ch = content.charAt(i);
			if (!Character.isJavaIdentifierPart(ch))
				break;
			sb.insert(0, ch);
		}
		String prefix = sb.toString();
		
		ArrayList<ICompletionProposal> list = new ArrayList<ICompletionProposal>();
		addProposals(list, prefix, offset, GLOBAL_VARIABLE, BUILTIN_SYSTEM_VARIABLES);
		addProposals(list, prefix, offset, FUNCTION, BUILTIN_FUNCTIONS);
		addProposals(list, prefix, offset, CONSTANT, BUILTIN_CONSTANTS);
		addProposals(list, prefix, offset, GLOBAL_VARIABLE, scriptEditor.getVariables());
		addProposals(list, prefix, offset, FUNCTION, scriptEditor.getFunctions());
		
		return list.toArray(new ICompletionProposal[list.size()]);
	}

	/**
	 * Add list of proposals
	 * 
	 * @param props proposals list to be populated
	 * @param type proposals type
	 * @param texts proposals texts
	 */
	private void addProposals(List<ICompletionProposal> props, String prefix, int offset, int type, String[] texts)
	{
		for(final String s : texts)
		{
			if (s.startsWith(prefix))
				props.add(new CompletionProposal(s, offset - prefix.length(), prefix.length(),
						s.length(), scriptEditor.getProposalIcon(type), s, null, null));
		}
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.text.contentassist.IContentAssistProcessor#computeContextInformation(org.eclipse.jface.text.ITextViewer, int)
	 */
	@Override
	public IContextInformation[] computeContextInformation(ITextViewer viewer, int offset)
	{
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.text.contentassist.IContentAssistProcessor#getCompletionProposalAutoActivationCharacters()
	 */
	@Override
	public char[] getCompletionProposalAutoActivationCharacters()
	{
		return new char[] { '$' };
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.text.contentassist.IContentAssistProcessor#getContextInformationAutoActivationCharacters()
	 */
	@Override
	public char[] getContextInformationAutoActivationCharacters()
	{
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.text.contentassist.IContentAssistProcessor#getContextInformationValidator()
	 */
	@Override
	public IContextInformationValidator getContextInformationValidator()
	{
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.text.contentassist.IContentAssistProcessor#getErrorMessage()
	 */
	@Override
	public String getErrorMessage()
	{
		return "No completion proposals";
	}
}

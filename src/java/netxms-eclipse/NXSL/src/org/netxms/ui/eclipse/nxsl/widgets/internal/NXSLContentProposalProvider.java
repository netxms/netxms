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
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import org.eclipse.jface.fieldassist.IContentProposal;
import org.eclipse.jface.fieldassist.IContentProposalProvider;
import org.netxms.ui.eclipse.nxsl.tools.NXSLLineStyleListener;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;

/**
 * Content proposal provider for NXSL editor
 *
 */
public class NXSLContentProposalProvider implements IContentProposalProvider
{
	private static final String[] BUILTIN_SYSTEM_VARIABLES = { "$1", "$2", "$3", "$4", "$5", "$6", "$7", "$8", "$9" };
	private static final String[] BUILTIN_FUNCTIONS = { "abs", "classof", "d2x", "exit", "exp", "gmtime",
		"index", "left", "length", "localtime", "log", "log10", "lower", "ltrim", "max", "min", "pow",
		"right", "rindex", "rtrim", "strftime", "substr", "time", "trace", "trim", "typeof", "upper",
		"AddrInRange", "AddrInSubnet", "SecondsToUptime" };
	private static final String[] BUILTIN_CONSTANTS = { "null", "true", "false" };
	
	private ScriptEditor editor;
	private NXSLLineStyleListener lineStyleListener;
		
	/**
	 * @param lineStyleListener
	 */
	public NXSLContentProposalProvider(ScriptEditor editor, NXSLLineStyleListener lineStyleListener)
	{
		this.editor = editor;
		this.lineStyleListener = lineStyleListener;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.fieldassist.IContentProposalProvider#getProposals(java.lang.String, int)
	 */
	@Override
	public IContentProposal[] getProposals(String contents, int position)
	{
		if (lineStyleListener.isOffsetWithinComment(position))
			return null;	// no proposals within comments
		
		List<IContentProposal> props = new ArrayList<IContentProposal>();

		StringBuilder sb = new StringBuilder();
		for(int i = position - 1; i >= 0; i--)
		{
			char ch = contents.charAt(i);
			if (!Character.isJavaIdentifierPart(ch))
				break;
			sb.insert(0, ch);
		}
		String prefix = sb.toString();

		addProposals(props, prefix, NXSLContentProposal.GLOBAL_VARIABLE, BUILTIN_SYSTEM_VARIABLES);
		addProposals(props, prefix, NXSLContentProposal.FUNCTION, BUILTIN_FUNCTIONS);
		addProposals(props, prefix, NXSLContentProposal.CONSTANT, BUILTIN_CONSTANTS);
		addProposals(props, prefix, NXSLContentProposal.FUNCTION, editor.getFunctions().toArray(new String[editor.getFunctions().size()]));
		addProposals(props, prefix, NXSLContentProposal.GLOBAL_VARIABLE, editor.getVariables().toArray(new String[editor.getVariables().size()]));
		Collections.sort(props, new Comparator<IContentProposal>() {
			@Override
			public int compare(IContentProposal arg0, IContentProposal arg1)
			{
				return arg0.getLabel().compareToIgnoreCase(arg1.getLabel());
			}
		});
		
		return props.toArray(new IContentProposal[props.size()]);
	}

	/**
	 * Add list of proposals
	 * 
	 * @param props proposals list to be populated
	 * @param type proposals type
	 * @param texts proposals texts
	 */
	private void addProposals(List<IContentProposal> props, String prefix, int type, String[] texts)
	{
		for(final String s : texts)
		{
			if (s.startsWith(prefix))
				props.add(new NXSLContentProposal(type, s, prefix.length()));
		}
	}
}

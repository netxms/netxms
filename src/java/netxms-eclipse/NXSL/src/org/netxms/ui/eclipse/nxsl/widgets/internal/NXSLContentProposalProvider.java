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

import org.eclipse.jface.fieldassist.IContentProposal;
import org.eclipse.jface.fieldassist.IContentProposalProvider;
import org.netxms.ui.eclipse.nxsl.tools.NXSLLineStyleListener;

/**
 * Content proposal provider for NXSL editor
 *
 */
public class NXSLContentProposalProvider implements IContentProposalProvider
{
	private static final String[] BUILTIN_SYSTEM_VARIABLES = { "$1", "$2", "$3", "$4", "$5", "$6", "$7", "$8", "$9" };
	
	private NXSLLineStyleListener lineStyleListener;
		
	/**
	 * @param lineStyleListener
	 */
	public NXSLContentProposalProvider(NXSLLineStyleListener lineStyleListener)
	{
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
		
		if (prefix.isEmpty())
		{
			addProposals(props, BUILTIN_SYSTEM_VARIABLES);
		}
		else if (prefix.equals("$"))
		{
			// System variables
			addProposals(props, BUILTIN_SYSTEM_VARIABLES);
		}
		else
		{
			
		}
		
		return props.toArray(new IContentProposal[props.size()]);
	}
	
	private void addProposals(List<IContentProposal> props, String[] texts)
	{
		for(final String s : texts)
		{
			IContentProposal p = new IContentProposal() {
				@Override
				public String getContent()
				{
					return s;
				}

				@Override
				public int getCursorPosition()
				{
					return s.length();
				}

				@Override
				public String getDescription()
				{
					return null;
				}

				@Override
				public String getLabel()
				{
					return s;
				}
			};
			props.add(p);
		}
	}
}

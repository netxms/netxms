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
package org.netxms.ui.eclipse.nxsl.widgets.internal;

import org.eclipse.jface.text.rules.ICharacterScanner;
import org.eclipse.jface.text.rules.IPredicateRule;
import org.eclipse.jface.text.rules.IToken;
import org.eclipse.jface.text.rules.Token;

/**
 * @author Victor
 *
 */
public class CodePatternRule implements IPredicateRule
{
	private IToken successToken;
	
	public CodePatternRule(IToken successToken)
	{
		this.successToken = successToken;
	}
	
	/**
	 * Test if current and/or previous characters are not part of code block
	 * 
	 * @param curr current character
	 * @param prev previous character
	 * @return number of characters to unread
	 */
	private int isCodePartEnd(char curr, char prev)
	{
		if (curr == '"')
			return 1;
		
		if ((prev == '/') && ((curr == '*') || (curr == '/')))
			return 2;
		
		return 0;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.text.rules.IRule#evaluate(org.eclipse.jface.text.rules.ICharacterScanner)
	 */
	@Override
	public IToken evaluate(ICharacterScanner scanner)
	{
		int prev = -1;
		int curr = -1;
		int offset = 0;
		int chars = 0;

		do
		{
			prev = curr;
			curr = scanner.read();
			if (curr == ICharacterScanner.EOF)
				break;
			offset = isCodePartEnd((char)curr, (char)prev);
			chars++;
		}
		while(offset == 0);
		
		for(int i = 0; i < offset; i++)
			scanner.unread();
		return (chars == 0) ? Token.UNDEFINED : successToken;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.text.rules.IPredicateRule#evaluate(org.eclipse.jface.text.rules.ICharacterScanner, boolean)
	 */
	@Override
	public IToken evaluate(ICharacterScanner scanner, boolean resume)
	{
		return evaluate(scanner);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.text.rules.IPredicateRule#getSuccessToken()
	 */
	@Override
	public IToken getSuccessToken()
	{
		return successToken;
	}
}

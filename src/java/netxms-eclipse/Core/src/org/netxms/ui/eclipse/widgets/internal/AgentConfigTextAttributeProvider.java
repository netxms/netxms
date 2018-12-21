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
package org.netxms.ui.eclipse.widgets.internal;

import org.eclipse.jface.text.TextAttribute;
import org.eclipse.jface.text.rules.IToken;
import org.eclipse.jface.text.rules.Token;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Display;

/**
 * Color provider for NXSL syntax highlighting
 *
 */
public class AgentConfigTextAttributeProvider
{
	private static final Color DEFAULT_COLOR = new Color(Display.getCurrent(), 0, 0, 0);
	private static final Color COMMENT_COLOR = new Color(Display.getCurrent(), 128, 128, 128);
	private static final Color ERROR_COLOR = new Color(Display.getCurrent(), 255, 255, 255);
	private static final Color ERROR_BK_COLOR = new Color(Display.getCurrent(), 255, 0, 0);
	private static final Color KEYWORD_COLOR = new Color(Display.getCurrent(), 157, 0, 157);
	private static final Color STRING_COLOR = new Color(Display.getCurrent(), 0, 0, 192);
	private static final Color SECTION_COLOR = new Color(Display.getCurrent(), 128, 128, 0);

	public static final int DEFAULT = 0;
	public static final int COMMENT = 1;
	public static final int ERROR = 2;
	public static final int KEYWORD = 3;
	public static final int STRING = 4;
	public static final int SECTION = 5;

	/**
	 * Get text attribute for given element (keyword, comment, etc.)
	 * 
	 * @param element source code element
	 * @return text attribute
	 */
	public static TextAttribute getTextAttribute(int element)
	{
		switch(element)
		{
			case COMMENT:
				return new TextAttribute(COMMENT_COLOR);
			case ERROR:
				return new TextAttribute(ERROR_COLOR, ERROR_BK_COLOR, SWT.NORMAL);
			case KEYWORD:
				return new TextAttribute(KEYWORD_COLOR, null, SWT.BOLD);
			case STRING:
				return new TextAttribute(STRING_COLOR);
			case SECTION:
				return new TextAttribute(SECTION_COLOR, null, SWT.BOLD | SWT.UNDERLINE_SINGLE);
			default:
				return new TextAttribute(DEFAULT_COLOR);
		}
	}
	
	/**
	 * Get token containing text attribute for given element (keyword, comment, etc.)
	 * 
	 * @param element source code element
	 * @return token with text attribute
	 */
	public static IToken getTextAttributeToken(int element)
	{
		return new Token(getTextAttribute(element));
	}
}

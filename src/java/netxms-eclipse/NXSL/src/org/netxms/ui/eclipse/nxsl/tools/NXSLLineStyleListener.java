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
package org.netxms.ui.eclipse.nxsl.tools;

import java.util.ArrayList;
import java.util.LinkedList;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ExtendedModifyEvent;
import org.eclipse.swt.custom.ExtendedModifyListener;
import org.eclipse.swt.custom.LineStyleEvent;
import org.eclipse.swt.custom.LineStyleListener;
import org.eclipse.swt.custom.StyleRange;
import org.eclipse.swt.custom.StyledText;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Display;

/**
 * Line style listener for NXSL syntax highlighting in StyledText. Object of this class should
 * be set both as line style listener and extended modify listener
 *
 */
public class NXSLLineStyleListener implements LineStyleListener, ExtendedModifyListener
{
	private static final int STATE_DEFAULT = 0;
	private static final int STATE_COMMENT = 1;
	private static final int STATE_EOL_COMMENT = 2;
	private static final int STATE_STRING = 3;
	
	private static final Color COMMENT_COLOR = new Color(Display.getCurrent(), 128, 128, 128);
	private static final Color CONSTANT_COLOR = new Color(Display.getCurrent(), 0, 0, 192);
	private static final Color ERROR_COLOR = new Color(Display.getCurrent(), 255, 255, 255);
	private static final Color ERROR_BK_COLOR = new Color(Display.getCurrent(), 255, 0, 0);
	private static final Color KEYWORD_COLOR = new Color(Display.getCurrent(), 96, 0, 0);
	private static final Color STRING_COLOR = new Color(Display.getCurrent(), 0, 0, 192);
	
	private static final String[] keywords = { "break", "classof", "continue", "do", "else", "exit", "for", "if", "ilike",
	                                           "imatch", "int32", "int64", "like", "match", "print", "println",
	                                           "real", "return", "string", "sub", "typeof", "uint32", "uint64",
	                                           "use", "while" };
	private static final String[] systemConstants = { "null", "NULL", "true", "TRUE", "false", "FALSE" };
	
	@SuppressWarnings("rawtypes")
	private LinkedList commentOffsets = new LinkedList();
	
	/**
	 * Create new style range for comments
	 * 
	 * @param start Start offset
	 * @param length Length
	 * @return New style range
	 */
	private StyleRange newCommentStyle(int start, int length)
	{
		return new StyleRange(start, length, COMMENT_COLOR, null);
	}
	
	/**
	 * Create new style range for keywords
	 * 
	 * @param start Start offset
	 * @param length Length
	 * @return New style range
	 */
	private StyleRange newKeywordStyle(int start, int length)
	{
		StyleRange style = new StyleRange(start, length, KEYWORD_COLOR, null);
		style.fontStyle = SWT.BOLD;
		return style;
	}
	
	/**
	 * Create new style range for system constants
	 * 
	 * @param start Start offset
	 * @param length Length
	 * @return New style range
	 */
	private StyleRange newConstantStyle(int start, int length)
	{
		StyleRange style = new StyleRange(start, length, CONSTANT_COLOR, null);
		style.fontStyle = SWT.ITALIC;
		return style;
	}
	
	/**
	 * Create new style range for string
	 * 
	 * @param start Start offset
	 * @param length Length
	 * @return New style range
	 */
	private StyleRange newStringStyle(int start, int length)
	{
		return new StyleRange(start, length, STRING_COLOR, null);
	}

	/**
	 * Create new style range for string without closing quote
	 * 
	 * @param start Start offset
	 * @param length Length
	 * @return New style range
	 */
	private StyleRange newOpenStringStyle(int start, int length)
	{
		return new StyleRange(start, length, ERROR_COLOR, ERROR_BK_COLOR);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.swt.custom.LineStyleListener#lineGetStyle(org.eclipse.swt.custom.LineStyleEvent)
	 */
	@Override
	public void lineGetStyle(LineStyleEvent event)
	{
		ArrayList<StyleRange> styles = new ArrayList<StyleRange>();
		
		int state = isOffsetWithinComment(event.lineOffset) ? STATE_COMMENT : STATE_DEFAULT;
		int start = 0;
		StringBuilder word = new StringBuilder();
	
		final int length = event.lineText.length();
		for(int i = 0; i < length; i++)
		{
			char ch = event.lineText.charAt(i);
			switch(state)
			{
				case STATE_DEFAULT:
					switch(ch)
					{
						case '/':
	   					final char next = (i < length - 1) ? event.lineText.charAt(i + 1) : 0;
	   					if (next == '*')
	   					{
	   						processKeyword(word, event.lineOffset + start, styles);
	   						state = STATE_COMMENT;
	   						start = i;
	   						i++;
	   					}
	   					else if (next == '/')
							{
	   						processKeyword(word, event.lineOffset + start, styles);
								styles.add(newCommentStyle(event.lineOffset + i, length - i));
								i = length;
	   						state = STATE_EOL_COMMENT;
							}
							break;
						case '"':
   						processKeyword(word, event.lineOffset + start, styles);
							state = STATE_STRING;
							start = i;
							break;
						default:
							if (Character.isLetter(ch))
							{
								if (word.length() == 0)
									start = i;
								word.append(ch);
							}
							else
							{
	   						processKeyword(word, event.lineOffset + start, styles);
							}
							break;
					}
					break;
				case STATE_STRING:
					switch(ch)
					{
						case '\\':
							i++;	// Skip next character
							break;
						case '"':
   						styles.add(newStringStyle(event.lineOffset + start, i - start + 1));
   						start = i + 1;
							state = STATE_DEFAULT;
							break;
					}
					break;
				case STATE_COMMENT:
					if (ch == '*')
					{
   					final char next = (i < length - 1) ? event.lineText.charAt(i + 1) : 0;
   					if (next == '/')
   					{
   						i++;
   						styles.add(newCommentStyle(event.lineOffset + start, i - start + 1));
   						start = i + 1;
   						state = STATE_DEFAULT;
   					}
					}
					break;
			}
		}
		
		// apply styling to the rest of the line
		switch(state)
		{
			case STATE_DEFAULT:
				processKeyword(word, event.lineOffset + start, styles);
				break;
			case STATE_COMMENT:
				styles.add(newCommentStyle(event.lineOffset + start, length - start));
				break;
			case STATE_STRING:
				styles.add(newOpenStringStyle(event.lineOffset + start, length - start));
				break;
		}
		
		event.styles = styles.toArray(new StyleRange[styles.size()]);
	}
	
	/**
	 * Check if collected word is a language keyword and set style if appropriate. Will clear
	 * supplied string builder after processing.
	 * 
	 * @param word String builder containing potential keyword
	 * @param start Start offset for the keyword
	 */
	private void processKeyword(StringBuilder word, int start, ArrayList<StyleRange> styles)
	{
		if (word.length() == 0)
			return;
		
		final String s = word.toString();
		for(int i = 0; i < keywords.length; i++)
		{
			if (s.compareTo(keywords[i]) == 0)
			{
				styles.add(newKeywordStyle(start, word.length()));
			}
		}

		for(int i = 0; i < systemConstants.length; i++)
		{
			if (s.compareTo(systemConstants[i]) == 0)
			{
				styles.add(newConstantStyle(start, word.length()));
			}
		}

		word.setLength(0);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.custom.ExtendedModifyListener#modifyText(org.eclipse.swt.custom.ExtendedModifyEvent)
	 */
	@Override
	public void modifyText(ExtendedModifyEvent event)
	{
		StyledText styledText = (StyledText)event.widget;
		refreshMultilineComments(styledText.getText());
		styledText.redraw();
	}
	
	/**
	 * Refreshes the offsets for all multiline comments in the parent StyledText.
	 * @param text Text from StyledText widget
	 */
	@SuppressWarnings("unchecked")
	private void refreshMultilineComments(final String text)
	{
		commentOffsets.clear();

	   // Go through all the instances of COMMENT_START
		int length = text.length();
		int state = STATE_DEFAULT;
		int[] offsets = null;
	   for(int i = 0; i < length; i++)
	   {
	   	char ch = text.charAt(i);
	   	switch(state)
	   	{
	   		case STATE_DEFAULT:
	   			switch(ch)
	   			{
	   				case '"':
	   					state = STATE_STRING;
	   					break;
	   				case '/':
	   					final char next = (i < length - 1) ? text.charAt(i + 1) : 0;
	   					if (next == '*')
	   					{
	   				   	offsets = new int[2];
	   				   	offsets[0] = i;
	   						state = STATE_COMMENT;
	   						i++;
	   					}
	   					else if (next == '/')
	   					{
	   						state = STATE_EOL_COMMENT;
	   						i++;
	   					}
	   					break;
	   			}
	   			break;
	   		case STATE_STRING:
	   			switch(ch)
	   			{
	   				case '"':
	   				case 0x0D:
	   				case 0x0A:
	   					state = STATE_DEFAULT;
	   					break;
	   				case '\\':
	   					i++;	// Ignore next character
	   					break;
	   			}
	   			break;
	   		case STATE_EOL_COMMENT:
	   			if ((ch == 0x0D) || (ch == 0x0A))
	   				state = STATE_DEFAULT;
	   			break;
	   		case STATE_COMMENT:
	   			if (ch == '*')
	   			{
   					final char next = (i < length - 1) ? text.charAt(i + 1) : 0;
   					if (next == '/')
   					{
   						i++;
   				   	offsets[1] = i;
   				      commentOffsets.add(offsets);
   						state = STATE_DEFAULT;
   					}
	   			}
	   			break;
	   	}
	   }
	   
	   // Add last offset if comment block is not closed
	   if (state == STATE_COMMENT)
	   {
	   	offsets[1] = length - 1;
	      commentOffsets.add(offsets);
	   }
	}
	
	/**
	 * Check if given offset is within multiline comment
	 * @param offset Offset
	 * @return true if given offset is within multiline comment
	 */
	public boolean isOffsetWithinComment(int offset)
	{
		for(int i = 0; i < commentOffsets.size(); i++)
		{
			int[] offsets = (int[])commentOffsets.get(i);
			if (offsets[0] > offset)
				break;
			if (offsets[1] > offset)
				return true;
		}
		return false;
	}

	/**
	 * @return the systemconstants
	 */
	public static String[] getSystemconstants()
	{
		return systemConstants;
	}
}

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

import org.eclipse.jface.fieldassist.IControlContentAdapter;
import org.eclipse.swt.custom.StyledText;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Control;

/**
 * Content adapter for styled text
 *
 */
public class StyledTextContentAdapter implements IControlContentAdapter
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.fieldassist.IControlContentAdapter#getControlContents(org.eclipse.swt.widgets.Control)
	 */
	@Override
	public String getControlContents(Control control)
	{
		return ((StyledText) control).getText();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.fieldassist.IControlContentAdapter#getCursorPosition(org.eclipse.swt.widgets.Control)
	 */
	@Override
	public int getCursorPosition(Control control)
	{
		return ((StyledText) control).getCaretOffset();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.fieldassist.IControlContentAdapter#getInsertionBounds(org.eclipse.swt.widgets.Control)
	 */
	@Override
	public Rectangle getInsertionBounds(Control control)
	{
		StyledText text = (StyledText) control;
		Point caretOrigin = text.getLocationAtOffset(text.getCaretOffset());
		return new Rectangle(caretOrigin.x + text.getClientArea().x, caretOrigin.y + text.getClientArea().y + 3, 1, text
				.getLineHeight());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.fieldassist.IControlContentAdapter#insertControlContents(org.eclipse.swt.widgets.Control, java.lang.String, int)
	 */
	@Override
	public void insertControlContents(Control control, String contents, int cursorPosition)
	{
		StyledText text = (StyledText) control;
		text.insert(contents);
		text.setCaretOffset(text.getCaretOffset() + cursorPosition);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.fieldassist.IControlContentAdapter#setControlContents(org.eclipse.swt.widgets.Control, java.lang.String, int)
	 */
	@Override
	public void setControlContents(Control control, String contents, int cursorPosition)
	{
		StyledText text = (StyledText) control;
		text.setText(contents);
		text.setCaretOffset(cursorPosition);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.fieldassist.IControlContentAdapter#setCursorPosition(org.eclipse.swt.widgets.Control, int)
	 */
	@Override
	public void setCursorPosition(Control control, int index)
	{
		((StyledText) control).setCaretOffset(index);
	}
}

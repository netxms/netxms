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
package org.netxms.ui.eclipse.tools;

/**
 * Interface for text filed input validator
 */
public interface TextFieldValidator
{
	/**
	 * Validate text.
	 * 
	 * @param text
	 * @return true if text is valid
	 */
	public abstract boolean validate(String text);
	
	/**
	 * Get error message for failed validation. This method always called
	 * immediately after failed validation.
	 * 
	 * @param text
	 * @param label
	 * @return
	 */
	public abstract String getErrorMessage(String text, String label);
}

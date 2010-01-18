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
package org.netxms.ui.eclipse.epp.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.StyledText;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.ui.eclipse.tools.NXSLLineStyleListener;

/**
 * Cell for filtering script
 *
 */
public class ScriptCell extends Cell
{
	private EventProcessingPolicyRule eppRule;
	private StyledText text;
	
	public ScriptCell(Rule rule, Object data)
	{
		super(rule, data);
		eppRule = (EventProcessingPolicyRule)data;
		
		text = new StyledText(this, SWT.MULTI);
		text.setEditable(false);
      final NXSLLineStyleListener listener = new NXSLLineStyleListener();
      text.addLineStyleListener(listener);
      text.addExtendedModifyListener(listener);
		text.setText(eppRule.getScript());
	}

}

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
package org.netxms.ui.eclipse.nxsl.widgets;

import org.eclipse.jface.bindings.keys.KeyStroke;
import org.eclipse.jface.bindings.keys.ParseException;
import org.eclipse.jface.fieldassist.ContentProposalAdapter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.StyledText;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.nxsl.tools.NXSLLineStyleListener;
import org.netxms.ui.eclipse.nxsl.widgets.internal.NXSLContentProposalProvider;
import org.netxms.ui.eclipse.nxsl.widgets.internal.StyledTextContentAdapter;

/**
 * NXSL script editor
 *
 */
public class ScriptEditor extends StyledText
{
	private Font editorFont;
	private boolean modified;

	/**
	 * @param parent
	 * @param style
	 */
	public ScriptEditor(Composite parent, int style)
	{
		super(parent, style);
		
		editorFont = new Font(getShell().getDisplay(), "Courier New", 10, SWT.NORMAL);
		
		setLayout(new FillLayout());
		setFont(editorFont);
		setTabs(3);
		setWordWrap(false);
      final NXSLLineStyleListener listener = new NXSLLineStyleListener();
      addLineStyleListener(listener);
      addExtendedModifyListener(listener);
      addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				modified = true;
			}
      });
      
      try
		{
			ContentProposalAdapter adapter = new ContentProposalAdapter(this,
					new StyledTextContentAdapter(),
					new NXSLContentProposalProvider(listener),
					KeyStroke.getInstance("Ctrl+Space"), new char[] { '$' });
			adapter.setEnabled(true);
			adapter.setFilterStyle(ContentProposalAdapter.FILTER_CHARACTER);
			adapter.setPropagateKeys(false);
		}
		catch(ParseException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Widget#dispose()
	 */
	@Override
	public void dispose()
	{
		editorFont.dispose();
		super.dispose();
	}

	/**
	 * @return the modified
	 */
	public boolean isModified()
	{
		return modified;
	}

	/**
	 * @param modified the modified to set
	 */
	public void setModified(boolean modified)
	{
		this.modified = modified;
	}
}

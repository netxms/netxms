/**
 * 
 */
package org.netxms.ui.eclipse.nxsl.widgets.internal;

import org.eclipse.jface.fieldassist.IContentProposal;

/**
 * Content proposal for NetXMS
 *
 */
public class NXSLContentProposal implements IContentProposal
{
	public static final int FUNCTION = 0;
	public static final int GLOBAL_VARIABLE = 1;
	public static final int LOCAL_VARIABLE = 2;
	public static final int CONSTANT = 3;
	
	private int type;		// proposal type
	private String content;
	private int offset;
	
	/**
	 * @param type
	 * @param content
	 */
	public NXSLContentProposal(int type, String content, int offset)
	{
		this.type = type;
		this.content = content;
		this.offset = offset;
	}

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.fieldassist.IContentProposal#getContent()
	 */
	@Override
	public String getContent()
	{
		return content.substring(offset);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.fieldassist.IContentProposal#getCursorPosition()
	 */
	@Override
	public int getCursorPosition()
	{
		return content.length() - offset;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.fieldassist.IContentProposal#getDescription()
	 */
	@Override
	public String getDescription()
	{
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.fieldassist.IContentProposal#getLabel()
	 */
	@Override
	public String getLabel()
	{
		return content;
	}
}

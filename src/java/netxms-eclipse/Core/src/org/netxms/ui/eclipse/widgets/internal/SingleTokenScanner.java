/**
 * 
 */
package org.netxms.ui.eclipse.widgets.internal;

import org.eclipse.jface.text.TextAttribute;
import org.eclipse.jface.text.rules.RuleBasedScanner;
import org.eclipse.jface.text.rules.Token;

/**
 * @author Victor
 *
 */
public class SingleTokenScanner extends RuleBasedScanner
{
	/**
	 * 
	 */
	public SingleTokenScanner(TextAttribute attr)
	{
		setDefaultReturnToken(new Token(attr));
	}
}

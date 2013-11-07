/**
 * 
 */
package org.netxms.ui.eclipse.nxsl.widgets.internal;

import org.eclipse.jface.text.Document;
import org.eclipse.jface.text.IDocument;
import org.eclipse.jface.text.rules.EndOfLineRule;
import org.eclipse.jface.text.rules.FastPartitioner;
import org.eclipse.jface.text.rules.IPredicateRule;
import org.eclipse.jface.text.rules.PatternRule;
import org.eclipse.jface.text.rules.RuleBasedPartitionScanner;
import org.eclipse.jface.text.rules.SingleLineRule;
import org.eclipse.jface.text.rules.Token;

/**
 * NXSL script representation as a document
 *
 */
public class NXSLDocument extends Document
{
	public static final String CONTENT_COMMENTS = "COMMENTS";
	public static final String CONTENT_STRING = "STRING";
	
	public static final String[] NXSL_CONTENT_TYPES = { IDocument.DEFAULT_CONTENT_TYPE, CONTENT_COMMENTS, CONTENT_STRING };

	// Document partitioning rules
	private static final IPredicateRule[] NXSL_RULES = {
		new PatternRule("/*", "*/", new Token(CONTENT_COMMENTS), (char)0, false, false, false),
		new EndOfLineRule("//", new Token(CONTENT_COMMENTS)),
		new SingleLineRule("\"", "\"", new Token(CONTENT_STRING), '\\', true, false),
		new CodePatternRule(new Token(DEFAULT_CONTENT_TYPE))
	};
	
	/**
	 * Creates NXSL document
	 */
	public NXSLDocument(String content)
	{
		super(content);

		final RuleBasedPartitionScanner scanner = new RuleBasedPartitionScanner();
		scanner.setPredicateRules(NXSL_RULES);
		scanner.setDefaultReturnToken(new Token(IDocument.DEFAULT_CONTENT_TYPE));
		final FastPartitioner partitioner = new FastPartitioner(scanner, NXSL_CONTENT_TYPES);
		setDocumentPartitioner(partitioner);
		partitioner.connect(this);
	}
}

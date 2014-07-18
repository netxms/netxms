package org.netxms.ui.eclipse.views;

/**
 * View or display element that can be limited to display no more then specific number of records. 
 */
public interface Limitable
{
	/**
	 * Inform user if the view is limited or not.
	 * Method has to be called in UI thread.
	 * 
	 * @param show
	 *            True, if the view is limited. False otherwise.
	 */
	void showLimitWarning(boolean show);
}

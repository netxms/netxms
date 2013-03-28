/**
 * 
 */
package org.netxms.ui.eclipse.objectview.api;

import org.eclipse.ui.IViewPart;
import org.netxms.client.objects.AbstractObject;

/**
 * Interface for object details providers
 */
public interface ObjectDetailsProvider
{
	/**
	 * Called by framework to check if provider can provide detailed information for given object.
	 * 
	 * @param object object to evaluate
	 * @return true if provider can provide information
	 */
	public abstract boolean canProvideDetails(AbstractObject object);
	
	/**
	 * Called by framework to provide additional information for object.
	 * 
	 * @param object object to provide information for
	 * @param viewPart active view, can be null
	 */
	public abstract void provideDetails(AbstractObject object, IViewPart viewPart);
}

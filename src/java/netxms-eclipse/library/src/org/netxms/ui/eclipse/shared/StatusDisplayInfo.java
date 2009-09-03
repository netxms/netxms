/**
 * 
 */
package org.netxms.ui.eclipse.shared;

import org.eclipse.jface.resource.ImageDescriptor;
import org.netxms.client.constants.Severity;
import org.netxms.ui.eclipse.library.Activator;

/**
 * @author Victor
 *
 */
public final class StatusDisplayInfo
{
	private static String[] statusText = new String[9];
	private static ImageDescriptor[] statusImage = new ImageDescriptor[9];
	
	/**
	 * Initialize static members. Intended to be called once by library activator.
	 */
	public static void init()
	{
		statusText[Severity.NORMAL] = "Normal";
		statusText[Severity.WARNING] = "Warning";
		statusText[Severity.MINOR] = "Minor";
		statusText[Severity.MAJOR] = "Major";
		statusText[Severity.CRITICAL] = "Critical";
		statusText[Severity.UNKNOWN] = "Unknown";
		statusText[Severity.UNMANAGED] = "Unmanaged";
		statusText[Severity.DISABLED] = "Disabled";
		statusText[Severity.TESTING] = "Testing";

		statusImage[Severity.NORMAL] = Activator.getImageDescriptor("icons/status/normal.png");
		statusImage[Severity.WARNING] = Activator.getImageDescriptor("icons/status/warning.png");
		statusImage[Severity.MINOR] = Activator.getImageDescriptor("icons/status/minor.png");
		statusImage[Severity.MAJOR] = Activator.getImageDescriptor("icons/status/major.png");
		statusImage[Severity.CRITICAL] = Activator.getImageDescriptor("icons/status/critical.png");
		statusImage[Severity.UNKNOWN] = Activator.getImageDescriptor("icons/status/unknown.png");
		statusImage[Severity.UNMANAGED] = Activator.getImageDescriptor("icons/status/unmanaged.png");
		statusImage[Severity.DISABLED] = Activator.getImageDescriptor("icons/status/disabled.png");
		statusImage[Severity.TESTING] = Activator.getImageDescriptor("icons/status/desting.png");
	}
	
	/**
	 * Get text for given status/severity code.
	 * 
	 * @param severity Status or severity code
	 * @return Text for given code
	 */
	public static String getStatusText(int severity)
	{
		return statusText[severity];
	}
	
	/**
	 * Get image descriptor for given status/severity code.
	 * 
	 * @param severity Status or severity code
	 * @return Image descriptor for given code
	 */
	public static ImageDescriptor getStatusImageDescriptor(int severity)
	{
		return statusImage[severity];
	}
}

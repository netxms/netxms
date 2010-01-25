/**
 * 
 */
package org.netxms.ui.eclipse.console;

import org.eclipse.core.runtime.preferences.AbstractPreferenceInitializer;
import org.eclipse.jface.preference.IPreferenceStore;

/**
 * @author victor
 *
 */
public class PreferenceInitializer extends AbstractPreferenceInitializer
{
	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.preferences.AbstractPreferenceInitializer#initializeDefaultPreferences()
	 */
	@Override
	public void initializeDefaultPreferences()
	{
		final IPreferenceStore ps = Activator.getDefault().getPreferenceStore();
		ps.setDefault("INITIAL_PERSPECTIVE", "org.netxms.ui.eclipse.console.DefaultPerspective");
		ps.setDefault("SHOW_COOLBAR", true);
	}
}

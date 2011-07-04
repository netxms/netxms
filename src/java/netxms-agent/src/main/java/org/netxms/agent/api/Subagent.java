/**
 * 
 */
package org.netxms.agent.api;

import java.util.List;

/**
 * Interface for subagents
 *
 */
public interface Subagent
{
	/**
	 * Initialize subagent. This is first method called by master agent
	 * after subagent instantiation.
	 * 
	 * @return true if initialization was successful
	 */
	public boolean init();
	
	/**
	 * Shutdown subagent.
	 */
	public void shutdown();
	
	/**
	 * Called by master subagent to obtain list of supported parameters.
	 * 
	 * @return list of supported parameters
	 */
	public List<Parameter> getParameters();
}

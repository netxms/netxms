/**
 * 
 */
package org.netxms.client.packages;

/**
 * Listener interface for package deployment progress
 */
public interface PackageDeploymentListener
{
	public static final int PENDING = 0;
	public static final int TRANSFER = 1;
	public static final int INSTALLATION = 2;
	public static final int COMPLETED = 3;
	public static final int FAILED = 4;
	public static final int INITIALIZE = 5;
	public static final int FINISHED = 255;
	
	/**
	 * Status update for node.
	 * 
	 * @param nodeId node object ID
	 * @param status status code (defined in PackageDeploymentListener interface)
	 * @param message error message, if applicable (otherwise null)
	 */
	public abstract void statusUpdate(long nodeId, int status, String message);
	
	/**
	 * Called by deployment thread when entire deployment task is completed.
	 */
	public abstract void deploymentComplete();
}

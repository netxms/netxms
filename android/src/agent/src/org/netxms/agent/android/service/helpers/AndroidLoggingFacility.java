/**
 * 
 */
package org.netxms.agent.android.service.helpers;

import org.netxms.base.LoggingFacility;

import android.util.Log;

/**
 * Logging facility for NetXMS client library
 *
 */
public class AndroidLoggingFacility implements LoggingFacility
{
	/* (non-Javadoc)
	 * @see org.netxms.base.LoggingFacility#writeLog(int, java.lang.String, java.lang.String, java.lang.Throwable)
	 */
	@Override
	public void writeLog(int level, String tag, String message, Throwable t)
	{
		switch (level)
		{
			case LoggingFacility.DEBUG:
				if (t != null)
					Log.d(tag, message, t);
				else
					Log.d(tag, message);
				break;
			case LoggingFacility.INFO:
				if (t != null)
					Log.i(tag, message, t);
				else
					Log.i(tag, message);
				break;
			case LoggingFacility.WARNING:
				if (t != null)
					Log.w(tag, message, t);
				else
					Log.w(tag, message);
				break;
			case LoggingFacility.ERROR:
				if (t != null)
					Log.e(tag, message, t);
				else
					Log.e(tag, message);
				break;
		}
	}
}

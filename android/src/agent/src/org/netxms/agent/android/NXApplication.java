package org.netxms.agent.android;

import org.acra.ACRA;
import org.acra.annotation.ReportsCrashes;

import android.app.Application;

@ReportsCrashes(formKey = "dGJ5RFRpbmg2ZVRvT2tERU9seHVlTWc6MQ")
public class NXApplication extends Application
{
	private static boolean activityVisible;

	@Override
	public void onCreate()
	{
		ACRA.init(this);
		super.onCreate();
	}

	public static boolean isActivityVisible()
	{
		return activityVisible;
	}

	public static void activityResumed()
	{
		activityVisible = true;
	}

	public static void activityPaused()
	{
		activityVisible = false;
	}
}

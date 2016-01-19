package org.netxms.ui.android;

import org.acra.ACRA;
import org.acra.ReportField;
import org.acra.ReportingInteractionMode;
import org.acra.annotation.ReportsCrashes;

import android.app.Application;

@ReportsCrashes(
		mailTo = "acra@netxms.org",
		customReportContent = { ReportField.APP_VERSION_CODE, ReportField.APP_VERSION_NAME, ReportField.ANDROID_VERSION, ReportField.PHONE_MODEL, ReportField.CUSTOM_DATA, ReportField.STACK_TRACE, ReportField.LOGCAT },
		mode = ReportingInteractionMode.TOAST,
		resToastText = R.string.crash_toast_text)
public class NXApplication extends Application
{
	private static boolean activityVisible;

	@Override
	public void onCreate()
	{
		ACRA.init(this);
		ACRA.getErrorReporter().checkReportsOnApplicationStart();
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

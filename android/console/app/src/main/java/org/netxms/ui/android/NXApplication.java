package org.netxms.ui.android;

import org.acra.ACRA;
import org.acra.annotation.AcraCore;
import org.acra.annotation.AcraMailSender;
import org.acra.annotation.AcraToast;

import android.app.Application;
import android.content.Context;

@AcraCore(buildConfigClass = BuildConfig.class)
@AcraMailSender(mailTo = "acra@netxms.org", reportAsFile = true)
@AcraToast(resText = R.string.crash_toast_text)
public class NXApplication extends Application
{
	private static boolean activityVisible;

	@Override
	protected void attachBaseContext(Context base) {
		super.attachBaseContext(base);

		ACRA.init(this);
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

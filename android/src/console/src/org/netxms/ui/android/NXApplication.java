package org.netxms.ui.android;

import org.acra.ACRA;
import org.acra.ErrorReporter;
import org.acra.annotation.ReportsCrashes;
import android.app.Application;

@ReportsCrashes(formKey = "dEhRMG50MjhWTXBWSU5jaDVubzlxT1E6MQ")
public class NXApplication extends Application
{
	@Override
	public void onCreate()
	{
		ACRA.init(this);
		ErrorReporter.getInstance().checkReportsOnApplicationStart();
		super.onCreate();
	}
}

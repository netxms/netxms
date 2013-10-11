package org.netxms.agent;

import java.util.*;

public final class Config {
	private volatile long configHandle; // actually a pointer to native Config

	private Config(long configHandle) {
	      this.configHandle = configHandle;
	}

	public native void lock();
	public native void unlock();

	public native void deleteEntry(String path);
	public native ConfigEntry getEntry(String path);
	public native ConfigEntry[] getSubEntries(String path, String mask);
	public native ConfigEntry[] getOrderedSubEntries(String path, String mask);

	public native String getValue(String path, String defaultValue); // string
	public native int getValueInt(String path, int defaultValue);  // signed int32
	public native long getValueLong(String path, long defaultValue); // signed int64
	public native boolean getValueBoolean(String path, boolean defaultValue); // boolean
	//public native double getValueDouble(String path, double defaultValue); // double
	// why is there no getValueDouble in nxconfig.h while there is setValue(path, double) ?

	public native boolean setValue(String path, String value); // string
	public native boolean setValue(String path, int value); // signed int32
	public native boolean setValue(String path, long value); // signed int64
	//public native boolean setValue(String path, boolean value); // boolean
	// why is there no setValue(path, boolean) in nxconfig.h while there is getValueBoolean?
	public native boolean setValue(String path, double value); // double
}

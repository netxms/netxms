package org.netxms.api.client.constants;

public enum ScheduleType
{
	Once(0), Daily(1), Weekly(2), Monthly(3);

	private final int code;

	ScheduleType(int code)
	{
		this.code = code;
	}

	public int getCode()
	{
		return code;
	}
}

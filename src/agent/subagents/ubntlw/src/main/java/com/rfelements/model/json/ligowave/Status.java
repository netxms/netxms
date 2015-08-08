package com.rfelements.model.json.ligowave;
/**
 * @author Pichanič Ján
 */
public class Status {

	private String operatingMode;

	private String deviceName;

	private Integer longitude;

	private Double uptime;

	private String deviceLocation;

	private FirmwareVersion firmwareVersion;

	private String serialNo;

	private Memory memory;

	private String systemClock;

	private String productName;

	private Integer latitude;

	public String getOperatingMode() {
		return operatingMode;
	}

	public void setOperatingMode(String operatingMode) {
		this.operatingMode = operatingMode;
	}

	public String getDeviceName() {
		return deviceName;
	}

	public void setDeviceName(String deviceName) {
		this.deviceName = deviceName;
	}

	public Integer getLongitude() {
		return longitude;
	}

	public void setLongitude(Integer longitude) {
		this.longitude = longitude;
	}

	public Double getUptime() {
		return uptime;
	}

	public void setUptime(Double uptime) {
		this.uptime = uptime;
	}

	public String getDeviceLocation() {
		return deviceLocation;
	}

	public void setDeviceLocation(String deviceLocation) {
		this.deviceLocation = deviceLocation;
	}

	public FirmwareVersion getFirmwareVersion() {
		return firmwareVersion;
	}

	public void setFirmwareVersion(FirmwareVersion firmwareVersion) {
		this.firmwareVersion = firmwareVersion;
	}

	public String getSerialNo() {
		return serialNo;
	}

	public void setSerialNo(String serialNo) {
		this.serialNo = serialNo;
	}

	public Memory getMemory() {
		return memory;
	}

	public void setMemory(Memory memory) {
		this.memory = memory;
	}

	public String getSystemClock() {
		return systemClock;
	}

	public void setSystemClock(String systemClock) {
		this.systemClock = systemClock;
	}

	public String getProductName() {
		return productName;
	}

	public void setProductName(String productName) {
		this.productName = productName;
	}

	public Integer getLatitude() {
		return latitude;
	}

	public void setLatitude(Integer latitude) {
		this.latitude = latitude;
	}
}

package com.rfelements.model.json.ligowave;
/**
 * @author Pichanič Ján
 */
public class RemoteInformation {

	private Wireless wireless;

	private String deviceName;

	private Integer modified;

	private String firmwareVersion;

	private String mac;

	private String deviceIp;

	private Integer seenBefore;

	public Wireless getWireless() {
		return wireless;
	}

	public void setWireless(Wireless wireless) {
		this.wireless = wireless;
	}

	public String getDeviceName() {
		return deviceName;
	}

	public void setDeviceName(String deviceName) {
		this.deviceName = deviceName;
	}

	public Integer getModified() {
		return modified;
	}

	public void setModified(Integer modified) {
		this.modified = modified;
	}

	public String getFirmwareVersion() {
		return firmwareVersion;
	}

	public void setFirmwareVersion(String firmwareVersion) {
		this.firmwareVersion = firmwareVersion;
	}

	public String getMac() {
		return mac;
	}

	public void setMac(String mac) {
		this.mac = mac;
	}

	public String getDeviceIp() {
		return deviceIp;
	}

	public void setDeviceIp(String deviceIp) {
		this.deviceIp = deviceIp;
	}

	public Integer getSeenBefore() {
		return seenBefore;
	}

	public void setSeenBefore(Integer seenBefore) {
		this.seenBefore = seenBefore;
	}
}

package com.rfelements.model.json.ligowave;
/**
 * @author Pichanič Ján
 */
public class Radios {

	private Dev dev;
	private String operationMode;
	private String countryCode;

	public Dev getDev() {
		return dev;
	}

	public void setDev(Dev dev) {
		this.dev = dev;
	}

	public String getOperationMode() {
		return operationMode;
	}

	public void setOperationMode(String operationMode) {
		this.operationMode = operationMode;
	}

	public String getCountryCode() {
		return countryCode;
	}

	public void setCountryCode(String countryCode) {
		this.countryCode = countryCode;
	}
}

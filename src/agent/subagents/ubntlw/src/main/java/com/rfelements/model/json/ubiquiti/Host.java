package com.rfelements.model.json.ubiquiti;
/**
 * @author Pichanič Ján
 */
public class Host {

	private Integer uptime;
	private String time;
	private String fwversion;
	private String fwprefix;
	private String hostname;
	private String devmodel;
	private String netrole;

	public Integer getUptime() {
		return uptime;
	}

	public void setUptime(Integer uptime) {
		this.uptime = uptime;
	}

	public String getTime() {
		return time;
	}

	public void setTime(String time) {
		this.time = time;
	}

	public String getFwversion() {
		return fwversion;
	}

	public void setFwversion(String fwversion) {
		this.fwversion = fwversion;
	}

	public String getFwprefix() {
		return fwprefix;
	}

	public void setFwprefix(String fwprefix) {
		this.fwprefix = fwprefix;
	}

	public String getHostname() {
		return hostname;
	}

	public void setHostname(String hostname) {
		this.hostname = hostname;
	}

	public String getDevmodel() {
		return devmodel;
	}

	public void setDevmodel(String devmodel) {
		this.devmodel = devmodel;
	}

	public String getNetrole() {
		return netrole;
	}

	public void setNetrole(String netrole) {
		this.netrole = netrole;
	}
}

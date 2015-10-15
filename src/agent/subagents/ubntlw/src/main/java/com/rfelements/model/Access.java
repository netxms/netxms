package com.rfelements.model;
/**
 * @author Pichanič Ján
 */
public class Access {

	private String protocol;

	private String ip;

	private String username;

	private String password;

	public Access(String protocol, String ip, String username, String password) {
		this.protocol = protocol;
		this.ip = ip;
		this.username = username;
		this.password = password;
	}

	public String getIp() {
		return ip;
	}

	public void setIp(String base_url) {
		this.ip = base_url;
	}

	public String getUsername() {
		return username;
	}

	public void setUsername(String username) {
		this.username = username;
	}

	public String getPassword() {
		return password;
	}

	public void setPassword(String password) {
		this.password = password;
	}

	public String getProtocol() {
		return protocol;
	}

	public void setProtocol(String protocol) {
		this.protocol = protocol;
	}
}

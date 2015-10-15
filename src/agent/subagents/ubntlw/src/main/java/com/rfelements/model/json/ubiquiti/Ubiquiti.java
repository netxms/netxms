package com.rfelements.model.json.ubiquiti;
/**
 * @author Pichanič Ján
 */
public class Ubiquiti {

	private Host host;

	private Wireless wireless;

	private AirView airview;

	private Services services;

	private Firewall firewall;

	private String genuine;

	private Interface[] interfaces;

	public Host getHost() {
		return host;
	}

	public void setHost(Host host) {
		this.host = host;
	}

	public Wireless getWireless() {
		return wireless;
	}

	public void setWireless(Wireless wireless) {
		this.wireless = wireless;
	}

	public AirView getAirview() {
		return airview;
	}

	public void setAirview(AirView airview) {
		this.airview = airview;
	}

	public Services getServices() {
		return services;
	}

	public void setServices(Services services) {
		this.services = services;
	}

	public Firewall getFirewall() {
		return firewall;
	}

	public void setFirewall(Firewall firewall) {
		this.firewall = firewall;
	}

	public String getGenuine() {
		return genuine;
	}

	public void setGenuine(String genuine) {
		this.genuine = genuine;
	}

	public Interface[] getInterfaces() {
		return interfaces;
	}

	public void setInterfaces(Interface[] interfaces) {
		this.interfaces = interfaces;
	}
}

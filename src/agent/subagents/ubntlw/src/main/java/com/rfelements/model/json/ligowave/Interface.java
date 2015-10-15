package com.rfelements.model.json.ligowave;
/**
 * @author Pichanič Ján
 */
public class Interface {

	private String hwaddr;
	private String name;
	private Double rxBytes;
	private String ip6Method;
	private String ipAddress;
	private Ip6Address ip6Address;
	private Integer rxErrors;
	private Integer txDrops;
	private Integer rxDrops;
	private Integer txErrors;
	private Integer rxPackets;
	private String ipMethod;

	public String getHwaddr() {
		return hwaddr;
	}

	public void setHwaddr(String hwaddr) {
		this.hwaddr = hwaddr;
	}

	public String getName() {
		return name;
	}

	public void setName(String name) {
		this.name = name;
	}

	public Double getRxBytes() {
		return rxBytes;
	}

	public void setRxBytes(Double rxBytes) {
		this.rxBytes = rxBytes;
	}

	public String getIp6Method() {
		return ip6Method;
	}

	public void setIp6Method(String ip6Method) {
		this.ip6Method = ip6Method;
	}

	public String getIpAddress() {
		return ipAddress;
	}

	public void setIpAddress(String ipAddress) {
		this.ipAddress = ipAddress;
	}

	public Ip6Address getIp6Address() {
		return ip6Address;
	}

	public void setIp6Address(Ip6Address ip6Address) {
		this.ip6Address = ip6Address;
	}

	public Integer getRxErrors() {
		return rxErrors;
	}

	public void setRxErrors(Integer rxErrors) {
		this.rxErrors = rxErrors;
	}

	public Integer getTxDrops() {
		return txDrops;
	}

	public void setTxDrops(Integer txDrops) {
		this.txDrops = txDrops;
	}

	public Integer getRxDrops() {
		return rxDrops;
	}

	public void setRxDrops(Integer rxDrops) {
		this.rxDrops = rxDrops;
	}

	public Integer getTxErrors() {
		return txErrors;
	}

	public void setTxErrors(Integer txErrors) {
		this.txErrors = txErrors;
	}

	public Integer getRxPackets() {
		return rxPackets;
	}

	public void setRxPackets(Integer rxPackets) {
		this.rxPackets = rxPackets;
	}

	public String getIpMethod() {
		return ipMethod;
	}

	public void setIpMethod(String ipMethod) {
		this.ipMethod = ipMethod;
	}
}

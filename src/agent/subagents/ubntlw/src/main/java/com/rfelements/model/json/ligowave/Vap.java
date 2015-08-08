package com.rfelements.model.json.ligowave;
/**
 * @author Pichanič Ján
 */
public class Vap {

	private String operationMode;

	private String name;

	private Integer vlan;

	private Boolean wds;

	private Integer rxErrors;

	private Integer ifIndex;

	private Integer txRetries;

	private String mac;

	private String radio;

	private String ssid;

	private Integer maxPeerCount;

	private Integer txPackets;

	private Integer peerCount;

	private Integer rxPackets;

	private String mcs;

	private String security;

	private Boolean hiddenSsid;

	public String getOperationMode() {
		return operationMode;
	}

	public void setOperationMode(String operationMode) {
		this.operationMode = operationMode;
	}

	public String getName() {
		return name;
	}

	public void setName(String name) {
		this.name = name;
	}

	public Integer getVlan() {
		return vlan;
	}

	public void setVlan(Integer vlan) {
		this.vlan = vlan;
	}

	public Boolean getWds() {
		return wds;
	}

	public void setWds(Boolean wds) {
		this.wds = wds;
	}

	public Integer getRxErrors() {
		return rxErrors;
	}

	public void setRxErrors(Integer rxErrors) {
		this.rxErrors = rxErrors;
	}

	public Integer getIfIndex() {
		return ifIndex;
	}

	public void setIfIndex(Integer ifIndex) {
		this.ifIndex = ifIndex;
	}

	public Integer getTxRetries() {
		return txRetries;
	}

	public void setTxRetries(Integer txRetries) {
		this.txRetries = txRetries;
	}

	public String getMac() {
		return mac;
	}

	public void setMac(String mac) {
		this.mac = mac;
	}

	public String getRadio() {
		return radio;
	}

	public void setRadio(String radio) {
		this.radio = radio;
	}

	public String getSsid() {
		return ssid;
	}

	public void setSsid(String ssid) {
		this.ssid = ssid;
	}

	public Integer getMaxPeerCount() {
		return maxPeerCount;
	}

	public void setMaxPeerCount(Integer maxPeerCount) {
		this.maxPeerCount = maxPeerCount;
	}

	public Integer getTxPackets() {
		return txPackets;
	}

	public void setTxPackets(Integer txPackets) {
		this.txPackets = txPackets;
	}

	public Integer getPeerCount() {
		return peerCount;
	}

	public void setPeerCount(Integer peerCount) {
		this.peerCount = peerCount;
	}

	public Integer getRxPackets() {
		return rxPackets;
	}

	public void setRxPackets(Integer rxPackets) {
		this.rxPackets = rxPackets;
	}

	public String getMcs() {
		return mcs;
	}

	public void setMcs(String mcs) {
		this.mcs = mcs;
	}

	public String getSecurity() {
		return security;
	}

	public void setSecurity(String security) {
		this.security = security;
	}

	public Boolean getHiddenSsid() {
		return hiddenSsid;
	}

	public void setHiddenSsid(Boolean hiddenSsid) {
		this.hiddenSsid = hiddenSsid;
	}
}

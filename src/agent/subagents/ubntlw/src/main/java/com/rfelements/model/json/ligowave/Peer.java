package com.rfelements.model.json.ligowave;

/**
 * @author Pichanič Ján
 */
public class Peer {

    private String deviceName;

    private Integer rxRate;

    private String ipAddress;

    private Integer[] signal;

    private String vap;

    private Integer rxCcqPercent;

    private Integer txRate;

    private String mac;

    private String peerIfName;

    private Double rxBytes;

    private String protocol;

    private Integer txCcqPercent;

    private Integer txRetryPercent;

    private Integer linkUptime;

    private Integer vapIfIndex;

    private Integer rxDropPercent;

    private Integer txPackets;

    private Integer wjet;

    private Integer rxPackets;

    private Double txBytes;

    private String security;

    private String ip6Address;

    public String getDeviceName() {
        return deviceName;
    }

    public void setDeviceName(String deviceName) {
        this.deviceName = deviceName;
    }

    public Integer getRxRate() {
        return rxRate;
    }

    public void setRxRate(Integer rxRate) {
        this.rxRate = rxRate;
    }

    public String getIpAddress() {
        return ipAddress;
    }

    public void setIpAddress(String ipAddress) {
        this.ipAddress = ipAddress;
    }

    public Integer[] getSignal() {
        return signal;
    }

    public void setSignal(Integer[] signal) {
        this.signal = signal;
    }

    public String getVap() {
        return vap;
    }

    public void setVap(String vap) {
        this.vap = vap;
    }

    public Integer getRxCcqPercent() {
        return rxCcqPercent;
    }

    public void setRxCcqPercent(Integer rxCcqPercent) {
        this.rxCcqPercent = rxCcqPercent;
    }

    public Integer getTxRate() {
        return txRate;
    }

    public void setTxRate(Integer txRate) {
        this.txRate = txRate;
    }

    public String getMac() {
        return mac;
    }

    public void setMac(String mac) {
        this.mac = mac;
    }

    public String getPeerIfName() {
        return peerIfName;
    }

    public void setPeerIfName(String peerIfName) {
        this.peerIfName = peerIfName;
    }

    public Double getRxBytes() {
        return rxBytes;
    }

    public void setRxBytes(Double rxBytes) {
        this.rxBytes = rxBytes;
    }

    public String getProtocol() {
        return protocol;
    }

    public void setProtocol(String protocol) {
        this.protocol = protocol;
    }

    public Integer getTxCcqPercent() {
        return txCcqPercent;
    }

    public void setTxCcqPercent(Integer txCcqPercent) {
        this.txCcqPercent = txCcqPercent;
    }

    public Integer getTxRetryPercent() {
        return txRetryPercent;
    }

    public void setTxRetryPercent(Integer txRetryPercent) {
        this.txRetryPercent = txRetryPercent;
    }

    public Integer getLinkUptime() {
        return linkUptime;
    }

    public void setLinkUptime(Integer linkUptime) {
        this.linkUptime = linkUptime;
    }

    public Integer getVapIfIndex() {
        return vapIfIndex;
    }

    public void setVapIfIndex(Integer vapIfIndex) {
        this.vapIfIndex = vapIfIndex;
    }

    public Integer getRxDropPercent() {
        return rxDropPercent;
    }

    public void setRxDropPercent(Integer rxDropPercent) {
        this.rxDropPercent = rxDropPercent;
    }

    public Integer getTxPackets() {
        return txPackets;
    }

    public void setTxPackets(Integer txPackets) {
        this.txPackets = txPackets;
    }

    public Integer getWjet() {
        return wjet;
    }

    public void setWjet(Integer wjet) {
        this.wjet = wjet;
    }

    public Integer getRxPackets() {
        return rxPackets;
    }

    public void setRxPackets(Integer rxPackets) {
        this.rxPackets = rxPackets;
    }

    public Double getTxBytes() {
        return txBytes;
    }

    public void setTxBytes(Double txBytes) {
        this.txBytes = txBytes;
    }

    public String getSecurity() {
        return security;
    }

    public void setSecurity(String security) {
        this.security = security;
    }

    public String getIp6Address() {
        return ip6Address;
    }

    public void setIp6Address(String ip6Address) {
        this.ip6Address = ip6Address;
    }
}

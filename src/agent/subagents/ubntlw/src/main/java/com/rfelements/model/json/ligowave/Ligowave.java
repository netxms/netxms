package com.rfelements.model.json.ligowave;

/**
 * @author Pichanič Ján
 */
public class Ligowave {

    private WirelessInformation wirelessInformation;

    private Status status;

    private RemoteInformation[] remoteInformation;

    private Network network;

    public WirelessInformation getWirelessInformation() {
        return wirelessInformation;
    }

    public void setWirelessInformation(WirelessInformation wirelessInformation) {
        this.wirelessInformation = wirelessInformation;
    }

    public Status getStatus() {
        return status;
    }

    public void setStatus(Status status) {
        this.status = status;
    }

    public RemoteInformation[] getRemoteInformation() {
        return remoteInformation;
    }

    public void setRemoteInformation(RemoteInformation[] remoteInformation) {
        this.remoteInformation = remoteInformation;
    }

    public Network getNetwork() {
        return network;
    }

    public void setNetwork(Network network) {
        this.network = network;
    }
}

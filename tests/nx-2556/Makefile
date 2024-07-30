SHELL := /bin/bash

DIRNAME = $(shell basename $$PWD)
VM_NAME_BASE ?= ${DIRNAME}
INCUS_DISTRO ?= ubuntu/24.04

.PHONY: all

all: all.created all.wg all.ovpn all.netxms.installed

_m2_settings.xml:
	echo Put your ~/.m2/settings.xml with Nexus credentials to $@
	false

check: all _m2_settings.xml
	incus exec nx-2556-gw -- mkdir -p /root/.m2
	incus file push _m2_settings.xml nx-2556-gw/root/.m2/settings.xml

	rsync -av -e "ssh -F ${VM_NAME_BASE}-gw.ssh_config" build-netxms-java netxms-build-tag.properties ${VM_NAME_BASE}-gw:
	incus exec ${VM_NAME_BASE}-gw -- apt install -V -y maven default-jdk
	incus exec ${VM_NAME_BASE}-gw -- git clone --branch master https://github.com/netxms/netxms.git netxms-java || true
	incus exec ${VM_NAME_BASE}-gw --cwd /root/netxms-java -- /root/build-netxms-java
	incus exec ${VM_NAME_BASE}-gw --cwd /root/netxms-java -- mvn -f tests/integration/pom.xml test -P integration-test -fae --debug

all.created:
	export VM_NAME=${VM_NAME_BASE}-aa; make $$VM_NAME.p.installed
	export VM_NAME=${VM_NAME_BASE}-bb; make $$VM_NAME.p.installed
	export VM_NAME=${VM_NAME_BASE}-gw; make $$VM_NAME.p.installed
	touch $@

all.wg:
	# copy to here all the pubkeys
	incus file pull -v ${VM_NAME_BASE}-aa//etc/wireguard/pubkey pubkey.aa
	incus file pull -v ${VM_NAME_BASE}-bb//etc/wireguard/pubkey pubkey.bb
	incus file pull -v ${VM_NAME_BASE}-gw//etc/wireguard/pubkey pubkey.gw
	incus file push -v pubkey.aa ${VM_NAME_BASE}-gw//etc/wireguard/
	incus file push -v pubkey.bb ${VM_NAME_BASE}-gw//etc/wireguard/
	incus file push -v pubkey.gw ${VM_NAME_BASE}-aa//etc/wireguard/
	incus file push -v pubkey.gw ${VM_NAME_BASE}-bb//etc/wireguard/
	incus file push -v ${VM_NAME_BASE}-gw.ipv4 ${VM_NAME_BASE}-aa//etc/wireguard/gw.ipv4
	incus file push -v ${VM_NAME_BASE}-gw.ipv4 ${VM_NAME_BASE}-bb//etc/wireguard/gw.ipv4
	incus file push -v ${VM_NAME_BASE}-gw.ipv6 ${VM_NAME_BASE}-aa//etc/wireguard/gw.ipv6
	incus file push -v ${VM_NAME_BASE}-gw.ipv6 ${VM_NAME_BASE}-bb//etc/wireguard/gw.ipv6
	# for each, generate the wg config
	incus file push ./generate-config-aa ${VM_NAME_BASE}-aa/root/
	incus file push ./generate-config-bb ${VM_NAME_BASE}-bb/root/
	incus file push ./generate-config-gw ${VM_NAME_BASE}-gw/root/
	incus exec ${VM_NAME_BASE}-aa -- ./generate-config-aa
	incus exec ${VM_NAME_BASE}-bb -- ./generate-config-bb
	incus exec ${VM_NAME_BASE}-gw -- ./generate-config-gw
	# start wg
	incus exec ${VM_NAME_BASE}-aa -- systemctl enable --now wg-quick@aa
	incus exec ${VM_NAME_BASE}-bb -- systemctl enable --now wg-quick@bb
	incus exec ${VM_NAME_BASE}-gw -- systemctl enable --now wg-quick@gw
	# Show interfaces status
	sleep 5
	for x in aa bb gw; do incus exec ${VM_NAME_BASE}-$$x -- wg; done
	touch $@

all.ovpn: all.wg
	for x in aa bb gw; do incus exec ${VM_NAME_BASE}-$$x -- apt install -V -y openvpn ; done

	echo $(shell cat ${VM_NAME_BASE}-gw.ipv6) gw my-server-1 | incus exec ${VM_NAME_BASE}-aa -- tee -a /etc/hosts
	incus exec ${VM_NAME_BASE}-aa -- cp /usr/share/doc/openvpn/examples/sample-config-files/client.conf /usr/share/doc/openvpn/examples/sample-keys/{ca.crt,client.crt,client.key,ta.key} /etc/openvpn
	incus exec ${VM_NAME_BASE}-aa -- systemctl enable --now openvpn@client

	echo $(shell cat ${VM_NAME_BASE}-gw.ipv4) gw my-server-1 | incus exec ${VM_NAME_BASE}-bb -- tee -a /etc/hosts
	incus exec ${VM_NAME_BASE}-bb -- cp /usr/share/doc/openvpn/examples/sample-config-files/client.conf /usr/share/doc/openvpn/examples/sample-keys/{ca.crt,client.crt,client.key,ta.key} /etc/openvpn
	incus exec ${VM_NAME_BASE}-bb -- systemctl enable --now openvpn@client

	incus exec ${VM_NAME_BASE}-gw -- cp /usr/share/doc/openvpn/examples/sample-config-files/server.conf /usr/share/doc/openvpn/examples/sample-keys/{ca.crt,server.crt,server.key,dh2048.pem,ta.key} /etc/openvpn
	incus exec ${VM_NAME_BASE}-gw -- sed -i -e 's/proto udp$$/proto udp6/' /etc/openvpn/server.conf # enable dualstack
	# allow clients with same key receive different address:
	incus exec ${VM_NAME_BASE}-gw -- sed -i -e 's/;duplicate-cn/duplicate-cn/' /etc/openvpn/server.conf
	incus exec ${VM_NAME_BASE}-gw -- systemctl enable --now openvpn@server

	for x in aa bb gw; do incus exec ${VM_NAME_BASE}-$$x -- ip -br -c addr; done
	for x in aa bb gw; do incus exec ${VM_NAME_BASE}-$$x -- ip -br -c route; done
	touch $@

all.netxms.installed: all.wg
	for x in aa bb gw; do incus exec ${VM_NAME_BASE}-$$x -- apt install -y rsync; done
	# caching the build for speedup
	rsync -av -e "ssh -F ${VM_NAME_BASE}-aa.ssh_config" netxms_build_cache/ ${VM_NAME_BASE}-aa:netxms || true
	export VM_NAME=${VM_NAME_BASE}-aa; make $$VM_NAME.netxms.installed
	rsync -av -e "ssh -F ${VM_NAME_BASE}-aa.ssh_config" ${VM_NAME_BASE}-aa:netxms/ netxms_build_cache

	rsync -av -e "ssh -F ${VM_NAME_BASE}-bb.ssh_config" netxms_build_cache/ ${VM_NAME_BASE}-bb:netxms || true
	export VM_NAME=${VM_NAME_BASE}-bb; make $$VM_NAME.netxms.installed
	rsync -av -e "ssh -F ${VM_NAME_BASE}-gw.ssh_config" netxms_build_cache/ ${VM_NAME_BASE}-gw:netxms || true
	export VM_NAME=${VM_NAME_BASE}-gw; make $$VM_NAME.netxms.installed
	touch $@

# For unattended SSH connection from hypervisor into guest
ssh_key:
	ssh-keygen -f ssh_key -N ''

${VM_NAME}.created:
	incus launch -p default -c security.privileged=false images:${INCUS_DISTRO} ${VM_NAME}
	touch $@

${VM_NAME}.access.configured: ${VM_NAME}.created ssh_key
	# Wait for ipv4 IP
	while incus list --format json ${VM_NAME} | jq --raw-output .[0].state.network.eth0.addresses[0].address | grep -q ':'; do echo -n .; sleep 1; done

	incus list --format json ${VM_NAME} | jq --raw-output .[0].state.network.eth0.addresses[0].address > ${VM_NAME}.ipv4
	incus list --format json ${VM_NAME} | jq --raw-output .[0].state.network.eth0.addresses[1].address > ${VM_NAME}.ipv6

	incus exec ${VM_NAME} -- apt install -y openssh-server
	# Ubuntu automatically enables the service, it is named `sshd` in systemd.
	ssh-keyscan -H `cat $(VM_NAME).ipv4` > ${VM_NAME}.known_hosts

	echo "Host ${VM_NAME} `cat $(VM_NAME).ipv4`"            > ${VM_NAME}.ssh_config
	echo "  Hostname `cat $(VM_NAME).ipv4`"                >> ${VM_NAME}.ssh_config
	echo "  IdentityFile ssh_key"                          >> ${VM_NAME}.ssh_config
	echo "  User root"                                     >> ${VM_NAME}.ssh_config
	echo "  GlobalKnownHostsFile=${VM_NAME}.known_hosts"   >> ${VM_NAME}.ssh_config

	incus exec ${VM_NAME} -- mkdir -p /root/.ssh
	incus file push -v ssh_key.pub ${VM_NAME}//root/.ssh/authorized_keys
	incus exec ${VM_NAME} -- chown -R root:root /root/.ssh
	incus exec ${VM_NAME} -- chmod u=rx,go= -R /root/.ssh
	touch $@

${VM_NAME}.p.installed: ${VM_NAME}.access.configured
	incus exec ${VM_NAME} -- apt update
	incus exec ${VM_NAME} -- apt install -V -y wireguard-tools
	incus exec ${VM_NAME} -- bash -c "cd /etc/wireguard && wg genkey | tee privkey | wg pubkey > pubkey"
	incus exec ${VM_NAME} -- apt install -V -y openvpn
	touch $@

${VM_NAME}.netxms.installed: ${VM_NAME}.access.configured
	rsync -av -e "ssh -F ${VM_NAME}.ssh_config" build-netxms netxms-build-tag.h ${VM_NAME}:
	incus exec ${VM_NAME} -- apt install -V -y git autoconf automake libtool bison flex libssl-dev libpcre3-dev rr gdb g++ gcc build-essential sqlite3
	incus exec ${VM_NAME} -- git clone --branch master https://github.com/netxms/netxms.git || true
	incus exec ${VM_NAME} -- bash -c "cd netxms && ~/build-netxms"
	incus exec ${VM_NAME} -- cp /usr/local/etc/{jenkins,}/nxagentd.conf
	incus exec ${VM_NAME} -- sed -i -e 's/ListenPort = /ListenPort = 4700/' /usr/local/etc/nxagentd.conf
	incus exec ${VM_NAME} -- sed -i -e 's:MasterServers = 127.0.0.1:MasterServers = 0.0.0.0/0:' /usr/local/etc/nxagentd.conf
	incus exec ${VM_NAME} -- ldconfig
	incus exec ${VM_NAME} -- nxagentd -d
	incus file push netxmsd.conf ${VM_NAME}/usr/local/etc/
	incus exec ${VM_NAME} -- nxdbmgr init
	echo "UPDATE users SET flags = 0 WHERE name = 'admin';" | incus exec ${VM_NAME} -- sqlite3 /usr/local/var/lib/netxms/netxms.db
	incus exec ${VM_NAME} -- netxmsd -d

	sleep 3
	incus exec ${VM_NAME} -- nxget -p4700 127.0.0.1 Net.Interfaces
	incus exec ${VM_NAME} -- nxget -p4700 127.0.0.1 Net.IP.RoutingTable
	incus exec ${VM_NAME} -- nxget -p4700 127.0.0.1 Net.Wireguard.Interfaces
	incus exec ${VM_NAME} -- nxget -p4700 127.0.0.1 Net.Wireguard.Peers
	touch $@

clean:
	incus delete --force ${VM_NAME_BASE}-aa || true
	incus delete --force ${VM_NAME_BASE}-bb || true
	incus delete --force ${VM_NAME_BASE}-gw || true
	rm -f *.created
	rm -f *.ipv4 *.ipv6
	rm -f *.known_hosts
	rm -f *.ssh_config
	rm -f ssh_key ssh_key.pub
	rm -f *.access.configured
	rm -f *.installed
	rm -f all.networked all.wg all.ovpn
	rm -f pubkey.*

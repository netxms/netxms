# NetXMS - Performance Data Storage Driver (PDS)

```
Armadillo, The Metric Eater - https://www.nationalgeographic.com/animals/mammals/group/armadillos/
              ,.-----__
          ,:::://///,:::-.
         /:''/////// ``:::`;/|/
        /'   ||||||     :://'`\
      .' ,   ||||||     `/(  e \
-===~__-'\__X_`````\_____/~`-._ `.
            ~~        ~~       `~-'
```

## List of stuff to add and optimize:

- [X] Optimize the UDP sending (Package batching)
- [X] Review and optimize the usage of all the strings
- [X] Better error handling
- [X] Standardize the metric name normalization and cleaning(Create a function)
- [X] Handling shutdown correcly (Transmit last batch)
- [ ] Parameterize the settings Host (pdsdrv.influxdb.host), Port (pdsdrv.influxdb.port) and BatchSize (pdsdrv.influxdb.batchSize)



## Setup Development Lab

- ### Requirements
 - Install the following build deps (CentOS)
```bash
wget --no-check-certificate --no-cookies --header "Cookie: oraclelicense=accept-securebackup-cookie" https://download.oracle.com/otn-pub/java/jdk/8u202-b08/1961070e4c9b4e26a04e7f5a083f551e/jdk-8u202-linux-x64.rpm
rpm -i jdk-8u202-linux-x64.rpm
yum groupinstall "Development Tools"
yum install maven.noarch
yum install https://download.postgresql.org/pub/repos/yum/11/redhat/rhel-7-x86_64/pgdg-centos11-11-2.noarch.rpm
yum install postgresql11-devel openssl-devel.x86_64 libcurl-devel.x86_64 libssh-devel.x86_64 openldap-devel.x86_64
yum install centos-release-scl
yum install devtoolset-8-gcc devtoolset-8-gcc-c++

echo "source /opt/rh/devtoolset-8/enable" >>~/.bashrc
echo -e "* soft core 1\n* hard core 1" >> /etc/security/limits.conf
```

 - Build the NetXMS + PDS Driver

```bash
# Clone code
mkdir ~/dev/
git https://github.com/netxms/netxms.git ~/dev/netxms.src
cd ~/dev/netxms.src/

# Build + Install NetXMS
./reconf
export JAVA_HOME=/usr/java/jdk1.8.0_202-amd64
CPPFLAGS="-I/usr/pgsql-11/include -I/usr/java/jdk1.8.0_202-amd64/include -I/usr/java/jdk1.8.0_202-amd64/include/linux" LDFLAGS=-L/usr/pgsql-11/lib ./configure --with-server --with-snmp --with-client --with-client-proxy --with-agent --with-pgsql --with-openssl --enable-ldap --enable-systemd --with-internal-zlib --disable-mqtt --with-jdk --with-sdk --enable-debug --prefix=$HOME/dev/netxms.bin
make
make install

# Build + Install PDS Driver
cd ~/dev/netxms.src/src/server/pdsdrv/influxdb
make
/bin/cp .libs/influxdb.so ~/dev/netxms.bin
```

 - Configure and start NetXMS

```bash
cd ~/dev/netxms.bin
cat > netxmsd.conf <<EOF
DBDriver = sqlite.ddr
LogFailedSQLQueries = yes
LogFile = $HOME/dev/netxms.bin/netxmsd.log
PerfDataStorageDriver = $HOME/dev/netxms.bin/influxdb.so
EOF

# Init DB
bin/nxdbmgr -c $PWD/netxmsd.conf init

# Start NetXMS
bin/netxmsd -c $PWD/netxmsd.conf -D3

# Enable PDS Driver debuging
debug pdsdrv.influxdb 4-8

# Core dump will be found under
~/dev/netxms.bin/var/lib/netxms/
```

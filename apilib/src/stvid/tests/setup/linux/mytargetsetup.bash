#!/bin/bash

# mytargetsetup.bash                    FQ, 25 July 2007
# This script must be adapted to user's environnement
# ***************************************************

echo "Set my personnal target paths..."

export MY_TARGETDIR=$LINUX_SH4/<TBD>        # example "$LINUX_SH4/target_cyril"
export MY_ROOTFS_DIR=$MY_TARGETDIR/<TBD>    # example "$MY_TARGETDIR/root/Cyril"

echo "Set IP addresses..."

MY_JEI_IP=<TBD>.<TBD>.<TBD>.<TBD>		# Name of the JEI, HTI or MicroConnect
MY_TARGETIP=<TBD>.<TBD>.<TBD>.<TBD>		# IP address to be given to the target
MY_GWIP=<TBD>.<TBD>.<TBD>.<TBD>			# IP address of your network gateway
MY_NETMASK=<TBD>.<TBD>.<TBD>.<TBD>		# Local network subnet mask
MY_SERVERIP=<TBD>.<TBD>.<TBD>.<TBD>		# IP address of NFS server (gnx2431)
MY_MAC_ADDRESS=<TBD>:<TBD>:<TBD>:<TBD>:<TBD must be hex equivalent of last but one of MY_TARGETIP>:<TBDmust be hex equivalent of last of MY_TARGETIP>


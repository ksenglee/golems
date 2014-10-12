#!/bin/sh

CHANNELS="ref-left state-left"
#CHANNELS="$CHANNELS ref-right state-right"
#CHANNELS="$CHANNELS sdhref-left sdhstate-left"
#CHANNELS="$CHANNELS sdhref-right sdhstate-right"

#*****************
# DEFINITIONS
#*****************

# SocketCAN interfaces
CAN_L=can0 # Left arm
CAN_R=can1 # Right arm
CAN_T=can2 # Torso

# esdCAN interfaces
CAN_SDH_L=0 # Left hand
CAN_SDH_R=1 # Right hand


# Check where SNS is located
if test -z $SNS; then
    if test -x /home/ana/local/etc/init.d/sns; then
	SNS=/home/ana/local/etc/init.d/sns
    elif test -x /usr/local/etc/init.d/sns; then
	SNS=/usr/local/etc/init.d/sns
    elif test -x /etc/init.d/sns; then
	SNS=/etc/init.d/sns
    else 
	echo "[ERROR] Could not find SNS program"
	exit 1
    fi

#**************
# FUNCTIONS
#**************

# Make channels
crichton_ach_mk() {
    for c in $CHANNELS; do
	ach mk -1 -o 666 $c
    done
}

# Remove channels
crichton_ach_rm() {
    for c in $CHANNELS; do
	ach rm $c
    done
}

# SNS service start
crichton_start_sns() {
    snslogd_is_running=${pgrep snslogd}

    if [ -z $snslogd_is_running ]
	then
	echo "\t * Starting SNS service"
	sudo service $SNS start
    else 
	echo "\t * SNS already is running"
    fi
}

# Start : Create channels + start SNS + run daemons
crichton_start() {
    crichton_ach_mk
    crichton_start_sns

    $SNS run -d -r lwa-left -- \
	can402 -f $CAN_L -R 1 -C 0 -n 3 -n 4 -n 5 -n 6 -n 7 -n 8 -n 9 -c ref-left -s state-left -v
#    $SNS run -d -r lwa-right -- \
#	can402 -f $CAN_R -R 1 -C 0 -n 3 -n 4 -n 5 -n 6 -n 7 -n 8 -n 9 -c ref-right -s state-right -v    
}

# Expunge: Remove temporal folders for log
crichton_expunge() {
    sudo rm -rf /var/tmp/sns/lwa-left
    sudo rm -rf /var/run/sns/lwa-left

#    sudo rm -rf /var/tmp/sns/lwa-right
#    sudo rm -rf /var/run/sns/lwa-right
}


# Stop: Stop daemons and programs
crichton_stop() {
    $SNS kill lwa-left
#    $SNS kill lwa-right
}

# Change ownerships of SNS temporal files
crichton_steal() {
    chown -R $1 /var/run/sns/lwa-left \
	/var/tmp/sns/lwa-left #\
#	/var/run/sns/lwa-right \
#	/var/tmp/sns/lwa-right
}

#***************************
# SET SCRIPT OPTIONS
#***************************
case "$1" in
    start)
	crichton_start
	;;
    stop)
	crichton_stop
	;;
    rm) 
	crichton_ach_rm
	;;
    mk)
	crichton_ach_mk
	;;
    steal)
	shift
	crichton_steal $@
	;;
    expunge)
	crichton_expunge
	;;
    *)
	echo "[ERROR] Invalid command. Options are start/stop/rm/mk/steal NEW_OWNER/expunge"
	exit 1
	;;
esac


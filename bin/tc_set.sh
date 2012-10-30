#!/bin/bash

DEV_OUT="virbr1"
#DEV_OUT="eth1"
DEV_IN="ifb0"

NETEM_DELAY="10000" # microseconds
NETEM_DELAY_JITTER="5000" # microseconds
NETEM_LOSS="0" # percentage
NETEM_DUPLICATE="0" # percentage
NETEM_LIMIT="100" # packets

TBF_RATE="256" # kbit/second
TBF_BURST="1600" # bytes
TBF_LIMIT="3000" # bytes
TBF_LATENCY="10" # microseconds

# is module ifb loaded?
lsmod | grep ifb > /dev/null
if [ $? -ne 0 ]
then
        # load ifb module
        modprobe ifb
        # set up ifb link
        ip link set dev ${DEV_IN} up
        # redirect incoming trafic to the the ifb pseudo-interface
        tc qdisc add dev ${DEV_OUT} ingress
        tc filter add dev ${DEV_OUT} parent ffff: protocol ip u32 match u32 0 0 flowid 1:1 action mirred egress redirect dev ${DEV_IN}
fi

# delete old qdisc for outgoing traffic
tc qdisc del dev ${DEV_OUT} parent root

# add tbf filter for output device
if [ ${TBF_RATE} -gt 0 ]
then
        tc qdisc add dev "${DEV_OUT}" parent root \
                handle 1:0 \
                tbf \
                rate ${TBF_RATE}kbit \
                burst ${TBF_BURST} \
                limit ${TBF_LIMIT}
                # latency ${TBF_LATENCY} \
fi

# delete old qdisc for input device
tc qdisc del dev ${DEV_IN} parent root

# add netem filter for input device
if [ ${NETEM_DELAY} -gt 0 ] && [ ${NETEM_DELAY_JITTER} -gt 0 ]
then
        tc qdisc add dev ${DEV_IN} parent root \
                handle 1:0 \
                netem \
                limit ${NETEM_LIMIT} \
                delay ${NETEM_DELAY}us ${NETEM_DELAY_JITTER}us \
                distribution normal \
                loss ${NETEM_LOSS}% \
                duplicate ${NETEM_DUPLICATE}%
fi

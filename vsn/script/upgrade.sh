#!/bin/bash

./script/sendToBBB.sh debian@192.168.200.55
./script/sendToBBB.sh debian@192.168.200.56
./script/sendToBBB.sh debian@192.168.200.61
./script/sendToBBB.sh debian@192.168.200.62

./script/restartBBB.sh debian@192.168.200.55
./script/restartBBB.sh debian@192.168.200.56
./script/restartBBB.sh debian@192.168.200.61
./script/restartBBB.sh debian@192.168.200.62

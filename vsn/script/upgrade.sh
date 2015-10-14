#!/bin/bash

clear

echo "Uploading to camera 1"
./script/sendToBBB.sh debian@192.168.200.55
echo "Uploading to camera 2"
./script/sendToBBB.sh debian@192.168.200.56
echo "Uploading to cooperator 1"
./script/sendToBBB.sh debian@192.168.200.61
echo "Uploading to cooperator 2"
./script/sendToBBB.sh debian@192.168.200.62

echo "- - - - - - - - - - - -"

echo "Restarting camera 1"
./script/restartBBB.sh debian@192.168.200.55
echo "Restarting camera 2"
./script/restartBBB.sh debian@192.168.200.56
echo "Restarting cooperator 1"
./script/restartBBB.sh debian@192.168.200.61
echo "Restarting cooperator 2"
./script/restartBBB.sh debian@192.168.200.62

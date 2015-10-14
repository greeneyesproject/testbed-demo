#!/bin/bash

rsync -qaze "ssh -q" pklot obj oneshot build-bbb/greeneyes-vsn util script/greeneyes network_config.xml $1:/opt/greeneyes-vsn/

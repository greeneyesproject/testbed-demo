#!/bin/bash

rsync -avz pklot oneshot build-bbb/greeneyes-vsn util script/greeneyes network_config.xml $1:/opt/greeneyes-vsn/

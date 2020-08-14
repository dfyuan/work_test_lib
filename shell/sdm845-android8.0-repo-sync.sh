#!/bin/bash

#step 0 mkdir a project path
#step 1 get repo tools
git clone git@10.0.16.12:research/firmware/rockchip/rktools/rkrepo.git

#step 2 get sdm845 opensource android manifest.xml
./rkrepo/repo init -u ssh://git@10.0.12.245/sdm845_android_o/platform/manifest -b master -m manifest.xml

#step 3 sync source code
.repo/repo/repo sync

#step 4 create new branches
.repo/repo/repo start --all sunny_master
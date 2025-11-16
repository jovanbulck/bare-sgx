#!/bin/bash

echo adding 9p mount command to $1/etc/fstab
pwd
mkdir -p $1/host
echo "host0   /host    9p      trans=virtio,version=9p2000.L   0 0" >> $1/etc/fstab

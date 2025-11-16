#!/bin/bash

KERNEL=bzImage
#KERNEL=buildroot/output/build/linux-6.12.27/arch/x86/boot/bzImage
ROOTFS=rootfs.ext2
SHARED_DIR=$(pwd)/../app/

if [ ! -e $ROOTFS ]; then
  unzip rootfs.zip
fi

qemu-system-x86_64 -kernel $KERNEL \
                   -drive format=raw,file=$ROOTFS,if=virtio \
                   -enable-kvm \
                   -cpu host,+sgx \
                   -object memory-backend-epc,id=mem1,size=64M,prealloc=on \
                   -M sgx-epc.0.memdev=mem1,sgx-epc.0.node=0 \
                   -nographic \
                   -device edu,bus=pci.0,addr=4.0 \
                   -net nic,model=virtio -net user \
                   -virtfs local,path=$SHARED_DIR,mount_tag=host0,security_model=mapped,id=host0 \
                   -append "root=/dev/vda console=ttyS0"

# exit with CTRL-A X

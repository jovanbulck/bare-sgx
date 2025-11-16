KERNEL_CONFIG=$(shell pwd)/kvm_guest.config
ROOTFS_OVERLAY_DIR=$(shell pwd)/rootfs_overlay/
ROOTFS_POST_BUILD_SCRIPT=$(shell pwd)/rootfs_post_build_script.sh
ROOTFS_SIZE="512M"

BR_DIR=buildroot/
BR_CONFIG=$(BR_DIR)/utils/config --file $(BR_DIR)/.config

rootfs:
	$(MAKE) -C $(BR_DIR) qemu_x86_64_defconfig
	$(BR_CONFIG) --enable  PACKAGE_NANO
	$(BR_CONFIG) --enable  PACKAGE_VIM
	$(BR_CONFIG) --enable  PACKAGE_OPENSSL
	$(BR_CONFIG) --set-str ROOTFS_OVERLAY "$(ROOTFS_OVERLAY_DIR)"
	$(BR_CONFIG) --enable  LINUX_KERNEL_USE_CUSTOM_CONFIG
	$(BR_CONFIG) --set-str LINUX_KERNEL_CONFIG_FRAGMENT_FILES "$(KERNEL_CONFIG)"
	$(BR_CONFIG) --set-str ROOTFS_POST_BUILD_SCRIPT "board/qemu/x86_64/post-build.sh $(ROOTFS_POST_BUILD_SCRIPT)"
	$(BR_CONFIG) --set-str TARGET_ROOTFS_EXT2_SIZE $(ROOTFS_SIZE)
	$(MAKE) -C $(BR_DIR) olddefconfig
	$(MAKE) -C $(BR_DIR) BR2_JLEVEL=`nproc` all
	cp $(BR_DIR)/output/images/bzImage .
	cp $(BR_DIR)/output/images/rootfs.ext2 .
	zip rootfs.zip rootfs.ext2

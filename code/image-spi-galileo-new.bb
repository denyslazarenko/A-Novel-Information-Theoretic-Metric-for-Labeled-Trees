require image-spi.bb

IMAGE_INSTALL += "galileo-target"
IMAGE_INSTALL += "mtd-utils-jffs2"
IMAGE_INSTALL += "quark-init"

ROOTFS_POSTPROCESS_COMMAND += "install_sketch ; "

SKETCH_FILE = "/home/osboxes/sketch.elf"

install_sketch() {
        install -d ${IMAGE_ROOTFS}/sketch
	install -m 0777 ${SKETCH_FILE} ${IMAGE_ROOTFS}/sketch/
}

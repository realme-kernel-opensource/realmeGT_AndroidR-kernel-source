ifeq ($(CONFIG_BUILD_ARM64_DT_OVERLAY),y)
dtbo-$(CONFIG_ARCH_LAHAINA) += \
            lemonadep-19815-overlay.dtbo \
            lemonadep-19815-overlay-t0.dtbo \
            lemonade-19825-overlay.dtbo \
            lemonadev-2080a-overlay.dtbo \
            fussi-20031-lahaina-overlay.dtbo \
            fussi-20031-lahaina-v2.1-overlay.dtbo \
            cupid-20627-lahaina-overlay.dtbo \
            cupid-20627-lahaina-v2.1-overlay.dtbo \
            riven-20659-lahaina-overlay.dtbo \
            riven-20659-lahaina-v2.1-overlay.dtbo

lemonadep-19815-overlay.dtbo-base := lahaina.dtb lahaina-v2.dtb lahaina-v2.1.dtb
lemonadep-19815-overlay-t0.dtbo-base := lahaina.dtb lahaina-v2.dtb lahaina-v2.1.dtb
lemonade-19825-overlay.dtbo-base := lahaina.dtb lahaina-v2.dtb lahaina-v2.1.dtb
lemonadev-2080a-overlay.dtbo-base := lahaina.dtb lahaina-v2.dtb lahaina-v2.1.dtb
fussi-20031-lahaina-overlay.dtbo-base := fussi-lahaina.dtb fussi-lahaina-v2.dtb fussi-lahaina-v2.1.dtb
fussi-20031-lahaina-v2.1-overlay.dtbo-base := fussi-lahaina.dtb fussi-lahaina-v2.dtb fussi-lahaina-v2.1.dtb
cupid-20627-lahaina-overlay.dtbo-base := cupid-lahaina.dtb cupid-lahaina-v2.dtb cupid-lahaina-v2.1.dtb
cupid-20627-lahaina-v2.1-overlay.dtbo-base := cupid-lahaina.dtb cupid-lahaina-v2.dtb cupid-lahaina-v2.1.dtb
riven-20659-lahaina-overlay.dtbo-base := riven-lahaina.dtb riven-lahaina-v2.dtb riven-lahaina-v2.1.dtb
riven-20659-lahaina-v2.1-overlay.dtbo-base := riven-lahaina.dtb riven-lahaina-v2.dtb riven-lahaina-v2.1.dtb
else
dtb-$(CONFIG_ARCH_LAHAINA) += 
	lemonadep-overlay.dtb
endif

always		:= $(dtb-y)
subdir-y	:= $(dts-dirs)
clean-files	:= *.dtb *.dtbo

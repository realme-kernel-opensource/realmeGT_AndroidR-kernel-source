obj-$(CONFIG_OPPO_FINGERPRINT_COMMON) += oppo_fp_common/

### Capacitive fingerprint sensor
obj-$(CONFIG_OPPO_FINGERPRINT_FPC) += fpc/

### Optical fingerprint sensor
obj-$(CONFIG_OPPO_FINGERPRINT_GOODIX_OPTICAL) += goodix_optical_fp/
obj-$(CONFIG_OPPO_FINGERPRINT_SILEAD_OPTICAL) += silead_optical_fp/
obj-$(CONFIG_OPPO_FINGERPRINT_EGIS_OPTICAL)   += egis_optical_fp/

### FEATURE MACRO
ccflags-$(CONFIG_OPPO_FINGERPRINT_NAVIGATION_ENABLE) += -DFP_NAVIGATION_ENABLE

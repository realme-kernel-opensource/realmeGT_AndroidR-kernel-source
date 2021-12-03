vendor := $(srctree)/$(src)

#ifndef VENDOR_EDIT
#/*Fei.Mo@BSP.kernel.Stability, 2019/07/11, Add for spllit dtbo*/
#ifneq "$(wildcard $(vendor)/qcom)" ""
#	subdir-y += qcom
#endif
#else
#	subdir-y += 20031
	subdir-y += qcom
	subdir-y += oplus
#endif

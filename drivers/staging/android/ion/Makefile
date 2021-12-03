# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_ION)	+= ion.o ion_buffer.o ion_dma_buf.o ion_heap.o
CFLAGS_ion_buffer.o 	= -I$(src)
obj-y			+= heaps/
#ifdef OPLUS_FEATURE_HEALTHINFO
#Huacai.Zhou@PSW.BSP.kernel.Peformance, 2018/06/18, add foreground process opt support
obj-$(CONFIG_OPLUS_HEALTHINFO) 	+= oplus_ion/
#endif /* OPLUS_FEATURE_HEALTHINFO */

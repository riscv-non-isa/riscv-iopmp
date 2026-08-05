libiopmp-objs-y =
libiopmp-objs-y += libiopmp.o
libiopmp-objs-y += iopmp_drv_common.o
libiopmp-objs-y += iopmp_drivers.carray.o

libiopmp-objs-$(CFG_IOPMP_DRV_VENDOR_EXAMPLE) += iopmp_drv_vendor_example.o
carray-iopmp_drivers-$(CFG_IOPMP_DRV_VENDOR_EXAMPLE) += iopmp_drv_vendor_example

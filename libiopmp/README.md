# libiopmp - A Library to Program RISC-V IOPMP

The `libiopmp` is intended to be driver of RISC-V IOPMP which:
* Complies with IOPMP specification **v0.8.2, 2026**
* Operates one or multiple IOPMPs
* Supports several IOPMP models and configurations
* Extensible for adding vendor-customized IOPMP driver
* Supports IOPMP with Multi-Faults Record (MFR) extension
* Supports IOPMP with Secondary Permission Setting (SPS) extension
* Supports IOPMP with Message-Signaled Interrupts (MSI) extension

## Adjust `config.mk`

The `libiopmp` has a `config.mk` configuration file which let you modularize
your `libiopmp` to reduce the code size. We describe each of the configurations
here:

* `DEBUG`: Turn on this option to build `libiopmp` without compiler optimization
and assert() macro will be enabled
* `CFG_IOPMP_REF_MODEL`: Turn on this option to enable compiling of register
read/write interface as weak functions. This is useful if the IOPMP you operate
is simulated by the reference model. If you want to control real IOPMP you just
turn off this option.
* `CFG_IOPMP_DRV_VENDOR_EXAMPLE`: Turn on this option to build the worked
example of a vendor driver in `src/iopmp_drv_vendor_example.c`. It is not part
of the default build; see [Vendor drivers](#vendor-drivers) below

## Compilation

`libiopmp` can be built by host compiler or RISC-V toolchain. The former one is
useful when testing `libiopmp` using the reference model, while the later one is
necessary if you want to use `libiopmp` on RISC-V platforms.

### Compiled by Host GCC

```shell
~/libiopmp$ make
 CC        libiopmp.o
 CC        iopmp_drv_common.o
 CARRAY    iopmp_drivers.carray.c
 CC        iopmp_drivers.carray.o
 AR        lib/libiopmp.a
```

### Compiled by RISC-V toolchain

To compiled `libiopmp` by RISC-V toolchain, you need to add the "path to your
RISC-V toolchain" into `$PATH` environment variable, and input the following
command:

For RV32 target:
```shell
~/libiopmp$ export CROSS_COMPILE=riscv32-unknown-elf-
~/libiopmp$ make
```

For RV64 target:
```shell
~/libiopmp$ export CROSS_COMPILE=riscv64-unknown-elf-
~/libiopmp$ make
```

## Usage

Assume the directory path to libiopmp is `$(LIBIOPMP_DIR)`, the output library
archive will be `$(LIBIOPMP_DIR)/build/lib/libiopmp.a`. All the data structures
and the APIs are declared in `$(LIBIOPMP_DIR)/include/libiopmp.h` header file.

Add the path to the library and header file into your build system. Assume
`CFLAGS` represents the compiler flags and `LDFLAGS` represents the linker
flags, please add the path to `libiopmp.h` into `CFLAGS` and `libiopmp.a` into
`LDFLAGS` accordingly:

```bash
CFLAGS  += -I$(LIBIOPMP_DIR)/include
LDFLAGS += $(LIBIOPMP_DIR)/build/lib/libiopmp.a
```

Then, including the `libiopmp.h` into your program and using the APIs to
operate your IOPMP:

```c
#include "libiopmp.h"
```

## Documentation

Please check the `libiopmp.pdf` under `docs` folder. 

## Vendor drivers

`libiopmp` implements every operation itself and calls it by name, so the nine
models the specification defines need no driver: pass their `SRCMD_FMT` and
`MDCFG_FMT` to `iopmp_init()` and it initializes them directly.

Write a driver when the hardware disagrees with the specification `libiopmp`
targets, which in practice means silicon built against an older draft of it. A
driver is a `struct iopmp_driver` that claims an implementation ID, plus a
table holding only the operations that differ:

```c
static const struct iopmp_operations_override vendor_example_ops = {
    .lock_entries = vendor_example_lock_entries,
};

static enum iopmp_error vendor_example_init(IOPMP_t *iopmp, uintptr_t addr)
{
    return iopmp_drv_init_common(iopmp, addr,
                                 iopmp_drv_vendor_example.srcmd_fmt,
                                 iopmp_drv_vendor_example.mdcfg_fmt,
                                 &vendor_example_ops);
}

const struct iopmp_driver iopmp_drv_vendor_example = {
    .srcmd_fmt = IOPMP_SRCMD_FMT_0,
    .mdcfg_fmt = IOPMP_MDCFG_FMT_0,
    .impid = VENDOR_EXAMPLE_IMPID,
    .init = vendor_example_init,
};
```

An operation left `NULL`, and an instance whose `ops_override` is `NULL`, keep
the built-in implementation, so the table names what differs and nothing else.
The choice is made per instance, so a conforming IOPMP elsewhere in the same
system still reaches the built-in operations.

`iopmp_init()` matches the driver on the implementation ID the application
passes, so an application drives both by passing `VENDOR_EXAMPLE_IMPID` for one
instance and `IOPMP_IMPID_NOT_SPECIFIED` for the other.

`src/iopmp_drv_vendor_example.c` is the whole of the above as compilable code.
Copy it, rename it, and register it in `src/objects.mk` the way that one is.

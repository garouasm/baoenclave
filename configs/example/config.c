#include <config.h>

VM_IMAGE(base_image, "../linux/lloader/linux.bin");
VM_IMAGE(top_image,
         "../bao-baremetal-guest/build/qemu-aarch64-virt/baremetal.bin");

vm_config_t top = {

    .image = {.base_addr = 0x00000000,
              .load_addr = VM_IMAGE_OFFSET(top_image),
              .size = VM_IMAGE_SIZE(top_image)},

    .entry = 0x00000000,
    .cpu_affinity = 0x3,

    .platform = {
        .cpu_num = 1,

        .region_num = 1,
        .regions =
            (struct mem_region[]){{.base = 0x00000000, .size = 0x4000000}},

        .dev_num = 2,
        .devs = (struct dev_region[]){{
                                          /* UART2, PL011 */
                                          .pa = 0x9000000,
                                          .va = 0xFF010000,
                                          .size = 0x10000,
                                          // .interrupt_num = 1,
                                          // .interrupts =
                                          //     (uint64_t[]) {33}
                                      },
                                      {.interrupt_num = 1,
                                       .interrupts = (uint64_t[]){27}}},

        .arch = {.gic = {
                     .gicd_addr = 0xF9010000,
                     .gicr_addr = 0xF9020000,
                 }}}};

vm_config_t base = {

    .image = {.base_addr = 0x00000000,
              .load_addr = VM_IMAGE_OFFSET(base_image),
              .size = VM_IMAGE_SIZE(base_image)},

    .entry = 0x00000000,
    .cpu_affinity = 0x3,

    .children_num = 1,
    .children = (vm_config_t*[]){&top},

    .platform = {
        .cpu_num = 1,

        .region_num = 1,
        .regions =
            (struct mem_region[]){{.base = 0x00000000, .size = 0x4000000}},

        .dev_num = 2,
        .devs = (struct dev_region[]){{/* UART2, PL011 */
                                       .pa = 0x9000000,
                                       .va = 0xFF010000,
                                       .size = 0x10000,
                                       .interrupt_num = 1,
                                       .interrupts = (uint64_t[]){33}},
                                      {.interrupt_num = 1,
                                       .interrupts = (uint64_t[]){27}}},

        .arch = {.gic = {
                     .gicd_addr = 0xF9010000,
                     .gicr_addr = 0xF9020000,
                 }}}};

struct config config = {

    CONFIG_HEADER

        .vmlist_size = 1,
    .vmlist = {&base},
};

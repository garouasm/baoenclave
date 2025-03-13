del break

add-symbol-file ../bao-baremetal-guest2/build/qemu-aarch64-virt/baremetal.elf
add-symbol-file ./bin/qemu-aarch64-virt/builtin-configs/enclave/bao.elf

break main
commands
    record full
    break lower_el_aarch64_sync
end

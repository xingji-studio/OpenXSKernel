#pragma once

#include <efi/boot.h>
#include <fs/vfs/vfs.h>
#include <installer_protocol.h>

bool installer_boot_active(const BOOT_CONFIG &boot_config);
int  installer_prepare_root(const BOOT_CONFIG &boot_config);
bool installer_root_is_tmpfs_ready();
const BOOT_CONFIG *installer_current_boot_config();
vfs_node_t installer_base_root();

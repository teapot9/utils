# utils

This repository contains personnal utilities.

 * **remount**: Allow to remount read-write or read-only filesystems.
   It uses suid to allow remounting read-only for all users.
 * **snapshot**: Manages snapshots of different filesystems.
   Handles BTRFS and XFS snapshots.
   Provides a rsync script to sync two directories.
 * **chrootmgmt**: Helper script to manage chroot environments on a system.
   The script can create/destroy chroot environments. Rollback the chroot
   to a defines "base" state. Define curent chroot state as "base" state.
   And enter the chroot and bind-mount overlays.


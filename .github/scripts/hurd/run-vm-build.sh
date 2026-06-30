#!/bin/bash
#
# Host-side orchestration for the GNU/Hurd build (runs on the ubuntu runner).
# There is no vmactions/hurd-vm, so this hand-rolls the VM: fetch Debian's
# pre-installed GNU/Hurd amd64 image, inject an ssh key + serial console, boot
# it in QEMU (rumpdisk → needs q35 + an AHCI disk; gnumach hangs on -smp>1),
# retry the boot up to 3x (VM boot can flake transiently), then build the
# xserver inside via run-xserver-build.sh. $REPO and $SHA come from the env.
set -euo pipefail

IMG_URL="${IMG_URL:-https://cdimage.debian.org/cdimage/ports/latest/hurd-amd64/debian-hurd.img.tar.gz}"
HERE="$(cd "$(dirname "$0")" && pwd)"

echo "==> fetch + extract image"
wget -q -O hurd.img.tar.gz "$IMG_URL"
tar -xzf hurd.img.tar.gz
img=$(echo debian-hurd-*.img)
ls -la "$img"

echo "==> inject ssh key + route guest console to serial"
ssh-keygen -t ed25519 -N '' -f hurd_key
loop=$(sudo losetup -fP --show "$img")
mkdir -p mnt
mounted=""
for part in "${loop}"p*; do
    if sudo mount -t ext2 "$part" mnt 2>/dev/null; then
        if [ -f mnt/etc/passwd ]; then mounted="$part"; break; fi
        sudo umount mnt
    fi
done
test -n "$mounted"
sudo mkdir -p mnt/root/.ssh
sudo cp hurd_key.pub mnt/root/.ssh/authorized_keys
sudo chmod 700 mnt/root/.ssh
sudo chmod 600 mnt/root/.ssh/authorized_keys
sudo chown -R 0:0 mnt/root/.ssh
if [ -f mnt/etc/ssh/sshd_config ]; then
    sudo sed -i 's/^#*PermitRootLogin.*/PermitRootLogin prohibit-password/' mnt/etc/ssh/sshd_config
    sudo sed -i 's/^#*PubkeyAuthentication.*/PubkeyAuthentication yes/' mnt/etc/ssh/sshd_config
fi
# GRUB + gnumach log to VGA; route to serial so the boot is visible on -nographic.
if [ -f mnt/boot/grub/grub.cfg ]; then
    sudo sed -i '1i serial --unit=0 --speed=115200\nterminal_input console serial\nterminal_output console serial' mnt/boot/grub/grub.cfg
    sudo sed -i '/^[[:space:]]*multiboot/ s/$/ console=com0/' mnt/boot/grub/grub.cfg
fi
sudo umount mnt
sudo losetup -d "$loop"

# /dev/kvm exists on GH runners but the runner user can't access it by default.
accel="-accel tcg"
if [ -e /dev/kvm ] && sudo chmod 666 /dev/kvm 2>/dev/null; then accel="-enable-kvm"; fi
echo "==> qemu accel: $accel"

SSH="ssh -i hurd_key -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=8 -p 2222 root@127.0.0.1"

qpid=""
cleanup() { [ -n "$qpid" ] && kill "$qpid" 2>/dev/null || true; }
trap cleanup EXIT

boot_to_ssh() {
    qemu-system-x86_64 $accel -M q35 -m 4G \
        -drive id=disk,file="$img",cache=writeback,format=raw,if=none \
        -device ich9-ahci,id=ahci \
        -device ide-hd,bus=ahci.0,drive=disk \
        -net user,hostfwd=tcp:127.0.0.1:2222-:22 \
        -net nic,model=e1000 \
        -nographic > qemu.log 2>&1 &
    qpid=$!
    sleep 8
    kill -0 "$qpid" 2>/dev/null || { echo "qemu exited immediately:"; cat qemu.log; return 1; }
    for i in $(seq 1 40); do
        kill -0 "$qpid" 2>/dev/null || { echo "qemu died during boot:"; tail -80 qemu.log; return 1; }
        if $SSH true 2>/dev/null; then return 0; fi
        [ $((i % 6)) -eq 0 ] && { echo "--- boot console after $i tries ---"; tail -20 qemu.log || true; }
        sleep 10
    done
    echo "ssh never came up:"; tail -80 qemu.log
    return 1
}

up=""
for attempt in 1 2 3; do
    echo "==> VM boot attempt $attempt/3"
    if boot_to_ssh; then up=1; break; fi
    echo "==> boot attempt $attempt failed; killing qemu and retrying"
    [ -n "$qpid" ] && kill "$qpid" 2>/dev/null || true
    qpid=""
    sleep 5
done
[ -n "$up" ] || { echo "::error::Hurd VM never booted to ssh after 3 attempts"; exit 1; }

echo "==> running build inside the VM"
$SSH "REPO='$REPO' SHA='$SHA' sh -s" < "$HERE/run-xserver-build.sh"

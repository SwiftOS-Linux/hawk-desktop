# Hawk Desktop Environment

## Contribute

### 1. Get The Source Code
```bash
git clone https://github.com/SwiftOS-Linux/hawk-desktop.git
cd hawk-desktop
```

### 2. Compile This Project
```bash
meson build
# meson compile -C build # For Release Version
meson compile --verbose -C build # For Debug Version
```
```bash
cd build
sudo ninja install
```

### 3. Debug This Project
Add Desktop Environment Shortcut
```bash
sudo bash -c 'cat > /usr/share/xsessions/hawk-desktop.desktop <<EOF
[Desktop Entry]
Name=Hawk Desktop
Comment=This session starts the Hawk desktop environment
Exec=sh -c "/usr/bin/hawk-session GTK_DEBUG=all GDK_DEBUG=all 2>&1 | tee ~/hawk-desktop.log"
TryExec=/usr/bin/hawk-session
Type=Application
EOF'
```
If you are Using LightDM
```bash
sudo bash -c 'echo "user-session=hawk-desktop" >> /etc/lightdm/lightdm.conf.d/10-hawk.conf'
```

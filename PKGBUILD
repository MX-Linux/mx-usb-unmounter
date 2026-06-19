# Maintainer: Adrian <adrian@mxlinux.org>
pkgname=mx-usb-unmounter
pkgver=${PKGVER:-25.6}
pkgrel=1
pkgdesc="MX USB Unmounter - Tray application for unmounting removable media"
arch=('x86_64' 'i686')
url="https://mxlinux.org"
license=('GPL3')
depends=('qt6-base' 'qt6-svg' 'glib2' 'dbus' 'xdg-utils')
makedepends=('cmake' 'ninja' 'qt6-tools')
source=()
sha256sums=()

build() {
    cd "${startdir}"

    # Clean any previous build artifacts
    rm -rf build

    # Configure with CMake
    cmake -G Ninja \
        -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DPROJECT_VERSION_OVERRIDE="${pkgver}"

    # Build
    cmake --build build --parallel
}

package() {
    cd "${startdir}"

    # Install binary
    install -Dm755 build/mx-usb-unmounter "${pkgdir}/usr/bin/mx-usb-unmounter"

    # Install translations
    install -dm755 "${pkgdir}/usr/share/mx-usb-unmounter/locale"
    install -Dm644 build/*.qm "${pkgdir}/usr/share/mx-usb-unmounter/locale/" 2>/dev/null || true

    # Install desktop file (both application and autostart)
    install -Dm644 mx-usb-unmounter.desktop "${pkgdir}/usr/share/applications/mx-usb-unmounter.desktop"
    if [ -d autostart ]; then
        install -dm755 "${pkgdir}/etc/xdg/autostart"
        install -Dm644 autostart/mx-usb-unmounter.desktop "${pkgdir}/etc/xdg/autostart/mx-usb-unmounter.desktop"
        install -dm755 "${pkgdir}/usr/share/mx-usb-unmounter/autostart"
        cp -r autostart/* "${pkgdir}/usr/share/mx-usb-unmounter/autostart/"
    fi

    # Install icons
    install -Dm644 images/mx-usb-unmounter.png "${pkgdir}/usr/share/pixmaps/mx-usb-unmounter.png"
    install -Dm644 images/usb-unmounter.svg "${pkgdir}/usr/share/pixmaps/usb-unmounter.svg"

    # Install help
    install -dm755 "${pkgdir}/usr/share/doc/mx-usb-unmounter"
    if [ -d help ]; then
        cp -r help/* "${pkgdir}/usr/share/doc/mx-usb-unmounter/" 2>/dev/null || true
    fi

    # Install changelog
    gzip -c debian/changelog > "${pkgdir}/usr/share/doc/mx-usb-unmounter/changelog.gz"

    # Install license
    install -Dm644 LICENSE "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE"
}

_pkgname=MangoHud-2
pkgname=mangohud-2
pkgver=v1
pkgrel=1
pkgdesc="MangoHud with server-client architecture"
arch=('x86_64')
_git_profile="https://github.com/17314642"
url="${_git_profile}/MangoHud-2"
makedepends=(
    # server
    'ninja' 'meson' 'python-mako' 'glslang' 'pkgconf' 'patch' 'gcc' 'libdrm' 'libcap'

    # client
    'appstream'
    'cmocka'
    'dbus'
    'git'
    'glslang'
    'libxnvctrl'
    'libxrandr'
    'meson'
    'python-mako'
    'vulkan-headers'
)
depends=(
    # server
    'libdrm'
    'libcap'

    # client
    'gcc-libs'
    'glfw'
    'glibc'
    'hicolor-icon-theme'
    'libglvnd'
    'libx11'
    'libxkbcommon'
    'python'
    'python-matplotlib'
    'python-numpy'
    'sh'
    'wayland'
)
optdepends=(
    'dbus: mangohudctl'
    'libxnvctrl: NVIDIA GPU metrics on older GPUs'
    'vulkan-icd-loader: Vulkan support'
)
license=('MIT')
source=(
    $_pkgname::"git+${url}"
    git+${_git_profile}/mangohud-server
    git+${_git_profile}/mangohud-client
)

sha256sums=(
    'SKIP'
    'SKIP'
    'SKIP'
)

prepare() {
    cd $_pkgname

    git submodule init

    git config submodule.mangohud-server.url "$srcdir"/mangohud-server
    git config submodule.mangohud-client.url "$srcdir"/mangohud-client

    git -c protocol.file.allow=always submodule update --remote 
}

build() {
    cd "$srcdir/$_pkgname"

    cd mangohud-server

    meson setup build
    ninja -C build

    cd ../

    cd mangohud-client

    meson setup build
    ninja -C build

    cd ../
}

package() {
    cd "$srcdir/$_pkgname"

    cd mangohud-server
    DESTDIR="$pkgdir/" ninja -C build install
    cd ../

    cd mangohud-client
    DESTDIR="$pkgdir/" ninja -C build install
    cd ../
}

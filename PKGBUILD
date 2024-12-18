# Maintainer: Mariusz Kuchta <mariusz@kuchta.dev>
pkgname=marbs-cooltify
pkgrel=1
pkgver=0.1.0
pkgdesc="lean one-line notification osd"
arch=('x86_64')
url="https://github.com/Kuchteq/marbs-cooltif"
license=('GPL')
makedepends=('git' 'make')
provides=("${pkgname%-git}")
conflicts=("${pkgname%-git}")

build() {
	cd ../
	make
}

package() {
	cd ../
	DESTDIR="$pkgdir/" PREFIX=/usr make install
}

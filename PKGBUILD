pkgname=aout2lda
pkgver=1.0
pkgrel=1
pkgdesc='PDP-11 a.out to absolute loader format converter'
url='https://github.com/hackyourlife/aout2lda'
arch=(x86_64 aarch64 armv7h)
license=(GPL3)
source=(aout2lda.c Makefile)
sha256sums=('de192b276745657cf4202e7f957868d176956285937bd7f5ec6b835043eff08a'
            '59507e241a1ad39efa25fdbba677467178b45e83f2f1355ca92f8d011053b32e')

build() {
  make
}

package() {
  install -dm755 "$pkgdir/usr/bin"
  install -m755 aout2lda "$pkgdir/usr/bin/aout2lda"
}

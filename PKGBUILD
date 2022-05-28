pkgname=aout2lda
pkgver=1.0
pkgrel=1
pkgdesc="PDP-11 a.out to absolute loader format converter"
arch=(x86_64 aarch64 armv7h)
license=(GPL3)
source=(aout2lda.c Makefile)
sha256sums=('c566ec61f8141e71f8fa5d04c649188e6314949320e81d07df2b2788546e8770'
            '59507e241a1ad39efa25fdbba677467178b45e83f2f1355ca92f8d011053b32e')

build() {
  make
}

package() {
  install -dm755 "$pkgdir/usr/bin"
  install -m755 aout2lda "$pkgdir/usr/bin/aout2lda"
}

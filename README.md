MixTri(x)
=========

mixtri.lv2 is a matrix mixer and trigger processor intended to be used
with http://x42.github.io/sisco.lv2/

This project is in its early development stages, stay tuned.

Install
-------

Compiling this plugin requires the LV2 SDK, gnu-make, a c-compiler, libltc,
gtk+2.0, libpango, libcairo and openGL (sometimes called: glu, glx, mesa).

```bash
  git clone git://github.com/x42/mixtri.lv2
  cd mixtri.lv2
  make submodules
  make
  sudo make install PREFIX=/usr
  
  # test run
  jalv.gtk 'http://gareus.org/oss/lv2/mixtri#lv2_gtk'
  
	# jalv with external-ui patch:
  jalv.gtk 'http://gareus.org/oss/lv2/mixtri#lv2'
```

Note to packagers: The Makefile honors `PREFIX` and `DESTDIR` variables as well
as `CFLAGS`, `LDFLAGS` and `OPTIMIZATIONS` (additions to `CFLAGS`).

Screenshots
-----------

![screenshot](https://raw.github.com/x42/mixtri.lv2/master/img/mixtrix.png "MixTri Window")
![screenshot](https://raw.github.com/x42/mixtri.lv2/master/img/mixtri.png "LTC example")
![screenshot](https://raw.github.com/x42/mixtri.lv2/master/img/trigger_modes3.png "Trigger Modes")

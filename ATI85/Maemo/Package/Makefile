all:
	make --directory=.. all

build:
	make --directory=.. all

clean:
	make --directory=.. clean

install:
	install -m 755 -D ../ati85 $(DESTDIR)/usr/bin/ati85
	install -m 755 -D run-ati85 $(DESTDIR)/usr/bin/run-ati85
	install -m 644 -D TI83P.ROM $(DESTDIR)/usr/share/ati85/TI83P.ROM
	install -m 644 -D TI82.ROM $(DESTDIR)/usr/share/ati85/TI82.ROM
	install -m 644 -D TI83.ROM $(DESTDIR)/usr/share/ati85/TI83.ROM
	install -m 644 -D TI85.ROM $(DESTDIR)/usr/share/ati85/TI85.ROM
	install -m 644 -D TI86.ROM $(DESTDIR)/usr/share/ati85/TI86.ROM
	install -m 644 -D Menu.png $(DESTDIR)/usr/share/ati85/Menu.png
	install -m 644 -D TI83P.png $(DESTDIR)/usr/share/ati85/TI83P.png
	install -m 644 -D TI82.png $(DESTDIR)/usr/share/ati85/TI82.png
	install -m 644 -D TI83.png $(DESTDIR)/usr/share/ati85/TI83.png
	install -m 644 -D TI85.png $(DESTDIR)/usr/share/ati85/TI85.png
	install -m 644 -D TI86.png $(DESTDIR)/usr/share/ati85/TI86.png
	install -m 644 -D ati85-26x26.png $(DESTDIR)/usr/share/icons/hicolor/26x26/hildon/ati85.png
	install -m 644 -D ati85-40x40.png $(DESTDIR)/usr/share/icons/hicolor/40x40/hildon/ati85.png
	install -m 644 -D ati85-64x64.png $(DESTDIR)/usr/share/icons/hicolor/scalable/hildon/ati85.png
	install -m 644 -D ati85.desktop $(DESTDIR)/usr/share/applications/hildon/ati85.desktop
	install -m 644 -D ati85.service $(DESTDIR)/usr/share/dbus-1/services/ati85.service

uninstall:
	rm -rf $(DESTDIR)/usr/share/ati85
	rm -f $(DESTDIR)/usr/bin/ati85
	rm -f $(DESTDIR)/usr/bin/run-ati85
	rm -f $(DESTDIR)/usr/share/applications/hildon/ati85.desktop
	rm -f $(DESTDIR)/usr/share/dbus-1/services/ati85.service
	rm -f $(DESTDIR)/usr/share/icons/hicolor/26x26/hildon/ati85.png
	rm -f $(DESTDIR)/usr/share/icons/hicolor/40x40/hildon/ati85.png
	rm -f $(DESTDIR)/usr/share/icons/hicolor/scalable/hildon/ati85.png

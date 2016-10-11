
all:
#nothing to do here

romfs install_target:
	mkdir -p $(ROMFSDIR)/usr/share/qt5/photoviewer
	$(ROMFSINST) photoviewer.qml \
		/usr/share/qt5/photoviewer/photoviewer.qml
	$(ROMFSINST) PhotoViewerCore \
		/usr/share/qt5/photoviewer/PhotoViewerCore

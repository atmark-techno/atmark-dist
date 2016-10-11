#
# Makefile for Qt5
#

CROSS_LIBDIR = /usr/$(CROSS_COMPILE:-=)/lib

subdir_y = qmlscene
qmldir_$(CONFIG_USER_QT5_PHOTOVIEWER) += photoviewer

BASE_LIBS = \
	libQt5Concurrent.so.5	\
	libQt5Core.so.5		\
	libQt5DBus.so.5		\
	libQt5Gui.so.5		\
	libQt5Network.so.5	\
	libQt5OpenGL.so.5	\
	libQt5PrintSupport.so.5	\
	libQt5Sql.so.5		\
	libQt5Test.so.5		\
	libQt5Widgets.so.5	\
	libQt5Xml.so.5

BASE_PLUGINS = \
	accessible/libqtaccessiblewidgets.so				\
	bearer/libqconnmanbearer.so					\
	bearer/libqgenericbearer.so					\
	bearer/libqnmbearer.so						\
	generic/libqevdevkeyboardplugin.so				\
	generic/libqevdevmouseplugin.so					\
	generic/libqevdevtabletplugin.so				\
	generic/libqevdevtouchplugin.so					\
	imageformats/libqgif.so						\
	imageformats/libqico.so						\
	imageformats/libqjpeg.so					\
	platforminputcontexts/libibusplatforminputcontextplugin.so	\
	platforminputcontexts/libmaliitplatforminputcontextplugin.so	\
	platforms/libqeglfs.so						\
	platforms/libqlinuxfb.so					\
	platforms/libqminimal.so					\
	platforms/libqminimalegl.so					\
	platforms/libqxcb.so						\
	printsupport/libcupsprintersupport.so				\
	sqldrivers/libqsqlite.so					\
	sqldrivers/libqsqlmysql.so					\
	sqldrivers/libqsqlodbc.so					\
	sqldrivers/libqsqlpsql.so					\
	sqldrivers/libqsqltds.so

JSBACKEND_LIBS = \
	libQt5V8.so.5

XMLPATTERNS_LIBS = \
	libQt5XmlPatterns.so.5

DECLARATIVE_LIBS = \
	libQt5Qml.so.5			\
	libQt5Quick.so.5		\
	libQt5QuickParticles.so.5	\
	libQt5QuickTest.so.5

DECLARATIVE_PLUGINS = \
	accessible/libqtaccessiblequick.so	\
	qmltooling/libqmldbg_qtquick2.so	\
	qmltooling/libqmldbg_tcp.so

DECLARATIVE_QMLPLUGINS = \
	Qt/labs/folderlistmodel/libqmlfolderlistmodelplugin.so	\
	QtQuick.2/libqtquick2plugin.so				\
	QtQuick/LocalStorage/libqmllocalstorageplugin.so	\
	QtQuick/Particles.2/libparticlesplugin.so		\
	QtQuick/Window.2/libwindowplugin.so			\
	QtQuick/XmlListModel/libqmlxmllistmodelplugin.so	\
	QtTest/libqmltestplugin.so

MULTIMEDIA_LIBS = \
	libQt5Multimedia.so.5			\
	libQt5MultimediaQuick_p.so.5		\
	libQt5MultimediaWidgets.so.5		\
	libqgsttools_p.so.1

MULTIMEDIA_PLUGINS = \
	audio/libqtmedia_pulse.so		\
	mediaservice/libgstaudiodecoder.so	\
	mediaservice/libgstcamerabin.so		\
	mediaservice/libgstmediacapture.so	\
	mediaservice/libgstmediaplayer.so	\
	playlistformats/libqtmultimedia_m3u.so

MULTIMEDIA_QMLPLUGINS = \
	QtAudioEngine/libdeclarative_audioengine.so	\
	QtMultimedia/libdeclarative_multimedia.so

LIBS_y += $(BASE_LIBS)
LIBS_y += $(JSBACKEND_LIBS)
LIBS_y += $(XMLPATTERNS_LIBS)
LIBS_y += $(DECLARATIVE_LIBS)
#LIBS_y += $(MULTIMEDIA_LIBS)

PLUGINS_y += $(BASE_PLUGINS)
PLUGINS_y += $(DECLARATIVE_PLUGINS)
#PLUGINS_y += $(MULTIMEDIA_PLUGINS)

QMLPLUGINS_y += $(DECLARATIVE_QMLPLUGINS)
#QMLPLUGINS_y += $(MULTIMEDIA_QMLPLUGINS)

all:
	for i in $(sort $(subdir_y)); do \
		if [ -d $$i ]; then \
			(cd $$i; qmake -spec linux-arm-gnueabihf-g++) || exit $$?; \
			make -C $$i || exit $$?; \
		fi; \
	done

romfs:
	for i in $(sort $(subdir_y) $(qmldir_y)); do \
		[ ! -d $$i ] || make INSTALL_ROOT=$(ROMFSDIR) -C $$i install_target || exit $$?; \
	done

	for i in $(LIBS_y); do \
		f=`readlink $(CROSS_LIBDIR)/$$i`; \
		cp -d $(CROSS_LIBDIR)/$$i $(ROMFSDIR)/lib/$$i; \
		cp $(CROSS_LIBDIR)/$$f $(ROMFSDIR)/lib/$$f; \
	done
	for i in $(PLUGINS_y); do \
		mkdir -p `dirname $(ROMFSDIR)/lib/qt5/plugins/$$i`; \
		cp $(CROSS_LIBDIR)/qt5/plugins/$$i \
			$(ROMFSDIR)/lib/qt5/plugins/$$i; \
	done
	for i in $(QMLPLUGINS_y); do \
		mkdir -p `dirname $(ROMFSDIR)/lib/qt5/qml/$$i`; \
		cp $(CROSS_LIBDIR)/qt5/qml/$$i \
			$(ROMFSDIR)/lib/qt5/qml/$$i; \
		cp $(CROSS_LIBDIR)/qt5/qml/`dirname $$i`/plugins.qmltypes \
			$(ROMFSDIR)/lib/qt5/qml/`dirname $$i`/plugins.qmltypes; \
		cp $(CROSS_LIBDIR)/qt5/qml/`dirname $$i`/qmldir \
			$(ROMFSDIR)/lib/qt5/qml/`dirname $$i`/qmldir; \
	done

	$(ROMFSINST) /etc

clean:
	-for i in $(sort $(subdir_y) $(subdir_)); do \
		[ ! -d $$i ] || make -C $$i clean; \
	done

distclean:
	-for i in $(sort $(subdir_y) $(subdir_)); do \
		[ ! -d $$i ] || make -C $$i distclean; \
	done

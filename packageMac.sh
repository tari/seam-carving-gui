PLUGINSDIR=/Developer/Applications/Qt/plugins/imageformats/
FRAMEWORKSDIR=/Library/Frameworks
APPDIR=SeamCarvingGui.app

#Make Frameworks Dir
mkdir -p $APPDIR/Contents/Frameworks
#Copy Qt Frameworks
cp -R $FRAMEWORKSDIR/QtCore.framework $APPDIR/Contents/Frameworks
cp -R $FRAMEWORKSDIR/QtGui.framework $APPDIR/Contents/Frameworks

#ID the Qt Frameworks
install_name_tool -id @executable_path/../Frameworks/QtCore.framework/Versions/4.0/QtCore $APPDIR/Contents/Frameworks/QtCore.framework/Versions/4.0/QtCore
install_name_tool -id @executable_path/../Frameworks/QtGui.framework/Versions/4.0/QtGui $APPDIR/Contents/Frameworks/QtGui.framework/Versions/4.0/QtGui

#Change currnent frameworks links
install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4.0/QtCore $APPDIR/Contents/MacOS/*
install_name_tool -change QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4.0/QtGui $APPDIR/Contents/MacOS/*

#Change QtGui's path to QtCore
install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4.0/QtCore $APPDIR/Contents/Frameworks/QtGui.framework/Versions/4.0/QtGui

mkdir -p $APPDIR/Contents/plugins/imageformats
cp $PLUGINSDIR/libqjpeg.dylib $APPDIR/Contents/plugins/imageformats
cp $PLUGINSDIR/libqgif.dylib $APPDIR/Contents/plugins/imageformats
cp $PLUGINSDIR/libqtiff.dylib $APPDIR/Contents/plugins/imageformats
install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4.0/QtCore $APPDIR/Contents/plugins/imageformats/libqjpeg.dylib
install_name_tool -change QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4.0/QtGui $APPDIR/Contents/plugins/imageformats/libqjpeg.dylib
install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4.0/QtCore $APPDIR/Contents/plugins/imageformats/libqgif.dylib
install_name_tool -change QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4.0/QtGui $APPDIR/Contents/plugins/imageformats/libqgif.dylib
install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4.0/QtCore $APPDIR/Contents/plugins/imageformats/libqtiff.dylib
install_name_tool -change QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4.0/QtGui $APPDIR/Contents/plugins/imageformats/libqtiff.dylib

#Clean out Include directores
rm -rf $APPDIR/Contents/Frameworks/QtCore.framework/Versions/4/Headers
rm -rf $APPDIR/Contents/Frameworks/QtGui.framework/Versions/4/Headers
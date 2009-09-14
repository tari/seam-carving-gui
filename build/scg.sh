#SeamCarvingGui build script

[clean_source]
PRINT Cleaning out source directory...
rm -rf $source_dir

[checkout_source]
PRINT Checking out source code
mkdir -p $product_name_abbrev
svn co $svn_url $source_dir

[update_source]
PRINT Updating source code
svn update $source_dir

[configure]
PRINT Configuring source
cd $source_dir; qmake -t vcapp $pro_file

[make-windows]
PRINT Makeing
cd $source_dir; devenv.com ${product_name_abbrev}.vcproj /build "Release|Win32"

[clean_stagearea]
PRINT Cleaning up old Stage Area (all data will be lost)...
rm -rf $stagearea_dir
mkdir -p $stagearea_dir


[prepare_stage]
PRINT Preparing stage area
cp $source_dir/GPL.txt $stagearea_dir/
cp $source_dir/README.txt $stagearea_dir/
cp $source_dir/pthreadVSE2.dll $stagearea_dir/
cp $source_dir/release/${product_binary_name} $stagearea_dir/
cp $QTDIR/bin/QtGui4.dll $stagearea_dir/
cp $QTDIR/bin/QtCore4.dll $stagearea_dir/
mkdir -p $stagearea_dir/plugins/imageformats/
cp $QTDIR/plugins/imageformats/qgif4.dll $stagearea_dir//plugins/imageformats/
cp $QTDIR/plugins/imageformats/qjpeg4.dll $stagearea_dir//plugins/imageformats/
cp $QTDIR/plugins/imageformats/qtiff4.dll $stagearea_dir//plugins/imageformats/
echo "[Paths]" > $stagearea_dir/qt.conf
echo "plugins=plugins" >> $stagearea_dir/qt.conf


[setup_package]
PRINT Making the installer in ${local_package_dir}
echo "#define MyAppName \"${product_name}\"" > ${source_dir}/install_script.iss
echo "#define MyAppNameAbbrev \"${product_name_abbrev}\"" >> ${source_dir}/install_script.iss
echo "#define MyAppVer \"${product_version}\"" >> ${source_dir}/install_script.iss
echo "#define MyAppVerName \"${product_name} ${os_abbrev} ${product_version}\"" >> ${source_dir}/install_script.iss
echo "#define MyAppPublisher \"By Gabe Rudy and Brain_Recall\"" >> ${source_dir}/install_script.iss
echo "#define MyAppURL \"${product_website}\"" >> ${source_dir}/install_script.iss
echo "#define MyAppExeName \"${product_binary_name}\"" >> ${source_dir}/install_script.iss
echo "#define MyLicFile \"${base_dir}/${stagearea_dir}/GPL.txt\"" >> ${source_dir}/install_script.iss
echo "#define MyPlatform \"${os_abbrev}\"" >> ${source_dir}/install_script.iss
echo "#define MyOutputDir \"${base_dir}/${local_package_dir}\"" >> ${source_dir}/install_script.iss
echo "#define MyStageDir \"${base_dir}/${stagearea_dir}/\"" >> ${source_dir}/install_script.iss
echo "#define MyRedistPath \"${base_dir}/${source_dir}/build/${redist_name}\"" >> ${source_dir}/install_script.iss
echo "#define MySetupIconPath \"${base_dir}/${source_dir}/build/install.ico\"" >> ${source_dir}/install_script.iss

tail -n +13 ${source_dir}/build/setup.iss >> ${source_dir}/install_script.iss
"$istool_exe" -compile ${source_dir}/install_script.iss
chmod a+rx ${local_package_dir}/${final_package_name}


set(ITKCommon_LOADED 1)
set(ITKCommon_ENABLE_SHARED "1")
set(ITKCommon_DEPENDS "ITKDoubleConversion;ITKKWIML;ITKKWSys;ITKVNLInstantiation")
set(ITKCommon_PUBLIC_DEPENDS "ITKKWIML")
set(ITKCommon_TRANSITIVE_DEPENDS "ITKKWIML;ITKKWSys;ITKVNLInstantiation")
set(ITKCommon_PRIVATE_DEPENDS "ITKDoubleConversion")
set(ITKCommon_LIBRARIES "ITKCommon")
set(ITKCommon_INCLUDE_DIRS "${ITK_INSTALL_PREFIX}/include/ITK-4.10")
set(ITKCommon_LIBRARY_DIRS "")
set(ITKCommon_RUNTIME_LIBRARY_DIRS "${ITK_INSTALL_PREFIX}/bin")
set(ITKCommon_TARGETS_FILE "")


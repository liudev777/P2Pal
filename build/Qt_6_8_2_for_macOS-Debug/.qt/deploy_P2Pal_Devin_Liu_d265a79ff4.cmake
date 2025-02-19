include(/Users/devin/Code/P2Pal_Devin_Liu/build/Qt_6_8_2_for_macOS-Debug/.qt/QtDeploySupport.cmake)
include("${CMAKE_CURRENT_LIST_DIR}/P2Pal_Devin_Liu-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE P2Pal_Devin_Liu.app
)

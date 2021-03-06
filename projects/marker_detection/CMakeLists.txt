project( marker_detection )

# Link directory
include_directories( ${CMAKE_SOURCE_DIR}/perception )
include_directories( ${CMAKE_SOURCE_DIR} )
link_directories( ${CMAKE_SOURCE_DIR}/lib )

# Alvar
find_package( alvar )
include_directories( ${ALVAR_INCLUDE_DIRS} )


# PCL
find_package( PCL COMPONENTS common search )
include_directories( ${PCL_INCLUDE_DIRS} )
link_directories( ${PCL_LIBRARY_DIRS} )
add_definitions( ${PCL_DEFINITIONS} )


# OpenCV
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

# Add aoi
set( GM_DIR $ENV{HOME}/local/share )
find_package(GM REQUIRED)
include_directories( ${GM_INCLUDE_DIRS} )
link_directories( ${GM_LIBRARY_DIRS} )

# Calibration v0
add_executable( marker_sendPose marker_sendPose.cpp ) 
target_link_libraries( marker_sendPose  motionControl bimanual_msgs crichton_markerDetector calibration ${PCL_COMMON_LIBRARIES} ${PCL_SEARCH_LIBRARIES} ${OpenCV_LIBRARIES} ${ALVAR_LIBRARIES} ${SNS_LIBRARIES} ${ACH_LIBRARIES} ${AMINO_LIBRARIES} ) 
set_target_properties( marker_sendPose PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin 
ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin ) 

add_executable( marker_send3DPos marker_send3DPos.cpp ) 
target_link_libraries( marker_send3DPos motionControl bimanual_msgs crichton_markerDetector calibration ${PCL_COMMON_LIBRARIES} ${PCL_SEARCH_LIBRARIES} ${OpenCV_LIBRARIES} ${ALVAR_LIBRARIES} ${SNS_LIBRARIES} ${ACH_LIBRARIES} ${AMINO_LIBRARIES} ) 
set_target_properties( marker_send3DPos PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin 
ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin ) 

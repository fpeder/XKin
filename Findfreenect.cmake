#
# FREENECT_FOUND
# FREENECT_INCLUDE_DIR
# FREENECT_LIBRARIES
#

set( LIBRARY_PATHS "/usr /usr/local /opt/local /sw" )

find_path( FREENECT_INCLUDE_DIR
        NAMES libfreenect_sync.h
	${LIBRARY_PATHS}
	PATH_SUFFIEXS include include/libfreenect 
	DOC "The dicrectory where libfreenect_sync.h resides"
)

find_library( FREENECT_SYNC_LIBRARIES 
	NAMES freenect_sync
	${LIBRARY_PATHS}
	PATH_SUFFIES lib lib64
	DOC "The freenect_sync library"
)

find_library( FREENECT_CV_LIBRARIES
	NAMES freenect_cv
	${LIBRARY_PATHS}
	PATH_SUFFIEXS lib lib64
	DOC "The freenect_cv library"
)

set( FREENECT_LIBRARIES ${FREENECT_SYNC_LIBRARIES} ${FREENECT_CV_LIBRARIES} )

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( freenect DEFAULT_MSG FREENECT_LIBRARIES FREENECT_INCLUDE_DIR )

mark_as_advanced( FREENECT_LIBRARIES FREENECT_INCLUDE_DIR )
if( FREENECT_FOUND )
	include_directories( ${FREENECT_INCLUDE_DIR} )
	message( STATUS "freenect found (include: ${FREENECT_INCLUDE_DIR}, 
 				      lib: ${FREENECT_LIBRARIES})")
endif( FREENECT_FOUND )

mark_as_advanced( FREENECT_FOUND )


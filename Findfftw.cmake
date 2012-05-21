#
# FFTW_FOUND
# FFTW_INCLUDE_DIR
# FFTW_LIBRARIES
#

find_path( FFTW_INCLUDE_DIR fftw3.h
	PATHS /usr /usr/local /sw /opt/local 
	PATH_SUFFIEXS include
	DOC "The dicrectory where fftw3.h resides"
)

find_library( FFTW_LIBRARIES
	NAMES fftw3
	PATHS /usr /usr/local /sw /opt/local
	PATH_SUFFIEXS lib lib64
	DOC "The fftw3 library"
)

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( fftw DEFAULT_MSG FFTW_LIBRARIES FFTW_INCLUDE_DIR ) 

mark_as_advanced( FFTW_LIBRARIES FFTW_INCLUDE_DIR )
if( FREENECT_FOUND )
	include_directories( ${FFTW_INCLUDE_DIR} )
	message( STATUS "fftw3 found (include: ${FFTW_INCLUDE_DIR}, 
 				      lib: ${FREENECT_LIBRARIES})")
endif( FREENECT_FOUND )

mark_as_advanced( FREENECT_FOUND )

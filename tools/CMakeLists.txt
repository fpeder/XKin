project( tools )

include_directories( "${CMAKE_SOURCE_DIR}/include" )

set( POSTURE_TOOLS testposture trainposture )
foreach( PROG ${POSTURE_TOOLS} )
	add_executable( ${PROG} "${PROG}.c" )
	target_link_libraries( ${PROG} body hand posture
		               ${OpenCV_LIBS}
		               ${FREENECT_LIBRARIES}
	)	
endforeach( PROG )


set( GESTURE_TOOLS genproto viewproto trainmodels testmodels testgesture ) 
foreach( PROG ${GESTURE_TOOLS} )
	add_executable( ${PROG} "${PROG}.c" )
	target_link_libraries( ${PROG} body hand posture gesture
			       ${OpenCV_LIBS}
			       ${FREENECT_LIBRARIES}
	)
endforeach( PROG )


add_executable( perfposture perfposture.c )
target_link_libraries( perfposture body hand posture
                       ${OpenCV_LIBS} ${FREENECT_LIBRARIES} )

add_executable( perfgesture perfgesture.c )
target_link_libraries( perfgesture body hand posture gesture 
                       ${OpenCV_LIBS} ${FREENECT_LIBRARIES} )

add_executable( genprotolive genprotolive.c )
target_link_libraries( genprotolive body hand posture gesture
                       ${OpenCV_LIBS} ${FREENECT_LIBRARIES} )


mark_as_advanced( PROG POSTURE_TOOLS GESTURE_TOOLS )


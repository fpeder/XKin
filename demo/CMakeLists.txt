project( demo )

include_directories( "${CMAKE_SOURCE_DIR}/include" )

add_executable( demoposture demoposture.c)
target_link_libraries( demoposture body hand posture
				   ${OpenCV_LIBS}
				   ${FREENECT_LIBRARIES}
)

add_executable( demogesture demogesture.c )
target_link_libraries( demogesture body hand posture gesture
                                   ${OpenCV_LIBS}
		                   ${FREENECT_LIBRARIES}
)

file(COPY img DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )


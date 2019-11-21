add_executable(receiver
		codeblockstate.c	codeblockstate.h
		receiver.c
		segment_list.c		segment_list.h
		segstate.c		segstate.h
		segstateset.c		segstateset.h
)
target_link_libraries(receiver demo1utils)
target_link_libraries(receiver ${RQLINKLIB})

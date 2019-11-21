add_executable(sender
		cli.c			cli.h
		sender.c
		udp.c			udp.h
)
target_link_libraries(sender demo1utils ${RQLINKLIB})

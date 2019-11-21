add_library(demo1utils STATIC
		fd_ops.c		fd_ops.h
		m3u8.c			m3u8.h
					packet_header.h
		param_deriv.c		param_deriv.h
		rq_memblocks.c		rq_memblocks.h
		timeutil.c		timeutil.h
)
target_include_directories(demo1utils PUBLIC .)
target_link_libraries(demo1utils ${RQLINKLIB} m)

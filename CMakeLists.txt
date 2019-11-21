cmake_minimum_required(VERSION 3.10)

project(RqDemo1
	VERSION 1.0
	DESCRIPTION "Single machine RaptorQ movie transfer demo"
)

# Use tvrqapi or CodornicesRq
if (CMAKE_RQ_API MATCHES TVRQ)
  set(RQLINKLIB "tvrqapi::tvrqapi")
  set(RQPACKAGE "tvrqapi")
  else()
  set(RQLINKLIB "CodornicesRq::CodornicesRq")
  set(RQPACKAGE "CodornicesRq")
endif()

find_package(${RQPACKAGE} REQUIRED)


add_subdirectory(utils)
add_subdirectory(test)
add_subdirectory(receiver)
add_subdirectory(sender)

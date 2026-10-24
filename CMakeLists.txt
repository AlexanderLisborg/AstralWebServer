cmake_minimum_required(VERSION 4.2.0)
project(
    AstralWebServer
    VERSION 1.0
    DESCRIPTION "Premature optimism is the root of all evil."
    LANGUAGES C
)

#set_property(TARGET PROPERTY C_STANDARD)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(server)

target_sources(server
    PRIVATE
        src/io_helper.c
        src/http_helper.c
        src/wserver.c
    PRIVATE
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            include/job_queue.h
            include/io_helper.h
            include/http_helper.h
)


#add_executable(tester)

#target_sources(tester
#    PRIVATE
#        src/io_helper.c
#        src/http_helper.c
#    PRIVATE
#        FILE_SET HEADERS
#        BASE_DIRS include
#        FILES
#            include/io_helper.h
#            include/http_helper.c
#)

#file(COPY html DESTINATION .)
aux_source_directory(. io_multi_src)
set(target_list)
foreach(file IN LISTS io_multi_src)
    string(REPLACE ".cc" "" target_name_temp ${file})
    string(REPLACE "./" "" target_name ${target_name_temp})
    add_executable(${target_name} ${file})
    #target_link_libraries(${target_name} ${LINK_LIBS})
    list(APPEND target_list ${target_name})
endforeach()

install(TARGETS ${target_list}
    RUNTIME DESTINATION bin/io_multi
)

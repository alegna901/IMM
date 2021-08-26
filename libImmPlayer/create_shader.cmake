cmake_minimum_required(VERSION 3.4.1)

function(convert_shader_file output f)
    # Get short filename
    string(REGEX MATCH "([^/]+)$" filename ${f})
    # remove file extension
    string(REGEX REPLACE "\\.(.+)$" "" filename ${filename})
    # Replace filename spaces & extension separator for C compatibility
    string(REGEX REPLACE "\\.| |-" "_" filename ${filename})
    # Read data from file
    file(READ ${f} filedata)
    # Append data to output file
    file(APPEND ${output} "\nconst char * ${filename} = R\"(\n${filedata}\n)\";\n")
#    file(APPEND ${output} "\nconst char * ${filename} = R\"(\n${filedata}\n)\";\n\nconst unsigned ${filename}_size = sizeof(${filename});\n")
endfunction()

function(convert_shader_dir output dir)
    # Collect input files
    file(GLOB bins ${dir}/*.es.glsl)
    list(REMOVE_DUPLICATES bins)
    # Iterate through input files
    foreach(bin ${bins})
        convert_shader_file(${output} ${bin})
    endforeach()
endfunction()

function(convert_shader output dir namespace)
    # Create empty output file
    if (EXISTS ${output})
        file(REMOVE ${output})
    endif()
    file(WRITE ${output} "// FILE GENERATED BY CMAKE\n")
    if (${namespace} STREQUAL "INVALID")
    else()
        file(APPEND ${output} "\nnamespace ${namespace} {\n")
    endif()
    if (IS_DIRECTORY ${dir})
        convert_shader_dir(${output} ${dir})
    else()
        convert_shader_file(${output} ${dir})
    endif()
    if (${namespace} STREQUAL "INVALID")
    else()
        file(APPEND ${output} "\n}\n")
    endif()
endfunction()

convert_shader(${OUTPUT} ${INPUT} ${NAMESPACE})

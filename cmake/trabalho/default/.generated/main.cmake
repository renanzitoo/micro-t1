include("${CMAKE_CURRENT_LIST_DIR}/rule.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/file.cmake")

set(trabalho_default_library_list )

# Handle files with suffix (s|as|asm|AS|ASM|As|aS|Asm), for group default-XC8
if(trabalho_default_default_XC8_FILE_TYPE_assemble)
add_library(trabalho_default_default_XC8_assemble OBJECT ${trabalho_default_default_XC8_FILE_TYPE_assemble})
    trabalho_default_default_XC8_assemble_rule(trabalho_default_default_XC8_assemble)
    list(APPEND trabalho_default_library_list "$<TARGET_OBJECTS:trabalho_default_default_XC8_assemble>")

endif()

# Handle files with suffix S, for group default-XC8
if(trabalho_default_default_XC8_FILE_TYPE_assemblePreprocess)
add_library(trabalho_default_default_XC8_assemblePreprocess OBJECT ${trabalho_default_default_XC8_FILE_TYPE_assemblePreprocess})
    trabalho_default_default_XC8_assemblePreprocess_rule(trabalho_default_default_XC8_assemblePreprocess)
    list(APPEND trabalho_default_library_list "$<TARGET_OBJECTS:trabalho_default_default_XC8_assemblePreprocess>")

endif()

# Handle files with suffix [cC], for group default-XC8
if(trabalho_default_default_XC8_FILE_TYPE_compile)
add_library(trabalho_default_default_XC8_compile OBJECT ${trabalho_default_default_XC8_FILE_TYPE_compile})
    trabalho_default_default_XC8_compile_rule(trabalho_default_default_XC8_compile)
    list(APPEND trabalho_default_library_list "$<TARGET_OBJECTS:trabalho_default_default_XC8_compile>")

endif()


# Main target for this project
add_executable(trabalho_default_image_z991AopA ${trabalho_default_library_list})

set_target_properties(trabalho_default_image_z991AopA PROPERTIES
    OUTPUT_NAME "default"
    SUFFIX ".elf"
    ADDITIONAL_CLEAN_FILES "${output_extensions}"
    RUNTIME_OUTPUT_DIRECTORY "${trabalho_default_output_dir}")
target_link_libraries(trabalho_default_image_z991AopA PRIVATE ${trabalho_default_default_XC8_FILE_TYPE_link})
# Add the link options from the rule file.
trabalho_default_link_rule( trabalho_default_image_z991AopA)




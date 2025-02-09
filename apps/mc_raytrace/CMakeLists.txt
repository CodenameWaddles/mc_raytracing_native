set( target_name "optixrenderer" )
set( "optixrenderer" "target_name" PARENT_SCOPE )


function(custom_add_library target_name_base target_name_var)
  set(target_name ${target_name_base})
  set(${target_name_var} ${target_name} PARENT_SCOPE)

  OPTIX_add_source_groups()

  CUDA_GET_SOURCES_AND_OPTIONS(source_files cmake_options options ${ARGN})

  # Isolate OBJ target files. NVCC should only process these files and leave PTX targets for NVRTC
  set(cu_obj_source_files)
  set(cu_optix_source_files)
  foreach(file ${source_files})
    get_source_file_property(_cuda_source_format ${file} CUDA_SOURCE_PROPERTY_FORMAT)
    if(${_cuda_source_format} MATCHES "OBJ")
      list(APPEND cu_obj_source_files ${file})
    else()
      list(APPEND cu_optix_source_files ${file})
    endif()
  endforeach()

  CUDA_WRAP_SRCS(${target_name} OBJ generated_files ${cu_obj_source_files} ${cmake_options} OPTIONS ${options})

  # Create the rules to build the PTX and/or OPTIX files.
  if(PRAYGROUND_INPUT_GENERATE_OPTIXIR)
    CUDA_WRAP_SRCS(${target_name} OPTIXIR generated_files2 ${cu_optix_source_files} ${cmake_options} OPTIONS ${options})
    list(APPEND generated_files ${generated_files2})
  endif()
  if(PRAYGROUND_INPUT_GENERATE_PTX AND NOT CUDA_NVRTC_ENABLED)
    CUDA_WRAP_SRCS(${target_name} PTX generated_files3 ${cu_optix_source_files} ${cmake_options} OPTIONS ${options})
    list(APPEND generated_files ${generated_files3})
  endif()

  # Replace add_executable with add_library
  add_library(${target_name} SHARED
    ${source_files}
    ${generated_files}
    ${cmake_options}
  )

  target_include_directories(${target_name} PRIVATE
    "${PRAYGROUND_DIR}"
    "${CMAKE_BINARY_DIR}/bin"
    "${OptiX_INCLUDE}"
  )

  target_link_libraries(${target_name}
    imgui
    nanovdb
    prayground
  )

  if(WIN32)
    target_compile_options(${target_name} PUBLIC "/source-charset:utf-8")
  endif()
endfunction()

# Add the shared library
custom_add_library(${target_name} SHARED
    params.h
    DataStructures.h
    enkimi.c
    enkimi.h
    SharedBuffer.cpp
    SharedBuffer.h
    DataManager.cpp
    DataManager.h
    OptixRenderer.cpp
    app.cpp 
    app.h
    kernels.cu
)

set(JAVA_HOME "C:/Program Files/Java/jdk-21")

# Include JNI headers
include_directories(${JAVA_HOME}/include)
include_directories(${JAVA_HOME}/include/win32)

if(MSVC)
  target_compile_options(${target_name} PRIVATE /W4)
else()
  target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_compile_definitions(
    ${target_name}
    PRIVATE
    APP_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    APP_NAME_DEFINE="${target_name}"
)

target_link_libraries(${target_name} ${CUDA_LIBRARIES})
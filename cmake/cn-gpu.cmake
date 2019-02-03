if (WITH_CN_GPU)
    set(CN_GPU_SOURCES src/crypto/cn_gpu_avx.cpp src/crypto/cn_gpu_ssse3.cpp)

    if (CMAKE_CXX_COMPILER_ID MATCHES GNU OR CMAKE_CXX_COMPILER_ID MATCHES Clang)
        set_source_files_properties(src/crypto/cn_gpu_avx.cpp PROPERTIES COMPILE_FLAGS "-mavx2")
    elseif (CMAKE_CXX_COMPILER_ID MATCHES MSVC)
        set_source_files_properties(src/crypto/cn_gpu_avx.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX")
    endif()
else()
    set(CN_GPU_SOURCES "")

    add_definitions(/DXMRIG_NO_CN_GPU)
endif()

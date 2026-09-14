# Enables a strict warning set on a target. Warnings are only promoted to
# errors when LMS_WARNINGS_AS_ERRORS is ON (CI turns it on).
function(lms_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
        if(LMS_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
            -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused
            -Woverloaded-virtual -Wdouble-promotion)
        # -Wnull-dereference is deliberately left out: GCC 12-14 report false
        # positives inside libstdc++ (std::string/vector) at -O2 and above.
        if(LMS_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

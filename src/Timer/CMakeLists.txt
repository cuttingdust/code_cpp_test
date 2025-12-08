cmake_minimum_required(VERSION 3.20)
get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_SOURCE_DIR} NAME)

project(${CURRENT_DIR})

set(DPS_LIBRARYS
   fmt::fmt
)

cpp_execute(${PROJECT_NAME})

macro(setup_edit_and_continue TARGET_NAME)
    if(MSVC)
        # 编译器选项
        target_compile_options(${TARGET_NAME} PRIVATE
            "$<$<CONFIG:Debug>:/ZI>"      # 编辑并继续
            "$<$<CONFIG:Debug>:/JMC>"     # 仅我的代码
            "$<$<CONFIG:Debug>:/Od>"      # 禁用优化
            "$<$<CONFIG:Debug>:/RTC1>"    # 运行时检查
            "$<$<CONFIG:Debug>:/W4>"      # 警告等级
            "$<$<CONFIG:Debug>:/MP>"      # 多处理器编译
        )
        
        # 链接器选项
        target_link_options(${TARGET_NAME} PRIVATE
            "$<$<CONFIG:Debug>:/INCREMENTAL>"   # 增量链接
            "$<$<CONFIG:Debug>:/DEBUG>"         # 调试信息
            "$<$<CONFIG:Debug>:/OPT:REF>"       # 引用优化
            "$<$<CONFIG:Debug>:/OPT:ICF>"       # COMDAT 优化
        )
        
        # 目标属性
        set_target_properties(${TARGET_NAME} PROPERTIES
            MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug>:EditAndContinue>"
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        )
    endif()
endmacro()

if(MSVC)
    #设为启动项

    # set_property(DIRECTORY ${CMAKE_BINARY_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME}) 
    set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${OUT_RUN_PATH})
    set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "XCPP")

    # set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/GL")
    # set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/LTCG")
    # set_target_properties(${PROJECT_NAME} PROPERTIES
    #     MSVC_DEBUG_INFORMATION_FORMAT "EditAndContinue"
    # )

    target_compile_options(${PROJECT_NAME} PRIVATE 
    "/utf-8"
    "/ZI"                        # 编辑并继续
    )
   
    target_link_options(${PROJECT_NAME} PRIVATE
        "/ENTRY:mainCRTStartup"      # 指定程序入口点
    )

endif()
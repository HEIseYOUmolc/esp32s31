# main 组件由应用逻辑、显示界面和系统入口三个源文件组成。
set(SOURCES
"esp_xiaozhi_chat_app.c"
"esp_xiaozhi_chat_display.c"
"main.c")
set(INCLUDE_DIRS ".")
set(REQUIRES_LIST)

if(CONFIG_XIAOZHI_CHAT_APP_SERVER_AI_TOY)
    list(APPEND REQUIRES_LIST ai_toy_ws_client)
    message(STATUS "Chat server backend: AI Toy WebSocket")
else()
    message(STATUS "Chat server backend: Xiaozhi")
endif()

# WHOLE_ARCHIVE 强制保留组件中的注册对象和回调，避免链接器按“未引用”裁剪。
idf_component_register(
     SRCS ${SOURCES}
     INCLUDE_DIRS ${INCLUDE_DIRS}
     REQUIRES ${REQUIRES_LIST}
     WHOLE_ARCHIVE
)
                       

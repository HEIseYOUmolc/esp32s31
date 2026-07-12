# main 组件由应用逻辑、显示界面和系统入口三个源文件组成。
set(SOURCES
"esp_xiaozhi_chat_app.c"
"esp_xiaozhi_chat_display.c"
"main.c")
set(INCLUDE_DIRS ".")

# WHOLE_ARCHIVE 强制保留组件中的注册对象和回调，避免链接器按“未引用”裁剪。
idf_component_register(
     SRCS ${SOURCES}
     INCLUDE_DIRS ${INCLUDE_DIRS}
     WHOLE_ARCHIVE
)
                       

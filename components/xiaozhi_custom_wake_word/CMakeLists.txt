idf_component_register(
    SRCS "xiaozhi_custom_wake_word.c"
    INCLUDE_DIRS "include"
    REQUIRES
        jason-mao__av_processor
        espressif__esp-sr
        espressif__gmf_ai_audio
)

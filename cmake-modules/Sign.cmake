function(sign_executable TARGET_NAME)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND signtool.exe sign /n "Simone Fiorenza" /t "http://time.certum.pl/" /fd sha256 /v "$<TARGET_FILE:${TARGET_NAME}>"
            COMMENT "Signing ${TARGET_NAME}..."
    )
endfunction()
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
        EXPORT "${PROJECT_NAME}Targets"
        COMPONENT ${PROJECT_NAME}
)
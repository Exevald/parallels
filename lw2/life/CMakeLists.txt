add_executable(
        life
        Grid.cpp
        LifeSimulator.cpp
        LifeCli.cpp
        main.cpp
)

target_link_libraries(life PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
)
# Minecraft
minecraft on c with opengl
compile with command: clang main.c -o minecraft \
                -I/opt/homebrew/opt/glfw/include \
                -L/opt/homebrew/opt/glfw/lib \
                -lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -lm

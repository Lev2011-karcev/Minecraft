#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Настройки
#define WIDTH 1280
#define HEIGHT 720
#define BLOCK_TYPES 8
#define WORLD_SIZE 32
#define GRAVITY 0.05f
#define JUMP_FORCE 0.15f
#define MOVE_SPEED 0.1f
#define MOUSE_SENSITIVITY 0.002f
#define REACH_DISTANCE 5.0f

// Типы блоков
typedef enum {
    BLOCK_AIR, BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE,
    BLOCK_WOOD, BLOCK_LEAVES, BLOCK_WATER, BLOCK_SAND
} BlockType;

// Мир
BlockType world[WORLD_SIZE][WORLD_SIZE][WORLD_SIZE];

// Игрок
typedef struct {
    float x, y, z;
    float vx, vy, vz;
    float yaw, pitch;
    bool onGround;
} Player;
Player player;

// Состояние игры
BlockType selectedBlock = BLOCK_GRASS;
bool cursorLocked = true;
bool wireframeMode = false;

// Цвета блоков
void getBlockColor(BlockType type, float* r, float* g, float* b) {
    switch(type) {
        case BLOCK_GRASS:  *r=0.1; *g=0.8; *b=0.1; break;
        case BLOCK_DIRT:   *r=0.5; *g=0.3; *b=0.1; break;
        case BLOCK_STONE:  *r=0.5; *g=0.5; *b=0.5; break;
        case BLOCK_WOOD:   *r=0.6; *g=0.4; *b=0.2; break;
        case BLOCK_LEAVES: *r=0.1; *g=0.6; *b=0.1; break;
        case BLOCK_WATER:  *r=0.2; *g=0.2; *b=0.8; break;
        case BLOCK_SAND:   *r=0.9; *g=0.8; *b=0.5; break;
        default:           *r=0;   *g=0;   *b=0;
    }
}

// Генерация мира
void generateWorld() {
    srand(time(NULL));
    for(int x = 0; x < WORLD_SIZE; x++) {
        for(int z = 0; z < WORLD_SIZE; z++) {
            int height = 5 + (int)(5.0 * sin(x/5.0) * cos(z/5.0));
            
            for(int y = 0; y < WORLD_SIZE; y++) {
                if(y > height) {
                    if(y < WORLD_SIZE/2) world[x][y][z] = BLOCK_AIR;
                    else if(y == WORLD_SIZE/2) world[x][y][z] = BLOCK_WATER;
                    else world[x][y][z] = BLOCK_AIR;
                }
                else if(y == height) {
                    if(height > WORLD_SIZE/2 + 2) world[x][y][z] = BLOCK_SAND;
                    else world[x][y][z] = BLOCK_GRASS;
                }
                else if(y > height - 3) world[x][y][z] = BLOCK_DIRT;
                else world[x][y][z] = BLOCK_STONE;
            }
            
            // Добавляем деревья
            if(rand() % 20 == 0 && height < WORLD_SIZE-5) {
                int treeHeight = 3 + rand() % 3;
                for(int y = height+1; y < height+1+treeHeight; y++) {
                    if(y < WORLD_SIZE) world[x][y][z] = BLOCK_WOOD;
                }
                for(int tx = -2; tx <= 2; tx++) {
                    for(int ty = -2; ty <= 2; ty++) {
                        for(int tz = -2; tz <= 2; tz++) {
                            if(abs(tx)+abs(ty)+abs(tz) < 4 && 
                               x+tx >= 0 && x+tx < WORLD_SIZE &&
                               z+tz >= 0 && z+tz < WORLD_SIZE &&
                               height+treeHeight+ty < WORLD_SIZE) {
                                world[x+tx][height+treeHeight+ty][z+tz] = BLOCK_LEAVES;
                            }
                        }
                    }
                }
            }
        }
    }
}

// Проверка, находится ли точка внутри мира
bool isInsideWorld(int x, int y, int z) {
    return x >= 0 && x < WORLD_SIZE && 
           y >= 0 && y < WORLD_SIZE && 
           z >= 0 && z < WORLD_SIZE;
}

// Рендеринг блока
void renderBlock(int x, int y, int z) {
    BlockType type = world[x][y][z];
    if(type == BLOCK_AIR) return;
    
    float r,g,b;
    getBlockColor(type, &r, &g, &b);
    
    if(wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glColor3f(1,1,1);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    glBegin(GL_QUADS);
        // Передняя грань
        if(!isInsideWorld(x,y,z+1) || world[x][y][z+1] == BLOCK_AIR) {
            if(!wireframeMode) glColor3f(r*0.9, g*0.9, b*0.9);
            glVertex3f(x,   y,   z+1);
            glVertex3f(x+1, y,   z+1);
            glVertex3f(x+1, y+1, z+1);
            glVertex3f(x,   y+1, z+1);
        }
        
        // Задняя грань
        if(!isInsideWorld(x,y,z-1) || world[x][y][z-1] == BLOCK_AIR) {
            if(!wireframeMode) glColor3f(r*0.7, g*0.7, b*0.7);
            glVertex3f(x,   y,   z);
            glVertex3f(x,   y+1, z);
            glVertex3f(x+1, y+1, z);
            glVertex3f(x+1, y,   z);
        }
        
        // Верхняя грань
        if(!isInsideWorld(x,y+1,z) || world[x][y+1][z] == BLOCK_AIR) {
            if(!wireframeMode) glColor3f(r, g, b);
            glVertex3f(x,   y+1, z);
            glVertex3f(x,   y+1, z+1);
            glVertex3f(x+1, y+1, z+1);
            glVertex3f(x+1, y+1, z);
        }
        
        // Нижняя грань
        if(!isInsideWorld(x,y-1,z) || world[x][y-1][z] == BLOCK_AIR) {
            if(!wireframeMode) glColor3f(r*0.5, g*0.5, b*0.5);
            glVertex3f(x,   y, z);
            glVertex3f(x+1, y, z);
            glVertex3f(x+1, y, z+1);
            glVertex3f(x,   y, z+1);
        }
        
        // Левая грань
        if(!isInsideWorld(x-1,y,z) || world[x-1][y][z] == BLOCK_AIR) {
            if(!wireframeMode) glColor3f(r*0.8, g*0.8, b*0.8);
            glVertex3f(x, y,   z);
            glVertex3f(x, y,   z+1);
            glVertex3f(x, y+1, z+1);
            glVertex3f(x, y+1, z);
        }
        
        // Правая грань
        if(!isInsideWorld(x+1,y,z) || world[x+1][y][z] == BLOCK_AIR) {
            if(!wireframeMode) glColor3f(r*0.8, g*0.8, b*0.8);
            glVertex3f(x+1, y,   z);
            glVertex3f(x+1, y+1, z);
            glVertex3f(x+1, y+1, z+1);
            glVertex3f(x+1, y,   z+1);
        }
    glEnd();
    
    if(wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

// Рендеринг прицела
void renderCrosshair() {
    if(!cursorLocked) return;
    
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, HEIGHT, 0);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glColor3f(1, 1, 1);
    glLineWidth(2);
    glBegin(GL_LINES);
        glVertex2f(WIDTH/2-10, HEIGHT/2);
        glVertex2f(WIDTH/2+10, HEIGHT/2);
        glVertex2f(WIDTH/2, HEIGHT/2-10);
        glVertex2f(WIDTH/2, HEIGHT/2+10);
    glEnd();
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}

// Рендеринг HUD с информацией
void renderHUD() {
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, HEIGHT, 0);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Отображение выбранного блока
    float r,g,b;
    getBlockColor(selectedBlock, &r, &g, &b);
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
        glVertex2f(20, HEIGHT-40);
        glVertex2f(50, HEIGHT-40);
        glVertex2f(50, HEIGHT-10);
        glVertex2f(20, HEIGHT-10);
    glEnd();
    
    // Рамка
    glColor3f(1, 1, 1);
    glLineWidth(2);
    glBegin(GL_LINE_LOOP);
        glVertex2f(20, HEIGHT-40);
        glVertex2f(50, HEIGHT-40);
        glVertex2f(50, HEIGHT-10);
        glVertex2f(20, HEIGHT-10);
    glEnd();
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}

// Проверка коллизий
bool checkCollision(float x, float y, float z) {
    int ix = (int)x;
    int iy = (int)y;
    int iz = (int)z;
    
    // Проверяем 8 ближайших блоков
    for(int dy = 0; dy <= 1; dy++) {
        for(int dx = 0; dx <= 1; dx++) {
            for(int dz = 0; dz <= 1; dz++) {
                int bx = ix + dx;
                int by = iy + dy;
                int bz = iz + dz;
                
                if(isInsideWorld(bx, by, bz)) {
                    if(world[bx][by][bz] != BLOCK_AIR) {
                        // Проверяем пересечение AABB
                        if(x+0.3 > bx && x-0.3 < bx+1 &&
                           y+1.8 > by && y-0.2 < by+1 &&
                           z+0.3 > bz && z-0.3 < bz+1) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

// Обработка ввода
void processInput(GLFWwindow* window) {
    static double lastEscPress = 0;
    double currentTime = glfwGetTime();
    
    // Переключение курсора по ESC
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && currentTime - lastEscPress > 0.3) {
        cursorLocked = !cursorLocked;
        glfwSetInputMode(window, GLFW_CURSOR, 
            cursorLocked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        if(cursorLocked) {
            glfwSetCursorPos(window, WIDTH/2, HEIGHT/2);
        }
        lastEscPress = currentTime;
    }
    
    // Переключение режима отображения
    static bool fKeyPressed = false;
    if(glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        if(!fKeyPressed) {
            wireframeMode = !wireframeMode;
            fKeyPressed = true;
        }
    } else {
        fKeyPressed = false;
    }
    
    // Вращение камеры только при залоченном курсоре
    if(cursorLocked) {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        player.yaw += (mouseX - WIDTH/2) * MOUSE_SENSITIVITY;
        player.pitch -= (mouseY - HEIGHT/2) * MOUSE_SENSITIVITY;
        if(player.pitch > 1.5) player.pitch = 1.5;
        if(player.pitch < -1.5) player.pitch = -1.5;
        glfwSetCursorPos(window, WIDTH/2, HEIGHT/2);
    }
    
    // Движение (только при залоченном курсоре)
    if(cursorLocked) {
        float moveX = 0, moveZ = 0;
        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            moveX += sin(player.yaw) * MOVE_SPEED;
            moveZ -= cos(player.yaw) * MOVE_SPEED;
        }
        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            moveX -= sin(player.yaw) * MOVE_SPEED;
            moveZ += cos(player.yaw) * MOVE_SPEED;
        }
        if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            moveX -= cos(player.yaw) * MOVE_SPEED;
            moveZ -= sin(player.yaw) * MOVE_SPEED;
        }
        if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            moveX += cos(player.yaw) * MOVE_SPEED;
            moveZ += sin(player.yaw) * MOVE_SPEED;
        }
        
        // Прыжок
        if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && player.onGround) {
            player.vy = JUMP_FORCE;
            player.onGround = false;
        }
        
        // Приседание
        if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            player.y -= 0.1f;
        }
        
        player.vx = moveX;
        player.vz = moveZ;
    }
    
    // Гравитация
    player.vy -= GRAVITY;
    
    // Проверка коллизий по Y
    player.onGround = false;
    if(checkCollision(player.x, player.y + player.vy, player.z)) {
        if(player.vy < 0) {
            player.onGround = true;
        }
        player.vy = 0;
    }
    
    // Проверка коллизий по X
    if(checkCollision(player.x + player.vx, player.y, player.z)) {
        player.vx = 0;
    }
    
    // Проверка коллизий по Z
    if(checkCollision(player.x, player.y, player.z + player.vz)) {
        player.vz = 0;
    }
    
    // Обновление позиции
    player.x += player.vx;
    player.y += player.vy;
    player.z += player.vz;
    
    // Ограничение высоты
    if(player.y < 0) player.y = 0;
    if(player.y > WORLD_SIZE-1) player.y = WORLD_SIZE-1;
    
    // Размещение/удаление блоков
    if(cursorLocked) {
        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            // Бросаем луч для определения блока
            float rayX = player.x;
            float rayY = player.y + 0.5f;
            float rayZ = player.z;
            
            for(float t = 0; t < REACH_DISTANCE; t += 0.1f) {
                rayX += sin(player.yaw) * cos(player.pitch) * 0.1f;
                rayY += sin(player.pitch) * 0.1f;
                rayZ -= cos(player.yaw) * cos(player.pitch) * 0.1f;
                
                int bx = (int)rayX;
                int by = (int)rayY;
                int bz = (int)rayZ;
                
                if(isInsideWorld(bx, by, bz)) {
                    if(world[bx][by][bz] != BLOCK_AIR) {
                        world[bx][by][bz] = BLOCK_AIR;
                        break;
                    }
                }
            }
        }
        
        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            // Бросаем луч для определения места установки
            float rayX = player.x;
            float rayY = player.y + 0.5f;
            float rayZ = player.z;
            int lastBX = -1, lastBY = -1, lastBZ = -1;
            
            for(float t = 0; t < REACH_DISTANCE; t += 0.1f) {
                rayX += sin(player.yaw) * cos(player.pitch) * 0.1f;
                rayY += sin(player.pitch) * 0.1f;
                rayZ -= cos(player.yaw) * cos(player.pitch) * 0.1f;
                
                int bx = (int)rayX;
                int by = (int)rayY;
                int bz = (int)rayZ;
                
                if(isInsideWorld(bx, by, bz)) {
                    if(world[bx][by][bz] != BLOCK_AIR) {
                        if(lastBX != -1 && !checkCollision(lastBX+0.5f, lastBY+0.5f, lastBZ+0.5f)) {
                            world[lastBX][lastBY][lastBZ] = selectedBlock;
                        }
                        break;
                    }
                    lastBX = bx; lastBY = by; lastBZ = bz;
                }
            }
        }
    }
    
    // Выбор блоков цифрами 1-7
    if(glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) selectedBlock = BLOCK_GRASS;
    if(glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) selectedBlock = BLOCK_DIRT;
    if(glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) selectedBlock = BLOCK_STONE;
    if(glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) selectedBlock = BLOCK_WOOD;
    if(glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) selectedBlock = BLOCK_LEAVES;
    if(glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) selectedBlock = BLOCK_WATER;
    if(glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) selectedBlock = BLOCK_SAND;
}

int main() {
    // Инициализация GLFW
    if(!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }
    
    // Создание окна
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Minecraft Clone", NULL, NULL);
    if(!window) {
        glfwTerminate();
        fprintf(stderr, "Failed to create GLFW window\n");
        return -1;
    }
    
    // Настройки окна
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(window, WIDTH/2, HEIGHT/2);
    
    // Настройки OpenGL
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    gluPerspective(60.0, (double)WIDTH/(double)HEIGHT, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
    
    // Инициализация игрока
    player.x = WORLD_SIZE/2;
    player.y = WORLD_SIZE/2 + 5;
    player.z = WORLD_SIZE/2;
    player.yaw = 0;
    player.pitch = 0;
    player.onGround = false;
    
    // Генерация мира
    generateWorld();
    
    // Главный цикл
    while(!glfwWindowShouldClose(window)) {
        // Очистка экрана
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();
        
        // Обработка ввода
        processInput(window);
        
        // Установка камеры
        if(cursorLocked) {
            gluLookAt(
                player.x, player.y + 0.5f, player.z,
                player.x + sin(player.yaw) * cos(player.pitch),
                player.y + 0.5f + sin(player.pitch),
                player.z - cos(player.yaw) * cos(player.pitch),
                0, 1, 0
            );
        } else {
            // Вид сверху, если курсор разлочен
            gluLookAt(
                WORLD_SIZE/2, WORLD_SIZE, WORLD_SIZE/2,
                WORLD_SIZE/2, 0, WORLD_SIZE/2,
                0, 0, 1
            );
        }
        
        // Рендеринг мира
        for(int x = 0; x < WORLD_SIZE; x++) {
            for(int y = 0; y < WORLD_SIZE; y++) {
                for(int z = 0; z < WORLD_SIZE; z++) {
                    renderBlock(x, y, z);
                }
            }
        }
        
        // Рендеринг HUD
        renderCrosshair();
        renderHUD();
        
        // Обновление экрана
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Завершение
    glfwTerminate();
    return 0;
}
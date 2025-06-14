#include "raylib.h"
#include "raymath.h"

#define DRAWING_MODE_WIDTH 1920
#define DRAWING_MODE_HEIGHT 1075

#define TOUCH_MODE_WIDTH 100
#define TOUCH_MODE_HEIGHT 800

typedef enum {
    MODE_DRAW,
    MODE_TOUCH
} AppMode;

Texture icons[5] = { 0 };
Camera2D uiCamera = { 0 };

#define BUTTON_COLOR  (Color){ 90, 100, 120, 255 }

void DrawRectangleShadowed(Rectangle rect, float rounding, Color color, Color shadowColor, Vector2 shadowOffset) {
    DrawRectangleRounded((Rectangle){rect.x + shadowOffset.x, rect.y + shadowOffset.y, rect.width, rect.height}, rounding, 16, shadowColor);
    DrawRectangleRounded(rect, rounding, 16, color);
}

bool Button(Rectangle rect, Color buttonColor, int iconID) {
    Vector2 mousePos = GetMousePosition();
    Vector2 worldMousePos = GetScreenToWorld2D(mousePos, uiCamera);

    bool isHovered = CheckCollisionPointRec(worldMousePos, rect);
    bool isPressed = isHovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool isClicked = isHovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);

    Color primaryColor =  isPressed ? ColorBrightness(buttonColor, -0.1f) : (isHovered ? ColorBrightness(buttonColor, 0.2f) : buttonColor);

    if(isPressed) {
        Rectangle new = (Rectangle){rect.x, rect.y + 3, rect.width, rect.height};
        DrawRectangleShadowed(new, 0.25f, primaryColor, ColorBrightness(buttonColor, -0.4f), (Vector2){ 0, 2 });
        DrawTexturePro(icons[iconID], (Rectangle) {0, 0, 256, 256}, (Rectangle){rect.x + 5, rect.y + 8, rect.width - 10, rect.height - 10}, (Vector2) {0,0}, 0, WHITE);

    } else {
        DrawRectangleShadowed(rect, 0.25f, primaryColor, ColorBrightness(buttonColor, -0.4f), (Vector2) {0,3});
        DrawTexturePro(icons[iconID], (Rectangle) {0, 0, 256, 256}, (Rectangle){rect.x + 5, rect.y + 5, rect.width - 10, rect.height - 10}, (Vector2) {0,0}, 0, WHITE);
    }

    return isClicked;
}

int main() {
    AppMode currentMode = MODE_DRAW;

    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_TOPMOST);
    InitWindow(DRAWING_MODE_WIDTH, DRAWING_MODE_HEIGHT, "Freaky Draw");
    SetTargetFPS(60);
    SetWindowPosition(0, 0);

    icons[0] = LoadTexture("assets/drag.png");
    icons[1] = LoadTexture("assets/trash.png");
    icons[2] = LoadTexture("assets/eraser.png");
    icons[3] = LoadTexture("assets/touch.png");
    icons[4] = LoadTexture("assets/exit.png");

    GenTextureMipmaps(&icons[0]);
    GenTextureMipmaps(&icons[1]);
    GenTextureMipmaps(&icons[2]);
    GenTextureMipmaps(&icons[3]);
    GenTextureMipmaps(&icons[4]);

    SetTextureFilter(icons[0], TEXTURE_FILTER_TRILINEAR);
    SetTextureFilter(icons[1], TEXTURE_FILTER_TRILINEAR);
    SetTextureFilter(icons[2], TEXTURE_FILTER_TRILINEAR);
    SetTextureFilter(icons[3], TEXTURE_FILTER_TRILINEAR);
    SetTextureFilter(icons[4], TEXTURE_FILTER_TRILINEAR);

    RenderTexture2D canvas = LoadRenderTexture(DRAWING_MODE_WIDTH, DRAWING_MODE_HEIGHT);

    BeginTextureMode(canvas);
    ClearBackground(BLANK);
    EndTextureMode();

    Vector2 lastMousePos = { 0 };
    Vector2 prevMousePosition = {0};
    bool drawing = false;
    bool eraser = false;
    Color brushColor = WHITE;
    float brushSize = 10.0f;

    Rectangle clearButton     = { 20, 490 + 75 * 0 , 60, 60 };
    Rectangle eraserButton    = { 20, 490 + 75 * 1 , 60, 60 };
    Rectangle touchButton     = { 20, 490 + 75 * 2 , 60, 60 };
    Rectangle exitButton      = { 20, 490 + 75 * 3 , 60, 60 };

    Rectangle brushSizeSliderBar =       { 25 + 42, 100, 10, 150 };
    Rectangle brushSizeSliderCollision = { 0,  100,  100, 150 };
    Rectangle brushSizeHandle =          { 20 + 42, 175 - 4, 20, 8 };

    uiCamera.offset = (Vector2){ 0, 0 };
    uiCamera.target = (Vector2){ 0, 0 };
    uiCamera.rotation = 0.0f;
    uiCamera.zoom = 1.0f;

    bool isDragging = false;
    Vector2 dragOffset = { 0 };
    Rectangle dragBar = { 0, 0, 100, 80 };

    Color colorPalette[] = { RED, GREEN, BLUE, YELLOW, ORANGE, DARKBROWN, WHITE, BLACK };
    int selectedColorIndex = 6;

    while (!WindowShouldClose()) {
        Vector2 mousePos = GetMousePosition();
        Vector2 worldMousePos = GetScreenToWorld2D(mousePos, uiCamera);

        // Bad idea delete later
        // if (IsGestureDetected(GESTURE_DOUBLETAP)) {
        //     eraser = !eraser;
        // }

        if (CheckCollisionPointRec(worldMousePos, dragBar)) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                isDragging = true;
                dragOffset.x = mousePos.x - uiCamera.offset.x;
                dragOffset.y = mousePos.y - uiCamera.offset.y;
                prevMousePosition = mousePos;
            }
        }


        if (isDragging) {
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                if (currentMode == MODE_DRAW) {
                    uiCamera.offset.x = Clamp(mousePos.x - dragOffset.x, 0, 1820);
                    uiCamera.offset.y = Clamp(mousePos.y - dragOffset.y, 0, 280);
                } else {
                    Vector2 delta = {mousePos.x - prevMousePosition.x, mousePos.y - prevMousePosition.y};
                    Vector2 windowPos = {GetWindowPosition().x + delta.x, GetWindowPosition().y + delta.y};
                    SetWindowPosition((int)Clamp(windowPos.x, 0, DRAWING_MODE_WIDTH - 100), (int)Clamp(windowPos.y, 0, DRAWING_MODE_HEIGHT - 800));
                }
            } else {
                isDragging = false;
            }
        }

        if (currentMode == MODE_DRAW && IsMouseButtonDown(MOUSE_LEFT_BUTTON) && !isDragging && !CheckCollisionPointRec(worldMousePos, (Rectangle){ 0, 0, TOUCH_MODE_WIDTH, TOUCH_MODE_HEIGHT })) {
            if(!drawing) {
                drawing = true;
                lastMousePos = mousePos;
            }

            BeginTextureMode(canvas);
            if (eraser) {
                BeginBlendMode(BLEND_SUBTRACT_COLORS);
                DrawLineEx(lastMousePos, mousePos, brushSize * 8, BLANK);
                DrawCircleV(mousePos, brushSize * 4, BLANK);
                EndBlendMode();
            } else {
                DrawLineEx(lastMousePos, mousePos, brushSize, brushColor);
                DrawCircleV(mousePos, brushSize / 2, brushColor);
            }

            EndTextureMode();
            lastMousePos = mousePos;
        } else {
            drawing = false;
            lastMousePos = mousePos;
        }

        BeginDrawing();
        ClearBackground(BLANK);
        DrawTextureRec(canvas.texture, (Rectangle){ 0, 0, canvas.texture.width, -canvas.texture.height }, (Vector2){ 0, 0 }, WHITE);

        BeginMode2D(uiCamera);
        DrawRectangleRounded((Rectangle){ 0, 0, TOUCH_MODE_WIDTH, TOUCH_MODE_HEIGHT}, 0.3f, 32, (Color){36, 36, 36, 255});

        for (int i = 0; i < 8; i++) {
            Rectangle colorRect = { 10 + (i / 4) * 45, 290 + (i % 4) * 45, 35, 35 };
            if (CheckCollisionPointRec(worldMousePos, colorRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                brushColor = colorPalette[i];
                selectedColorIndex = i;
                eraser = false;
            }
            if (i == selectedColorIndex) {
                int radius = colorRect.width / 2;
                DrawCircle(colorRect.x + radius, colorRect.y + radius, radius + 1, WHITE);
                DrawCircle(colorRect.x + radius, colorRect.y + radius, radius - 1, colorPalette[i]);
            } else {
                DrawRectangleRec(colorRect, colorPalette[i]);
            }
        }

        if (Button(clearButton, BUTTON_COLOR, 1)) {
            BeginTextureMode(canvas);
            ClearBackground(BLANK);
            EndTextureMode();
        }

        if (Button(eraserButton, eraser ? (Color){40, 120, 60, 255} : BUTTON_COLOR, 2)) {
            eraser = !eraser;
        }

        if (Button(touchButton, currentMode == MODE_TOUCH ? (Color){40, 120, 60, 255} : BUTTON_COLOR, 3)) {
            if (currentMode == MODE_DRAW) {
                currentMode = MODE_TOUCH;
                SetWindowSize(TOUCH_MODE_WIDTH, TOUCH_MODE_HEIGHT);
                SetWindowPosition(uiCamera.offset.x,  uiCamera.offset.y);
                uiCamera.offset = (Vector2){ 0, 0 };
            } else {
                currentMode = MODE_DRAW;
                SetWindowSize(DRAWING_MODE_WIDTH, DRAWING_MODE_HEIGHT);
                uiCamera.offset = GetWindowPosition();
                SetWindowPosition(0, 0);
            }
        }

        if (Button(exitButton, (Color){128, 32, 16, 255}, 4)) {
            break;
        }

        if (!isDragging && IsMouseButtonDown(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(worldMousePos, brushSizeSliderCollision)) {
            brushSizeHandle.y = worldMousePos.y - brushSizeHandle.height / 2; // Adjust handle's y-position
            if (brushSizeHandle.y < brushSizeSliderBar.y) brushSizeHandle.y = brushSizeSliderBar.y; // Clamp to the top of the bar
            if (brushSizeHandle.y > brushSizeSliderBar.y + brushSizeSliderBar.height - brushSizeHandle.height)
                brushSizeHandle.y = brushSizeSliderBar.y + brushSizeSliderBar.height - brushSizeHandle.height; // Clamp to the bottom of the bar

            brushSize = 20.0f - ((brushSizeHandle.y - brushSizeSliderBar.y) / brushSizeSliderBar.height) * 20.0f; // Adjust brush size based on vertical position
            if (brushSize < 1.0f) brushSize = 1.0f;
        }

        DrawRectangleRounded(brushSizeSliderBar, 1.0f, 16, DARKGRAY);
        DrawRectangleRounded(brushSizeHandle, 1.0f, 16, LIGHTGRAY);
        DrawCircle(brushSizeSliderBar.x - 40, 175, brushSize / 2, WHITE);
        DrawTexturePro(icons[0], (Rectangle) {0, 0, 256, 256}, (Rectangle){25, 15, 50, 50}, (Vector2) {0,0}, 0, LIGHTGRAY);

        EndMode2D();

        if(eraser && IsMouseButtonDown(MOUSE_LEFT_BUTTON) && !isDragging && !CheckCollisionPointRec(worldMousePos, (Rectangle){ 0, 0, TOUCH_MODE_WIDTH, TOUCH_MODE_HEIGHT })){
            DrawCircleLinesV(mousePos, brushSize * 4, WHITE);
            DrawCircleLinesV(mousePos, brushSize * 4 + 1, BLACK);
        }

        EndDrawing();
    }

    CloseWindow();
}

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

// Windows DPI and DWM declarations
__declspec(dllimport) int __stdcall SetProcessDPIAware(void);

typedef struct {
    int cxLeftWidth;
    int cxRightWidth;
    int cyTopHeight;
    int cyBottomHeight;
} MARGINS;
__declspec(dllimport) long __stdcall DwmExtendFrameIntoClientArea(void* hWnd, const MARGINS* pMarInset);

#define BASE_TOOLBAR_WIDTH  100.0f
#define BASE_TOOLBAR_HEIGHT 800.0f
#define BUTTON_COLOR (Color){ 90, 100, 120, 255 }

typedef enum {
    MODE_DRAW,
    MODE_TOUCH
} AppMode;

static Texture icons[5] = { 0 };

static void DrawRectangleShadowed(Rectangle rect, float rounding, Color color, Color shadowColor, Vector2 shadowOffset) {
    DrawRectangleRounded((Rectangle){ rect.x + shadowOffset.x, rect.y + shadowOffset.y, rect.width, rect.height }, rounding, 16, shadowColor);
    DrawRectangleRounded(rect, rounding, 16, color);
}

static bool Button(Rectangle rect, Color buttonColor, int iconID, float scale) {
    Vector2 mousePos = GetMousePosition();
    bool isHovered = CheckCollisionPointRec(mousePos, rect);
    bool isPressed = isHovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool isClicked = isHovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);

    Color primaryColor = isPressed ? ColorBrightness(buttonColor, -0.1f) : (isHovered ? ColorBrightness(buttonColor, 0.2f) : buttonColor);
    float offset = isPressed ? (2.0f * scale) : 0.0f;
    float shadowOffset = isPressed ? (1.5f * scale) : (3.0f * scale);

    Rectangle drawRect = { rect.x, rect.y + offset, rect.width, rect.height };
    DrawRectangleShadowed(drawRect, 0.25f, primaryColor, ColorBrightness(buttonColor, -0.4f), (Vector2){ 0, shadowOffset });

    if (iconID >= 0 && iconID < 5 && icons[iconID].id > 0) {
        float pad = 9.0f * scale;
        Rectangle iconRect = { drawRect.x + pad, drawRect.y + pad, drawRect.width - pad * 2.0f, drawRect.height - pad * 2.0f };
        DrawTexturePro(icons[iconID],
                       (Rectangle){ 0, 0, (float)icons[iconID].width, (float)icons[iconID].height },
                       iconRect, (Vector2){ 0, 0 }, 0, WHITE);
    }

    return isClicked;
}

static void EnableWindowTransparency(void) {
    MARGINS margins = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(GetWindowHandle(), &margins);
}

static float GetScreenDpiScale(int screenHeight) {
    SetProcessDPIAware();
    float scale = (float)screenHeight / 1080.0f;
    if (scale < 1.0f) scale = 1.0f;
    // Cap toolbar height to fit screen comfortably
    if (BASE_TOOLBAR_HEIGHT * scale > screenHeight * 0.92f) {
        scale = (screenHeight * 0.92f) / BASE_TOOLBAR_HEIGHT;
    }
    return scale;
}

int main(void) {
    // 1. Enable DPI awareness before creating window
    SetProcessDPIAware();

    AppMode currentMode = MODE_DRAW;

    // 2. Configure window flags
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_TOPMOST | FLAG_VSYNC_HINT);
    InitWindow(320, 240, "ScreenPen");

    int monitor = GetCurrentMonitor();
    int screenWidth = GetMonitorWidth(monitor);
    int screenHeight = GetMonitorHeight(monitor);
    Vector2 monitorPosition = GetMonitorPosition(monitor);

    if (screenWidth <= 0) screenWidth = 1920;
    if (screenHeight <= 0) screenHeight = 1080;

    // Calculate HighDPI UI scale factor
    float scale = GetScreenDpiScale(screenHeight);
    float toolbarW = BASE_TOOLBAR_WIDTH * scale;
    float toolbarH = BASE_TOOLBAR_HEIGHT * scale;

    // Use screenHeight - 1 in fullscreen to avoid Windows DWM DirectFlip bypass (black screen / flicker)
    SetWindowSize(screenWidth, screenHeight - 1);
    SetWindowPosition((int)monitorPosition.x, (int)monitorPosition.y);
    SetTargetFPS(60);

    EnableWindowTransparency();

    // 3. Load UI icon textures
    icons[0] = LoadTexture("assets/drag.png");
    icons[1] = LoadTexture("assets/trash.png");
    icons[2] = LoadTexture("assets/eraser.png");
    icons[3] = LoadTexture("assets/touch.png");
    icons[4] = LoadTexture("assets/exit.png");

    for (int i = 0; i < 5; i++) {
        if (icons[i].id > 0) {
            GenTextureMipmaps(&icons[i]);
            SetTextureFilter(icons[i], TEXTURE_FILTER_TRILINEAR);
        }
    }

    // 4. Drawing Canvas (RenderTexture2D)
    RenderTexture2D canvas = LoadRenderTexture(screenWidth, screenHeight);
    BeginTextureMode(canvas);
    ClearBackground(BLANK);
    EndTextureMode();

    Vector2 lastMousePos = { 0 };
    bool drawing = false;
    bool eraser = false;
    Color brushColor = WHITE;

    // Toolbar state (direct pixel coordinates, no camera)
    Vector2 toolbarPos = { 20.0f * scale, 80.0f * scale };
    bool isDragging = false;
    Vector2 dragOffset = { 0 };

    float brushProgress = 0.4f; // 0.0 (thin) to 1.0 (thick)
    Color colorPalette[] = { RED, GREEN, BLUE, YELLOW, ORANGE, DARKBROWN, WHITE, BLACK };
    int selectedColorIndex = 6;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE)) break;

        Vector2 mousePos = GetMousePosition();

        float tx = (currentMode == MODE_DRAW) ? toolbarPos.x : 0.0f;
        float ty = (currentMode == MODE_DRAW) ? toolbarPos.y : 0.0f;

        Rectangle toolbarRect = { tx, ty, toolbarW, toolbarH };
        Rectangle dragBar     = { tx, ty, toolbarW, 70.0f * scale };

        // Dragging the toolbar
        if (CheckCollisionPointRec(mousePos, dragBar)) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                isDragging = true;
                if (currentMode == MODE_DRAW) {
                    dragOffset = (Vector2){ mousePos.x - toolbarPos.x, mousePos.y - toolbarPos.y };
                } else {
                    dragOffset = mousePos;
                }
            }
        }

        if (isDragging) {
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                if (currentMode == MODE_DRAW) {
                    toolbarPos.x = Clamp(mousePos.x - dragOffset.x, 0.0f, (float)(screenWidth - toolbarW));
                    toolbarPos.y = Clamp(mousePos.y - dragOffset.y, 0.0f, (float)(screenHeight - toolbarH));
                } else {
                    Vector2 winPos = GetWindowPosition();
                    SetWindowPosition((int)(winPos.x + mousePos.x - dragOffset.x),
                                      (int)(winPos.y + mousePos.y - dragOffset.y));
                }
            } else {
                isDragging = false;
            }
        }

        bool overToolbar = CheckCollisionPointRec(mousePos, toolbarRect);

        // Slider dimensions
        Rectangle sliderBar       = { tx + (toolbarW - 10.0f * scale) / 2.0f, ty + 90.0f * scale, 10.0f * scale, 140.0f * scale };
        Rectangle sliderCollision = { tx, ty + 85.0f * scale, toolbarW, 150.0f * scale };

        // Handle brush slider interaction
        if (!isDragging && IsMouseButtonDown(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePos, sliderCollision)) {
            brushProgress = 1.0f - Clamp((mousePos.y - sliderBar.y) / sliderBar.height, 0.0f, 1.0f);
        }

        float brushSize = (3.0f + brushProgress * 30.0f) * scale;
        float handleY = sliderBar.y + (1.0f - brushProgress) * sliderBar.height;
        Rectangle handleRect = { sliderBar.x - 7.0f * scale, handleY - 6.0f * scale, 24.0f * scale, 12.0f * scale };

        // Drawing / Erasing on Canvas
        if (currentMode == MODE_DRAW && IsMouseButtonDown(MOUSE_LEFT_BUTTON) && !isDragging && !overToolbar) {
            if (!drawing) {
                drawing = true;
                lastMousePos = mousePos;
            }

            BeginTextureMode(canvas);
            if (eraser) {
                // Erase cleanly to transparent (0, 0, 0, 0)
                rlSetBlendFactors(RL_ZERO, RL_ONE_MINUS_SRC_ALPHA, RL_FUNC_ADD);
                rlSetBlendMode(RL_BLEND_CUSTOM);
                DrawLineEx(lastMousePos, mousePos, brushSize * 3.0f, WHITE);
                DrawCircleV(mousePos, brushSize * 1.5f, WHITE);
                rlSetBlendMode(RL_BLEND_ALPHA);
            } else {
                DrawLineEx(lastMousePos, mousePos, brushSize, brushColor);
                DrawCircleV(mousePos, brushSize * 0.5f, brushColor);
            }
            EndTextureMode();

            lastMousePos = mousePos;
        } else {
            drawing = false;
            lastMousePos = mousePos;
        }

        // Color Palette Hitboxes (2 cols x 4 rows)
        float swatchSize = 34.0f * scale;
        float swatchGapX = 44.0f * scale;
        float swatchGapY = 42.0f * scale;
        float swatchesStartX = tx + (toolbarW - (swatchGapX + swatchSize)) / 2.0f;
        float swatchesStartY = ty + 260.0f * scale;

        if (!isDragging && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            for (int i = 0; i < 8; i++) {
                Rectangle colorRect = { swatchesStartX + (i / 4) * swatchGapX,
                                        swatchesStartY + (i % 4) * swatchGapY,
                                        swatchSize, swatchSize };
                if (CheckCollisionPointRec(mousePos, colorRect)) {
                    brushColor = colorPalette[i];
                    selectedColorIndex = i;
                    eraser = false;
                    break;
                }
            }
        }

        // Buttons Layout
        float btnSize = 60.0f * scale;
        float btnX = tx + (toolbarW - btnSize) / 2.0f;
        float btnStartY = ty + 450.0f * scale;
        float btnGapY = 75.0f * scale;

        Rectangle clearButton  = { btnX, btnStartY + 0.0f * btnGapY, btnSize, btnSize };
        Rectangle eraserButton = { btnX, btnStartY + 1.0f * btnGapY, btnSize, btnSize };
        Rectangle touchButton  = { btnX, btnStartY + 2.0f * btnGapY, btnSize, btnSize };
        Rectangle exitButton   = { btnX, btnStartY + 3.0f * btnGapY, btnSize, btnSize };

        // ----------------- RENDER FRAME -----------------
        BeginDrawing();
        ClearBackground(BLANK);

        // 1. Draw Canvas
        if (currentMode == MODE_DRAW) {
            DrawTextureRec(canvas.texture, (Rectangle){ 0, 0, (float)canvas.texture.width, (float)-canvas.texture.height }, (Vector2){ 0, 0 }, WHITE);
        }

        // 2. Toolbar Body
        DrawRectangleRounded(toolbarRect, 0.25f, 32, (Color){ 36, 36, 36, 255 });

        // 3. Drag Icon
        if (icons[0].id > 0) {
            float dragIconSize = 42.0f * scale;
            DrawTexturePro(icons[0],
                           (Rectangle){ 0, 0, (float)icons[0].width, (float)icons[0].height },
                           (Rectangle){ tx + (toolbarW - dragIconSize) / 2.0f, ty + 15.0f * scale, dragIconSize, dragIconSize },
                           (Vector2){ 0, 0 }, 0, LIGHTGRAY);
        }

        // 4. Slider Track & Handle
        DrawRectangleRounded(sliderBar, 1.0f, 16, DARKGRAY);
        DrawRectangleRounded(handleRect, 0.5f, 16, LIGHTGRAY);
        // Brush preview circle
        DrawCircle((int)(tx + 22.0f * scale), (int)(sliderBar.y + sliderBar.height / 2.0f), (brushSize * 0.4f), brushColor);

        // 5. Color Palette Swatches
        for (int i = 0; i < 8; i++) {
            Rectangle colorRect = { swatchesStartX + (i / 4) * swatchGapX,
                                    swatchesStartY + (i % 4) * swatchGapY,
                                    swatchSize, swatchSize };
            if (i == selectedColorIndex) {
                int radius = (int)(swatchSize / 2.0f);
                DrawCircle((int)(colorRect.x + radius), (int)(colorRect.y + radius), (float)(radius + 3.0f * scale), WHITE);
                DrawCircle((int)(colorRect.x + radius), (int)(colorRect.y + radius), (float)(radius - 1.0f * scale), colorPalette[i]);
            } else {
                DrawRectangleRounded(colorRect, 0.2f, 8, colorPalette[i]);
            }
        }

        // 6. Action Buttons (Rendered and Checked inside Drawing block)
        if (Button(clearButton, BUTTON_COLOR, 1, scale)) {
            BeginTextureMode(canvas);
            ClearBackground(BLANK);
            EndTextureMode();
        }

        if (Button(eraserButton, eraser ? (Color){ 40, 140, 70, 255 } : BUTTON_COLOR, 2, scale)) {
            eraser = !eraser;
        }

        if (Button(touchButton, currentMode == MODE_TOUCH ? (Color){ 40, 140, 70, 255 } : BUTTON_COLOR, 3, scale)) {
            if (currentMode == MODE_DRAW) {
                currentMode = MODE_TOUCH;
                SetWindowSize((int)toolbarW, (int)toolbarH);
                SetWindowPosition((int)(monitorPosition.x + toolbarPos.x), (int)(monitorPosition.y + toolbarPos.y));
            } else {
                currentMode = MODE_DRAW;
                Vector2 winPos = GetWindowPosition();
                toolbarPos = (Vector2){ winPos.x - monitorPosition.x, winPos.y - monitorPosition.y };
                toolbarPos.x = Clamp(toolbarPos.x, 0.0f, (float)(screenWidth - toolbarW));
                toolbarPos.y = Clamp(toolbarPos.y, 0.0f, (float)(screenHeight - toolbarH));
                SetWindowSize(screenWidth, screenHeight - 1);
                SetWindowPosition((int)monitorPosition.x, (int)monitorPosition.y);
                EnableWindowTransparency();
            }
        }

        if (Button(exitButton, (Color){ 160, 36, 24, 255 }, 4, scale)) {
            break;
        }

        // 7. Eraser indicator circle on screen
        if (eraser && currentMode == MODE_DRAW && !overToolbar) {
            DrawCircleLinesV(mousePos, brushSize * 1.5f, WHITE);
            DrawCircleLinesV(mousePos, brushSize * 1.5f + 1.0f, BLACK);
        }

        EndDrawing();
    }

    // Cleanup
    for (int i = 0; i < 5; i++) {
        if (icons[i].id > 0) UnloadTexture(icons[i]);
    }
    UnloadRenderTexture(canvas);
    CloseWindow();

    return 0;
}

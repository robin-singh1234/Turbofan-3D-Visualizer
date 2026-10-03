 #include "raylib.h"
#include <cmath>
#include <algorithm>
#include <fstream>

float ReadRPM()
{
    std::ifstream file("rpm.txt");
    float rpm = 3200;

    if (file.is_open())
        file >> rpm;

    return std::clamp(rpm, 1000.0f, 10000.0f);
}

bool ReadEngine()
{
    std::ifstream file("engine.txt");
    int state = 1;

    if (file.is_open())
        file >> state;

    return state == 1;
}

void Panel(int x, int y, int w, int h)
{
    DrawRectangle(x, y, w, h, Color{18, 25, 42, 240});
    DrawRectangleLines(x, y, w, h, Color{55, 100, 150, 255});
}

void RPMGauge(int cx, int cy, int radius, float rpm)
{
    DrawCircleLines(cx, cy, radius, Color{60, 80, 105, 255});
    DrawCircleLines(cx, cy, radius - 8, Color{35, 45, 65, 255});

    float value = (rpm - 1000.0f) / 9000.0f;
    value = std::clamp(value, 0.0f, 1.0f);

    for (int i = 0; i < 40; i++)
    {
        float a1 = PI * 0.75f + PI * 1.5f * i / 40.0f;
        float a2 = PI * 0.75f + PI * 1.5f * (i + 1) / 40.0f;

        Color c = (i < value * 40)
            ? SKYBLUE
            : Color{45, 55, 75, 255};

        Vector2 p1{
            cx + cosf(a1) * (radius - 15),
            cy + sinf(a1) * (radius - 15)
        };

        Vector2 p2{
            cx + cosf(a2) * (radius - 15),
            cy + sinf(a2) * (radius - 15)
        };

        DrawLineEx(p1, p2, 5, c);
    }

    const char* text = TextFormat("RPM %d", (int)rpm);

    DrawText(
        text,
        cx - MeasureText(text, 26) / 2,
        cy - 13,
        26,
        WHITE
    );
}

int main()
{
    const int W = 1200;
    const int H = 700;

    InitWindow(W, H, "Turbofan 3D Visualizer");
    SetTargetFPS(60);

    // =========================
    // AUDIO
    // =========================

    InitAudioDevice();

    Music engineSound = {0};
    bool soundLoaded = false;

    if (FileExists("assets/engine.mp3"))
    {
        engineSound = LoadMusicStream("assets/engine.mp3");
        soundLoaded = true;

        engineSound.looping = true;

        PlayMusicStream(engineSound);
    }

    // =========================
    // CAMERA
    // =========================

    Camera3D camera = {0};

    camera.position = {9, 5, 11};
    camera.target = {0, 0, 0};
    camera.up = {0, 1, 0};
    camera.fovy = 45;
    camera.projection = CAMERA_PERSPECTIVE;

    float rotation = 0;
    float cameraAngle = 0.75f;
    float cameraDistance = 14;

    float history[100] = {};
    int historyIndex = 0;
    float historyTimer = 0;

    // =========================
    // MAIN LOOP
    // =========================

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        float rpm = ReadRPM();
        bool engineRunning = ReadEngine();

        // AUDIO UPDATE
        if (soundLoaded)
        {
            UpdateMusicStream(engineSound);

            if (engineRunning)
            {
                if (!IsMusicStreamPlaying(engineSound))
                    PlayMusicStream(engineSound);
            }
            else
            {
                if (IsMusicStreamPlaying(engineSound))
                    PauseMusicStream(engineSound);
            }
        }

        // =========================
        // CAMERA CONTROL
        // =========================

        if (IsKeyDown(KEY_LEFT))
            cameraAngle -= 1.2f * dt;

        if (IsKeyDown(KEY_RIGHT))
            cameraAngle += 1.2f * dt;

        if (IsKeyDown(KEY_UP))
            cameraDistance -= 5 * dt;

        if (IsKeyDown(KEY_DOWN))
            cameraDistance += 5 * dt;

        cameraDistance = std::clamp(
            cameraDistance,
            8.0f,
            25.0f
        );

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            Vector2 mouse = GetMouseDelta();

            cameraAngle += mouse.x * 0.005f;
            camera.position.y -= mouse.y * 0.03f;

            camera.position.y = std::clamp(
                camera.position.y,
                2.0f,
                12.0f
            );
        }

        float wheel = GetMouseWheelMove();

        cameraDistance -= wheel;

        cameraDistance = std::clamp(
            cameraDistance,
            8.0f,
            25.0f
        );

        camera.position = {
            cosf(cameraAngle) * cameraDistance,
            camera.position.y,
            sinf(cameraAngle) * cameraDistance
        };

        camera.target = {0, 0, 0};

        // =========================
        // ENGINE
        // =========================

        if (engineRunning)
            rotation += rpm * 0.000012f;

        float temperature =
            300.0f +
            (rpm / 10000.0f) * 1100.0f;

        float pressure =
            15.0f +
            (rpm / 10000.0f) * 85.0f;

        // =========================
        // RPM HISTORY
        // =========================

        historyTimer += dt;

        if (historyTimer >= 0.1f)
        {
            historyTimer = 0;

            history[historyIndex] = rpm;

            historyIndex++;

            if (historyIndex >= 100)
                historyIndex = 0;
        }

        BeginDrawing();

        ClearBackground(
            Color{7, 11, 20, 255}
        );

        // =========================
        // 3D ENGINE
        // =========================

        BeginMode3D(camera);

        DrawGrid(30, 1);

        // Main engine body
        DrawCylinder(
            {0, 0, 0},
            2.2f,
            2.2f,
            5,
            48,
            DARKGRAY
        );

        // Fan housing
        DrawCylinder(
            {0, 0, -2.55f},
            2.45f,
            2.45f,
            0.35f,
            48,
            GRAY
        );

        DrawCylinder(
            {0, 0, -2.78f},
            1.9f,
            1.9f,
            0.2f,
            40,
            Color{25, 32, 45, 255}
        );

        // Fan hub
        DrawSphere(
            {0, 0, -2.95f},
            0.62f,
            LIGHTGRAY
        );

        // Fan blades
        for (int i = 0; i < 16; i++)
        {
            float angle =
                rotation +
                i * (2 * PI / 16);

            float x = cosf(angle) * 1.25f;
            float y = sinf(angle) * 1.25f;

            DrawCube(
                {x, y, -2.95f},
                0.25f,
                1.0f,
                0.16f,
                BLUE
            );
        }

        // Central shaft
        DrawCylinder(
            {0, 0, 0},
            0.22f,
            0.22f,
            7,
            24,
            LIGHTGRAY
        );

        // =========================
        // COMPRESSOR
        // =========================

        for (int stage = 0; stage < 4; stage++)
        {
            float z = -1.7f + stage * 0.85f;

            float size =
                1.75f - stage * 0.08f;

            DrawCylinder(
                {0, 0, z},
                size,
                size,
                0.28f,
                36,
                Color{65, 75, 90, 255}
            );

            for (int b = 0; b < 10; b++)
            {
                float angle =
                    rotation * 1.2f +
                    b * (2 * PI / 10);

                float x =
                    cosf(angle) *
                    (1.05f - stage * 0.06f);

                float y =
                    sinf(angle) *
                    (1.05f - stage * 0.06f);

                DrawCube(
                    {x, y, z},
                    0.14f,
                    0.60f,
                    0.12f,
                    SKYBLUE
                );
            }
        }

        // =========================
        // COMBUSTION
        // =========================

        DrawCylinder(
            {0, 0, 1.1f},
            1.65f,
            1.65f,
            1.3f,
            40,
            MAROON
        );

        DrawCylinder(
            {0, 0, 1.15f},
            1.25f,
            1.25f,
            1,
            36,
            ORANGE
        );

        if (engineRunning)
        {
            DrawSphere(
                {0, 0, 1.55f},
                0.75f,
                Color{255, 100, 20, 130}
            );
        }

        // =========================
        // TURBINE
        // =========================

        for (int stage = 0; stage < 3; stage++)
        {
            float z = 2.0f + stage * 0.65f;

            float size =
                1.45f - stage * 0.08f;

            DrawCylinder(
                {0, 0, z},
                size,
                size,
                0.25f,
                36,
                GRAY
            );

            for (int b = 0; b < 10; b++)
            {
                float angle =
                    -rotation * 1.3f +
                    b * (2 * PI / 10);

                float x =
                    cosf(angle) *
                    (0.95f - stage * 0.05f);

                float y =
                    sinf(angle) *
                    (0.95f - stage * 0.05f);

                DrawCube(
                    {x, y, z},
                    0.13f,
                    0.52f,
                    0.12f,
                    LIGHTGRAY
                );
            }
        }

        // =========================
        // EXHAUST
        // =========================

        DrawCylinder(
            {0, 0, 3.35f},
            1.0f,
            1.0f,
            1.4f,
            32,
            DARKGRAY
        );

        if (engineRunning)
        {
            float flame =
                0.35f +
                rpm / 10000.0f * 0.8f;

            DrawSphere(
                {0, 0, 4.15f},
                flame,
                Color{255, 80, 10, 150}
            );

            DrawSphere(
                {0, 0, 4.55f},
                flame * 0.55f,
                Color{255, 210, 60, 180}
            );
        }

        EndMode3D();

        // =========================
        // TOP HUD
        // =========================

        DrawRectangle(
            0,
            0,
            W,
            82,
            Color{12, 18, 32, 245}
        );

        DrawText(
            "TURBOFAN 3D VISUALIZER",
            25,
            16,
            30,
            WHITE
        );

        DrawText(
            engineRunning
                ? "ENGINE STATUS: RUNNING"
                : "ENGINE STATUS: STOPPED",
            28,
            51,
            17,
            engineRunning ? GREEN : RED
        );

        DrawText(
            TextFormat("RPM: %d", (int)rpm),
            900,
            18,
            22,
            SKYBLUE
        );

        DrawText(
            "HAND CONTROL ENABLED",
            875,
            50,
            14,
            LIGHTGRAY
        );

        // =========================
        // TELEMETRY
        // =========================

        Panel(
            20,
            105,
            270,
            210
        );

        DrawText(
            "ENGINE TELEMETRY",
            40,
            122,
            20,
            WHITE
        );

        DrawText(
            TextFormat("RPM          %d", (int)rpm),
            40,
            165,
            18,
            SKYBLUE
        );

        DrawText(
            TextFormat("Temperature  %.0f K", temperature),
            40,
            200,
            18,
            ORANGE
        );

        DrawText(
            TextFormat("Pressure     %.1f bar", pressure),
            40,
            235,
            18,
            GREEN
        );

        DrawText(
            "Rotation     ACTIVE",
            40,
            270,
            18,
            engineRunning ? GREEN : RED
        );

        // =========================
        // RPM GAUGE
        // =========================

        Panel(
            20,
            335,
            270,
            280
        );

        DrawText(
            "RPM GAUGE",
            100,
            350,
            20,
            WHITE
        );

        RPMGauge(
            155,
            485,
            105,
            rpm
        );

        // =========================
        // RPM GRAPH
        // =========================

        const int graphX = 315;
        const int graphY = 490;
        const int graphW = 540;
        const int graphH = 125;

        Panel(
            graphX,
            graphY,
            graphW,
            graphH
        );

        DrawText(
            "LIVE RPM HISTORY",
            graphX + 15,
            graphY + 10,
            18,
            WHITE
        );

        for (int i = 1; i < 100; i++)
        {
            int a =
                (historyIndex + i - 1) % 100;

            int b =
                (historyIndex + i) % 100;

            float v1 =
                (history[a] - 1000) / 9000.0f;

            float v2 =
                (history[b] - 1000) / 9000.0f;

            v1 = std::clamp(v1, 0.0f, 1.0f);
            v2 = std::clamp(v2, 0.0f, 1.0f);

            Vector2 p1{
                graphX + 15 + i * 5.0f,
                graphY + 105 - v1 * 75
            };

            Vector2 p2{
                graphX + 15 + (i + 1) * 5.0f,
                graphY + 105 - v2 * 75
            };

            DrawLineEx(
                p1,
                p2,
                2,
                SKYBLUE
            );
        }

        // =========================
        // GESTURE CONTROL
        // =========================

        Panel(
            880,
            105,
            300,
            510
        );

        DrawText(
            "GESTURE CONTROL",
            920,
            125,
            22,
            WHITE
        );

        DrawText(
            "OPEN HAND",
            915,
            175,
            18,
            GREEN
        );

        DrawText(
            "+10 RPM",
            1040,
            175,
            16,
            LIGHTGRAY
        );

        DrawText(
            "FIST",
            915,
            220,
            18,
            RED
        );

        DrawText(
            "-10 RPM",
            1040,
            220,
            16,
            LIGHTGRAY
        );

        DrawText(
            "ONE FINGER",
            915,
            265,
            18,
            YELLOW
        );

        DrawText(
            "START / STOP",
            1040,
            265,
            14,
            LIGHTGRAY
        );

        DrawText(
            "TWO FINGERS",
            915,
            310,
            18,
            ORANGE
        );

        DrawText(
            "BOOST +50",
            1040,
            310,
            14,
            LIGHTGRAY
        );

        DrawText(
            "THREE FINGERS",
            915,
            355,
            18,
            SKYBLUE
        );

        DrawText(
            "NORMAL",
            1040,
            355,
            14,
            LIGHTGRAY
        );

        DrawText(
            "CAMERA",
            915,
            420,
            19,
            WHITE
        );

        DrawText(
            "Arrow Keys",
            915,
            455,
            16,
            LIGHTGRAY
        );

        DrawText(
            "Right Mouse Drag",
            915,
            485,
            16,
            LIGHTGRAY
        );

        DrawText(
            "Mouse Wheel = Zoom",
            915,
            515,
            16,
            LIGHTGRAY
        );

        DrawText(
            "RPM RANGE",
            915,
            565,
            17,
            WHITE
        );

        DrawText(
            "1000 - 10000",
            915,
            595,
            18,
            SKYBLUE
        );

        // =========================
        // FOOTER
        // =========================

        DrawRectangle(
            0,
            H - 32,
            W,
            32,
            Color{10, 15, 25, 245}
        );

        DrawText(
            "C++ + Raylib + MediaPipe",
            25,
            H - 24,
            14,
            LIGHTGRAY
        );

        DrawText(
            "ESC = EXIT",
            1080,
            H - 24,
            13,
            LIGHTGRAY
        );

        EndDrawing();
    }

    // =========================
    // CLEANUP
    // =========================

    if (soundLoaded)
    {
        StopMusicStream(engineSound);
        UnloadMusicStream(engineSound);
    }

    CloseAudioDevice();
    CloseWindow();

    return 0;
}
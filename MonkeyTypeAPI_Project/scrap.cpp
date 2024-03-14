#include "apiManager.h"
#include "plotData.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "implot.h"
#include <windows.h>
#include <d3d11.h>
#include <tchar.h>
#include <vector>
#include <sqlite3.h> 
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <thread>
#pragma comment(lib, "sqlite3.lib") // Make sure you link against SQLite3 if using a precompiled binary
#pragma comment(lib, "d3d11.lib")
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/*
// Global DirectX objects
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// APIManager instance
APIManager apiManager;

PlotData plotData;

std::string apiResponse;

// Load data callback function for sqlite3_exec
static int LoadDataCallback(void* data, int argc, char** argv, char** azColName) {
    auto* plotData = reinterpret_cast<PlotData*>(data);
    if (argc == 13 && argv[0] && argv[1] && argv[2] && argv[3] && argv[4] ) {
        // Assuming argv[0] is ID, argv[1] is WPM, and argv[2] is RawWPM
        // Convert ID, WPM, and RawWPM from string to float and add to the plot data vectors
        float id = static_cast<float>(atoi(argv[0]));
        float wpm = static_cast<float>(atof(argv[1]));
        float rawWPM = static_cast<float>(atof(argv[2]));
        float accuracy = static_cast<float>(atof(argv[3]));
        std::string timestamp = argv[4];
        std::string mode = argv[5];
        std::string mode2 = argv[6];
        int correctChars = atoi(argv[7]);
        int incorrectChars = atoi(argv[8]);
        int extraChars = atoi(argv[9]);
        int missedChars = atoi(argv[10]);
        int restartCount = atoi(argv[11]);
        int testDuration = atoi(argv[12]);


        plotData->ids.push_back(id);
        plotData->wpms.push_back(wpm);
        plotData->rawWPMs.push_back(rawWPM);
        plotData->accuracies.push_back(accuracy);
        plotData->timestamps.push_back(timestamp);
        plotData->modes.push_back(mode);
        plotData->modes2.push_back(mode2);
        plotData->correctChars.push_back(correctChars);
        plotData->incorrectChars.push_back(incorrectChars);
        plotData->extraChars.push_back(incorrectChars);
        plotData->missedChars.push_back(missedChars);
        plotData->restartCounts.push_back(restartCount);
        plotData->testDurations.push_back(testDuration);


    }
    return 0;
}

// Function to query WPM data from the database
bool GetWPMDataFromDatabase(PlotData& plotData) {
    sqlite3* db;
    char* errMsg = nullptr;

    if (sqlite3_open("monkeytype.db", &db) != SQLITE_OK) {
        sqlite3_close(db);
        return false;
    }

    const char* sql = "SELECT ID, WPM, rawWPM, Accuracy, Timestamp, Mode, Mode2, CorrectChars, IncorrectChars, ExtraChars, MissedChars, RestartCount, TestDuration FROM TestResults ORDER BY ID ASC;";
    if (sqlite3_exec(db, sql, LoadDataCallback, &plotData, &errMsg) != SQLITE_OK) {
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    sqlite3_close(db);
    return true;
}
bool newDataFetched = false;

// Function to periodically update the plot
void UpdatePlot(PlotData& plotData) {
    // Fetch new data from the database
    if (GetWPMDataFromDatabase(plotData)) {
        // Clear the plot data vectors before parsing new data
        plotData.clear();

        // Parse the fetched data and populate the plot data struct
        apiManager.parseData(apiResponse, plotData);

        // Set the flag to indicate that new data has been fetched
        newDataFetched = true;
    }
    else {
        // If fetching data fails, set the flag to false
        newDataFetched = false;
    }
}
// Main application entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui + ImPlot Example"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Dear ImGui + ImPlot SQLite Example"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, nCmdShow);
    ::UpdateWindow(hwnd);

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext(); // Initialize ImPlot context
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    auto lastUpdateTime = std::chrono::steady_clock::now();

    while (msg.message != WM_QUIT) {
        if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }

        // Check if 5 seconds have elapsed since the last update
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastUpdateTime).count();
        if (elapsedTime >= 5 || newDataFetched) {
            // Update plot data every 5 seconds or when new data is fetched
            UpdatePlot(plotData);
            lastUpdateTime = currentTime; // Update last update time

            // Reset the flag after updating the plot
            newDataFetched = false;
        }

        // Start the ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();




        // PLOTTING HERE --------------------------------------------------------------------------------------------------------
        

        // Load data from the database into the PlotData struct
        if (GetWPMDataFromDatabase(plotData)) {
            if (ImGui::Begin("WPM Chart")) {
                // Use BeginPlot with title and no specific flags for axes
                if (ImPlot::BeginPlot("WPM Over Time")) {

                    // Assuming ids are unique and sorted, manually set x-axis ticks
                    std::vector<double> idTicks(plotData.ids.begin(), plotData.ids.end()); // Convert float to double for ImPlot
                    std::vector<std::string> idLabels(plotData.ids.size());
                    std::transform(plotData.ids.begin(), plotData.ids.end(), idLabels.begin(), [](float id) {
                        return std::to_string(static_cast<int>(id));
                        });

                    // Convert std::string labels to const char* for ImPlot
                    std::vector<const char*> idLabelCStrs;
                    for (const auto& label : idLabels) {
                        idLabelCStrs.push_back(label.c_str());
                    }

                    // Set up integer ticks for the x-axis
                    ImPlot::SetupAxisTicks(ImAxis_X1, idTicks.data(), static_cast<int>(idTicks.size()), idLabelCStrs.data());

                    // Plot the line with dots for each data point
                    ImPlot::PlotLine("WPM", plotData.ids.data(), plotData.wpms.data(), static_cast<int>(plotData.wpms.size()));

                    // Plot dots for each data point
                    ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Circle);
                    ImPlot::PlotScatter("Data Points", plotData.ids.data(), plotData.wpms.data(), static_cast<int>(plotData.wpms.size()));

                    // Add a boolean flag to track if a tooltip is being displayed
                        static bool tooltipShown = false;

                    // Inside the plotting section, modify the tooltip logic as follows:
                    if (ImPlot::IsPlotHovered()) {
                        for (size_t i = 0; i < plotData.ids.size(); ++i) {
                            ImVec2 plot_pos = ImPlot::PlotToPixels(plotData.ids[i], plotData.wpms[i]); // Convert data point to plot space
                            ImVec2 mouse_pos = ImGui::GetMousePos(); // Get mouse position

                            // Calculate distance between mouse and data point
                            float distance = sqrtf((plot_pos.x - mouse_pos.x) * (plot_pos.x - mouse_pos.x) +
                                (plot_pos.y - mouse_pos.y) * (plot_pos.y - mouse_pos.y));

                            // Define a threshold for proximity to consider the point hovered
                            float threshold = 10.0f; // Adjust as needed

                            if (distance < threshold) {
                                if (!tooltipShown) {
                                    ImGui::BeginTooltip();
                                    // Display all the details associated with the hovered data point
                                    ImGui::Text("ID: %d", static_cast<int>(plotData.ids[i]));
                                    ImGui::Text("WPM: %.2f", plotData.wpms[i]);
                                    // Add RawWPM data to the tooltip
                                    ImGui::Text("RawWPM: %.2f", plotData.rawWPMs[i]);
                                    ImGui::Text("Accuracy: %.2f", plotData.accuracies[i]);
                                    ImGui::Text("Timestamp: %s", plotData.timestamps[i].c_str());
                                    ImGui::Text("Mode 1: %s", plotData.modes[i].c_str());
                                    ImGui::Text("Mode 2: %s seconds.", plotData.modes2[i].c_str());
                                    ImGui::Text("CorrectChars: %d", plotData.correctChars[i]);
                                    ImGui::Text("IncorrectChars: %d", plotData.incorrectChars[i]);
                                    ImGui::Text("ExtraChars: %d", plotData.extraChars[i]);
                                    ImGui::Text("MissedChars: %d", plotData.missedChars[i]);
                                    ImGui::Text("RestartCount: %d", plotData.restartCounts[i]);
                                    ImGui::Text("TestDuration: %d seconds.", plotData.testDurations[i]);
                                    ImGui::EndTooltip();

                                    tooltipShown = true;
                                }
                                break;
                            }
                        }
                    }
                    else {
                        tooltipShown = false;
                    }

                    ImPlot::PopStyleVar();

                    ImPlot::EndPlot();
                }
                ImGui::End();
            }
        }

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.45f, 0.55f, 0.60f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    UINT createDeviceFlags = 0;
    // createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}*/
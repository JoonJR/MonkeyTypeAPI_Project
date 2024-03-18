
#include "apiManager.h"
#include "plotData.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "implot.h"
#define NOMINMAX
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
#pragma comment(lib, "sqlite3.lib") // Make sure you link against SQLite3 if using a precompiled binary
#pragma comment(lib, "d3d11.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

// Load data callback function for sqlite3_exec
static int LoadDataCallback(void* data, int argc, char** argv, char** azColName) {
    auto* plotData = reinterpret_cast<PlotData*>(data);
    if (argc == 13 && argv[0] && argv[1] && argv[2] && argv[3] && argv[4]) {
     
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

    const char* sql = "SELECT  ID, WPM, rawWPM, Accuracy, Timestamp, Mode, Mode2, CorrectChars, IncorrectChars, ExtraChars, MissedChars, RestartCount, TestDuration FROM TestResults ORDER BY ID ASC;";
    if (sqlite3_exec(db, sql, LoadDataCallback, &plotData, &errMsg) != SQLITE_OK) {
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    sqlite3_close(db);
    return true;
}

float PointToLineDistance(const ImVec2& pt, const ImVec2& lineStart, const ImVec2& lineEnd) {
    float l2 = pow(lineEnd.x - lineStart.x, 2) + pow(lineEnd.y - lineStart.y, 2);
    if (l2 == 0.0f) return sqrt(pow(pt.x - lineStart.x, 2) + pow(pt.y - lineStart.y, 2));
    float t = ((pt.x - lineStart.x) * (lineEnd.x - lineStart.x) + (pt.y - lineStart.y) * (lineEnd.y - lineStart.y)) / l2;
    t = std::max(0.0f, std::min(1.0f, t));
    ImVec2 projection = ImVec2(lineStart.x + t * (lineEnd.x - lineStart.x), lineStart.y + t * (lineEnd.y - lineStart.y));
    return sqrt(pow(pt.x - projection.x, 2) + pow(pt.y - projection.y, 2));
}

// Main application entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("MonkeyType API"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("MonkeyType API2"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

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
    
    // ImGui Style

    ImGuiStyle& customStyle = ImGui::GetStyle();
    customStyle.Colors[ImGuiCol_Text] = ImVec4(0.529f, 0.16f, 0.89f, 1.00f);
    customStyle.Colors[ImGuiCol_Border] = ImVec4(0.529f, 0.16f, 0.89f, 1.00f);
    customStyle.Colors[ImGuiCol_BorderShadow] = ImVec4(0.80f, 0.0f, 0.0f, 1.00f);
    customStyle.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
    customStyle.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
    customStyle.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
    customStyle.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.5f); // Color when resizing grip is active
    customStyle.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.5f); // Color when resizing grip is hovered

    //ImPlot Style 
    
    ImPlotStyle& plotStyle = ImPlot::GetStyle();
    plotStyle.Colors[ImPlotCol_Line] = ImVec4(0.529f, 0.16f, 1.0f, 1.00f);
    plotStyle.LineWeight = 2.0f;
    plotStyle.PlotPadding = ImVec2(0, 0); // Adjusts padding around the plot area
    plotStyle.Colors[ImPlotCol_AxisGrid] = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // White Grid
    plotStyle.Colors[ImPlotCol_AxisBgHovered] = ImVec4(0.529f, 0.16f, 0.89f, 0.25f);
    plotStyle.Colors[ImPlotCol_AxisBgActive] = ImVec4(0.529f, 0.16f, 0.89f, 0.25f);
  
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    // Variables to keep track of time for API call
    std::chrono::steady_clock::time_point lastAPICallTime = std::chrono::steady_clock::now();
    const std::chrono::seconds apiCallInterval(5); // Make API call every 5 seconds

    while (msg.message != WM_QUIT) {
        // Check for messages
        if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }

        // Calculate elapsed time since last API call
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastAPICallTime);

        // Check if it's time to make the API call
        if (elapsedTime >= apiCallInterval) {
            // Make API call
            APIManager apiManager;
            apiManager.fetchParseAndInsertData();

            // Update last API call time
            lastAPICallTime = std::chrono::steady_clock::now();
        }

        // Start the ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        // PLOTTING HERE 
        PlotData plotData;
        
        // Load data from the database into the PlotData struct
        if (GetWPMDataFromDatabase(plotData)) {

            std::vector<float> runningAverages = plotData.calculateRunningAverage(plotData.wpms);
            int totalDurationSeconds = plotData.getTotalTestDuration();
            int minutes = totalDurationSeconds / 60;
            int seconds = totalDurationSeconds % 60;
            float bestWPM = plotData.getBestWPM();
            float worstWPM = plotData.getWorstWPM();
            float averageWPM = plotData.getAverageWPM();
            float averageAccuracy = plotData.getAverageAccuracy();

            if (ImGui::Begin("WPM Chart")) {
                
                if(ImGui::BeginChild("Time Display", ImVec2(1030, 50), true)) {
                    ImGui::Indent(45.0f);
                    ImGui::Text("Total Time Typed: %d minutes and %d seconds", minutes, seconds);
                    ImGui::SameLine(); // Keep the next item on the same line
                    ImGui::Text("| Best WPM: %.2f", bestWPM);
                    ImGui::SameLine();
                    ImGui::Text("| Worst WPM: %.2f", worstWPM);
                    ImGui::SameLine();
                    ImGui::Text("| Average WPM: %.2f", averageWPM);
                    ImGui::SameLine();
                    ImGui::Text("| Average Accuracy: %.2f", averageAccuracy);

                }
                ImGui::EndChild();

                ImGui::Spacing(); // Add a little space between the child window and the plot.

                if (ImPlot::BeginPlot("WPM Over Time", ImVec2(-1, 500))) {
                    
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


                    // Plot the running average
                    if (!runningAverages.empty()) {
                        ImPlot::SetNextLineStyle(ImVec4(0.77f, 0.54f, 0.8f, 1.0f), 2.0f); 
                        ImPlot::PlotLine("Average WPM", plotData.ids.data(), runningAverages.data(), static_cast<int>(runningAverages.size()));
                    }
                  
                    // Plot dots for each data point
                    ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Circle);
                    ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5.0f, ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 1.0f, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                    ImPlot::PlotScatter("Data Points", plotData.ids.data(), plotData.wpms.data(), static_cast<int>(plotData.wpms.size()));

                    // Tooltip for specific data points
                    
                    ImVec2 mouse_pos = ImGui::GetMousePos();
                    ImPlotPoint mouse_plot_pos = ImPlot::GetPlotMousePos();

                    // Checking if mouse is close to any data point
                    bool isCloseToDataPoint = false;

                    for (size_t i = 0; i < plotData.ids.size(); ++i) {

                        ImPlotPoint plotPos = ImPlot::PlotToPixels(plotData.ids[i], plotData.wpms[i]);
                        float distance = sqrt(pow(plotPos.x - mouse_pos.x, 2) + pow(plotPos.y - mouse_pos.y, 2));
                        float threshold = 5.0f; // Threshold in screen pixels

                        if (distance < threshold) {
                            isCloseToDataPoint = true;
                            ImGui::BeginTooltip();

                            // Display all the details associated with the hovered data point
                            ImGui::Text("ID: %d", static_cast<int>(plotData.ids[i]));
                            ImGui::Text("WPM: %.2f", plotData.wpms[i]);

                            // Add detailted data to the tooltip
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
                            break;
                    }
                }
                    // Tooltip for running average when not close to a specific data point
                     // Determine if mouse is close to the average line
                    bool isCloseToAverageLine = false;
                    if (!isCloseToDataPoint) { // Only check if not already showing a data point tooltip
                        for (size_t i = 1; i < plotData.ids.size(); ++i) {
                            ImPlotPoint line_start = ImPlot::PlotToPixels(plotData.ids[i - 1], runningAverages[i - 1]);
                            ImPlotPoint line_end = ImPlot::PlotToPixels(plotData.ids[i], runningAverages[i]);

                            float distance = PointToLineDistance(mouse_pos, ImVec2(line_start.x, line_start.y), ImVec2(line_end.x, line_end.y));
                            if (distance < 5.0f) { // Threshold in pixels for "closeness" to the line
                                isCloseToAverageLine = true;
                                ImGui::BeginTooltip();
                                float avgWPM = runningAverages[std::min((size_t)mouse_plot_pos.x, runningAverages.size() - 1)];
                                ImGui::Text("Running Avg WPM up to this point: %.2f", avgWPM);
                                ImGui::EndTooltip();
                                break;
                            }
                        }
                    }
                    ImPlot::PopStyleVar();

                    ImPlot::EndPlot();
                }
                ImGui::End();
            }
        }

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, 1.00f };
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
}
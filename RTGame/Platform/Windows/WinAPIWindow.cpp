#include "WinAPIWindow.h"
#include "../DearImGui/imgui_impl_win32.h"

static const wchar_t windowClassName[] = L"RTGameWindowClass";

std::unique_ptr< Window > Window::Create( int width, int height, bool fullscreen )
{
  return std::unique_ptr< Window >( new WinAPIWindow( width, height, fullscreen ) );
}

WinAPIWindow::WinAPIWindow( int width, int height, bool fullscreen )
{
  WNDCLASSW windowClass;
  windowClass.style         = CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc   = WinAPIWindow::WindowProcStatic;
  windowClass.cbClsExtra    = 0;
  windowClass.cbWndExtra    = sizeof( this );
  windowClass.hInstance     = GetModuleHandleW( nullptr );
  windowClass.hIcon         = nullptr;
  windowClass.hCursor       = nullptr;
  windowClass.hbrBackground = nullptr;
  windowClass.lpszMenuName  = nullptr;
  windowClass.lpszClassName = windowClassName;
  if ( !RegisterClassW( &windowClass ) )
    return;

  POINT zeroPoint = { 0, 0 };
  auto monitor = MonitorFromPoint( zeroPoint, MONITOR_DEFAULTTOPRIMARY );
  MONITORINFO monitorInfo;
  monitorInfo.cbSize = sizeof( monitorInfo );
  GetMonitorInfoW( monitor, &monitorInfo );

  RECT     windowRect;
  unsigned windowStyle;
  unsigned windowStyleEx;

  if ( fullscreen )
  {
    width  = monitorInfo.rcMonitor.right  - monitorInfo.rcMonitor.left;
    height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

    windowRect    = monitorInfo.rcMonitor;
    windowStyle   = WS_OVERLAPPEDWINDOW & ~( WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX );
    windowStyleEx = 0;
  }
  else
  {
    RECT desktopRect;
    HWND desktopWindow = GetDesktopWindow();
    GetWindowRect( desktopWindow, &desktopRect );

    POINT centerPoint = { ( monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left ) / 2, ( monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top ) / 2 };
    windowRect        = { centerPoint.x - width / 2, centerPoint.y - height / 2, centerPoint.x + width / 2, centerPoint.y + height / 2 };
    windowStyle       = WS_OVERLAPPEDWINDOW;
    windowStyleEx     = 0;
  }

  AdjustWindowRectEx( &windowRect, WS_OVERLAPPEDWINDOW, FALSE, windowStyleEx );
  HWND newWindowHandle = CreateWindowExW( windowStyleEx
                                        , windowClassName
                                        , L"RTGame"
                                        , WS_OVERLAPPEDWINDOW
                                        , windowRect.left
                                        , windowRect.top
                                        , windowRect.right  - windowRect.left
                                        , windowRect.bottom - windowRect.top
                                        , nullptr
                                        , nullptr
                                        , windowClass.hInstance
                                        , nullptr );

  if ( fullscreen )
  {
    SetWindowLong( newWindowHandle, GWL_STYLE, windowStyle );
    SetWindowPos( newWindowHandle
                , HWND_TOP
                , windowRect.left
                , windowRect.top
                , windowRect.right - windowRect.left
                , windowRect.bottom - windowRect.top
                , SWP_NOACTIVATE | SWP_FRAMECHANGED );
  }

  SetWindowLongPtr( newWindowHandle, 0, (LONG_PTR)this );

  ShowWindow( newWindowHandle, SW_MAXIMIZE );

  windowHandle = newWindowHandle;

  ImGui_ImplWin32_Init( windowHandle );
}

LRESULT WinAPIWindow::WindowProcStatic( HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam )
{
  if ( auto window = reinterpret_cast< WinAPIWindow* >( GetWindowLongPtrW( windowHandle, 0 ) ) )
    return window->WindowProc( windowHandle, message, wParam, lParam );

  return DefWindowProcW( windowHandle, message, wParam, lParam );
}

WinAPIWindow::~WinAPIWindow()
{
  ImGui_ImplWin32_Shutdown();
  CloseWindow( windowHandle );
}

bool WinAPIWindow::IsValid() const
{
  return windowHandle != nullptr;
}

HWND WinAPIWindow::GetWindowHandle() const
{
  return windowHandle;
}

bool WinAPIWindow::ProcessMessages()
{
  MSG message;
  while ( PeekMessageW( &message, windowHandle, 0, 0, PM_REMOVE ) )
  {
    TranslateMessage( &message );
    DispatchMessageW( &message );
  }

  if ( GetActiveWindow() != windowHandle )
    Sleep( 1000 / 33 );

  return !closeRequested;
}

int WinAPIWindow::GetClientWidth()
{
  RECT clientRect;
  GetClientRect( windowHandle, &clientRect );
  return clientRect.right - clientRect.left;
}

int WinAPIWindow::GetClientHeight()
{
  RECT clientRect;
  GetClientRect( windowHandle, &clientRect );
  return clientRect.bottom - clientRect.top;
}

void WinAPIWindow::SetCaption( const wchar_t* caption )
{
  SetWindowTextW( windowHandle, caption );
}

void WinAPIWindow::DearImGuiNewFrame()
{
  ImGui_ImplWin32_NewFrame();
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

static bool DearImGuiTrapMouse()
{
  return ImGui::IsAnyWindowHovered();
}

static bool DearImGuiTrapKeyboard()
{
  return ImGui::IsAnyItemActive();
}

LRESULT WinAPIWindow::WindowProc( HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam )
{
  if ( ImGui_ImplWin32_WndProcHandler( windowHandle, message, wParam, lParam ) )
    return true;

  switch ( message )
  {
  case WM_CLOSE: closeRequested = true; break;

  case WM_LBUTTONDOWN: if ( !DearImGuiTrapMouse() ) SigMouseDown( 0, LOWORD( lParam ), HIWORD( lParam ) ); break;
  case WM_RBUTTONDOWN: if ( !DearImGuiTrapMouse() ) SigMouseDown( 1, LOWORD( lParam ), HIWORD( lParam ) ); break;
  case WM_MBUTTONDOWN: if ( !DearImGuiTrapMouse() ) SigMouseDown( 2, LOWORD( lParam ), HIWORD( lParam ) ); break;
  case WM_LBUTTONUP:   if ( !DearImGuiTrapMouse() ) SigMouseUp( 0 ); break;
  case WM_RBUTTONUP:   if ( !DearImGuiTrapMouse() ) SigMouseUp( 1 ); break;
  case WM_MBUTTONUP:   if ( !DearImGuiTrapMouse() ) SigMouseUp( 2 ); break;
  
  case WM_KEYUP:
    if ( !DearImGuiTrapKeyboard() )
      SigKeyUp( int( wParam ) );
    break;

  case WM_KEYDOWN:
    if ( wParam == VK_ESCAPE )
      closeRequested = true;
    else if ( !std::bitset< 32 >( lParam ).test( 30 ) && !DearImGuiTrapKeyboard() )
      SigKeyDown( int( wParam ) ); 
    break;

  case WM_MOUSEMOVE:
  {
    POINT mousePos = { GET_X_LPARAM( lParam ), GET_Y_LPARAM( lParam ) };
    if ( lastMousePosition.has_value() )
      SigMouseMoveBy( mousePos.x - lastMousePosition->x, mousePos.y - lastMousePosition->y );
    SigMouseMoveTo( mousePos.x, mousePos.y );
    lastMousePosition = mousePos;
    break;
  }

  case WM_SIZE:
    SigSizeChanged( LOWORD( lParam ), HIWORD( lParam ) );
    break;

  default:
    return DefWindowProcW( windowHandle, message, wParam, lParam );
  }

  return 0;
}


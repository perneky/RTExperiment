#pragma once

#include "../Window.h"

class WinAPIWindow final : public Window
{
  friend struct Window;

public:
  ~WinAPIWindow();

  bool IsValid() const override;

  HWND GetWindowHandle() const;

  bool ProcessMessages() override;

  int GetClientWidth() override;
  int GetClientHeight() override;

  void SetCaption( const wchar_t* caption ) override;

  void DearImGuiNewFrame() override;

private:
  WinAPIWindow( int width, int height, bool fullscreen );

  LRESULT WindowProc( HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam );
  static LRESULT CALLBACK WindowProcStatic( HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam );

  HWND windowHandle = nullptr;

  bool closeRequested = false;

  std::optional< POINT > lastMousePosition;
};
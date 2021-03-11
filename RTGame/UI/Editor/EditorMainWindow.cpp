#include "EditorMainWindow.h"
#include "Game/GameStage.h"
#include "Platform/Window.h"
#include "../DearImGui/imgui.h"
#include "../External/tinyxml2/tinyxml2.h"

std::wstring BrowseForFile( bool save )
{
  std::wstring result;

  CComPtr< IFileDialog > dialog;
  if SUCCEEDED( CoCreateInstance( save ? CLSID_FileSaveDialog : CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS( &dialog ) ) )
  {
    dialog->SetDefaultExtension( L"xml" );
    if SUCCEEDED( dialog->Show( nullptr ) )
    {
      CComPtr< IShellItem > item;
      if SUCCEEDED( dialog->GetResult( &item ) )
      {
        PWSTR pszFilePath;
        if SUCCEEDED( item->GetDisplayName( SIGDN_FILESYSPATH, &pszFilePath ) )
        {
          result = pszFilePath;
          CoTaskMemFree( pszFilePath );
        }
      }
    }
  }

  return result;
}

EditorMainWindow::EditorMainWindow( Window& window )
  : window( &window )
  , autoLoadLastLevel( true )
{
  if ( autoLoadLastLevel && !openedScenePath->empty() )
    requestInitialLoad = true;
}

EditorMainWindow::~EditorMainWindow()
{
}

void EditorMainWindow::Tick( CommandList& commandList, double timeElapsed )
{
  if ( requestInitialLoad )
  {
    requestInitialLoad = false;
    SigStageChangeRequested( openedScenePath->data() );
  }

  if ( ImGui::BeginMainMenuBar() )
  {
    if ( ImGui::BeginMenu( "File" ) )
    {
      if ( ImGui::MenuItem( "New scene", 0, false, true ) )
        SigStageChangeRequested( L"" );

      if ( ImGui::MenuItem( "Load scene...", 0, false, true ) )
      {
        auto path = BrowseForFile( false );
        if ( !path.empty() )
          SigStageChangeRequested( path.data() );
      }

      if ( ImGui::MenuItem( "Save scene as...", 0, false, !!stage ) )
      {
        auto path = BrowseForFile( true );
        if ( !path.empty() )
          SaveStage( path.data() );
      }

      if ( ImGui::MenuItem( "Save scene", "Ctrl+S", false, stage && !openedScenePath->empty() ) )
        SaveStage( openedScenePath->data() );

      ImGui::Separator();

      if ( ImGui::MenuItem( "Auto load last level on start", 0, autoLoadLastLevel ) )
        autoLoadLastLevel = !autoLoadLastLevel;

      ImGui::EndMenu();
    }

    if ( ImGui::BeginMenu( "View" ) )
    {
      if ( ImGui::MenuItem( "Show light markers", "Ctrl+L", showLightMarkers ) )
        showLightMarkers = !showLightMarkers;

      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }
}

void EditorMainWindow::SetStage( GameStage* stage, const wchar_t* path )
{
  this->stage = stage;
  openedScenePath = path;

  if ( path && path[ 0 ] != 0 )
    window->SetCaption( ( std::wstring( L"RTGame - " ) + path ).data() );
  else
    window->SetCaption( L"RTGame" );
}

bool EditorMainWindow::ShouldShowLightMarkers() const
{
  return showLightMarkers;
}

void EditorMainWindow::SaveStage( const wchar_t* path )
{
  using namespace tinyxml2;

  FILE* file = nullptr;
  if ( _wfopen_s( &file, path, L"wb" ) == 0 )
  {
    tinyxml2::XMLDocument doc;
    auto rootNode = doc.InsertFirstChild( doc.NewElement( "RTGame" ) );

    stage->Serialize( *rootNode );

    if ( doc.SaveFile( file ) == XMLError::XML_SUCCESS )
      openedScenePath = path;

    fclose( file );
  }
}

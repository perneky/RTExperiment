#pragma once

#include "Game/GameStage.h"
#include "Game/Entity.h"
#include "Game/CameraEntity.h"
#include "Platform/Window.h"
#include "Render/Camera.h"
#include "UI/Editor/ToolWindow.h"

namespace Sandbox
{
  std::vector< unsigned > windowSignalsForCamera;
  std::vector< std::function< void( CommandList&, float ) > > tickFunctions;

  void SetupStage( CommandList& commandList, GameStage& stage )
  {
    EntityId e;

    stage.SetSky( commandList, ParseGUID( "{ED542769-36A3-4AC8-ADE5-8E08E2F1EB32}" ) );

    stage.AddModelEntity( commandList, ParseGUID( "{BA35471F-C2B2-4D3C-A4BB-1A59B4675702}" ) ); // sidewalk

    e = stage.AddModelEntity( commandList, ParseGUID( "{98E09B9C-6220-46BA-9751-A8887E643395}" ) ); // fire hydrant
    e.GetEntity()->SetPosition( XMVectorSet( 3, 0, -3, 1 ) );

    e = stage.AddModelEntity( commandList, ParseGUID( "{4279F6AD-27DE-42B7-A1D4-04BD868F0637}" ) ); // newspaper
    e.GetEntity()->SetPosition( XMVectorSet( -3, 0, -2, 1 ) );

    e = stage.AddModelEntity( commandList, ParseGUID( "{D688A397-AB8E-43A2-B440-9C38E313E876}" ) ); // sign
    e.GetEntity()->SetPosition( XMVectorSet( -1.4f, 0, -1.3f, 1 ) );
    e.GetEntity()->SetRotation( XMVectorSet( 0, 80, 0, 1 ) );

    e = stage.AddModelEntity( commandList, ParseGUID( "{B87A8906-BDC5-4DC9-A052-47519E9C7AED}" ) ); // building floor level walls
    e.GetEntity()->SetPosition( XMVectorSet( 0.2f, 0, -0.2f, 1 ) );

    e = stage.AddModelEntity( commandList, ParseGUID( "{3AA0AAD2-D287-4EC3-91C8-FF50704C0781}" ) ); // bus stop
    e.GetEntity()->SetPosition( XMVectorSet( 3, 0, 3, 1 ) );
  }

  float cameraMoveX    = 0;
  float cameraMoveY    = 0;
  float cameraMoveZ    = 0;
  bool  cameraRotating = false;
  bool  cameraFastMode = false;
  bool  cameraSlowMode = false;

  void SetupCameraControl( GameStage& stage, Entity& cameraEntity, Window& window, ToolWindow& toolWindow )
  {
    if ( !windowSignalsForCamera.empty() )
    {
      assert( windowSignalsForCamera.size() == 6 );

      window.SigKeyDown.Disconnect( windowSignalsForCamera[ 0 ] );
      window.SigKeyUp.Disconnect( windowSignalsForCamera[ 1 ] );
      window.SigMouseDown.Disconnect( windowSignalsForCamera[ 2 ] );
      window.SigMouseUp.Disconnect( windowSignalsForCamera[ 3 ] );
      window.SigMouseMoveBy.Disconnect( windowSignalsForCamera[ 4 ] );
      window.SigMouseMoveTo.Disconnect( windowSignalsForCamera[ 5 ] );

      windowSignalsForCamera.clear();
    }

    windowSignalsForCamera.emplace_back( window.SigKeyDown.Connect( []( int key )
    {
      switch ( key )
      {
      case 'W':        cameraMoveZ   += 1;    break;
      case 'S':        cameraMoveZ   -= 1;    break;
      case 'A':        cameraMoveX   -= 1;    break;
      case 'D':        cameraMoveX   += 1;    break;
      case 'Q':        cameraMoveY   += 1;    break;
      case 'E':        cameraMoveY   -= 1;    break;
      case VK_SHIFT:   cameraFastMode = true; break;
      case VK_CONTROL: cameraSlowMode = true; break;
      }
    } ) );
    windowSignalsForCamera.emplace_back( window.SigKeyUp.Connect( []( int key )
    {
      switch ( key )
      {
      case 'W':        cameraMoveZ   -= 1;     break;
      case 'S':        cameraMoveZ   += 1;     break;
      case 'A':        cameraMoveX   += 1;     break;
      case 'D':        cameraMoveX   -= 1;     break;
      case 'Q':        cameraMoveY   -= 1;     break;
      case 'E':        cameraMoveY   += 1;     break;
      case VK_SHIFT:   cameraFastMode = false; break;
      case VK_CONTROL: cameraSlowMode = false; break;
      }
    } ) );
    windowSignalsForCamera.emplace_back( window.SigMouseDown.Connect( [&]( int button, int x, int y )
    {
      if ( button == 0 )
      {
        if ( toolWindow.GetActiveTool() == ToolWindow::ActiveTool::Wetness )
        {
          XMFLOAT3 hitPoint;
          if ( stage.Intersect( x, y, hitPoint ) )
          {
            tickFunctions.emplace_back( [&stage, &toolWindow, hitPoint]( CommandList& commandList, float timeElapsed )
            {
              float brushStrength;
              float brushRadius;
              float brushExponent;
              toolWindow.GetWetnessParams( brushStrength, brushRadius, brushExponent );
              stage.PaintWetness( commandList, hitPoint.x, hitPoint.z, brushStrength, brushRadius, brushExponent );
            } );

          }
        }
        else
          stage.Interact( x, y, true );
      }
      if ( button == 1 )
        cameraRotating = true;
    } ) );
    windowSignalsForCamera.emplace_back( window.SigMouseUp.Connect( [&]( int button )
    {
      if ( button == 0 )
        stage.ReleaseGizmo();
      if ( button == 1 )
        cameraRotating = false;
    } ) );
    windowSignalsForCamera.emplace_back( window.SigMouseMoveBy.Connect( [&]( int dx, int dy )
    {
      if ( cameraRotating )
      {
        if ( dx )
          cameraEntity.SetRotation( XMVectorAdd( cameraEntity.GetRotation(), XMVectorSet( 0, float( dx ), 0, 1 ) ) );
        if ( dy )
          cameraEntity.SetRotation( XMVectorAdd( cameraEntity.GetRotation(), XMVectorSet( float( dy ), 0, 0, 1 ) ) );
      }
    } ) );
    windowSignalsForCamera.emplace_back( window.SigMouseMoveTo.Connect( [&]( int x, int y )
    {
      if ( cameraRotating )
        return;

      stage.Interact( x, y, false );
    } ) );
  }

  void Tick( CommandList& commandList, float timeElapsed )
  {
    for ( auto& fn : tickFunctions )
      fn( commandList, timeElapsed );

    tickFunctions.clear();
  }

  void TickCamera( Entity& cameraEntity, float timeElapsed )
  {
    auto& camera = static_cast< CameraEntity* >( &cameraEntity )->GetCamera();

    XMVECTOR offset = XMVectorSet( 0, 0, 0, 1 );
    if ( cameraMoveX != 0 )
    {
      auto t = cameraMoveX * timeElapsed;
      offset = XMVectorMultiplyAdd( camera.GetRight(), XMVectorSet( t, t, t, 1 ), offset );
    }
    if ( cameraMoveY != 0 )
    {
      auto t = cameraMoveY * timeElapsed;
      offset = XMVectorMultiplyAdd( camera.GetUp(), XMVectorSet( t, t, t, 1 ), offset );
    }
    if ( cameraMoveZ != 0 )
    {
      auto t = cameraMoveZ * timeElapsed;
      offset = XMVectorMultiplyAdd( camera.GetDirection(), XMVectorSet( t, t, t, 1 ), offset );
    }

    if ( cameraFastMode )
      offset = XMVectorMultiply( offset, XMVectorSet( 10, 10, 10, 1 ) );
    else if ( cameraSlowMode )
      offset = XMVectorMultiply( offset, XMVectorSet( 0.2f, 0.2f, 0.2f, 1 ) );
    else
      offset = XMVectorMultiply( offset, XMVectorSet( 5, 5, 5, 1 ) );

    cameraEntity.SetPosition( XMVectorAdd( cameraEntity.GetPosition(), offset ) );
  }
}

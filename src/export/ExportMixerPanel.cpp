/**********************************************************************

  Audacity: A Digital Audio Editor

  ExportMixerPanel.cpp

  Dominic Mazzoni

**********************************************************************/


#include "ExportMixerPanel.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include <wx/bitmap.h>
#include <wx/brush.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/event.h>
#include <wx/font.h>
#include <wx/pen.h>
#include <wx/string.h>

#include "Internat.h"
#include "MixerOptions.h"
#include "AColor.h"

namespace
{
constexpr int kMinItemStep = 24;
constexpr int kMaxItemStep = 36;
constexpr int kVerticalPadding = 8;
}

BEGIN_EVENT_TABLE(ExportMixerPanel, wxScrolledWindow)
    EVT_PAINT(ExportMixerPanel::OnPaint)
    EVT_MOUSE_EVENTS(ExportMixerPanel::OnMouseEvent)
    EVT_SIZE(ExportMixerPanel::OnSize)
END_EVENT_TABLE()

ExportMixerPanel::ExportMixerPanel( wxWindow *parent, wxWindowID id,
      MixerOptions::Downmix *mixerSpec,
      wxArrayString trackNames,
      const wxPoint& pos, const wxSize& size):
   wxScrolledWindow(parent, id, pos, size, wxVSCROLL | wxTAB_TRAVERSAL | wxNO_BORDER)
   , mWidth{ 0 }
   , mHeight{ 0 }
   , mMixerSpec{ mixerSpec }
   , mChannelRects( mMixerSpec ? mMixerSpec->GetMaxNumChannels() : 0 )
   , mTrackRects( mMixerSpec ? mMixerSpec->GetNumTracks() : 0 )
   , mSelectedTrack{ -1 }
   , mSelectedChannel{ -1 }
   , mTrackNames{ std::move(trackNames) }
   , mBoxWidth{ 0 }
   , mChannelHeight{ 0 }
   , mTrackHeight{ 0 }
{
   SetBackgroundStyle( wxBG_STYLE_PAINT );
   SetScrollRate( 0, 12 );
   // Repaint via OnPaint rather than pixel-scrolling the client bitmap
   EnableScrolling( false, false );
}

ExportMixerPanel::~ExportMixerPanel() = default;

void ExportMixerPanel::UpdateScrollbars()
{
   if( !mMixerSpec )
      return;

   const unsigned int numTracks = mMixerSpec->GetNumTracks();
   const unsigned int numChannels = mMixerSpec->GetNumChannels();

   if( mSelectedTrack >= static_cast<int>( numTracks ) )
      mSelectedTrack = -1;
   if( mSelectedChannel >= static_cast<int>( numChannels ) )
      mSelectedChannel = -1;

   int clientWidth = 0, clientHeight = 0;
   GetClientSize( &clientWidth, &clientHeight );
   if( clientWidth <= 0 || clientHeight <= 0 )
      return;

   const int maxItems = static_cast<int>( std::max( numTracks, numChannels ) );
   const int desiredHeight = std::max(
      clientHeight, maxItems * kMinItemStep + 2 * kVerticalPadding );

   const wxSize currentVirtual = GetVirtualSize();
   if( currentVirtual.GetWidth() != clientWidth ||
       currentVirtual.GetHeight() != desiredHeight )
   {
      SetVirtualSize( clientWidth, desiredHeight );
   }
}

void ExportMixerPanel::OnSize(wxSizeEvent &event)
{
   UpdateScrollbars();
   Refresh( false );
   event.Skip();
}

//set the font on memDC such that text can fit in specified width and height
bool ExportMixerPanel::SetFont(wxMemoryDC &memDC, const wxString &text, int width,
      int height )
{
   if( width <= 2 || height <= 2 || text.empty() )
      return false;

   int l = 1, u = 13, w = 0, h = 0;
   bool fits = false;
   wxFont font = memDC.GetFont();

   font.SetPointSize( l );
   memDC.SetFont( font );
   memDC.GetTextExtent( text, &w, &h );
   if( w >= width || h >= height )
      return false;
   fits = true;

   while( l < u - 1 )
   {
      const int m = ( l + u ) / 2;
      font.SetPointSize( m );
      memDC.SetFont( font );
      memDC.GetTextExtent( text, &w, &h );

      if( w < width && h < height )
         l = m;
      else
         u = m;
   }
   font.SetPointSize( l );
   memDC.SetFont( font );
   return fits;
}

void ExportMixerPanel::OnPaint(wxPaintEvent & WXUNUSED(event))
{
   wxPaintDC dc( this );

   int width = 0, height = 0;
   GetClientSize( &width, &height );

   if( width <= 0 || height <= 0 )
      return;

   if( !mBitmap || mWidth != width || mHeight != height )
   {
      mWidth = width;
      mHeight = height;
      mBitmap = std::make_unique<wxBitmap>( mWidth, mHeight, 24 );
   }

   if( !mBitmap || !mBitmap->IsOk() )
      return;

   wxMemoryDC memDC;
   memDC.SelectObject( *mBitmap );

   //draw background on client viewport
   memDC.SetBrush( *wxWHITE_BRUSH );
   memDC.SetPen( *wxBLACK_PEN );
   memDC.DrawRectangle( 0, 0, mWidth, mHeight );

   if( !mMixerSpec )
   {
      dc.Blit( 0, 0, mWidth, mHeight, &memDC, 0, 0, wxCOPY, false );
      memDC.SelectObject( wxNullBitmap );
      return;
   }

   const unsigned int numTracks = mMixerSpec->GetNumTracks();
   const unsigned int numChannels = mMixerSpec->GetNumChannels();

   if( mSelectedTrack >= static_cast<int>( numTracks ) )
      mSelectedTrack = -1;
   if( mSelectedChannel >= static_cast<int>( numChannels ) )
      mSelectedChannel = -1;

   if( mTrackRects.size() < numTracks )
      mTrackRects.resize( numTracks );
   if( mChannelRects.size() < numChannels )
      mChannelRects.resize( numChannels );

   const int maxItems = static_cast<int>( std::max( numTracks, numChannels ) );
   const int canvasHeight = std::max(
      mHeight, maxItems * kMinItemStep + 2 * kVerticalPadding );
   const bool useScrollLayout = canvasHeight > mHeight;
   const int scrollY = CalcUnscrolledPosition( wxPoint( 0, 0 ) ).y;
   const int viewTop = scrollY;
   const int viewBottom = scrollY + mHeight;

   memDC.SetDeviceOrigin( 0, -scrollY );

   //box dimensions
   mBoxWidth = std::max( 1, mWidth / 6 );

   static const double PI = 2.0 * acos( 0.0 );
   double radius = 0.0;
   double totAngle = 0.0;

   if( !useScrollLayout )
   {
      mTrackHeight = numTracks > 0
         ? std::clamp<int>( ( mHeight * 3 ) / ( static_cast<int>( numTracks ) * 4 ), 1, 30 )
         : 0;
      mChannelHeight = numChannels > 0
         ? std::clamp<int>( ( mHeight * 3 ) / ( static_cast<int>( numChannels ) * 4 ), 1, 30 )
         : 0;

      const double baseAngle = atan( ( 3.0 * mHeight ) / mWidth );
      const double denom = std::max( 1e-6, 2.0 * sin( PI - 2.0 * baseAngle ) );
      radius = mHeight / denom;
      const double ratio = std::clamp( mHeight / ( 2.0 * radius ), -1.0, 1.0 );
      totAngle = asin( ratio ) * 2.0;
   }
   else
   {
      const int usableHeight = std::max( 1, canvasHeight - 2 * kVerticalPadding );
      const int trackStep = numTracks > 0
         ? std::clamp<int>( usableHeight / static_cast<int>( numTracks ), kMinItemStep, kMaxItemStep )
         : kMinItemStep;
      const int channelStep = numChannels > 0
         ? std::clamp<int>( usableHeight / static_cast<int>( numChannels ), kMinItemStep, kMaxItemStep )
         : kMinItemStep;

      mTrackHeight = numTracks > 0
         ? std::clamp( ( trackStep * 3 ) / 4, 1, 30 )
         : 0;
      mChannelHeight = numChannels > 0
         ? std::clamp( ( channelStep * 3 ) / 4, 1, 30 )
         : 0;
   }

   auto getTrackName = [this]( unsigned int index ) -> wxString {
      if( index < mTrackNames.size() )
         return mTrackNames[ index ];
      return wxString::Format( _( "Track %d" ), static_cast<int>( index + 1 ) );
   };

   //draw tracks
   if( numTracks > 0 )
   {
      memDC.SetBrush( AColor::envelopeBrush );

      wxString longestTrackName = getTrackName( 0 );
      for( unsigned int i = 1; i < numTracks; i++ )
      {
         wxString candidate = getTrackName( i );
         if( candidate.length() > longestTrackName.length() )
            longestTrackName = std::move( candidate );
      }

      const bool drawTrackText =
         SetFont( memDC, longestTrackName, mBoxWidth - 4, mTrackHeight );

      const double trackAngle = totAngle / ( numTracks + 1 );
      const int usableHeight = std::max( 1, canvasHeight - 2 * kVerticalPadding );
      const int trackStep = std::clamp<int>(
         usableHeight / static_cast<int>( numTracks ), kMinItemStep, kMaxItemStep );

      for( unsigned int i = 0; i < numTracks; i++ )
      {
         if( !useScrollLayout )
         {
            mTrackRects[ i ].x = static_cast<int>(
               mBoxWidth * 2 + radius -
               radius * cos( totAngle * 0.5 - trackAngle * ( i + 1 ) ) -
               mBoxWidth + 0.5 );
            mTrackRects[ i ].y = static_cast<int>(
               mHeight * 0.5 -
               radius * sin( totAngle * 0.5 - trackAngle * ( i + 1.0 ) ) -
               0.5 * mTrackHeight + 0.5 );
         }
         else
         {
            mTrackRects[ i ].x = mBoxWidth;
            mTrackRects[ i ].y = kVerticalPadding +
               static_cast<int>( i ) * trackStep +
               ( trackStep - mTrackHeight ) / 2;
         }

         mTrackRects[ i ].width = mBoxWidth;
         mTrackRects[ i ].height = mTrackHeight;

         if( mTrackRects[ i ].y + mTrackHeight < viewTop ||
             mTrackRects[ i ].y > viewBottom )
            continue;

         memDC.SetPen( mSelectedTrack == static_cast<int>( i ) ? *wxRED_PEN : *wxBLACK_PEN );
         memDC.DrawRectangle( mTrackRects[ i ] );

         if( drawTrackText )
         {
            const wxString name = getTrackName( i );
            int w = 0, h = 0;
            memDC.GetTextExtent( name, &w, &h );
            memDC.DrawText( name,
               mTrackRects[ i ].x + ( mBoxWidth - w ) / 2,
               mTrackRects[ i ].y + ( mTrackHeight - h ) / 2 );
         }
      }
   }

   //draw channels
   if( numChannels > 0 )
   {
      memDC.SetBrush( AColor::playRegionBrush[ 0 ] );

      const wxString sampleChannelLabel = wxString::Format(
         _( "Channel: %2d" ),
         static_cast<int>( std::max( 99u, numChannels ) ) );
      const bool drawChannelText =
         SetFont( memDC, sampleChannelLabel, mBoxWidth - 4, mChannelHeight );

      const double channelAngle = totAngle / ( numChannels + 1 );
      const int usableHeight = std::max( 1, canvasHeight - 2 * kVerticalPadding );
      const int channelStep = std::clamp<int>(
         usableHeight / static_cast<int>( numChannels ), kMinItemStep, kMaxItemStep );

      for( unsigned int i = 0; i < numChannels; i++ )
      {
         if( !useScrollLayout )
         {
            mChannelRects[ i ].x = static_cast<int>(
               mBoxWidth * 4 - radius +
               radius * cos( totAngle * 0.5 - channelAngle * ( i + 1 ) ) + 0.5 );
            mChannelRects[ i ].y = static_cast<int>(
               mHeight * 0.5 -
               radius * sin( totAngle * 0.5 - channelAngle * ( i + 1.0 ) ) -
               0.5 * mChannelHeight + 0.5 );
         }
         else
         {
            mChannelRects[ i ].x = mBoxWidth * 4;
            mChannelRects[ i ].y = kVerticalPadding +
               static_cast<int>( i ) * channelStep +
               ( channelStep - mChannelHeight ) / 2;
         }

         mChannelRects[ i ].width = mBoxWidth;
         mChannelRects[ i ].height = mChannelHeight;

         if( mChannelRects[ i ].y + mChannelHeight < viewTop ||
             mChannelRects[ i ].y > viewBottom )
            continue;

         memDC.SetPen( mSelectedChannel == static_cast<int>( i ) ? *wxRED_PEN : *wxBLACK_PEN );
         memDC.DrawRectangle( mChannelRects[ i ] );

         if( drawChannelText )
         {
            const wxString chanLabel = wxString::Format(
               _( "Channel: %2d" ), static_cast<int>( i + 1 ) );
            int w = 0, h = 0;
            memDC.GetTextExtent( chanLabel, &w, &h );
            memDC.DrawText( chanLabel,
               mChannelRects[ i ].x + ( mBoxWidth - w ) / 2,
               mChannelRects[ i ].y + ( mChannelHeight - h ) / 2 );
         }
      }
   }

   //draw links
   memDC.SetPen( wxPen( *wxBLACK, std::max( 1, mHeight / 200 ) ) );
   for( unsigned int i = 0; i < numTracks; i++ )
   {
      const int x1 = mTrackRects[ i ].x + mBoxWidth;
      const int y1 = mTrackRects[ i ].y + mTrackHeight / 2;
      for( unsigned int j = 0; j < numChannels; j++ )
      {
         if( !mMixerSpec->mMap[ i ][ j ] )
            continue;

         const int x2 = mChannelRects[ j ].x;
         const int y2 = mChannelRects[ j ].y + mChannelHeight / 2;
         if( std::max( y1, y2 ) < viewTop || std::min( y1, y2 ) > viewBottom )
            continue;

         AColor::Line( memDC, x1, y1, x2, y2 );
      }
   }

   memDC.SetDeviceOrigin( 0, 0 );
   dc.Blit( 0, 0, mWidth, mHeight, &memDC, 0, 0, wxCOPY, false );
   memDC.SelectObject( wxNullBitmap );
}

double ExportMixerPanel::Distance( wxPoint &a, wxPoint &b )
{
   const double dx = static_cast<double>( a.x ) - b.x;
   const double dy = static_cast<double>( a.y ) - b.y;
   return std::hypot( dx, dy );
}

//checks if p is on the line connecting la, lb with tolerance
bool ExportMixerPanel::IsOnLine( wxPoint p, wxPoint la, wxPoint lb )
{
   return Distance( p, la ) + Distance( p, lb ) - Distance( la, lb ) < 0.25;
}

void ExportMixerPanel::OnMouseEvent(wxMouseEvent & event)
{
   if( !event.ButtonDown() )
   {
      event.Skip();
      return;
   }

   if( !mMixerSpec )
      return;

   const wxPoint clickPos = CalcUnscrolledPosition( event.GetPosition() );
   const unsigned int numTracks = std::min(
      mMixerSpec->GetNumTracks(),
      static_cast<unsigned int>( mTrackRects.size() ) );
   const unsigned int numChannels = std::min(
      mMixerSpec->GetNumChannels(),
      static_cast<unsigned int>( mChannelRects.size() ) );

   bool reset = true;
   //check tracks
   for( unsigned int i = 0; i < numTracks; i++ )
      if( mTrackRects[ i ].Contains( clickPos.x, clickPos.y ) )
      {
         reset = false;
         if( mSelectedTrack == static_cast<int>( i ) )
            mSelectedTrack = -1;
         else
         {
            mSelectedTrack = static_cast<int>( i );
            if( mSelectedChannel >= 0 &&
                mSelectedChannel < static_cast<int>( numChannels ) )
               mMixerSpec->mMap[ mSelectedTrack ][ mSelectedChannel ] =
                  !mMixerSpec->mMap[ mSelectedTrack ][ mSelectedChannel ];
         }
         goto found;
      }

   //check channels
   for( unsigned int i = 0; i < numChannels; i++ )
      if( mChannelRects[ i ].Contains( clickPos.x, clickPos.y ) )
      {
         reset = false;
         if( mSelectedChannel == static_cast<int>( i ) )
            mSelectedChannel = -1;
         else
         {
            mSelectedChannel = static_cast<int>( i );
            if( mSelectedTrack >= 0 &&
                mSelectedTrack < static_cast<int>( numTracks ) )
               mMixerSpec->mMap[ mSelectedTrack ][ mSelectedChannel ] =
                  !mMixerSpec->mMap[ mSelectedTrack ][ mSelectedChannel ];
         }
         goto found;
      }

   //check links
   for( unsigned int i = 0; i < numTracks; i++ )
      for( unsigned int j = 0; j < numChannels; j++ )
         if( mMixerSpec->mMap[ i ][ j ] &&
             IsOnLine(
                clickPos,
                wxPoint( mTrackRects[ i ].x + mBoxWidth,
                         mTrackRects[ i ].y + mTrackHeight / 2 ),
                wxPoint( mChannelRects[ j ].x,
                         mChannelRects[ j ].y + mChannelHeight / 2 ) ) )
            mMixerSpec->mMap[ i ][ j ] = false;

found:
   if( reset )
      mSelectedTrack = mSelectedChannel = -1;
   Refresh( false );
}

/**********************************************************************

  Audacity: A Digital Audio Editor

  ExportMixerPanel.h

  Dominic Mazzoni

**********************************************************************/

#pragma once

#include <memory>
#include <vector>

#include <wx/arrstr.h>
#include <wx/bitmap.h>
#include <wx/gdicmn.h>
#include <wx/scrolwin.h>

class wxMemoryDC;

namespace MixerOptions { class Downmix; }

///\class ExportMixerPanel
///\brief Panel that displays mixing for advanced mixing option.
class ExportMixerPanel final : public wxScrolledWindow
{
public:
   ExportMixerPanel( wxWindow *parent, wxWindowID id,
         MixerOptions::Downmix *mixerSpec, wxArrayString trackNames,
         const wxPoint& pos = wxDefaultPosition,
         const wxSize& size = wxDefaultSize);
   ~ExportMixerPanel() override;

   void UpdateScrollbars();

   void OnMouseEvent(wxMouseEvent & event);
   void OnPaint(wxPaintEvent & event);
   void OnSize(wxSizeEvent & event);

private:
   std::unique_ptr<wxBitmap> mBitmap;
   wxRect mEnvRect;
   int mWidth;
   int mHeight;
   MixerOptions::Downmix *mMixerSpec;
   std::vector<wxRect> mChannelRects;
   std::vector<wxRect> mTrackRects;
   int mSelectedTrack, mSelectedChannel;
   wxArrayString mTrackNames;
   int mBoxWidth, mChannelHeight, mTrackHeight;

   bool SetFont( wxMemoryDC &memDC, const wxString &text, int width, int height );
   double Distance( wxPoint &a, wxPoint &b );
   bool IsOnLine( wxPoint p, wxPoint la, wxPoint lb );

   DECLARE_EVENT_TABLE()
};

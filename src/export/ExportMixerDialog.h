/**********************************************************************

  Audacity: A Digital Audio Editor

  ExportMixerDialog.h

  Dominic Mazzoni

**********************************************************************/

#pragma once

#include <memory>

#include <wx/arrstr.h>

#include "wxPanelWrapper.h"

#include "Track.h"

class wxSlider;
class wxSpinCtrl;
class wxSpinEvent;
class wxStaticText;

class ExportMixerPanel;
class WaveTrack;

class TranslatableString;

namespace MixerOptions { class Downmix; }

///\class ExportMixerDialog
///\brief Dialog for advanced mixing.
class ExportMixerDialog final : public wxDialogWrapper
{
public:
   // constructors and destructors
   ExportMixerDialog(TrackIterRange<const WaveTrack> tracks, MixerOptions::Downmix* mixerSpec,
         wxWindow *parent, wxWindowID id, const TranslatableString &title,
         const wxPoint& pos = wxDefaultPosition,
         const wxSize& size = wxDefaultSize,
         long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER );
   virtual ~ExportMixerDialog();

private:
   ExportMixerPanel *mMixerPanel{};
   wxStaticText *mChannelsText{};
   wxSlider *mChannelsSlider{};
   wxSpinCtrl *mChannelsSpin{};
   MixerOptions::Downmix *mMixerSpec{};
   wxArrayString mTrackNames;

private:
   void SetNumChannels( int numChannels, bool updateSpin = true );

   void OnOk( wxCommandEvent &event );
   void OnCancel( wxCommandEvent &event );
   void OnMixerPanelHelp( wxCommandEvent &event );
   void OnSlider( wxCommandEvent &event );
   void OnSpin( wxSpinEvent &event );
   void OnSpinText( wxCommandEvent &event );
   void OnSize( wxSizeEvent &event );

private:
   DECLARE_EVENT_TABLE()
};

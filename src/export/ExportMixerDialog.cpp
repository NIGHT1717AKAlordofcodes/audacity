/**********************************************************************

  Audacity: A Digital Audio Editor

  ExportMixerDialog.cpp

  Dominic Mazzoni

**********************************************************************/

#include "ExportMixerDialog.h"
#include "ExportMixerPanel.h"
#include "ExportUtils.h"

#include <algorithm>

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "WaveTrack.h"
#include "MixerOptions.h"

#include "ShuttleGui.h"
#include "HelpSystem.h"

enum
{
   ID_MIXERPANEL = 10001,
   ID_SLIDER_CHANNEL,
   ID_SPIN_CHANNEL
};

BEGIN_EVENT_TABLE( ExportMixerDialog, wxDialogWrapper )
   EVT_BUTTON( wxID_OK, ExportMixerDialog::OnOk )
   EVT_BUTTON( wxID_CANCEL, ExportMixerDialog::OnCancel )
   EVT_BUTTON( wxID_HELP, ExportMixerDialog::OnMixerPanelHelp )
   EVT_SIZE( ExportMixerDialog::OnSize )
   EVT_SLIDER( ID_SLIDER_CHANNEL, ExportMixerDialog::OnSlider )
   EVT_SPINCTRL( ID_SPIN_CHANNEL, ExportMixerDialog::OnSpin )
   EVT_TEXT( ID_SPIN_CHANNEL, ExportMixerDialog::OnSpinText )
END_EVENT_TABLE()

ExportMixerDialog::~ExportMixerDialog() = default;

ExportMixerDialog::ExportMixerDialog(TrackIterRange<const WaveTrack> tracks, MixerOptions::Downmix* mixerSpec,
      wxWindow *parent, wxWindowID id, const TranslatableString &title,
      const wxPoint &position, const wxSize& size, long style )
   : wxDialogWrapper( parent, id, title, position, size, style | wxRESIZE_BORDER )
   , mMixerSpec(mixerSpec)
{
   SetName();

   for (auto t : tracks)
   {
      const wxString sTrackName = (t->GetName()).Left(20);
      const auto nChannels = t->NChannels();
      if (nChannels == 1)
         // No matter whether it's panned hard left or right
         mTrackNames.push_back(sTrackName);
      else if (nChannels == 2) {
         /* i18n-hint: track name and L abbreviating Left channel */
         mTrackNames.push_back(XO("%s - L").Format(sTrackName).Translation());
         /* i18n-hint: track name and R abbreviating Right channel */
         mTrackNames.push_back(XO("%s - R").Format(sTrackName).Translation());
      }
      else {
         for (size_t ch = 0; ch < nChannels; ++ch)
            mTrackNames.push_back(
               wxString::Format(wxT("%s - %d"), sTrackName, static_cast<int>(ch + 1)));
      }
   }

   const int maxChannels = mMixerSpec
      ? static_cast<int>( std::max( 1u, mMixerSpec->GetMaxNumChannels() ) )
      : 1;
   const int curChannels = mMixerSpec
      ? std::clamp( static_cast<int>( mMixerSpec->GetNumChannels() ), 1, maxChannels )
      : 1;

   auto label = XO("Output Channels: %2d").Format( curChannels );

   ShuttleGui S{ this, eIsCreating };
   {
      S.SetBorder( 5 );

      mMixerPanel = safenew ExportMixerPanel(
         S.GetParent(), ID_MIXERPANEL, mMixerSpec,
         mTrackNames, wxDefaultPosition, wxSize(400, -1));
      S.Prop(1)
         .Name(XO("Mixer Panel"))
         .Position(wxEXPAND | wxALL)
         .AddWindow(mMixerPanel);

      S.StartHorizontalLay(wxALIGN_CENTER | wxALL, 0);
      {
         mChannelsText = S.AddVariableText(
            label,
            false, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL );

         if (mChannelsText)
         {
            // Reserve enough width for the maximum channel count (e.g. 255)
            const wxString maxLabel =
               XO("Output Channels: %2d").Format( maxChannels ).Translation();
            int maxW = 0, maxH = 0;
            mChannelsText->GetTextExtent( maxLabel, &maxW, &maxH );
            mChannelsText->SetMinSize( wxSize( maxW + 8, -1 ) );
         }

         if (maxChannels > 1)
         {
            mChannelsSlider = S
               .Id(ID_SLIDER_CHANNEL)
               .Name(label)
               .Size({300, -1})
               .Style(wxSL_HORIZONTAL)
               .Position(wxEXPAND | wxALL)
               .AddSlider( {}, curChannels, maxChannels, 1 );

            mChannelsSpin = S
               .Id(ID_SPIN_CHANNEL)
               .Name(label)
               .Position(wxALIGN_CENTER_VERTICAL | wxALL)
               .AddSpinCtrl( {}, curChannels, maxChannels, 1 );
         }
      }
      S.EndHorizontalLay();

      S.AddStandardButtons( eCancelButton | eOkButton | eHelpButton );
   }

   SetAutoLayout(true);
   GetSizer()->Fit( this );
   GetSizer()->SetSizeHints( this );

   SetSizeHints( 640, 480, 20000, 20000 );

   SetSize( 640, 480 );
   if (mMixerPanel)
      mMixerPanel->UpdateScrollbars();
   Center();
}


void ExportMixerDialog::OnSize(wxSizeEvent &event)
{
   if (mMixerPanel)
   {
      mMixerPanel->UpdateScrollbars();
      mMixerPanel->Refresh( false );
   }
   event.Skip();
}

void ExportMixerDialog::SetNumChannels( int numChannels, bool updateSpin )
{
   if (!mMixerSpec)
      return;

   const int maxChannels =
      static_cast<int>( std::max( 1u, mMixerSpec->GetMaxNumChannels() ) );
   const int clamped = std::clamp( numChannels, 1, maxChannels );

   mMixerSpec->SetNumChannels( static_cast<unsigned>( clamped ) );

   const int actual = static_cast<int>( mMixerSpec->GetNumChannels() );
   if (mChannelsSlider && mChannelsSlider->GetValue() != actual)
      mChannelsSlider->SetValue( actual );
   if (updateSpin && mChannelsSpin && mChannelsSpin->GetValue() != actual)
      mChannelsSpin->SetValue( actual );

   const wxString label =
      XO("Output Channels: %2d").Format( actual ).Translation();
   if (mChannelsText)
      mChannelsText->SetLabel( label );
   if (mChannelsSlider)
      mChannelsSlider->SetName( label );
   if (mChannelsSpin)
      mChannelsSpin->SetName( label );

   Layout();

   if (mMixerPanel)
   {
      mMixerPanel->UpdateScrollbars();
      mMixerPanel->Refresh( false );
   }
}

void ExportMixerDialog::OnSlider( wxCommandEvent & WXUNUSED(event))
{
   if (mChannelsSlider)
      SetNumChannels( mChannelsSlider->GetValue(), true );
}

void ExportMixerDialog::OnSpin( wxSpinEvent & WXUNUSED(event))
{
   if (mChannelsSpin)
      SetNumChannels( mChannelsSpin->GetValue(), true );
}

void ExportMixerDialog::OnSpinText( wxCommandEvent &event )
{
   if (!mMixerSpec)
      return;

   long value = 0;
   if (!event.GetString().ToLong( &value ))
      return;

   const long maxChannels =
      static_cast<long>( std::max( 1u, mMixerSpec->GetMaxNumChannels() ) );
   if (value >= 1 && value <= maxChannels)
      SetNumChannels( static_cast<int>( value ), false );
}

void ExportMixerDialog::OnOk(wxCommandEvent & WXUNUSED(event))
{
   if (mChannelsSpin)
      SetNumChannels( mChannelsSpin->GetValue() );
   EndModal( wxID_OK );
}

void ExportMixerDialog::OnCancel(wxCommandEvent & WXUNUSED(event))
{
   EndModal( wxID_CANCEL );
}

void ExportMixerDialog::OnMixerPanelHelp(wxCommandEvent & WXUNUSED(event))
{
   HelpSystem::ShowHelp(this, L"Advanced_Mixing_Options", true);
}

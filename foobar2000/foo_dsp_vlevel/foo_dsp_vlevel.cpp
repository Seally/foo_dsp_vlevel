// Copyright 2003-2007 Tom Felker, wore, wiesl
//
// VLevel foobar2000 plugin
//
// VLevel is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// VLevel is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with VLevel; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

// foo_dsp_vlevel.cpp - the foobar2000 plugin, uses VolumeLeveler
// based on vlevel-bin.cpp
// wore <info@wore.ma.cx>

#include "stdafx.h"

#include <cmath>
#include <cfloat>
#include "volumeleveler.h"
#include "resource.h"

//#define FOO_DSP_VLEVEL_DEBUG

#define snprintf _snprintf

// http://en.wikipedia.org/wiki/Variadic_macro
// http://en.wikipedia.org/wiki/C_preprocessor
// http://www.geocities.com/davinpearson/research/1999/sm4dcp.html

// 		PRINT_DEBUG("on_playlist_created begin (\"%s\", %li)", p_name, p_index)
#ifdef FOO_DSP_VLEVEL_DEBUG
#define PRINT_DEBUG(format, ...)                                       \
	do                                                           \
	{                                                            \
		MyDebug::fprintf_flush((format), __VA_ARGS__);             \
	} while (0);
#else
#define PRINT_DEBUG(format, ...)
#endif

#ifdef FOO_DSP_VLEVEL_DEBUG
  #define PRINT_DEBUG_INFO(prefix, var) PRINT_DEBUG("%s: Using Strength=%lf, MaxMultiplier=%li, BufferLength=%li", (prefix), (double)((var).strength) / 100, (long)((var).max_multiplier), (long)((var).buffer_length));
#else
  #define PRINT_DEBUG_INFO(prefix, var)
#endif

#ifdef FOO_DSP_VLEVEL_DEBUG
class MyDebug
{
protected:
	static FILE* log;

public:
	MyDebug()
	{
		log = fopen("C:\\foobar2000.log", "w");
		PRINT_DEBUG("opened log"); 
	}
	virtual ~MyDebug()
	{
		PRINT_DEBUG("closed log");
		(void)fclose(log);
	}

	static void fprintf_flush(const char* format, ...)
	{
		// http://www.ddj.com/cpp/184403430
		// http://gcc.gnu.org/ml/gcc-bugs/2001-09/msg00147.html
		// http://www.eskimo.com/~scs/cclass/int/sx11c.html
		// http://lists.mysql.com/internals/2180?f=plain

		va_list va_args;
		va_start(va_args, format);
		vfprintf(log, format, va_args);
		va_end(va_args);

		fprintf(log, "\n");

		flush();
	}

	static void flush()
	{
		fflush(log);
	}
};

FILE* MyDebug::log = NULL;
static MyDebug static_MyDebug;
#endif

struct vlevel_preset_info
{
	t_int8 strength;
	t_int8 max_multiplier;
	t_int8 buffer_length;

	bool get_data(const dsp_preset & p_data)
    {

		PRINT_DEBUG("vlevel_preset_info::get_data() entry");

		PRINT_DEBUG("vlevel_preset_info::datasize=%li, should be=%li", (long)p_data.get_data_size(), sizeof(vlevel_preset_info));

		if (p_data.get_data_size() != sizeof(vlevel_preset_info)) return false;
		vlevel_preset_info temp = *(vlevel_preset_info *)p_data.get_data();
		strength = temp.strength;
		max_multiplier = temp.max_multiplier;
		buffer_length = temp.buffer_length;

		PRINT_DEBUG_INFO("vlevel_preset_info::get_data()", *this);
		
		PRINT_DEBUG("vlevel_preset_info::get_data() return");
		return true;
	}

	bool set_data(dsp_preset & p_data)
	{
		PRINT_DEBUG("vlevel_preset_info::set_data() entry");

		PRINT_DEBUG_INFO("vlevel_preset_info::set_data()", *this);

		vlevel_preset_info temp = {strength, max_multiplier, buffer_length};
		//((dsp_preset &)p_data).set_data(&temp, sizeof(temp));
		p_data.set_data(&temp, sizeof(temp));
		
		PRINT_DEBUG("vlevel_preset_info::set_data() return");

		return true;
	}
};

class CVLevelDSPPopup : public CDialogImpl<CVLevelDSPPopup> {
    vlevel_preset_info& m_info;

    CTrackBarCtrl m_strengthSlider;
    CTrackBarCtrl m_maxMultiplierSlider;
    CTrackBarCtrl m_bufferLengthSlider;

public:
    CVLevelDSPPopup(vlevel_preset_info& p_info) : m_info(p_info) {
        PRINT_DEBUG_INFO("CVLevelDSPPopup::CVLevelDSPPopup", m_info);
        m_bMsgHandled;
    }

    enum { IDD = IDD_CONFIG };

    BEGIN_MSG_MAP_EX(CVLevelDSPPopup)
        MSG_WM_INITDIALOG(_OnInitDialog)
        COMMAND_HANDLER_EX(IDOK, BN_CLICKED, _OnButtonOk)
        COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, _OnButtonCancel)
        MSG_WM_HSCROLL(_OnHScroll)
    END_MSG_MAP()

private:
    void _RefreshStrengthLabel() {
        const auto strength = m_strengthSlider.GetPos();
        pfc::string_formatter msg;

        // convert strength slider position to display the actual strength value
        // VLevel will receive later on
        msg << pfc::format_float(strength / 100.0, 1, 2);

        ::uSetDlgItemText(*this, IDC_STATIC_STRENGTH, msg);
    }

    void _RefreshMaxMultiplierLabel() {
        const auto maxMultiplier = m_maxMultiplierSlider.GetPos();
        const auto decibels = 20 * std::log10(maxMultiplier);

        pfc::string_formatter msg;
        msg << maxMultiplier << " (+" << pfc::format_float(decibels, 3, 1) << " dB)";

        ::uSetDlgItemText(*this, IDC_STATIC_MAX_MULTIPLIER, msg);
    }

    void _RefreshBufferLengthLabel() {
        const auto bufferLength = m_bufferLengthSlider.GetPos();

        pfc::string_formatter msg;
        msg << pfc::format_float(bufferLength / 10.0, 1, 1) << " s";

        ::uSetDlgItemText(*this, IDC_STATIC_BUFFER_LENGTH, msg);
    }

    void _RefreshLabels() {
        _RefreshStrengthLabel();
        _RefreshMaxMultiplierLabel();
        _RefreshBufferLengthLabel();
    }

    BOOL _OnInitDialog(CWindow, LPARAM) {
        m_strengthSlider = GetDlgItem(IDC_STRENGTH);
        m_maxMultiplierSlider = GetDlgItem(IDC_MAX_MULTIPLIER);
        m_bufferLengthSlider = GetDlgItem(IDC_BUFFER_LENGTH);

        // allow strength slider settings of 0-100 (strength*100) so users can
        // set this parameter with 0.01 granularity
        m_strengthSlider.SetRange(0, 100, FALSE);
        m_strengthSlider.SetPos(m_info.strength);

        // GW: Changed from 1-40 instead of 0-40 (so MAX_HUGE can not be set!)
        m_maxMultiplierSlider.SetRange(1, 100, FALSE);
        m_maxMultiplierSlider.SetPos(m_info.max_multiplier);

        m_bufferLengthSlider.SetRange(1, 50, FALSE);
        m_bufferLengthSlider.SetPos(m_info.buffer_length);

        _RefreshLabels();

        return TRUE;
    }

    void _OnButtonOk(UINT, int id, CWindow) {
        PRINT_DEBUG_INFO("CVLevelDSPPopup::OnButtonOk()", m_info);

        m_info.strength = m_strengthSlider.GetPos();
        m_info.max_multiplier = m_maxMultiplierSlider.GetPos();
        m_info.buffer_length = m_bufferLengthSlider.GetPos();

        PRINT_DEBUG_INFO("CVLevelDSPPopup::OnButtonOk()", m_info);

        EndDialog(id);
    }

    void _OnButtonCancel(UINT, int id, CWindow) {
        EndDialog(id);
    }

    void _OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar) {
        _RefreshLabels();
    }
};


class dsp_vlevel : public dsp_impl_base
{
	VolumeLeveler *vl;

	// always == vl->GetChannels()
	size_t channels;
	
	// only applies to our local buffers which are cached across on_chunk() calls
	size_t samples;

	// our local buffers
	value_t *raw_value_buf;
	value_t **bufs;

	// only used to SetLengthAndChannels and to create chunks
	UINT srate;

	t_int8 buffer_length;

	// make sure GUI resets the flag using the update_configuration method
	bool buffer_length_changed;

	bool ready()
	{
		// could be optimized to vl&&bufs, or expanded for complete sanity checks
		return vl && bufs && raw_value_buf && channels == vl->GetChannels();
	}

	// insert the contents of our buffers into the stream
	// can only be called from on_endoftrack(), on_endofplayback(), and on_chunk()
	void flush_data()
	{
		// foobar2000 sometimes calls this when it shouldn't, so this is necessary.
		if(!ready()) return;
		
		// do nothing if there's no audio in the vl
		// otherwise on_chunk() will make this chunk empty, and foobar2000 complains
		// next vl version will have GetGoodSamples(){return samples - silence};
		if(vl->GetSamples() - vl->GetSilence() == 0) return;

		// make a new chunk filled with enough silence to flush the buffers.
		audio_chunk *chunk = insert_chunk(vl->GetSamples() * channels);
		t_size new_size = vl->GetSamples() * channels;
		if (new_size > chunk->get_data_size())
			chunk->set_data_size(new_size);
		chunk->set_channels(channels);
		chunk->set_srate(srate);
		chunk->set_sample_count(0);
		chunk->pad_with_silence(vl->GetSamples());

		// yes, it's that simple.
		on_chunk(chunk);
		vl->Flush();
	}

	// only affects the local buffers, not the volumeleveler
	void allocate_buffers(size_t new_samples, size_t new_channels)
	{
		cleanup_buffers();
		samples = new_samples;
		channels = new_channels;
		
		raw_value_buf = new value_t[samples * channels];
		bufs = new value_t*[channels];
		for(size_t ch = 0; ch < channels; ++ch)
			bufs[ch] = new value_t[samples];
	}

	// only affects the local buffers, not the volumeleveler
	void cleanup_buffers(void)
	{
		if (raw_value_buf!=0) {
			delete [] raw_value_buf;
			raw_value_buf=0;
		}
		if (bufs!=0) {
			for(size_t ch = 0; ch < channels; ++ch)
				delete [] bufs[ch];
			delete [] bufs;
			bufs=0;
		}
		samples = 0;
	}

	virtual bool on_chunk(audio_chunk * chunk)
	{
		// we must change the vl's settings if buffer length or channels changes
		if(chunk->get_srate() != srate || chunk->get_channels() != channels || buffer_length_changed) {
			flush_data();
			srate = chunk->get_srate();
			channels = chunk->get_channels();
			vl->SetSamplesAndChannels((size_t)(((double)buffer_length) / 10 * srate), channels);
			buffer_length_changed = false;
		}

		// if this chunk is a different size than last time, we must get new temp buffers
		if(chunk->get_sample_count() != samples || chunk->get_channels() != channels)
			allocate_buffers(chunk->get_sample_count(), chunk->get_channels());

		assert(ready());

		audio_sample *data = chunk->get_data();

		// de-interleave the data
		size_t s;
		for(s = 0; s < samples; ++s)
			for(size_t ch = 0; ch < channels; ++ch)
				bufs[ch][s] = data[s * channels + ch];

		// do the exchange
		size_t silence_samples = vl->Exchange(bufs, bufs, samples);

		// interleave the data
		for(s = silence_samples; s < samples; ++s)
			for(size_t ch = 0; ch < channels; ++ch)
				data[(s - silence_samples) * channels + ch] = bufs[ch][s];

		// set the amount of good samples in the chunk
		chunk->set_sample_count(samples - silence_samples);

		return !chunk->is_empty();
	}

public:
	dsp_vlevel(const dsp_preset& p_data)
	{
		PRINT_DEBUG("dsp_vlevel::dsp_vlevel() entry");

		raw_value_buf = 0;
		bufs = 0;
		samples = channels = srate = 0;

		buffer_length = 20;
		buffer_length_changed = true;
		value_t strength = 0.8;
		value_t maxmultiplier = 25;

		vlevel_preset_info info;
		dsp_preset_impl temp = p_data;
		if (info.get_data(temp))
		{
			PRINT_DEBUG("dsp_vlevel::dsp_vlevel() using NON default values");
			PRINT_DEBUG_INFO("dialog_vlevel_config::on_message(IDCANCEL)", info);
			strength = ((double)info.strength) / 100; // must be divided by 100!!
			maxmultiplier = info.max_multiplier;
			buffer_length = info.buffer_length;
		}
		else
		{
			PRINT_DEBUG("dsp_vlevel::dsp_vlevel() using default values");
		}

		vl = new VolumeLeveler();
		vl->SetStrength(strength);
		vl->SetMaxMultiplier(maxmultiplier);

		PRINT_DEBUG("dsp_vlevel::dsp_vlevel() return");
	}

	~dsp_vlevel()
	{
		PRINT_DEBUG("dsp_vlevel::~dsp_vlevel() entry");

		cleanup_buffers();
		delete vl;
		
		PRINT_DEBUG("dsp_vlevel::~dsp_vlevel() return");
	}

	static GUID g_get_guid()
	{
		PRINT_DEBUG("dsp_vlevel::g_get_guid() entry");

		// {EC001F79-9D79-4dc8-B7CB-40818D7A1009}
		static const GUID guid =
		{ 0xec001f79, 0x9d79, 0x4dc8, { 0xb7, 0xcb, 0x40, 0x81, 0x8d, 0x7a, 0x10, 0x9 } };
		return guid;
	}

	static void g_get_name(pfc::string_base & p_out)
	{
		PRINT_DEBUG("dsp_vlevel::g_get_name() entry");

		p_out = "VLevel";
	}

	static bool g_get_default_preset(dsp_preset & p_out)
	{
		PRINT_DEBUG("dsp_vlevel::g_get_default_preset() entry");

		vlevel_preset_info info = {80, 25, 20};
		p_out.set_owner(g_get_guid());
		info.set_data(p_out);

		PRINT_DEBUG("dsp_vlevel::g_get_default_preset() return");
		return true;
	}

	static bool g_have_config_popup()
	{
		PRINT_DEBUG("dsp_vlevel::g_have_config_popup() entry");
		return true;
	}

	static bool g_show_config_popup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
	{
		PRINT_DEBUG("dsp_vlevel::g_show_config_popup() entry");

		vlevel_preset_info info;
		dsp_preset_impl temp = p_data;
		if (!info.get_data(temp)) return false;

        CVLevelDSPPopup popup(info);

        const auto commandResult = popup.DoModal(p_parent);

        PRINT_DEBUG("popup.DoModal()=%i", commandResult);

        if (commandResult == IDOK) {
            bool retval = info.set_data(temp);
            p_callback.on_preset_changed(temp);

            PRINT_DEBUG("dsp_vlevel::g_show_config_popup() return");

            return retval;
        }

        PRINT_DEBUG("dsp_vlevel::g_show_config_popup() return");
        return false;
	}

	bool set_data(const dsp_preset & p_data)
	{
		PRINT_DEBUG("dsp_vlevel::set_data() entry");

		vlevel_preset_info info;
		if (!info.get_data(p_data)) return false;
		
		PRINT_DEBUG_INFO("dsp_vlevel::set_data()", info);

		vl->SetStrength((double)info.strength / 100);
		vl->SetMaxMultiplier(info.max_multiplier);
		buffer_length = info.buffer_length;
		buffer_length_changed = true;

		PRINT_DEBUG("dsp_vlevel::set_data() return");
		return true;
	}

	bool on_chunk(audio_chunk * chunk, foobar2000_io::abort_callback & dummy)
	{
		PRINT_DEBUG("dsp_vlevel::on_chunk() entry");

		bool ret = on_chunk(chunk);

		PRINT_DEBUG("dsp_vlevel::on_chunk() return");

		return ret;
	}

	// this is redundant iff on_endoftrack is always called first.
	virtual void on_endofplayback(foobar2000_io::abort_callback & dummy)
	{
		PRINT_DEBUG("dsp_vlevel::on_endofplayback() entry");

		console::info("on_endofplayback");
		flush_data();

		PRINT_DEBUG("dsp_vlevel::on_endofplayback() return");
	}

	// we output the contents of our buffers
	virtual void on_endoftrack(foobar2000_io::abort_callback & dummy)
	{
		PRINT_DEBUG("dsp_vlevel::on_endoftrack() entry");

		console::info("on_endoftrack");
		flush_data();

		PRINT_DEBUG("dsp_vlevel::on_endofplayback() return");
	}

	// called on seeks, so we drop our data
	virtual void flush()
	{
		PRINT_DEBUG("dsp_vlevel::flush() entry");

		console::info("flush");
		vl->Flush();

		PRINT_DEBUG("dsp_vlevel::flush() return");
	}

	// this is called very often while playing
	virtual double get_latency()
	{
		PRINT_DEBUG("dsp_vlevel::get_latency()");

		return (srate == 0) ? 0 : ((double)(vl->GetSamples() - vl->GetSilence()) / srate);
	}

	virtual bool need_track_change_mark()
	{
		PRINT_DEBUG("dsp_vlevel::need_track_change_mark()");

		// FIXME: have someone take a look at this, g-lite
		return false;
	}
};

static dsp_factory_t<dsp_vlevel> foo;


// and last but not least the brand spankin' new about box - ss97
#ifdef _DEBUG
#define COMPONENT_VERSION_NAME "VLevel - PRINT_DEBUG BUILD!"
#else
#define COMPONENT_VERSION_NAME "VLevel"
#endif
DECLARE_COMPONENT_VERSION(COMPONENT_VERSION_NAME,"20230209",
						  "volume levelling plugin\n\n"
                          "updated for 64-bit by Seally\n\n"
						  "made ready for 0.9.x series, config settings now work well, added useful limits for max multiplier, added dB scale, added debug code by wiesl\n\n"
						  "written by wore\n"
						  "additional code by Tom Felker and ssamadhi97\n"
						  "ported to foobar2000 0.9 by G-Lite\n\n"
						  "based on VLevel 0.5 by Tom Felker (http://vlevel.sourceforge.net/news/)" 
						  );
//wore

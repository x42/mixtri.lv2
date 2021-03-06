// generated by lv2ttl2c from
// http://gareus.org/oss/lv2/mixtri#lv2

extern const LV2_Descriptor* lv2_descriptor(uint32_t index);
extern const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index);

static const RtkLv2Description _plugin_mixtri = {
	&lv2_descriptor,
	&lv2ui_descriptor
	, 0 // uint32_t dsp_descriptor_id
	, 0 // uint32_t gui_descriptor_id
	, "Mixer'n'Trigger" // const char *plugin_human_id
	, (const struct LV2Port[42])
	{
		{ "in1", AUDIO_IN, nan, nan, nan, "Audio Input 1"},
		{ "in2", AUDIO_IN, nan, nan, nan, "Audio Input 2"},
		{ "in3", AUDIO_IN, nan, nan, nan, "Audio Input 3"},
		{ "in4", AUDIO_IN, nan, nan, nan, "Audio Input 4"},
		{ "out1", AUDIO_OUT, nan, nan, nan, "Audio Output 1"},
		{ "out2", AUDIO_OUT, nan, nan, nan, "Audio Output 2"},
		{ "out3", AUDIO_OUT, nan, nan, nan, "Audio Output 3"},
		{ "trigger_out", AUDIO_OUT, nan, nan, nan, "Audio Output Trigger"},
		{ "gain_in_0", CONTROL_IN, 0.000000, -60.000000, 30.000000, "Input Gain 0"},
		{ "gain_in_1", CONTROL_IN, 0.000000, -60.000000, 30.000000, "Input Gain 1"},
		{ "gain_in_2", CONTROL_IN, 0.000000, -60.000000, 30.000000, "Input Gain 2"},
		{ "gain_in_3", CONTROL_IN, 0.000000, -60.000000, 30.000000, "Input Gain 3"},
		{ "mix_0_0", CONTROL_IN, 1.000000, -6.000000, 6.000000, "Mix 0 0"},
		{ "mix_0_1", CONTROL_IN, 0.000000, -6.000000, 6.000000, "Mix 0 1"},
		{ "mix_0_2", CONTROL_IN, 0.000000, -6.000000, 6.000000, "Mix 0 2"},
		{ "mix_1_0", CONTROL_IN, 0.000000, -6.000000, 6.000000, "Mix 1 0"},
		{ "mix_1_1", CONTROL_IN, 1.000000, -6.000000, 6.000000, "Mix 1 1"},
		{ "mix_1_2", CONTROL_IN, 0.000000, -6.000000, 6.000000, "Mix 1 2"},
		{ "mix_2_0", CONTROL_IN, 0.000000, -6.000000, 6.000000, "Mix 2 0"},
		{ "mix_2_1", CONTROL_IN, 0.000000, -6.000000, 6.000000, "Mix 2 1"},
		{ "mix_2_2", CONTROL_IN, 1.000000, -6.000000, 6.000000, "Mix 2 2"},
		{ "mix_3_0", CONTROL_IN, 0.000000, -6.000000, 6.000000, "Mix 3 0"},
		{ "mix_3_1", CONTROL_IN, 0.000000, -6.000000, 6.000000, "Mix 3 1"},
		{ "mix_3_2", CONTROL_IN, 0.000000, -6.000000, 6.000000, "Mix 3 2"},
		{ "delay_in_0", CONTROL_IN, 0.000000, 0.000000, 48000.000000, "Delay input 0"},
		{ "delay_in_1", CONTROL_IN, 0.000000, 0.000000, 48000.000000, "Delay input 1"},
		{ "delay_in_2", CONTROL_IN, 0.000000, 0.000000, 48000.000000, "Delay input 2"},
		{ "delay_in_3", CONTROL_IN, 0.000000, 0.000000, 48000.000000, "Delay input 3"},
		{ "delay_out_0", CONTROL_IN, 0.000000, 0.000000, 48000.000000, "Delay output 0"},
		{ "delay_out_1", CONTROL_IN, 0.000000, 0.000000, 48000.000000, "Delay output 1"},
		{ "delay_out_2", CONTROL_IN, 0.000000, 0.000000, 48000.000000, "Delay output 2"},
		{ "mod_in_0", CONTROL_IN, 0.000000, 0.000000, 3.000000, "Input 0 Mode"},
		{ "mod_in_1", CONTROL_IN, 0.000000, 0.000000, 3.000000, "Input 1 Mode"},
		{ "mod_in_2", CONTROL_IN, 0.000000, 0.000000, 3.000000, "Input 2 Mode"},
		{ "mod_in_3", CONTROL_IN, 0.000000, 0.000000, 3.000000, "Input 3 Mode"},
		{ "trigger_channel", CONTROL_IN, 3.000000, 0.000000, 3.000000, "Trigger Channel"},
		{ "trigger_mode", CONTROL_IN, 0.000000, 0.000000, 12.000000, "Trigger Mode"},
		{ "trigger_edge", CONTROL_IN, 1.000000, 1.000000, 3.000000, "Trigger Edge Mode"},
		{ "trigger_level1", CONTROL_IN, 0.000000, -1.000000, 1.000000, "Trigger Level1"},
		{ "trigger_level2", CONTROL_IN, 0.000000, -1.000000, 1.000000, "Trigger Level2"},
		{ "trigger_time1", CONTROL_IN, 0.000000, 0.000000, 192000.000000, "Trigger time1"},
		{ "trigger_time2", CONTROL_IN, 0.000000, 0.000000, 192000.000000, "Trigger time2"},
	}
	, 42 // uint32_t nports_total
	, 4 // uint32_t nports_audio_in
	, 4 // uint32_t nports_audio_out
	, 0 // uint32_t nports_midi_in
	, 0 // uint32_t nports_midi_out
	, 0 // uint32_t nports_atom_in
	, 0 // uint32_t nports_atom_out
	, 34 // uint32_t nports_ctrl
	, 34 // uint32_t nports_ctrl_in
	, 0 // uint32_t nports_ctrl_out
	, 8192 // uint32_t min_atom_bufsiz
	, false // bool send_time_info
	, UINT32_MAX // uint32_t latency_ctrl_port
};

#ifdef X42_PLUGIN_STRUCT
#undef X42_PLUGIN_STRUCT
#endif
#define X42_PLUGIN_STRUCT _plugin_mixtri

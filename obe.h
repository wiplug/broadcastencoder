/*****************************************************************************
 * obe.h : Open Broadcast Encoder API
 *****************************************************************************
 * Copyright (C) 2010 Open Broadcast Systems Ltd.
 *
 * Authors: Kieran Kunhya <kieran@kunhya.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 *****************************************************************************/

#ifndef OBE_H
#define OBE_H

#include <inttypes.h>
#include <libavutil/audioconvert.h>
#include <x264.h>

#define OBE_VERSION_MAJOR 0
#define OBE_VERSION_MINOR 1

/* Opaque Handle */
typedef struct obe_t obe_t;

/**** Initialisation Function ****/
obe_t *obe_setup( void );

enum input_video_connection
{
    INPUT_VIDEO_CONNECTION_SDI,
    INPUT_VIDEO_CONNECTION_HDMI,
    INPUT_VIDEO_CONNECTION_OPTICAL_SDI,
    INPUT_VIDEO_CONNECTION_COMPONENT,
    INPUT_VIDEO_CONNECTION_COMPOSITE,
    INPUT_VIDEO_CONNECTION_S_VIDEO,
};

enum input_audio_connection
{
    INPUT_AUDIO_EMBEDDED,
    INPUT_AUDIO_AES_EBU,
    INPUT_AUDIO_ANALOGUE,
};

enum input_type_e
{
    INPUT_URL,
//    INPUT_DEVICE_SDI_DECKLINK,
//    INPUT_DEVICE_SDI_V4L2,
//    INPUT_DEVICE_ASI,
};

typedef struct
{
    int input_type;
    char *location;

    int video_connection;
    int audio_connection;
} obe_input_t;

/**** Stream Formats ****/
enum stream_type_e
{
    STREAM_TYPE_VIDEO,
    STREAM_TYPE_AUDIO,
    STREAM_TYPE_SUBTITLE,
    STREAM_TYPE_MISC,
};

enum stream_formats_e
{
    /* Separate Streams */
    VIDEO_UNCOMPRESSED,
    VIDEO_AVC,
    VIDEO_MPEG2,

    AUDIO_PCM,
    AUDIO_MP2,    /* MPEG-1 Layer II */
    AUDIO_AC_3,   /* ATSC A/52B / AC-3 */
    AUDIO_E_AC_3, /* ATSC A/52B Annex E / Enhanced AC-3 */
//    AUDIO_E_DIST, /* E Distribution Audio */
    AUDIO_AAC,

    SUBTITLES_DVB,
    MISC_TELETEXT,

    /* Per-frame Streams/Data */
    SUBTITLES_CEA_608,
    SUBTITLES_CEA_708,

    /* Vertical Ancillary */
};

typedef struct
{
    int stream_id;
    int stream_type;
    int stream_format;

    char lang_code[4];

    char *transport_desc_text;
    char *codec_desc_text;

    /** Video **/
    int csp;
    int width;
    int height;
    int sar_num;
    int sar_den;
    int interlaced;
    int timebase_num;
    int timebase_den;

    // TODO multiple extra streams

    /** Audio **/
    int64_t channel_layout;
    int sample_rate;

    /* Raw Audio */
    int sample_format;

    /* Compressed Audio */
    int bitrate;
    int aac_is_latm; /* LATM is sometimes known as MPEG-4 Encapsulation */

    /** Subtitles **/
    /* Has display definition segment (i.e HD subtitling) */
    int dvb_has_dds;

    /* Vertical Ancillary */
}obe_input_stream_t;

typedef struct
{
    char *name;
    char *provider_name;

    int num_streams;
    obe_input_stream_t *streams;
} obe_input_program_t;

/* Only one program is returned */
int obe_probe_device( obe_t *h, obe_input_t *input_device, obe_input_program_t *program );

enum stream_action_e
{
    STREAM_PASSTHROUGH,
    STREAM_ENCODE,
//    STREAM_LATM_TO_ADTS,
//    STREAM_ADTS_TO_LATM,
};

typedef struct
{
    int passthrough_opts;

    int pid;

    int write_lang_code;
    char lang_code[4];
    int audio_type;

    int has_stream_identifier;
    int stream_identifier;

} obe_ts_stream_opts_t;

/**** AVC Encoding ****/
/* Use this function to let OBE guess the encoding profile.
 * You can use the functions in the x264 API for tweaking or edit the parameter struct directly.
 * Be aware that some parameters will affect speed of encoding and hardware support.
 *
 * For device compatibility reasons OBE will choose Main Profile for SD streams. Many devices
 * support High Profile and so High Profile is recommended where possible
 */
int obe_populate_avc_encoder_params( obe_t *h, int input_stream_id, x264_param_t *param );

/**** 3DTV ****/
/* Arrangements - Frame Packing */
enum frame_packing_arrangement_e
{
    FRAME_PACKING_CHECKERBOARD,
    FRAME_PACKING_COLUMN,
    FRAME_PACKING_ROW,
    FRAME_PACKING_SIDE_BY_SIDE,
    FRAME_PACKING_TOP_BOTTOM,
    FRAME_PACKING_TEMPORAL,
};

/**** AC-3 Encoding ****/

/**** AAC Encoding ****/
typedef struct
{
    int he_aac;
    int capped_vbr;

    int latm_output;
} obe_aac_opts_t;

/* Stream Options:
 *
 * stream_id - stream id. Streams cannot be duplicated
 * stream_action - stream action. Video streams must be encoded
 *
 * Encode Options: (ignored in passthrough mode)
 * stream_format - stream_format
 *
 */

typedef struct
{
    int stream_id;
    int stream_action;

    /** Encode options **/
    int stream_format;
    /* AVC */
    x264_param_t avc_param;

    /* Audio */
    int bitrate;

    /* AC-3 */
    // TODO

    /* AAC */
    obe_aac_opts_t aac_opts;

    /** Mux options **/
    /* MPEG-TS */
    obe_ts_stream_opts_t ts_opts;
} obe_output_stream_t;

int obe_setup_streams( obe_t *h, obe_output_stream_t *output_streams, int num_streams );

/**** Muxers *****/
enum muxers_e
{
    MUXERS_MPEGTS,
};

/**** Transport Stream ****/
enum obe_ts_type_t
{
    OBE_TS_TYPE_GENERIC,
    OBE_TS_TYPE_DVB,
    OBE_TS_TYPE_CABLELABS,
    OBE_TS_TYPE_ATSC,
    OBE_TS_TYPE_ISDB,
};

typedef struct
{
    int muxer;

    /** MPEG-TS **/
    int ts_type;
    int cbr;
    int ts_muxrate;

    int passthrough;
// TODO mention only passthrough next four things
// TODO mention default pids
    int ts_id;
    int program_num;
    int pmt_pid;
    int pcr_pid;

    int pcr_period;
    int pat_period;

    int is_3dtv;

    /* DVB */

    /* ATSC */
    int sb_leak_rate;
    int sb_size;
} obe_mux_opts_t;

int obe_setup_muxer( obe_t *h, obe_mux_opts_t *mux_opts );

/**** Output *****/
enum output_e
{
    OUTPUT_UDP, /* MPEG-TS in UDP */
    OUTPUT_RTP, /* MPEG-TS in RTP in UDP */
//    OUTPUT_ASI,
};

/* Output structure
 *
 * location - TODO document url parameters
 *
 */

typedef struct
{
    int output;
    char *location;
} obe_output_opts_t;

int obe_setup_output( obe_t *h, obe_output_opts_t *output_opts );

int obe_start( obe_t *h );
int obe_stop( obe_t *h );

void obe_close( obe_t *h );

#endif
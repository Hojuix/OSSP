/*
 * OpenSubsonicPlayer
 * Goldenkrew3000 2025
 * License: GNU General Public License 3.0
 * Info: Gstreamer Handler
 */

#include <stdio.h>
#include <gst/gst.h>
#include <math.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "../configHandler.h"
#include "../discordrpc.h"
#include "../libopensubsonic/logger.h"
#include "../libopensubsonic/endpoint_getSong.h"
#include "../libopensubsonic/httpclient.h"
#include "playQueue.hpp"
#include "player.h"

#if defined(__ANDROID__)
#include <android/log.h>
#define printf(...) __android_log_print(ANDROID_LOG_INFO, "OSSP", __VA_ARGS__)
#endif

#include <gst/audio/audio-channels.h>

// TESTING
#include "scrobbler_lastFm.h"

extern OSSP_config_t* configObj;
static int rc = 0;
GstElement *pipeline, *playbin, *filter_bin, *conv_in, *conv_out, *in_volume, *equalizer, *sofa, *pitch, *reverb, *out_volume;
GstPad *sink_pad, *src_pad;
GstBus* bus;
guint bus_watch_id;
GMainLoop* loop;
bool isPlaying = false;

static void gst_playbin3_sourcesetup_callback(GstElement* playbin, GstElement* source, gpointer udata) {
    g_object_set(G_OBJECT(source), "user-agent", "OSSP/1.0 (avery@hojuix.org)", NULL);
}

static gboolean gst_bus_call(GstBus* bus, GstMessage* message, gpointer data) {
    GMainLoop* loop = (GMainLoop*)data;

    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_EOS:
            logger_log_important(__func__, "[GBus] End of stream");
            OSSPQ_advancePos(); // Move to next song in queue
            gst_element_set_state(pipeline, GST_STATE_NULL);
            isPlaying = false;
            break;
        case GST_MESSAGE_BUFFERING: {
            gint percent = 0;
            gst_message_parse_buffering(message, &percent);
            //printf("Buffering (%d%%)...\n", (int)percent);
            break;
        }
        case GST_MESSAGE_ERROR: {
            gchar* debug;
            GError* error;
            gst_message_parse_error(message, &error, &debug);
            printf("Gstreamer Error: %s\n", error->message);
            g_error_free(error);
            g_free(debug);
            break;
        }
        case GST_MESSAGE_STATE_CHANGED:
            //printf("State changed\n");
            break;
        case GST_MESSAGE_NEW_CLOCK:
            //
            break;
        case GST_MESSAGE_LATENCY:
            //
            break;
        case GST_MESSAGE_STREAM_START:
            //
            break;
        case GST_MESSAGE_ELEMENT:
            //
            break;
        case GST_MESSAGE_ASYNC_DONE:
            //
            break;
        case GST_MESSAGE_STREAM_STATUS:
            //
            break;
        case GST_MESSAGE_STREAMS_SELECTED:
            //
            break;
        case GST_MESSAGE_STREAM_COLLECTION:
            //
            break;
        case GST_MESSAGE_DURATION_CHANGED:
            //
            break;
        case GST_MESSAGE_TAG:
            // Unused
            break;
        default:
            printf("Unknown Message. Type %ld\n", GST_MESSAGE_TYPE(message));
            break;
    }

    return TRUE;
}

void* OSSPlayer_GMainLoop(void* arg) {
    (void)arg;
    logger_log_important(__func__, "GMainLoop thread running.");
    // This is needed for the Gstreamer bus to work, but it hangs the thread
    g_main_loop_run(loop);
}

void* OSSPlayer_ThrdInit(void* arg) {
    (void)arg;
    bool haveIssuedDiscordRPCIdle = true;
    bool haveScrobbledSong = false;

    // Player init function for pthread entry
    logger_log_important(__func__, "Player thread running.");
    OSSPlayer_GstInit();

    // Launch GMainLoop thread
    pthread_t pthr_gml;
    pthread_create(&pthr_gml, NULL, OSSPlayer_GMainLoop, NULL);

    // Poll play queue for new items to play
    while (true) { // TODO use global bool instead
        if (OSSPQ_getTotalPos() != 0 &&
            OSSPQ_getCurrentPos() != OSSPQ_getTotalPos() &&
            isPlaying == false) {
            // Player is not playing and a song is in the song queue

            // Pull new song from the song queue
            OSSPQ_SongStruct* songObject = OSSPQ_getAtPos(OSSPQ_getCurrentPos());
            if (songObject == NULL) {
                // Severe error - There was an item in the queue, but fetching it didn't work
                printf("[OSSPlayer]\n");
                // TODO: this
            }

            // Reset scrobble
            haveScrobbledSong = false;

            if (songObject->mode == OSSPQ_MODE_INTERNETRADIO) {
                // Setup Discord RPC
                /*
                OSSP_discordrpc_t* discordrpc = NULL;
                discordrpc_struct_init(&discordrpc);
                discordrpc->state = DISCORDRPC_STATE_PLAYING_INTERNETRADIO;
                discordrpc->startTime = time(NULL);
                discordrpc->songTitle = strdup(songObject->title);
                discordrpc_update(&discordrpc);
                discordrpc_struct_deinit(&discordrpc);
                */

                // Configure playbin3, and start playing
                g_object_set(playbin, "uri", songObject->streamUrl, NULL);
                //opensubsonic_httpClient_URL_cleanup(&stream_url);
                isPlaying = true;
                gst_element_set_state(pipeline, GST_STATE_PLAYING);
            } else if (songObject->mode == OSSPQ_MODE_OPENSUBSONIC) {
                // Issue initial LastFM scrobble
                /*
                scrobbler_data* scrobblerData = malloc(sizeof(scrobbler_data));
                opensubsonic_scrobble_init(scrobblerData);
                scrobblerData->songTitle = strdup(songObject->title);
                scrobblerData->songAlbum = strdup(songObject->album);
                scrobblerData->songArtist = strdup(songObject->artist);
                opensubsonic_scrobble_lastFm(scrobblerData);
                opensubsonic_scrobble_free(scrobblerData);
                */

                // Prepare Discord RPC
                /*
                discordrpc_data* discordrpc = NULL;
                discordrpc_struct_init(&discordrpc);
                discordrpc->state = DISCORDRPC_STATE_PLAYING_OPENSUBSONIC;
                discordrpc->startTime = time(NULL);
                discordrpc->songLength = songObject->duration;
                discordrpc->songTitle = strdup(songObject->title);
                discordrpc->songArtist = strdup(songObject->artist);
                if (configObj->discordrpc_showCoverArt) {
                    discordrpc->coverArtUrl = strdup(songObject->coverArtUrl);
                }
                discordrpc_update(&discordrpc);
                discordrpc_struct_deinit(&discordrpc);
                */

                // Free song queue object
                //OSSPQ_FreeSongObjectC(songObject);

                // Configure discord RPC idle boolean for when a song isn't playing
                haveIssuedDiscordRPCIdle = false;

                // Configure playbin3, free stream URL, send discord RPC, and start playing
                g_object_set(playbin, "uri", songObject->streamUrl, NULL);
                OSSPQ_FreeSongObjectC(songObject);
                isPlaying = true;
                gst_element_set_state(pipeline, GST_STATE_PLAYING);
            } else if (songObject->mode == OSSPQ_MODE_LOCALFILE) {
                // Prepare Discord RPC
                /*
                OSSP_discordrpc_t* discordrpc = NULL;
                discordrpc_struct_init(&discordrpc);
                discordrpc->state = DISCORDRPC_STATE_PLAYING_LOCALFILE;
                discordrpc->startTime = time(NULL);
                discordrpc->songLength = songObject->duration;
                discordrpc->songTitle = strdup(songObject->title);
                discordrpc->songArtist = strdup(songObject->artist);
                discordrpc_update(&discordrpc);
                discordrpc_struct_deinit(&discordrpc);
                */

                haveIssuedDiscordRPCIdle = false;

                // Configure playbin3, free stream URL, send discord RPC, and start playing
                g_object_set(playbin, "uri", songObject->streamUrl, NULL);
                printf("STREAM URI IS %s\n", songObject->streamUrl);
                OSSPQ_FreeSongObjectC(songObject);
                isPlaying = true;
                gst_element_set_state(pipeline, GST_STATE_PLAYING);
            }
        }

        if (OSSPQ_getCurrentPos() == OSSPQ_getTotalPos() && isPlaying == false) {
            // No song currently playing, and the queue is empty

            // Only send idle Discord RPC if needed to avoid spamming
            if (!haveIssuedDiscordRPCIdle) {
                printf("Issuing idle Discord RPC\n");
                haveIssuedDiscordRPCIdle = true;

                /*
                OSSP_discordrpc_t* discordrpc = NULL;
                discordrpc_struct_init(&discordrpc);
                discordrpc->state = DISCORDRPC_STATE_IDLE;
                discordrpc_update(&discordrpc);
                discordrpc_struct_deinit(&discordrpc);
                */
            }
        }

        // Scrobbler
        // Nothing playing: 0.00
        // Oh and end of song (EOS) -> 0.00
        
        // If song is >3/4 finished, perform final scrobble
        // Else, perform an in-progress scrobble every 45s (This can be handled later)
        // Have to fetch the total playback from Gstreamer, otherwise im malloc'ing every 200ms, a little fucking dramatic

        // NOTE: Cannot query Playbin3 for the length, as the OpenSubsonic /stream endpoint seems to be technically livestreaming it

        // Bad idea: Could technically do it the same way the DiscordRPC one does it
        // Gather the data at the same time, send a couple arguments and encapsulate it in it's own thread...
        // Kinda wasteful of a process though

        if (isPlaying == true) {
            float songLength = (float)OSSPQ_getSongLength(OSSPQ_getCurrentPos());
            printf("Song length: %f\n", songLength);
            printf("Current: %f\n", OSSPlayer_GstECont_Playbin3_Position_Get());

            // Check if song is >=3/4 finished
            if (OSSPlayer_GstECont_Playbin3_Position_Get() >= (songLength / 4 * 3) && haveScrobbledSong == false) {
                // Finalize song scrobble
                OSSPQ_SongStruct* songObject = OSSPQ_getAtPos(OSSPQ_getCurrentPos());

                /*
                scrobbler_data* scrobblerData = malloc(sizeof(scrobbler_data));
                opensubsonic_scrobble_init(scrobblerData);
                scrobblerData->finalize = 1;
                scrobblerData->songTitle = strdup(songObject->title);
                scrobblerData->songAlbum = strdup(songObject->album);
                scrobblerData->songArtist = strdup(songObject->artist);
                opensubsonic_scrobble_lastFm(scrobblerData);
                opensubsonic_scrobble_free(scrobblerData);
                */

                haveScrobbledSong = true;
            }
        }


           // gst_element_set_state(pipeline, GST_STATE_PLAYING);


        usleep(200 * 1000);
    }
}

/*
 * THE SPATIAL AUDIO CODE IS HIGHLY EXPERIMENTAL (BOTH HERE AND IN UPSTREAM GSTREAMER)
 * THIS IS MOSTLY A PROOF OF CONCEPT
 * DO NOT EXPECT THIS TO WORK WELL
 * Like this does _not_ sound right, but it _does_ work, just poorly
 * Plugin is gst-plugins-rs/hrtf (sofalizer)
 * Requires >=GStreamer 1.29.2
 */

#include <math.h>
#include <sys/socket.h>
#include <sys/un.h>
#define DEGREES_TO_RADIANS(deg) ((deg) * (float)(M_PI / 180.0))
float spatial_last_yaw = 0.00;
float yaw_delta_degrees = 90.00;

float orig_x[6] = { 0.00 };
float orig_z[6] = { 0.00 };
int cap[6] = { 0 };

void* OSSPlayer_SpatialThread(void* arg) {
    printf("[OSSPlayer] Running Spatial Thread.\n");

    while (true) {
        // Read current yaw from socket
        printf("[OSSPlayer/Spatial] Current yaw is %f\n", yaw_delta_degrees);

        // Extract current spatial objects
        GValue spatial_array = G_VALUE_INIT;
        g_object_get_property(G_OBJECT(sofa), "spatial-objects", &spatial_array);
        int spatial_objects = gst_value_array_get_size(&spatial_array);
        if (spatial_objects > 0) {
            // Spatial objects have appeared in array
            for (int i = 0; i < spatial_objects; i++) {
                // There should either be 2 or 6 spacial objects
                // 2 -> Virtual stereo surround
                // 6 -> Dolby EAC3 5.1
                GValue* spatial_object_element = gst_value_array_get_value(&spatial_array, i);
                if (G_VALUE_HOLDS(spatial_object_element, GST_TYPE_STRUCTURE)) {
                    GstStructure* spatial_object = g_value_get_boxed(spatial_object_element);

                    float x = 0, y = 0, z = 0, dg = 0;
                    GValue *x_val = gst_structure_get_value(spatial_object, "x");
                    GValue *y_val = gst_structure_get_value(spatial_object, "y");
                    GValue *z_val = gst_structure_get_value(spatial_object, "z");
                    GValue *dg_val = gst_structure_get_value(spatial_object, "distance-gain");
                    if (x_val) { x = g_value_get_float(x_val); }
                    if (y_val) { y = g_value_get_float(y_val); }
                    if (z_val) { z = g_value_get_float(z_val); }
                    if (dg_val) { dg = g_value_get_float(dg_val); }
                    printf("[OSSPlayer/Spatial] Spatial Object %d - X: %.4f, Y: %.4f, Z: %.4f, DG: %.4f\n",
                        i, x, y, z, dg);

                    if (cap[i] == 0) {
                        orig_x[i] = x;
                        orig_z[i] = z;
                        cap[i] = 1;
                    }
                    
                    float yaw_rad = DEGREES_TO_RADIANS(yaw_delta_degrees); // TODO
                    //float rotated_x = x * cosf(yaw_rad) + z * sinf(yaw_rad);
                    //float rotated_z = -x * sinf(yaw_rad) + z * cosf(yaw_rad);
                    float rotated_x = orig_x[i] * cosf(yaw_rad) + orig_z[i] * sinf(yaw_rad);
                    float rotated_z = -orig_x[i] * sinf(yaw_rad) + orig_z[i] * cosf(yaw_rad);

                    gst_structure_set(spatial_object,
                        "x", G_TYPE_FLOAT, rotated_x,
                        "z", G_TYPE_FLOAT, rotated_z,
                        "distance-gain", G_TYPE_FLOAT, dg,
                        NULL);
                }
            }
            g_object_set_property(G_OBJECT(sofa), "spatial-objects", &spatial_array);

            yaw_delta_degrees += 6.00;
            if (yaw_delta_degrees > 360.00) { yaw_delta_degrees = 0.00; }
            usleep(1000 * 100);
        } else {
            usleep(1000 * 100); // Wait 100ms before checking again
        }
    }
}

#if 0
static void on_deinterleave_pad_added(GstElement *deinterleave, GstPad *new_pad, gpointer data) {
    GstElement *target_queue = (GstElement *)data;
    GstPad *sink_pad = NULL;
    gchar *pad_name = gst_pad_get_name(new_pad);

    g_print("Deinterleave dynamically added pad: %s\n", pad_name);

    //Check if this is the first audio channel (Left channel / src_0)
    if (g_str_equal(pad_name, "src_0")) {
        //Get the static sink pad of your target branch queue (q_c1)
        sink_pad = gst_element_get_static_pad(target_queue, "sink");

        if (sink_pad) {
            if (!gst_pad_is_linked(sink_pad)) {
                GstPadLinkReturn link_res = gst_pad_link(new_pad, sink_pad);
                if (GST_PAD_LINK_SUCCESSFUL(link_res)) {
                    g_print("Successfully linked deinterleave:%s to queue:sink\n", pad_name);
                } else {
                    g_printerr("Failed to link pads. Error code: %d\n", link_res);
                }
            }
            gst_object_unref(sink_pad);
        }
    } else {
        g_print("Ignoring pad %s (if you want to process the right channel, add a check for 'src_1')\n", pad_name);
    }

    g_free(pad_name);
}
#endif

#if 0
static void on_deinterleave_pad_added(GstElement *deinterleave, GstPad *new_pad, gpointer data) {
    AudioBranches *branches = (AudioBranches *)data;
    gchar *pad_name = gst_pad_get_name(new_pad);
    GstPad *sink_pad = NULL;

    g_print("Splitter found audio channel pad: %s\n", pad_name);

    if (g_str_equal(pad_name, "src_0")) {
        /* Route Channel 0 to your active custom branch queue */
        sink_pad = gst_element_get_static_pad(branches->queue_left, "sink");

        if (sink_pad) {
            if (!gst_pad_is_linked(sink_pad)) {
                gst_pad_link(new_pad, sink_pad);
                g_print("Successfully linked active channel: %s\n", pad_name);
            }
            gst_object_unref(sink_pad);
        }
    } else {
        /* FIX: Dynamically create a discarded pathway for ALL unhandled surround channels (src_1 to src_5) */
        g_print("Discarding unhandled multi-channel pad: %s\n", pad_name);

        /* 1. Create a dynamic fakesink to swallow this specific channel's data */
        gchar *sink_name = g_strdup_printf("fakesink_%s", pad_name);
        GstElement *fakesink = gst_element_factory_make("fakesink", sink_name);
        g_free(sink_name);

        /* 2. Sync must be TRUE so it handles the timing properly without rushing ahead */
        g_object_set(G_OBJECT(fakesink), "sync", TRUE, NULL);

        /* 3. Get the parent bin (custom_sink_bin) and add the fakesink to it */
        GstElement *bin = GST_ELEMENT(gst_element_get_parent(deinterleave));
        gst_bin_add(GST_BIN(bin), fakesink);

        /* 4. Sync the fakesink state with the rest of the running pipeline */
        gst_element_sync_state_with_parent(fakesink);
        gst_object_unref(bin);

        /* 5. Cleanly link the dynamic pad straight to our new fakesink */
        sink_pad = gst_element_get_static_pad(fakesink, "sink");
        if (sink_pad) {
            gst_pad_link(new_pad, sink_pad);
            gst_object_unref(sink_pad);
        }
    }

    g_free(pad_name);
}
#endif

#if 0
static void on_deinterleave_pad_added(GstElement *deinterleave, GstPad *new_pad, gpointer data) {
    /* FIX: Cast data directly to your left queue element variable */
    GstElement *q_left = (GstElement *)data;
    gchar *pad_name = gst_pad_get_name(new_pad);
    GstPad *sink_pad = NULL;

    g_print("Splitter found audio channel pad: %s\n", pad_name);

    if (g_str_equal(pad_name, "src_0")) {
        /* Get the static sink pad of your left queue variable directly */
        sink_pad = gst_element_get_static_pad(q_left, "sink");

        if (sink_pad) {
            if (!gst_pad_is_linked(sink_pad)) {
                gst_pad_link(new_pad, sink_pad);
                g_print("Successfully linked active channel: %s\n", pad_name);
            }
            gst_object_unref(sink_pad);
        }
    } else {
        g_print("Discarding unhandled multi-channel pad: %s\n", pad_name);

        gchar *sink_name = g_strdup_printf("fakesink_%s", pad_name);
        GstElement *fakesink = gst_element_factory_make("fakesink", sink_name);
        g_free(sink_name);

        g_object_set(G_OBJECT(fakesink), "sync", TRUE, NULL);

        GstElement *bin = GST_ELEMENT(gst_element_get_parent(deinterleave));
        gst_bin_add(GST_BIN(bin), fakesink);

        gst_element_sync_state_with_parent(fakesink);
        gst_object_unref(bin);

        sink_pad = gst_element_get_static_pad(fakesink, "sink");
        if (sink_pad) {
            gst_pad_link(new_pad, sink_pad);
            gst_object_unref(sink_pad);
        }
    }

    g_free(pad_name);
}
#endif

#include <stdlib.h> // for strtol

static void on_deinterleave_pad_added(GstElement *deinterleave, GstPad *new_pad, gpointer data) {
    /* Cast the data parameter back to your array of GstElement pointers */
    GstElement **queues = (GstElement **)data;
    gchar *pad_name = gst_pad_get_name(new_pad);
    GstPad *sink_pad = NULL;

    g_print("Splitter found channel pad: %s\n", pad_name);

    /* Extract the channel index number from the pad name string (e.g., "src_3" -> 3) */
    if (g_str_has_prefix(pad_name, "src_")) {
        int channel_idx = (int)g_ascii_strtoull(pad_name + 4, NULL, 10);

        /* Ensure the detected channel index fits within your 6 manual branches */
        if (channel_idx >= 0 && channel_idx < 6) {
            g_print("Routing %s dynamically to channel queue index %d...\n", pad_name, channel_idx + 1);

            sink_pad = gst_element_get_static_pad(queues[channel_idx], "sink");
            if (sink_pad) {
                if (!gst_pad_is_linked(sink_pad)) {
                    GstPadLinkReturn link_res = gst_pad_link(new_pad, sink_pad);
                    if (!GST_PAD_LINK_SUCCESSFUL(link_res)) {
                        g_printerr("Failed linking %s to branch queue. Error: %d\n", pad_name, link_res);
                    }
                }
                gst_object_unref(sink_pad);
            }
        } else {
            g_printerr("Detected channel index %d is out of bounds for 6-channel config!\n", channel_idx);
        }
    }

    g_free(pad_name);
}



int OSSPlayer_GstInit() {
    printf("[OSSP] Initializing Gstreamer...\n");

    // If a custom LV2 path is defined in the configuration, use it
    if (configObj->lv2_use_custom_path) {
        printf("[OSSPlayer] Using custom LV2 path: %s\n", configObj->lv2_custom_path);
        setenv("LV2_PATH", configObj->lv2_custom_path, 1);
    }

    // Initialize gstreamer
    gst_init(NULL, NULL);
    loop = g_main_loop_new(NULL, FALSE);

    // Create base pipeline elements
    pipeline = gst_pipeline_new("pipeline");
    playbin = gst_element_factory_make("playbin3", "player");
    // TODO: Fix erroring
    if (!pipeline) {
        logger_log_error(__func__, "Could not initialize pipeline.");
    }
    if (!playbin) {
        logger_log_error(__func__, "Could not initialize playbin3");
    }

    // Add message handler
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    // TODO: Check bus is made properly
    bus_watch_id = gst_bus_add_watch(bus, gst_bus_call, loop);
    gst_object_unref(bus);


    filter_bin = gst_bin_new("filter-bin");
    conv_in = gst_element_factory_make("audioconvert", "convert-in");
    conv_out = gst_element_factory_make("audioconvert", "convert-out");
    // TODO: Check creation

    // Create configuration defined elements
    in_volume = gst_element_factory_make("volume", "in-volume");
    if (configObj->audio_equalizer_enable) {
        // LSP Para x32 LR Equalizer
        equalizer = gst_element_factory_make(configObj->lv2_parax32_filter_name, "equalizer");
    }
    //if (configObj->audio_spatial_enable) {
        sofa = gst_element_factory_make("sofalizer", "sofa");
    //}
    if (configObj->audio_pitch_enable) {
        // Soundtouch Pitch
        pitch = gst_element_factory_make("pitch", "pitch");
    }
    if (configObj->audio_reverb_enable) {
        // Calf Studio Plugins Reverb
        reverb = gst_element_factory_make(configObj->lv2_reverb_filter_name, "reverb");
    }
    out_volume = gst_element_factory_make("volume", "out-volume");
    // TODO: Make better error messages for here, and exit out early
    if (!equalizer) {
        logger_log_error(__func__, "Could not initialize equalizer.");
    }
    if (!sofa) {
        printf("[OSSPlayer] Could not initialize SOFA\n");
    }
    if (!pitch) {
        logger_log_error(__func__, "Could not initialize pitch.");
    }
    if (!reverb) {
        logger_log_error(__func__, "Could not initialize reverb.");
    }




    g_object_set(sofa, "sofa", "/home/user/Downloads/ClubFritz4.sofa", NULL);
    pthread_t pthr_spatial;
    //pthread_create(&pthr_spatial, NULL, OSSPlayer_SpatialThread, NULL);

    GstElement* audioconvert = gst_element_factory_make("audioconvert", "filter-convert");
    GstElement* audioresample = gst_element_factory_make("audioresample", "filter-resample");

    GstElement* q_c1 = gst_element_factory_make("queue", "queue_c1");
    GstElement* q_c2 = gst_element_factory_make("queue", "queue_c2");
    GstElement* q_c3 = gst_element_factory_make("queue", "queue_c3");
    GstElement* q_c4 = gst_element_factory_make("queue", "queue_c4");
    GstElement* q_c5 = gst_element_factory_make("queue", "queue_c5");
    GstElement* q_c6 = gst_element_factory_make("queue", "queue_c6");

    GstElement* queues[] = { q_c1, q_c2, q_c3, q_c4, q_c5, q_c6 };
    for (int i = 0; i < 6; i++) {
        g_object_set(queues[i],
                     "max-size-time", (guint64)2000000000, // 2 seconds buffer capacity
                     "max-size-bytes", 0,
                     "max-size-buffers", 0,
                     NULL);
    }

    GstElement* conv_c1 = gst_element_factory_make("audioconvert", "conv_c1");
    GstElement* conv_c2 = gst_element_factory_make("audioconvert", "conv_c2");
    GstElement* conv_c3 = gst_element_factory_make("audioconvert", "conv_c3");
    GstElement* conv_c4 = gst_element_factory_make("audioconvert", "conv_c4");
    GstElement* conv_c5 = gst_element_factory_make("audioconvert", "conv_c5");
    GstElement* conv_c6 = gst_element_factory_make("audioconvert", "conv_c6");

    GstElement* pitch_c1 = gst_element_factory_make("pitch", "pitch_c1");
    GstElement* pitch_c2 = gst_element_factory_make("pitch", "pitch_c2");
    GstElement* pitch_c3 = gst_element_factory_make("pitch", "pitch_c3");
    GstElement* pitch_c4 = gst_element_factory_make("pitch", "pitch_c4");
    GstElement* pitch_c5 = gst_element_factory_make("pitch", "pitch_c5");
    GstElement* pitch_c6 = gst_element_factory_make("pitch", "pitch_c6");

    float scaleFactor_ca = OSSPlayer_CentsToPSF(configObj->audio_pitch_cents);
    g_object_set(pitch_c1, "pitch", scaleFactor_ca, NULL);
    g_object_set(pitch_c2, "pitch", scaleFactor_ca, NULL);
    g_object_set(pitch_c3, "pitch", scaleFactor_ca, NULL);
    g_object_set(pitch_c4, "pitch", scaleFactor_ca, NULL);
    g_object_set(pitch_c5, "pitch", scaleFactor_ca, NULL);
    g_object_set(pitch_c6, "pitch", scaleFactor_ca, NULL);



    //GstElement* equalizer_c1 = gst_element_factory_make(configObj->lv2_parax32_filter_name, "equalizer");




    GstElement* deinterleave = gst_element_factory_make("deinterleave", "splitter");
    GstElement* interleave = gst_element_factory_make("interleave", "recombiner");

    // Single output sink instead of 6
    GstElement* post_convert = gst_element_factory_make("audioconvert", "post_convert");
    GstElement* post_resample = gst_element_factory_make("audioresample", "post_resample");

    if (!post_convert || !post_resample) {
        printf("Nope!\n");
    }

    GstElement* audio_sink = gst_element_factory_make("autoaudiosink", "audio_output");
    if (!audio_sink) { printf("Nope\n"); }

    gst_bin_add_many(GST_BIN(filter_bin), conv_in, pitch_c1, deinterleave,
                     q_c1, conv_c1, // effects go after conv_cX
                     q_c2, conv_c2,
                     q_c3, conv_c3,
                     q_c4, conv_c4,
                     q_c5, conv_c5,
                     q_c6, conv_c6,
                     interleave, post_convert, sofa, post_resample, audio_sink,
                     NULL);

    // Link input to deinterleave
    if (!gst_element_link_many(conv_in, pitch_c1, deinterleave, NULL)) {
        g_printerr("Failed to link conv_in to deinterleave.\n");
    }

    // Link each channel branch
    gst_element_link_many(q_c1, conv_c1, NULL);
    gst_element_link_many(q_c2, conv_c2, NULL);
    gst_element_link_many(q_c3, conv_c3, NULL);
    gst_element_link_many(q_c4, conv_c4, NULL);
    gst_element_link_many(q_c5, conv_c5, NULL);
    gst_element_link_many(q_c6, conv_c6, NULL);

    // Create array for callback
    GstElement **channel_queues = g_new0(GstElement*, 6);
    channel_queues[0] = q_c1;
    channel_queues[1] = q_c2;
    channel_queues[2] = q_c3;
    channel_queues[3] = q_c4;
    channel_queues[4] = q_c5;
    channel_queues[5] = q_c6;

    g_signal_connect(deinterleave, "pad-added", G_CALLBACK(on_deinterleave_pad_added), channel_queues);

    // Link all channel endpoints to interleave
    GstElement* branch_endpoints[] = { conv_c1, conv_c2, conv_c3, conv_c4, conv_c5, conv_c6 };

    for (int i = 0; i < 6; i++) {
        gchar pad_name[16];
        g_snprintf(pad_name, sizeof(pad_name), "sink_%d", i);

        GstPad* sink_pad = gst_element_get_request_pad(interleave, pad_name);
        GstPad* src_pad = gst_element_get_static_pad(branch_endpoints[i], "src");

        if (sink_pad && src_pad) {
            if (gst_pad_link(src_pad, sink_pad) != GST_PAD_LINK_OK) {
                g_printerr("Failed to link branch %d to interleave\n", i);
            }
        }
        if (src_pad) gst_object_unref(src_pad);
        if (sink_pad) gst_object_unref(sink_pad);
    }

    // Link interleave to final sink
    if (!gst_element_link_many(interleave, post_convert, sofa, post_resample, audio_sink, NULL)) {
        g_printerr("Failed to link interleave to audio_sink.\n");
    }

    for (int i = 0; i < 6; i++) {
        g_object_set(queues[i],
                     "max-size-time", (guint64)5000000000, // 5 seconds instead of 2
                     "max-size-bytes", 0,
                     "max-size-buffers", 0,
                     NULL);
    }

    // Configure the output sink
    g_object_set(G_OBJECT(audio_sink),
                 "sync", TRUE,
                 "provide-clock", TRUE,
                 "slave-method", 1,
                 "alignment-threshold", (GstClockTime)40000000,
                 "drift-threshold", (GstClockTime)100000,
                 NULL);

    GstStructure *stream_props = gst_structure_new("properties",
                                                   "media.role", G_TYPE_STRING, "video",
                                                   "production", G_TYPE_BOOLEAN, TRUE,
                                                   NULL);
    g_object_set(G_OBJECT(audio_sink), "stream-properties", stream_props, NULL);
    gst_structure_free(stream_props);

    // Ghost pad for input
    GstPad* sink_pad = gst_element_get_static_pad(conv_in, "sink");
    gst_element_add_pad(filter_bin, gst_ghost_pad_new("sink", sink_pad));
    gst_object_unref(sink_pad);

    g_object_set(playbin, "audio-sink", filter_bin, NULL);
    g_signal_connect(playbin, "source-setup", G_CALLBACK(gst_playbin3_sourcesetup_callback), NULL);


    //gst_element_link_many(q_c1, conv_out, o_c1, NULL);


    // Add and link elements to the filter bin
    // TODO: Check creation and dynamic as per config
    /*
    gst_bin_add_many(GST_BIN(filter_bin),
                     conv_in,
                     in_volume,
                     sofa,
                     equalizer, pitch, out_volume,
                     conv_out,
                     NULL);
    gst_element_link_many(conv_in,
                          in_volume,
                          sofa,
                          equalizer, pitch, out_volume,
                          conv_out,
                          NULL);*/


    //sink_pad = gst_element_get_static_pad(conv_in, "sink");
    //src_pad = gst_element_get_static_pad(conv_out, "src");
    //gst_element_add_pad(filter_bin, gst_ghost_pad_new("sink", sink_pad));
    //gst_element_add_pad(filter_bin, gst_ghost_pad_new("src", src_pad));
    //gst_object_unref(sink_pad);
    //gst_object_unref(src_pad);

    //GstPad* tee_pad_passthrough = gst_element_request_pad_simple(input_tee, "src_%u");
    //gst_element_add_pad(filter_bin, gst_ghost_pad_new("src", tee_pad_passthrough));
    //gst_object_unref(tee_pad_passthrough);


    // Setup playbin3 (Configure audio plugins and set user agent)

    // Add playbin3 to the pipeline
    gst_bin_add(GST_BIN(pipeline), playbin);

    // Initialize in-volume (Volume before the audio reaches the plugins)
    g_object_set(in_volume, "volume", 0.145, NULL); // 0.175

    // Initialize out-volume (Volume after the audio plugins)
    g_object_set(out_volume, "volume", 1.00, NULL);

    // Initialize equalizer
    if (configObj->audio_equalizer_enable) {
        // Dynamically append settings to the equalizer to match the config file
        for (int i = 0; i < configObj->audio_equalizer_presets[0].graph_count; i++) {
            char* ftl_name = NULL;
            char* ftr_name = NULL;
            char* gl_name = NULL;
            char* gr_name = NULL;
            char* ql_name = NULL;
            char* qr_name = NULL;
            char* fl_name = NULL;
            char* fr_name = NULL;

            asprintf(&ftl_name, "%s%d", configObj->lv2_parax32_filter_type_left, i);
            asprintf(&ftr_name, "%s%d", configObj->lv2_parax32_filter_type_right, i);
            asprintf(&gl_name, "%s%d", configObj->lv2_parax32_gain_left, i);
            asprintf(&gr_name, "%s%d", configObj->lv2_parax32_gain_right, i);
            asprintf(&ql_name, "%s%d", configObj->lv2_parax32_quality_left, i);
            asprintf(&qr_name, "%s%d", configObj->lv2_parax32_quality_right, i);
            asprintf(&fl_name, "%s%d", configObj->lv2_parax32_frequency_left, i);
            asprintf(&fr_name, "%s%d", configObj->lv2_parax32_frequency_right, i);

            g_object_set(equalizer, ftl_name, 1, NULL);
            g_object_set(equalizer, ftr_name, 1, NULL);

            // NOTE: Making an extra variable here to avoid nesting a function within a function
            float gain = (float)configObj->audio_equalizer_presets[0].audio_equalizer_graph[i].gain;
            gain = OSSPlayer_DbLinMul(gain);
            g_object_set(equalizer, gl_name, gain, NULL);
            g_object_set(equalizer, gr_name, gain, NULL);

            g_object_set(equalizer, ql_name, 4.36, NULL);
            g_object_set(equalizer, qr_name, 4.36, NULL);

            // NOTE: Same function nesting mitigation here
            if (configObj->audio_equalizer_presets[0].follow_pitch) {
                // Adjust equalizer frequency to match pitch adjustment
                // TODO: Should I also check if pitch is enabled, or just if pitch follow is enabled??
                // TODO: Also check that freq following is working properly as per swift version
                float freq = (float)configObj->audio_equalizer_presets[0].audio_equalizer_graph[i].frequency;
                float semitone = (float)configObj->audio_pitch_cents / 100.0;
                freq = OSSPlayer_PitchFollow(freq, semitone);
                printf("EQ band %d - F: %.2f(Fp) / G: %.2f / Q: 4.36\n", i + 1, freq, gain);
                g_object_set(equalizer, fl_name, freq, NULL);
                g_object_set(equalizer, fr_name, freq, NULL);
            } else {
                printf("EQ band %d - F: %.2f(Nfp) / G: %.2f / Q: 4.36\n", i + 1, (float)configObj->audio_equalizer_presets[0].audio_equalizer_graph[i].frequency, gain);
                g_object_set(equalizer, fl_name, (float)configObj->audio_equalizer_presets[0].audio_equalizer_graph[i].frequency, NULL);
                g_object_set(equalizer, fr_name, (float)configObj->audio_equalizer_presets[0].audio_equalizer_graph[i].frequency, NULL);
            }

            free(ftl_name);
            free(ftr_name);
            free(gl_name);
            free(gr_name);
            free(ql_name);
            free(qr_name);
            free(fl_name);
            free(fr_name);
        }

        g_object_set(equalizer, "enabled", true, NULL);
    }

    // Initialize pitch
    if (configObj->audio_pitch_enable) {
        float scaleFactor = OSSPlayer_CentsToPSF(configObj->audio_pitch_cents);
        printf("Pitch Cents: %.2f, Scale factor: %.6f\n", configObj->audio_pitch_cents, scaleFactor);
        g_object_set(pitch, "pitch", scaleFactor, NULL);
    }



    // Initialize reverb


}

int OSSPlayer_GstDeInit() {
    //
}

/*
 * Player Queue Control Functions
 */
int OSSPlayer_QueueAppend_Song(char* title, char* artist, char* id, long duration) {
    // Call to C++ function
    // Note: I would receive a song struct instead of individual elements, but it would significantly slow down the GUI
    //
}

int OSSPlayer_QueueAppend_Radio(char* name, char* id, char* radioUrl) {
    // Call to C++ function
    // Append radio station to the play queue
    //internal_OSSPQ_AppendToEnd(name, NULL, id, 0, 1, radioUrl);
}

OSSPQ_SongStruct* OSSPlayer_QueuePopFront() {
    // Call to C++ function

    //OSSPQ_SongStruct* songObject = internal_OSSPQ_PopFromFront();

    //if (songObject == NULL) {
        // Queue is empty TODO
        //printf("FUCKFUCKFUCK\n");
    //}
    //return songObject;
}

/*
 * Gstreamer Element Control Functions
 */
// TODO: Consolidate volume functions?
float OSSPlayer_GstECont_InVolume_Get() {
    gdouble vol;
    g_object_get(in_volume, "volume", &vol, NULL);
    return (float)vol;
}

void OSSPlayer_GstECont_InVolume_set(float val) {
    g_object_set(in_volume, "volume", val, NULL);
}

float OSSPlayer_GstECont_OutVolume_Get() {
    gdouble vol;
    g_object_get(out_volume, "volume", &vol, NULL);
    return (float)vol;
}

void OSSPlayer_GstECont_OutVolume_set(float val) {
    g_object_set(out_volume, "volume", val, NULL);
}

float OSSPlayer_GstECont_Playbin3_Position_Get() {
    gint64 seek_pos;
    if (gst_element_query_position(playbin, GST_FORMAT_TIME, &seek_pos)) {
        return (float)seek_pos / GST_SECOND;
    } else {
        return (float)0.00;
    }
}

void OSSPLayer_GstECont_Playbin3_Position_Set(float seek_pos) {
    gst_element_seek_simple(playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, (gint64)(seek_pos * GST_SECOND));
}

float OSSPlayer_GstECont_Pitch_Get() {
    //
}

void OSSPlayer_GstECont_Pitch_Set(float cents) {
    float psf = OSSPlayer_CentsToPSF(cents);
    g_object_set(pitch, "pitch", psf, NULL);
}

void OSSPlayer_GstECont_Playbin3_Stop() {
    OSSPQ_advancePos();
    gst_element_set_state(pipeline, GST_STATE_NULL); // Stop playbin3
    isPlaying = false; // Notify player thread to attempt to load next song
}

// Returns 0 if now paused, 1 if now playing
int OSSPlayer_GstECont_Playbin3_PlayPause() {
    GstState state;
    gst_element_get_state(pipeline, &state, NULL, 0);

    if (state == GST_STATE_PLAYING) {
        gst_element_set_state (pipeline, GST_STATE_PAUSED);

        // Issue Pause to Discord RPC
        OSSPlayer_DiscordRPC_SendPaused();
        
        return 0;
    } else {
        gst_element_set_state (pipeline, GST_STATE_PLAYING);

        // Get current position in song, and find start time for Discord RPC
        float curr_pos = OSSPlayer_GstECont_Playbin3_Position_Get();
        time_t curr_time = time(NULL);
        time_t start_time = curr_time - (int)curr_pos;

        // Issue Playing to Discord RPC
        OSSPlayer_DiscordRPC_SendPlaying(start_time);

        return 1;
    }
}

void OSSPlayer_GstECont_Playbin3_Prev() {
    // Move queue back by one, then stop playbin3 and notify player thread
    printf("[OSSPP] Moving the player queue back by one.\n");
    OSSPQ_backtrackPos();
    gst_element_set_state(pipeline, GST_STATE_NULL);
    isPlaying = false;
}

void OSSPlayer_GstECont_Playbin3_Next() {
    // Same as *Playbin3_Prev(), but advance queue by one
    printf("[OSSPP] Moving the player forward back by one.\n");
    OSSPQ_advancePos();
    gst_element_set_state(pipeline, GST_STATE_NULL);
    isPlaying = false;
}

void OSSPlayer_GstECont_Playbin3_StartQueue() {
    //
}

void OSSPlayer_GstECont_Playbin3_EndQueue() {
    //
}

/*
 * Utility Functions
 */
float OSSPlayer_DbLinMul(float db) {
    // Convert dB to Linear Multiplier
    return pow(10.0, db / 20.0);
}

float OSSPlayer_PitchFollow(float freq, float semitone) {
    // Calculate new EQ frequency from semitone adjustment
    return freq * pow(2.0, semitone / 12.0);
}

float OSSPlayer_CentsToPSF(float cents) {
    // Convert Cents to a Pitch Scale Factor
    float semitone = cents / 100.0;
    return pow(2, (semitone / 12.0f));
}

/*
 * Functions that utilize Discord RPC
 */
void OSSPlayer_DiscordRPC_SendPaused() {
    /*
    OSSP_discordrpc_t* discordrpc = NULL;
    discordrpc_struct_init(&discordrpc);
    discordrpc->state = DISCORDRPC_STATE_PAUSED;
    discordrpc_update(&discordrpc);
    discordrpc_struct_deinit(&discordrpc);
    */
}

void OSSPlayer_DiscordRPC_SendIdle() {
    /*
    OSSP_discordrpc_t* discordrpc = NULL;
    discordrpc_struct_init(&discordrpc);
    discordrpc->state = DISCORDRPC_STATE_PAUSED;
    discordrpc_update(&discordrpc);
    discordrpc_struct_deinit(&discordrpc);
    */
}

void OSSPlayer_DiscordRPC_SendPlaying(time_t startTime) {
    OSSPQ_SongStruct* songObject = OSSPQ_getAtPos(OSSPQ_getCurrentPos());
    if (songObject == NULL) {
        // Severe error - There was an item in the queue, but fetching it didn't work
        printf("[OSSPlayer]\n");
        // TODO: this
    }

    if (songObject->mode == OSSPQ_MODE_OPENSUBSONIC) {
        // Prepare Discord RPC
        /*
        OSSP_discordrpc_t* discordrpc = NULL;
        discordrpc_struct_init(&discordrpc);
        discordrpc->state = DISCORDRPC_STATE_PLAYING_OPENSUBSONIC;
        discordrpc->startTime = startTime;
        discordrpc->songLength = songObject->duration;
        discordrpc->songTitle = strdup(songObject->title);
        discordrpc->songArtist = strdup(songObject->artist);
        if (configObj->discordrpc_showCoverArt) {
            discordrpc->coverArtUrl = strdup(songObject->coverArtUrl);
        }
        discordrpc_update(&discordrpc);
        discordrpc_struct_deinit(&discordrpc);
        */
    }

    OSSPQ_FreeSongObjectC(songObject);
}

/*
 * Functions that utilize scrobblers
 */
void OSSPlayer_Scrobbler_LastFM(int final) {
    //
}

#include <stdlib.h>
#include <gtk/gtk.h>
#include <alsa/asoundlib.h>

#include "common.h"

#define SOUND_SAMPLE_FILE RESOURCE_DIR "sample.wav"
#define SOUND_REC_FILE TEMPORARY_DIR "rec.wav"

static int get_capture_channel(unsigned int *ch)
{
	snd_pcm_t *pcm;
	snd_pcm_hw_params_t *hw_params;
	int err;

	err = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_CAPTURE,
			   SND_PCM_NONBLOCK);
	if (err < 0)
		return err;

	snd_pcm_hw_params_alloca(&hw_params);
	err = snd_pcm_hw_params_any(pcm, hw_params);
	if (err < 0) {
		snd_pcm_close(pcm);
		return err;
	}

	err = snd_pcm_hw_params_get_channels_min(hw_params, ch);
	if (err < 0) {
		snd_pcm_close(pcm);
		return err;
	}

	err = snd_pcm_hw_params_test_channels(pcm, hw_params, *ch);
	if (err < 0) {
		snd_pcm_close(pcm);
		return err;
	}

	snd_pcm_close(pcm);

	return 0;
}

static void sound_audio_play_callback(GtkWidget *widget, gpointer data)
{
	system("aplay -M -twav " SOUND_SAMPLE_FILE " >/dev/null 2>&1");
}

static void sound_rec_rec_callback(GtkWidget *widget, gpointer data)
{
	char rec_command[256];
	unsigned int ch;
	int err;

	system("rm -f " SOUND_REC_FILE " >/dev/null 2>&1");

	err = get_capture_channel(&ch);
	if (err < 0)
		return;

	sprintf(rec_command, "arecord -M -c %d -twav -fS16_LE -r48000 -d5 "
		SOUND_REC_FILE " >/dev/null 2>&1", ch);

	system(rec_command);
}

static void sound_rec_play_callback(GtkWidget *widget, gpointer data)
{
	system("aplay -M -twav " SOUND_REC_FILE " >/dev/null 2>&1");
}

int task_sound_setup(struct task *task, GtkWidget *box)
{
	GtkWidget *vbox, *hbox1, *hbox2;
	GtkWidget *frame1, *frame2;
	GtkWidget *align1, *align2;
	GtkWidget *bt_play1, *bt_play2, *bt_rec2;

	vbox = gtk_vbox_new(FALSE, 20);
	hbox1 = gtk_hbox_new(FALSE, 0);
	hbox2 = gtk_hbox_new(FALSE, 0);
	align1 = gtk_alignment_new(0.5, 0.5, 0.5, 0);
	align2 = gtk_alignment_new(0.5, 0.5, 0.5, 0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align1), 10, 10, 0, 0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align2), 10, 10, 0, 0);
	frame1 = gtk_frame_new("Audio Test");
	frame2 = gtk_frame_new("Recording Test");
	bt_play1 = gtk_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
	bt_play2 = gtk_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
	bt_rec2 = gtk_button_new_from_stock(GTK_STOCK_MEDIA_RECORD);

	gtk_box_pack_start(GTK_BOX(box), vbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), frame1, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), frame2, TRUE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame1), align1);
	gtk_container_add(GTK_CONTAINER(frame2), align2);
	gtk_container_add(GTK_CONTAINER(align1), hbox1);
	gtk_container_add(GTK_CONTAINER(align2), hbox2);
	gtk_box_pack_start(GTK_BOX(hbox1), bt_play1, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox2), bt_rec2, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox2), bt_play2, TRUE, FALSE, 0);

	/* setup signal */
	g_signal_connect(G_OBJECT(bt_play1), "clicked",
			 G_CALLBACK(sound_audio_play_callback), NULL);
	g_signal_connect(G_OBJECT(bt_play2), "clicked",
			 G_CALLBACK(sound_rec_play_callback), NULL);
	g_signal_connect(G_OBJECT(bt_rec2), "clicked",
			 G_CALLBACK(sound_rec_rec_callback), NULL);
	gtk_widget_show_all(box);

	return 0;
}

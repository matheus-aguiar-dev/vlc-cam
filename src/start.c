#include <gtk/gtk.h>
#include <unistd.h>
#include <math.h> // Include the math library
#include <gdk/gdkx.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <vlc/vlc.h>
#include <fcntl.h>
#include <linux/videodev2.h>

libvlc_instance_t * inst;
libvlc_media_player_t *mp;
libvlc_media_t *m;
GtkWidget *window,*player_widget;
GtkWidget *picture_button, *record_button;
GtkWidget *thumb_img; 
GtkWidget *recording_timer; 
guint timer_id; 
GtkBuilder *builder;
int count = 0 ;
struct CamInfo cam;
time_t rawtime;
struct tm *timeinfo;
char buffer[80];
char time_str[80];
bool is_recording = false;
const char* const vlc_args[] = {
	"--vout=xcb_x11",
	"--v4l2-chroma=mjpg",
	"--v4l2-width=500",
	"--v4l2-height=400",
	"--video-filter=transform",
	"--transform-type=hflip",
};

struct CamInfo{
	char name[100];
	int sizes[20][2];
	size_t size;
};

void release_instance(){
	libvlc_media_player_release(mp);
	libvlc_release(inst);
}
void load_instance(){
		inst = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);
		m = libvlc_media_new_location(inst, "v4l2:///dev/video0");
		mp = libvlc_media_player_new_from_media (m);
		libvlc_media_release(m);
		libvlc_media_player_play(mp);
		libvlc_media_player_set_xwindow(mp, GDK_WINDOW_XID(gtk_widget_get_window(player_widget)));
}
void configure_callback(GtkWidget *widget, GdkEventConfigure *event, gpointer data){
	int width = event->width; 
	int height = event->height;
	if(width > 1300 && height >900){
		gtk_widget_set_size_request(player_widget, width-240, height-100);
	}
	else{
		gtk_widget_set_size_request(player_widget, width-150, height-80);
	}
}
gboolean update_label(gpointer data) {
	char buffer[8];
	sprintf(buffer, "%02d:%02d", count / 60, count % 60);
	gtk_label_set_text(GTK_LABEL(recording_timer), buffer);
	count++;
	return G_SOURCE_CONTINUE;
}
void on_picture_button_clicked(GtkButton *button, gpointer user_data){
	gtk_widget_set_sensitive(picture_button, FALSE);
	release_instance();
	char widthArg[32];
	char heightArg[32];
	snprintf(widthArg, sizeof(widthArg), "--v4l2-width=%d", cam.sizes[0][0]);
	snprintf(heightArg, sizeof(heightArg), "--v4l2-height=%d", cam.sizes[0][1]);
	const char* const vlc_picture_args[] = {
		"--vout=xcb_x11",
		"--v4l2-chroma=mjpg",
		widthArg, 
		heightArg, 
		"--video-filter=transform",
		"--transform-type=hflip",
	};
	int i = 0;
	inst = libvlc_new(sizeof(vlc_picture_args) / sizeof(vlc_picture_args[0]), vlc_picture_args);
	m = libvlc_media_new_location(inst, "v4l2:///dev/video0");
	mp = libvlc_media_player_new_from_media (m);
	libvlc_media_player_play(mp);
	libvlc_media_player_set_xwindow(mp, GDK_WINDOW_XID(gtk_widget_get_window(player_widget)));
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H-%M-%S.png", timeinfo);
	gtk_widget_hide(record_button);
	g_usleep(300000);
	libvlc_video_take_snapshot(mp,0,buffer,0,0);
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(buffer, 0);
	GdkPixbuf *scaledPixbuf = gdk_pixbuf_scale_simple(pixbuf, 70, 70, GDK_INTERP_BILINEAR);
	gtk_image_set_from_pixbuf(GTK_IMAGE(thumb_img), scaledPixbuf);
	gtk_widget_show(picture_button);
	gtk_widget_show(record_button);
	gtk_widget_set_sensitive(picture_button,TRUE);
	release_instance();
	load_instance();
}

void on_record_button_clicked(GtkButton *button, gpointer user_data){
	GtkStyleContext *context = gtk_widget_get_style_context(record_button);
	char ffmpegCommand[256];
	char deleteFile[100];
	if(is_recording){
		gtk_style_context_remove_class(context, "record");
		gtk_label_set_text(GTK_LABEL(recording_timer), "00:00");
		count = 0;
		gtk_widget_hide(recording_timer);
		is_recording=false;
		libvlc_media_player_release(mp);
		libvlc_release(inst);
		inst = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);
		m = libvlc_media_new_location(inst, "v4l2:///dev/video0");
		mp = libvlc_media_player_new_from_media (m);
		libvlc_media_release(m);
		libvlc_media_player_play(mp);
		libvlc_media_player_set_xwindow(mp, GDK_WINDOW_XID(gtk_widget_get_window(player_widget)));
		gtk_widget_show(picture_button);
		g_source_remove(timer_id);
		snprintf(ffmpegCommand, sizeof(ffmpegCommand),
				"ffmpeg -i %s.ogg -vf \"hflip\" -c:v libx264 -preset medium -crf 18 -c:a libvorbis -y %s.mp4",
			time_str,time_str	);

		// Execute the FFmpeg command
		int result = system(ffmpegCommand);

		if (result == 0) {
			snprintf(deleteFile, sizeof(deleteFile),
					"%s.ogg",
					time_str	);
			if (remove(deleteFile) == 0)
				printf("Deleted successfully");
			else
				printf("Unable to delete the file");

			return 0;
			printf("FFmpeg command executed successfully.\n");
		} else {
			printf("FFmpeg command failed.\n");
		}
		fflush(stdout);
	}
	else{
		timer_id = g_timeout_add_seconds(1, update_label, NULL);
		time_t current_time;
		struct tm *time_info;
		time(&current_time);
		time_info = localtime(&current_time);
		strftime(time_str, sizeof(time_str), "%Y-%m-%d_%H-%M-%S", time_info);
		gtk_widget_show(recording_timer);
		gtk_style_context_add_class(context,"record");
		is_recording=true;
		libvlc_media_player_release(mp);
		libvlc_release(inst);
		inst = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]),vlc_args);
		m = libvlc_media_new_location(inst, "v4l2:///dev/video0");
		const char *audio_options = ":input-slave=alsa://";  // Modify this to match your audio source
		libvlc_media_add_option(m, audio_options);
		const char *transcode_options = g_strdup_printf(
				":sout=#transcode{vcodec=theo,vb=1600,fps=15,scale=1,acodec=vorb,ab=90,channels=1,samplerate=44100}:duplicate{dst=display{noaudio},dst=std{access=file,mux=ogg,dst=%s.ogg}}",
    time_str
);
		libvlc_media_add_option(m, transcode_options);
		mp = libvlc_media_player_new_from_media(m);
		libvlc_media_release(m);
		libvlc_media_player_play(mp);
		libvlc_media_player_set_xwindow(mp, GDK_WINDOW_XID(gtk_widget_get_window(player_widget)));
		gtk_widget_hide(picture_button);
	}
}
void player_widget_on_realize(GtkWidget *widget, gpointer data) {
	inst = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);
	m = libvlc_media_new_location(inst, "v4l2:///dev/video0");
	mp = libvlc_media_player_new_from_media (m);
	libvlc_media_release(m);
	libvlc_media_player_play(mp);
	libvlc_media_player_set_xwindow(mp, GDK_WINDOW_XID(gtk_widget_get_window(widget)));
	g_signal_connect(G_OBJECT(window), "configure-event", G_CALLBACK(configure_callback), NULL);    
}
static void activate (GtkApplication* app,
		gpointer        cam_data)
{
	gtk_window_set_title (GTK_WINDOW (window), "Policorp");
	gtk_widget_add_events(GTK_WIDGET(window), GDK_CONFIGURE);  
	player_widget = GTK_DRAWING_AREA(gtk_builder_get_object(builder, "cam-area"));
	g_signal_connect(G_OBJECT(player_widget), "realize", G_CALLBACK(player_widget_on_realize), NULL);    
	gtk_widget_show (window);
}
static void cam_compatibilites(struct CamInfo *cam){
	int fd = open("/dev/video0", O_RDWR);
	if (fd == -1) {
		perror("Unable to open webcam");
	}
	struct v4l2_capability cap;
	if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
		perror("VIDIOC_QUERYCAP");
		close(fd);
	}
	printf("Webcam: %s\n", cap.card);
	strcpy(cam->name,cap.card);;
	struct v4l2_frmsizeenum frmsize;
	frmsize.index = 0;
	frmsize.pixel_format = V4L2_PIX_FMT_YUYV;
	while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) != -1) {
		if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
			printf("%dx%d\n", frmsize.discrete.width,frmsize.discrete.height);
			cam->sizes[frmsize.index][0] = frmsize.discrete.width; // Divide by 2 and round
			cam->sizes[frmsize.index][1] = frmsize.discrete.height; // Divide by 2 and round
		}
		frmsize.index++;
	}
	cam->size = frmsize.index;
	close(fd);
}
	int
main (int    argc,
		char **argv)
{   
	int status;
	GtkApplication *app;
	gtk_init(&argc, &argv);
	cam_compatibilites(&cam);
	fflush(stdout);
	builder = gtk_builder_new_from_file("../data/main-window.ui");
	GtkCssProvider *css_provider = gtk_css_provider_new();
	GError *error = NULL;
	if (!gtk_css_provider_load_from_path(css_provider, "style.css", &error)) {
		g_printerr("Error loading CSS file: %s\n", error->message);
		g_error_free(error);
		return 1;
	}
	GdkScreen *screenGtk = gdk_screen_get_default();
	gtk_style_context_add_provider_for_screen(screenGtk,
			GTK_STYLE_PROVIDER(css_provider),
			GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	thumb_img = GTK_WIDGET(gtk_builder_get_object(builder, "thumb-img"));
	recording_timer= GTK_WIDGET(gtk_builder_get_object(builder, "recording_timer"));
	picture_button = GTK_WIDGET(gtk_builder_get_object(builder, "picture_button"));
	record_button = GTK_WIDGET(gtk_builder_get_object(builder, "record_button"));
	g_signal_connect(picture_button, "clicked", G_CALLBACK(on_picture_button_clicked), NULL);
	g_signal_connect(record_button, "clicked", G_CALLBACK(on_record_button_clicked), NULL);
	app = gtk_application_new("com.example.myapp", G_APPLICATION_FLAGS_NONE);
	window = GTK_WIDGET(gtk_builder_get_object(builder, "main-window"));
	gtk_builder_connect_signals(builder, NULL);
	g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);
	gtk_main();
	return status;
}

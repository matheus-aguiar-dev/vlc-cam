#include <gtk/gtk.h>
#include <unistd.h>
#include <math.h> // Include the math library
#include <gdk/gdkx.h>
#include <stdlib.h>
#include <string.h>
#include <vlc/vlc.h>
#include <fcntl.h>
#include <linux/videodev2.h>

libvlc_instance_t * inst;
libvlc_media_player_t *mp;
libvlc_media_t *m;
GtkWidget *window,*player_widget;
GtkBuilder *builder;
struct CamInfo cam;
const char* const vlc_args[] = {
	"--v4l2-chroma=YUYV",
	"--v4l2-width=800",
	"--v4l2-height=600",
	"--video-filter=transform",
	"--transform-type=hflip",
};

void player_widget_on_realize(GtkWidget *widget, gpointer data);

struct CamInfo{
	char name[100];
	int sizes[20][2];
	size_t size;
};

void configure_callback(GtkWidget *widget, GdkEvent *event, gpointer data){
	//gtk_widget_set_size_request(player_widget, cam.sizes[0][0], cam.sizes[0][1]);
	/*    libvlc_release(inst);
	      inst = libvlc_new (0,NULL);
	      gtk_widget_set_visible(player_widget, false);
	      libvlc_media_player_release(mp);
	      mp = libvlc_media_player_new_from_media (m);
	      libvlc_media_player_play(mp);
	      libvlc_media_player_set_xwindow(mp, GDK_WINDOW_XID(gtk_widget_get_window(widget)));
	      gtk_widget_set_visible(player_widget,true);
	      */
}
void player_widget_on_realize(GtkWidget *widget, gpointer data) {
	inst = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);
	m = libvlc_media_new_location(inst, "v4l2:///dev/video0");
	mp = libvlc_media_player_new_from_media (m);
	libvlc_media_player_play(mp);
	libvlc_media_release (m);
	libvlc_media_player_set_xwindow(mp, GDK_WINDOW_XID(gtk_widget_get_window(widget)));
	g_signal_connect(G_OBJECT(widget), "configure-event", G_CALLBACK(configure_callback), NULL);    
	//gtk_widget_set_size_request(player_widget, cam.sizes[1][0], cam.sizes[1][1]);
}
static void activate (GtkApplication* app,
		gpointer        cam_data)
{

	gtk_window_set_title (GTK_WINDOW (window), "Policorp");
	gtk_widget_add_events(GTK_WIDGET(window), GDK_CONFIGURE);  
	player_widget = GTK_DRAWING_AREA(gtk_builder_get_object(builder, "cam-area"));
	g_signal_connect(G_OBJECT(player_widget), "realize", G_CALLBACK(player_widget_on_realize), NULL);    
	gtk_widget_show_all (window);
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
	for (int i = 0; i < cam->size; i++) {
	}
	close(fd);
}
	int
main (int    argc,
		char **argv)
{   
	int status;
	GtkApplication *app;
	cam_compatibilites(&cam);
	fflush(stdout);
	gtk_init(&argc, &argv);
	builder = gtk_builder_new_from_file("/home/kriza/Downloads/cam-app/main/data/main-window.ui");
	GtkCssProvider *css_provider = gtk_css_provider_new();
	GError *error = NULL;
	if (!gtk_css_provider_load_from_path(css_provider, "style.css", &error)) {
		g_printerr("Error loading CSS file: %s\n", error->message);
		g_error_free(error);
		return 1;
	}

	// Apply the CSS provider to the default screen
	GdkScreen *screenGtk = gdk_screen_get_default();
	gtk_style_context_add_provider_for_screen(screenGtk,
			GTK_STYLE_PROVIDER(css_provider),
			GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	app = gtk_application_new("com.example.myapp", G_APPLICATION_FLAGS_NONE);
	window = GTK_WIDGET(gtk_builder_get_object(builder, "main-window"));
	gtk_builder_connect_signals(builder, NULL);
	g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);
	gtk_main();
	return status;
}

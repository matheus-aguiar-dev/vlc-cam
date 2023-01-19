#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <vlc/vlc.h>
libvlc_instance_t * inst;
libvlc_media_player_t *mp;
libvlc_media_t *m;
GtkWidget *player_widget;


void player_widget_on_realize(GtkWidget *widget, gpointer data);


static void activate (GtkApplication* app,
          gpointer        user_data)
{
    GtkWidget *window, *vbox;;
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); 
    window = gtk_application_window_new (app);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_window_set_title (GTK_WINDOW (window), "Policorp");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size (GTK_WINDOW (window), 1200,600);
    player_widget = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(vbox), player_widget, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(player_widget), "realize", G_CALLBACK(player_widget_on_realize), NULL);    
   gtk_widget_show_all (window);
   
}
void player_widget_on_realize(GtkWidget *widget, gpointer data) {
    inst = libvlc_new (0,NULL);
    m = libvlc_media_new_location (inst, "v4l2:///dev/video0");
    mp = libvlc_media_player_new_from_media (m);
    libvlc_media_release (m);
    libvlc_media_player_play(mp);
    libvlc_media_player_set_xwindow(mp, GDK_WINDOW_XID(gtk_widget_get_window(widget)));
}

int
main (int    argc,
      char **argv)
{   
    GtkApplication  *app;
    int status;
    app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    return status;

}
